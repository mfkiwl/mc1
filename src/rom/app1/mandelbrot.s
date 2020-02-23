; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; This is the main boot program.
; ----------------------------------------------------------------------------

.include "system/memory.inc"
.include "system/mmio.inc"

FB_WIDTH  = 640
FB_HEIGHT = 360

    .text

; ----------------------------------------------------------------------------
; Draw a Mandelbrot fractal.
;
; void mandelbrot(int frame_no, void* fb_start)
;   s1 = frame number (0, 1, ...)
;   s2 = framebuffer start
; ----------------------------------------------------------------------------

    .p2align 2
    .global mandelbrot
mandelbrot:
    add     sp, sp, #-16
    stw     s16, sp, #12
    stw     s17, sp, #8
    stw     s18, sp, #4
    stw     s20, sp, #0

    mov     s14, s2                 ; s14 = pixel_data
    bz      s14, mandelbrot_fail

    and     s1, s1, #127
    slt     s2, s1, #64
    bs      s2, 1$
    sub     s1, #128, s1
1$:

    ldw     s13, pc, #coord_step@pc
    ldw     s17, pc, #max_num_iterations@pc
    ldw     s18, pc, #max_distance_sqr@pc

    ; Calculate a zoom factor.
    itof    s1, s1, z
    fmul    s1, s1, s1
    ldi     s2, #0x3c23c000         ; ~0.01
    fmul    s2, s1, s2
    ldi     s3, #0x3f800000         ; 1.0
    fadd    s2, s2, s3              ;
    fdiv    s20, s3, s2             ; s20 = 1.0 / (1.0 + frameno^2 * 0.01)

    fmul    s13, s13, s20           ; s13 = coord_step * zoom_factor

    ldi     s16, #FB_HEIGHT ; s16 = loop counter for y
    asr     s3, s16, #1
    itof    s3, s3, z
    fmul    s3, s13, s3
    ldw     s2, pc, #center_im@pc
    fsub    s2, s2, s3      ; s2 = min_im = center_im - coord_step * FB_HEIGHT/2

outer_loop_y:
    ldi     s15, #FB_WIDTH  ; s15 = loop counter for x
    asr     s3, s15, #1
    itof    s3, s3, z
    fmul    s3, s13, s3
    ldw     s1, pc, #center_re@pc
    fsub    s1, s1, s3      ; s1 = min_re = center_re - coord_step * FB_WIDTH/2

outer_loop_x:
    or      s3, z, z        ; s3 = re(z) = 0.0
    or      s4, z, z        ; s4 = im(z) = 0.0

    ldi     s9, #0          ; Iteration count.

inner_loop:
    fmul    s5, s3, s3      ; s5 = re(z)^2
    fmul    s6, s4, s4      ; s6 = im(z)^2
    add     s9, s9, #1
    fmul    s4, s3, s4
    fsub    s3, s5, s6
    fadd    s5, s5, s6      ; s5 = |z|^2
    fadd    s4, s4, s4      ; s4 = 2*re(z)*im(z)
    fadd    s3, s3, s1      ; s3 = re(z)^2 - im(z)^2 + re(c)
    slt     s10, s9, s17    ; num_iterations < max_num_iterations?
    fadd    s4, s4, s2      ; s4 = 2*re(z)*im(z) + im(c)
    fslt    s5, s5, s18     ; |z|^2 < 4.0?

    bns     s5, inner_loop_done
    bs      s10, inner_loop   ; max_num_iterations no reached yet?

    ldi     s9, #0          ; This point is part of the set -> color = 0

inner_loop_done:
    lsl     s9, s9, #1      ; color * 2 for more intense levels

    ; Write color to pixel matrix.
    stb     s9, s14, #0
    add     s14, s14, #1

    ; Increment along the x axis.
    add     s15, s15, #-1
    fadd    s1, s1, s13     ; re(c) = re(c) + coord_step
    bgt     s15, outer_loop_x

    ; Increment along the y axis.
    add     s16, s16, #-1
    fadd    s2, s2, s13     ; im(c) = im(c) + coord_step
    bgt     s16, outer_loop_y

mandelbrot_fail:
    ldw     s20, sp, #0
    ldw     s18, sp, #4
    ldw     s17, sp, #8
    ldw     s16, sp, #12
    add     sp, sp, #16
    ret


max_num_iterations:
    .word   127

max_distance_sqr:
    .float  4.0

center_re:
    .float  -1.156362697351

center_im:
    .float  -0.279199711590

coord_step:
    .float  0.007

