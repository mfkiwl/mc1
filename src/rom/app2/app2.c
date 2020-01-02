// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2019 Marcus Geelnard
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

#include <system/leds.h>
#include <system/memory.h>
#include <system/time.h>

int main(void) {
  sevseg_print_dec(123456);

  void* ptr = mem_alloc(1234, MEM_TYPE_ANY);

  for (int i = 1000; i > 0; --i) {
    sevseg_print_dec(i);
    msleep(1000);
    sevseg_print_hex((unsigned)ptr);
    msleep(1000);
  }

  return 0;
}

