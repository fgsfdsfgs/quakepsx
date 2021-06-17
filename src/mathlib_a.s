.set noreorder

.include "gtereg.h"
.include "inline_s.h"

.section .text

# psn00bsdk lacks this one, gotta do it ourselves
.global Square12
.type Square12, @function
Square12:
  # a0 - Pointer to input vector (v0)
  # a1 - Pointer to output vector (v1)

  lwc2  C2_IR1, 0($a0)
  lwc2  C2_IR2, 4($a0)
  lwc2  C2_IR3, 8($a0)

  nSQR(1)

  swc2  C2_IR1, 0($a1)
  swc2  C2_IR2, 4($a1)
  swc2  C2_IR3, 8($a1)

  jr    $ra
  nop

# calculates `(x * y) >> 12`, where x and y are F1.19.12 fixed points
.global xmul32
.type xmul32, @function
xmul32:
  # a0, a1 - numbers
  # v0     - return
  mult  $a0, $a1    # multiply; 64-bit result is now in $lo + $hi
  mflo  $v0         # get the lo and hi words of result
  mfhi  $t0
  srl   $v0, 12     # >> 12
  and   $t0, 0x0FFF
  sll   $t0, 20
  jr    $ra
  or    $v0, $t0    # combine hi word with lo word

# calculates `(x * 4096) / y`, where x and y are F1.19.12 fixed points
.global xdiv32
.type xdiv32, @function
xdiv32:
  # a0, a1 - numbers
  # v0     - return
  sll   $a0, 12
  div   $a0, $a1
  jr    $ra
  mflo  $v0

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
