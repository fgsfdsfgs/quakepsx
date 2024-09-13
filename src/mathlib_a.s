.set noreorder
.set noat

.include "gtereg.inc"
.include "inline_s.inc"

.set C2_D1, $0
.set C2_D2, $2
.set C2_D3, $4

.section .text

# calculates dot product of two x32vec3s pointed to by a0 and a1
.global XVecDot32x32
.type XVecDot32x32, @function
XVecDot32x32:
  # a0, a1 - pointers to vectors
  # v0     - return
  lw    $t0, 0($a0)
  lw    $t1, 4($a0)
  lw    $t2, 8($a0)
  lw    $t3, 0($a1)
  lw    $t4, 4($a1)
  lw    $t5, 8($a1)
  move  $v0, $0
  # a.x * b.x
  mult  $t0, $t3
  mflo  $t0
  mfhi  $t3
  srl   $t0, 12
  and   $t3, 0x0FFF
  sll   $t3, 20
  or    $t0, $t3
  add   $v0, $t0
  # a.y * b.y
  mult  $t1, $t4
  mflo  $t0
  mfhi  $t3
  srl   $t0, 12
  and   $t3, 0x0FFF
  sll   $t3, 20
  or    $t0, $t3
  add   $v0, $t0
  # a.z * b.z
  mult  $t2, $t5
  mflo  $t0
  mfhi  $t3
  srl   $t0, 12
  and   $t3, 0x0FFF
  sll   $t3, 20
  or    $t0, $t3
  jr    $ra
  add   $v0, $t0

# calculates dot product of an x16vec3 and an x32vec3 pointed to by a0 and a1
.global XVecDot16x32
.type XVecDot16x32, @function
XVecDot16x32:
  # a0     - pointer to x16vec3_t
  # a1     - pointer to x32vec3_t
  # v0     - return
  lh    $t0, 0($a0)
  lh    $t1, 2($a0)
  lh    $t2, 4($a0)
  lw    $t3, 0($a1)
  lw    $t4, 4($a1)
  lw    $t5, 8($a1)
  move  $v0, $0
  # a.x * b.x
  mult  $t0, $t3
  mflo  $t0
  mfhi  $t3
  srl   $t0, 12
  and   $t3, 0x0FFF
  sll   $t3, 20
  or    $t0, $t3
  add   $v0, $t0
  # a.y * b.y
  mult  $t1, $t4
  mflo  $t0
  mfhi  $t3
  srl   $t0, 12
  and   $t3, 0x0FFF
  sll   $t3, 20
  or    $t0, $t3
  add   $v0, $t0
  # a.z * b.z
  mult  $t2, $t5
  mflo  $t0
  mfhi  $t3
  srl   $t0, 12
  and   $t3, 0x0FFF
  sll   $t3, 20
  or    $t0, $t3
  jr    $ra
  add   $v0, $t0

# calculates square of length of F1.19.12 vector pointed to by a0, in F1.19.12
.global XVecLengthSqr32
.type XVecLengthSqr32, @function
XVecLengthSqr32:
  # a0 - pointer to input vector
  # v0 - return
  # first, square the vector
  lwc2  C2_IR1, 0($a0)
  lwc2  C2_IR2, 4($a0)
  lwc2  C2_IR3, 8($a0)
  nSQR(1)
  mfc2  $t0, C2_IR1
  mfc2  $t1, C2_IR2
  mfc2  $t2, C2_IR3
  move  $v0, $0
  addu  $v0, $t0
  addu  $v0, $t1
  jr    $ra
  addu  $v0, $t2

# calculates cross product of two x16vec3s pointed to by a0 and a1 and puts it in a2
.global XVecCross16
.type XVecCross16, @function
XVecCross16:
  # a0     - pointer to x16vec3_t
  # a1     - pointer to x16vec3_t
  # a2     - pointer to output x16vec3_t
  lh    $t0, 0($a0)
  lh    $t1, 2($a0)
  ctc2  $t0, C2_D1
  lh    $t2, 4($a0)
  ctc2  $t1, C2_D2
  ctc2  $t2, C2_D3
  lh    $t3, 0($a1)
  lh    $t4, 2($a1)
  ctc2  $t3, C2_IR1
  lh    $t5, 4($a1)
  ctc2  $t4, C2_IR2
  ctc2  $t5, C2_IR3
  nop
  nop
  cop2 0x0178000C
  mfc2  $t0, C2_IR1
  mfc2  $t1, C2_IR2
  mfc2  $t2, C2_IR3
  sh    $t0, 0($a2)
  sh    $t1, 2($a2)
  jr    $ra
  sh    $t2, 4($a2)
