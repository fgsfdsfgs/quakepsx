.set noreorder

.include "gtereg.h"
.include "inline_s.h"

.section .text

.set CLUT_VALUE, 0x3C00
.set OT_SHIFT, 2

.global R_SortTriangleFan
.type R_SortTriangleFan, @function
R_SortTriangleFan:
  # a0 - pointer to vertex array (svert_t[])
  # a1 - number of vertices
  # a2 - texture page
  # load first 3 verts
  lwc2  C2_VXY0,  0($a0)
  lwc2  C2_VZ0,   4($a0)
  lwc2  C2_VXY1, 16($a0)
  lwc2  C2_VZ1,  20($a0)
  lwc2  C2_VXY2, 32($a0)
  lwc2  C2_VZ2,  36($a0)
  # keep a0 pointing to vertex 0, but store pointers to vertex i-1 and i
  # do this shit in NOP slots
  addiu $t0, $a0, 16  # i - 1
  addiu $t1, $a0, 32  # i
  # transform verts
  RTPT
  # load primitive pointer for later
  lw    $v0, gpu_ptr
  lw    $v1, gpu_ot
  # remember XYZ0
  mfc2  $t6, C2_SXY0
  mfc2  $t7, C2_SZ1
  # check if we're exceeding near/far clip (bits 17/18 of the FLAGS register)
  cfc2  $t8, C2_FLAG
  li    $t2, 0x60000    # ((1 << 17) | (1 << 18))
  and   $t8, $t2
  bnez  $t8, end_loop
  nop
  # calculate and scale OT Z
  AVSZ3
  mfc2  $t9, C2_OTZ
  # store shit out to the primitive
  swc2  C2_SXY0,  8($v0)
  swc2  C2_SXY1, 20($v0)
  swc2  C2_SXY2, 32($v0)
  # copy texcoords and colors
  lw    $t2,  8($a0) # ld rgbc0
  lw    $t3,  8($t0) # ld rgbc1
  lw    $t4,  8($t1) # ld rgbc2
  sw    $t2,  4($v0) # store rgbc0
  sw    $t3, 16($v0) # store rgbc1
  sw    $t4, 28($v0) # store rgbc2
  lh    $t2, 12($a0) # ld uv0
  lh    $t3, 12($t0) # ld uv1
  lh    $t4, 12($t1) # ld uv2
  sh    $t2, 12($v0) # store uv0
  sh    $t3, 24($v0)
  sh    $t4, 36($v0)
  # store texture and CLUT pages
  li    $t2, CLUT_VALUE
  sh    $t2, 14($v0) # store clut
  sh    $a2, 26($v0) # store tpage
  # link primitive
  sra   $t9, OT_SHIFT  # rescale Z
  sll   $t9, 2         # multiply by sizeof(u32)
  add   $t2, $v1, $t9
  lw    $t3, 0($t2)    # $t3 = gpu_ot[$t9]
  li    $t5, 0x9000000
  or    $t3, $t5
  li    $t5, 0xFFFFFF
  and   $t4, $v0, $t5
  sw    $t3, 0($v0)    # prim->tag = $t3
  sw    $t4, 0($t2)
  b     end_loop
  addiu $v0, 40        # sizeof(POLY_GT3)
  # now we only have to load and transform one vert at a time, otherwise it's all the same
rtps_loop:
  lwc2  C2_VXY0,  0($t1)
  lwc2  C2_VZ0,   4($t1)
  li    $t2, 0x60000 # ((1 << 17) | (1 << 18))
  RTPS
  cfc2  $t8, C2_FLAG
  and   $t8, $t2
  bnez  $t8, end_loop
  addi  $a1, -1
  AVSZ3
  mfc2  $t9, C2_OTZ
  swc2  C2_SXY0,  8($v0)
  swc2  C2_SXY1, 20($v0)
  swc2  C2_SXY2, 32($v0)
  lw    $t2,  8($a0) # ld rgbc0
  lw    $t3,  8($t0) # ld rgbc1
  lw    $t4,  8($t1) # ld rgbc2
  sw    $t2,  4($v0) # store rgbc0
  sw    $t3, 16($v0) # store rgbc1
  sw    $t4, 28($v0) # store rgbc2
  lh    $t2, 12($a0) # ld uv0
  lh    $t3, 12($t0) # ld uv1
  lh    $t4, 12($t1) # ld uv2
  sh    $t2, 12($v0) # store uv0
  sh    $t3, 24($v0)
  sh    $t4, 36($v0)
  li    $t2, CLUT_VALUE
  sh    $t2, 14($v0)
  sh    $a2, 26($v0)
  sra   $t9, OT_SHIFT
  sll   $t9, 2
  add   $t2, $v1, $t9
  lw    $t3, 0($t2)
  li    $t5, 0x9000000
  or    $t3, $t5
  li    $t5, 0xFFFFFF
  and   $t4, $v0, $t5
  sw    $t3, 0($v0)
  sw    $t4, 0($t2)
  addiu $v0, 40        # sizeof(POLY_GT3)
end_loop:
  mtc2  $t6, C2_SXY1
  mtc2  $t7, C2_SZ2
  addiu $t0, 16        # sizeof(svert_t)
  bge   $a1, 3, rtps_loop
  addiu $t1, 16
  sw    $v0, gpu_ptr
  jr    $ra
  nop
