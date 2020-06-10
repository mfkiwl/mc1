// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2020 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

#include <mc1/fast_math.h>
#include <mc1/mmio.h>
#include <mc1/mr32intrin.h>
#include <mc1/vcp.h>

#include <stddef.h>
#include <stdint.h>

#define SINE_LUT_ENTIRES 1024
#define PIXEL_WORDS      16

typedef struct {
  void* base_ptr;
  uint32_t* vcp1;
  uint32_t* vcp2;
  uint32_t* vcp3;
  uint32_t* pixels1;
  int16_t* sine_lut;
  int32_t width;
  int32_t height;
  int32_t sky_height;
} retro_t;

static retro_t s_retro;


//--------------------------------------------------------------------------------------------------
// Drawing routines.
//--------------------------------------------------------------------------------------------------

static int sin16(int x) {
  return (int)s_retro.sine_lut[((unsigned)x) & (SINE_LUT_ENTIRES - 1u)];
}

static int iabs(int x) {
  return x < 0 ? -x : x;
}

static uint8x4_t lerp8(uint8x4_t a, uint8x4_t b, int w) {
  uint8x4_t w1 = _mr32_shuf(255 - w, _MR32_SHUFCTL(0, 0, 0, 0, 0));
  uint8x4_t w2 = _mr32_shuf(w, _MR32_SHUFCTL(0, 0, 0, 0, 0));
  return _mr32_addsu_b(_mr32_mulhiu_b(a, w1), _mr32_mulhiu_b(b, w2));
}

static void render_layer1(const int frame_no) {
  // Create a gradient at the top of the screen ("sky").
  {
    const float w_scale = 255.0f / (float)s_retro.sky_height;
    uint32_t* vcp = s_retro.vcp1 + 3;
    for (int y = 0; y < s_retro.sky_height; ++y) {
      // Modulate the sky with a slow blue sine.
      const int s = 128 + (sin16(frame_no * 2 + y * 3) >> 8);
      const uint8x4_t sin_mod =
          _mr32_mulhiu_b(0x00301008u, _mr32_shuf(s, _MR32_SHUFCTL(0, 0, 0, 0, 0)));

      // Generate a sweet sky gradient.
      // TODO(m): Make it so!
      const int w = (int)(((float)y) * w_scale);
      const uint8x4_t sky_col =
          _mr32_mulhiu_b(0x00105080u, _mr32_shuf(w, _MR32_SHUFCTL(0, 0, 0, 0, 0)));
      *vcp = _mr32_addsu_b(sky_col, sin_mod);

      vcp += 3;
    }
   }

  // Create the checker board at the bottom of the screen.
  {
    const int checker_height = s_retro.height - s_retro.sky_height;
    const float width_div2 = _mr32_itof(s_retro.width, 1);
    const float scale_step = 10.0f / (float)checker_height;
    float scale_div = 1.0f;
    const int offs_base = 0x10000000 + (sin16(frame_no) * 64);
    const float check_fade_scale = 255.0f / (float)checker_height;
    uint32_t* vcp = s_retro.vcp2 + 5;
    for (int y = 0; y < checker_height; ++y) {
      // Calculate and set the XOFFS and XINCR registers.
      const float scale = (1.0f / 8.0f) / scale_div;
      scale_div += scale_step;
      const float offs = scale * width_div2;
      const int32_t xoffs = offs_base - _mr32_ftoir(offs, 16);
      const int32_t xincr = _mr32_ftoir(scale, 16);
      vcp[0] = vcp_emit_setreg(VCR_XOFFS, xoffs & 0x000fffffu);
      vcp[1] = vcp_emit_setreg(VCR_XINCR, xincr & 0x00ffffffu);

      // Calculate and set the palette colors.
      // 1) Use alternating colors to achieve the checker effect.
      uint32_t color0 = 0x00ffd0c0u;
      uint32_t color1 = 0x00102030u;
      if (((_mr32_ftoi(scale, 13) + frame_no) & 32) != 0) {
        uint32_t tmp = color1;
        color1 = color0;
        color0 = tmp;
      }
      // 2) Fade towards a common color at the horizon.
      const int w = (int)(check_fade_scale * (float)y);
      vcp[3] = lerp8(0x00403020u, color0, w);
      vcp[4] = lerp8(0x00403020u, color1, w);

      vcp += 6;
    }
  }
}

static void render_layer2(const int frame_no) {
  // Clear the raster colors.
  {
    // TODO(m): Use a vector loop instead.
    uint32_t* color_ptr = s_retro.vcp3 + 3;
    for (int y = 0; y < s_retro.height; ++y) {
      *color_ptr = 0u;
      color_ptr += 3;
    }
  }

  // Draw a few raster bars.
  {
    const int NUM_BARS = 16;
    const uint8x4_t bar_color_1 = 0xff44ffc7u;
    const uint8x4_t bar_color_2 = 0xffff43ffu;

    for (int k = 0; k < NUM_BARS; ++k) {
      // Calculate the bar position.
      int pos = sin16((frame_no + 4 * k) * (SINE_LUT_ENTIRES / 256));
      pos = (s_retro.height >> 1) + (((s_retro.height * 3) * pos) >> 18);

      // Calculate the bar alpha.
      int alpha = (sin16((frame_no >> 1) + 2 * k) >> 7) + 100;
      alpha = (alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha));

      // Calculate the bar color.
      const uint32_t w1 = k * (255 / (NUM_BARS - 1));
      const uint32_t w2 = 255 - w1;
      const uint8x4_t bar_color = _mr32_addsu_b(
          _mr32_mulhiu_b(bar_color_1, _mr32_shuf(w1, _MR32_SHUFCTL(0, 0, 0, 0, 0))),
          _mr32_mulhiu_b(bar_color_2, _mr32_shuf(w2, _MR32_SHUFCTL(0, 0, 0, 0, 0))));

      // Draw the bar.
      for (int i = -32; i <= 32; ++i) {
        const int y = pos + i;
        const uint32_t intensity = ((uint32_t)(alpha * (32 - iabs(i)))) >> 5;
        const uint8x4_t color = _mr32_mulhiu_b(bar_color, _mr32_shuf(intensity, 0));
        uint8x4_t* color_ptr = s_retro.vcp3 + 3 + 3 * y;
        *color_ptr = _mr32_maxu_b(color, *color_ptr);
      }
    }
  }
}

static void render(const int frame_no) {
  render_layer1(frame_no);
  render_layer2(frame_no);
}


//--------------------------------------------------------------------------------------------------
// Public API.
//--------------------------------------------------------------------------------------------------

void retro_init(void) {
  if (s_retro.base_ptr != NULL) {
    return;
  }

  // Get the native video resolution.
  s_retro.width = (int)MMIO(VIDWIDTH);
  s_retro.height = (int)MMIO(VIDHEIGHT);
  s_retro.sky_height = (s_retro.height * 5) >> 3;

  // VCP 1 (top of layer 1).
  const int32_t vcp1_height = s_retro.sky_height;
  const size_t vcp1_size = sizeof(uint32_t) * (1 + vcp1_height * 3);

  // VCP 2 (bottom of layer 1).
  const int32_t vcp2_height = s_retro.height - s_retro.sky_height;
  const size_t vcp2_size = sizeof(uint32_t) * (4 + vcp2_height * 6 + 1);

  // VCP 3 (layer 2).
  const size_t vcp3_size = sizeof(uint32_t) * (1 + s_retro.height * 3 + 1);

  // Pixels for layer 1.
  const size_t pix1_size = sizeof(uint32_t) * PIXEL_WORDS;

  // Sine LUT.
  const size_t sine_size = sizeof(int16_t) * SINE_LUT_ENTIRES;

  // Calculate the required memory size.
  const size_t total_size = vcp1_size + vcp2_size + vcp3_size + pix1_size + sine_size;

  uint8_t* mem = (uint8_t*)mem_alloc(total_size, MEM_TYPE_VIDEO | MEM_CLEAR);
  if (mem == NULL) {
    return;
  }
  s_retro.base_ptr = mem;

  s_retro.vcp1 = (uint32_t*)(mem);
  s_retro.vcp2 = (uint32_t*)(mem + vcp1_size);
  s_retro.vcp3 = (uint32_t*)(mem + vcp1_size + vcp2_size);
  s_retro.pixels1 = (uint32_t*)(mem + vcp1_size + vcp2_size + vcp3_size);
  s_retro.sine_lut = (int16_t*)(mem + vcp1_size + vcp2_size + vcp3_size + pix1_size);

  // Create the VCP for layer 1 (VCP 1 + VCP 2).
  {
    uint32_t* vcp = s_retro.vcp1;

    // Prologue.
    // Set the dither mode.
    *vcp++ = vcp_emit_setreg(VCR_RMODE, 0x135);

    // The sky.
    int y = 0;
    for (; y < s_retro.sky_height; ++y) {
      *vcp++ = vcp_emit_waity(y);
      *vcp++ = vcp_emit_setpal(0, 1);
      ++vcp;  // Palette color 0
    }

    // Checkerboard prologue.
    *vcp++ = vcp_emit_waity(y);
    *vcp++ = vcp_emit_setreg(VCR_ADDR, to_vcp_addr((uint32_t)s_retro.pixels1));
    *vcp++ = vcp_emit_setreg(VCR_HSTOP, s_retro.width);
    *vcp++ = vcp_emit_setreg(VCR_CMODE, CMODE_PAL1);

    // The checker board.
    {
      for (; y < s_retro.height; ++y) {
        *vcp++ = vcp_emit_waity(y);
        *vcp++ = vcp_emit_setreg(VCR_XOFFS, 0);
        *vcp++ = vcp_emit_setreg(VCR_XINCR, 0x000400);
        *vcp++ = vcp_emit_setpal(0, 2);
        vcp += 2;  // Palette colors 0 & 1
      }
    }

    // Epilogue.
    *vcp = vcp_emit_waity(32767);
  }

  // Create the VCP for layer 2.
  {
    uint32_t* vcp = s_retro.vcp3;

    // Prologue.
    // Set the blend mode.
    *vcp++ = vcp_emit_setreg(VCR_RMODE, 0x135);

    // Per-line commands.
    for (int y = 0; y < s_retro.height; ++y) {
      *vcp++ = vcp_emit_waity(y);
      *vcp++ = vcp_emit_setpal(0, 1);
      ++vcp;  // Palette color 0
    }

    // Epilogue.
    *vcp = vcp_emit_waity(32767);
  }

  // Create the pixels for the checker board.
  for (int k = 0; k < PIXEL_WORDS; ++k) {
    s_retro.pixels1[k] = 0x0f0f0f0fu;
  }

  // Create the sine LUT.
  for (int k = 0; k < SINE_LUT_ENTIRES; ++k) {
    float y = fast_sin(k * (6.283185307f / (float)SINE_LUT_ENTIRES));
    s_retro.sine_lut[k] = (int16_t)(32767.0f * y);
  }
}

void retro_deinit(void) {
  if (s_retro.base_ptr != NULL) {
    mem_free(s_retro.base_ptr);
    s_retro.base_ptr = NULL;

    vcp_set_prg(LAYER_1, NULL);
    vcp_set_prg(LAYER_2, NULL);
  }
}

void retro(int frame_no) {
  if (s_retro.base_ptr == NULL) {
    return;
  }

  vcp_set_prg(LAYER_1, s_retro.vcp1);
  vcp_set_prg(LAYER_2, s_retro.vcp3);

  render(frame_no);
}

