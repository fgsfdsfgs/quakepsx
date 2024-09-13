.set noreorder
.set noat

.include "gtereg.inc"
.include "inline_s.inc"

.set C2_D1, $0
.set C2_D2, $2
.set C2_D3, $4

.section .text

# calculates dot product of two x32vec3s pointed to by a0 and a1
.global XVecDotLL
.type XVecDotLL, @function
XVecDotLL:
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
.global XVecDotSL
.type XVecDotSL, @function
XVecDotSL:
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
.global XVecLengthSqrL
.type XVecLengthSqrL, @function
XVecLengthSqrL:
  # a0 - pointer to input vector
  # v0 - return
  lwc2  C2_IR1, 0($a0)
  lwc2  C2_IR2, 4($a0)
  lwc2  C2_IR3, 8($a0)
  nSQR(1)
  mfc2  $t0, C2_MAC1
  mfc2  $t1, C2_MAC2
  mfc2  $t2, C2_MAC3
  move  $v0, $t0
  addu  $v0, $t1
  jr    $ra
  addu  $v0, $t2

# calculates square of length of F1.19.12 2D vector pointed to by a0, in F1.19.12
.global XVec2LengthSqrL
.type XVec2LengthSqrL, @function
XVec2LengthSqrL:
  # a0 - pointer to input vector
  # v0 - return
  lwc2  C2_IR1, 0($a0)
  lwc2  C2_IR2, 4($a0)
  ctc2  $0, C2_IR3
  nSQR(1)
  mfc2  $t0, C2_MAC1
  mfc2  $t1, C2_MAC2
  move  $v0, $0
  addu  $v0, $t0
  jr    $ra
  addu  $v0, $t1

# calculates cross product of two x16vec3s pointed to by a0 and a1 and puts it in a2
.global XVecCrossSS
.type XVecCrossSS, @function
XVecCrossSS:
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

.global XVecNormLS
.type XVecNormLS, @function
XVecNormLS:
  lw     $t0, 0($a0)
  lw     $t1, 4($a0)
  srl    $t0, 12
  lw     $t2, 8($a0)
  srl    $t1, 12
  mtc2   $t0, C2_IR1
  srl    $t2, 12
  mtc2   $t1, C2_IR2
  mtc2   $t2, C2_IR3

  nSQR(0)

  mfc2   $t3, C2_MAC1
  mfc2   $t4, C2_MAC2
  mfc2   $t5, C2_MAC3

  add    $t3, $t4
  add    $v0, $t3, $t5
  mtc2   $v0, C2_LZCS
  sw     $t3, 0($a2)
  nop
  mfc2   $v1, C2_LZCR

  addiu  $at, $0 , -2
  and    $v1, $at

  addiu  $t6, $0 , 0x1f
  sub    $t6, $v1
  sra    $t6, 1
  addiu  $t3, $v1, -24

  bltz   $t3, .Lvalue_neg
  nop
  b      .Lvalue_pos
  sllv   $t4, $v0, $t3
.Lvalue_neg:
  addiu  $t3, $0 , 24
  sub    $t3, $v1
  srav   $t4, $v0, $t3
.Lvalue_pos:
  addi   $t4, -64
  sll    $t4, 1

  la     $t5, _norm_table
  addu   $t5, $t4
  lh     $t5, 0($t5)

  mtc2   $t0, C2_IR1
  mtc2   $t1, C2_IR2
  mtc2   $t2, C2_IR3
  mtc2   $t5, C2_IR0

  nGPF(0)

  mfc2   $t0, C2_MAC1
  mfc2   $t1, C2_MAC2
  mfc2   $t2, C2_MAC3

  sra    $t0, $t6
  sra    $t1, $t6
  sra    $t2, $t6

  sh     $t0, 0($a1)
  sh     $t1, 2($a1)
  jr     $ra
  sh     $t2, 4($a1)

.type _norm_table, @object
_norm_table:
  .hword 0x1000, 0x0fe0, 0x0fc1, 0x0fa3, 0x0f85, 0x0f68, 0x0f4c, 0x0f30
  .hword 0x0f15, 0x0efb, 0x0ee1, 0x0ec7, 0x0eae, 0x0e96, 0x0e7e, 0x0e66
  .hword 0x0e4f, 0x0e38, 0x0e22, 0x0e0c, 0x0df7, 0x0de2, 0x0dcd, 0x0db9
  .hword 0x0da5, 0x0d91, 0x0d7e, 0x0d6b, 0x0d58, 0x0d45, 0x0d33, 0x0d21
  .hword 0x0d10, 0x0cff, 0x0cee, 0x0cdd, 0x0ccc, 0x0cbc, 0x0cac, 0x0c9c
  .hword 0x0c8d, 0x0c7d, 0x0c6e, 0x0c5f, 0x0c51, 0x0c42, 0x0c34, 0x0c26
  .hword 0x0c18, 0x0c0a, 0x0bfd, 0x0bef, 0x0be2, 0x0bd5, 0x0bc8, 0x0bbb
  .hword 0x0baf, 0x0ba2, 0x0b96, 0x0b8a, 0x0b7e, 0x0b72, 0x0b67, 0x0b5b
  .hword 0x0b50, 0x0b45, 0x0b39, 0x0b2e, 0x0b24, 0x0b19, 0x0b0e, 0x0b04
  .hword 0x0af9, 0x0aef, 0x0ae5, 0x0adb, 0x0ad1, 0x0ac7, 0x0abd, 0x0ab4
  .hword 0x0aaa, 0x0aa1, 0x0a97, 0x0a8e, 0x0a85, 0x0a7c, 0x0a73, 0x0a6a
  .hword 0x0a61, 0x0a59, 0x0a50, 0x0a47, 0x0a3f, 0x0a37, 0x0a2e, 0x0a26
  .hword 0x0a1e, 0x0a16, 0x0a0e, 0x0a06, 0x09fe, 0x09f6, 0x09ef, 0x09e7
  .hword 0x09e0, 0x09d8, 0x09d1, 0x09c9, 0x09c2, 0x09bb, 0x09b4, 0x09ad
  .hword 0x09a5, 0x099e, 0x0998, 0x0991, 0x098a, 0x0983, 0x097c, 0x0976
  .hword 0x096f, 0x0969, 0x0962, 0x095c, 0x0955, 0x094f, 0x0949, 0x0943
  .hword 0x093c, 0x0936, 0x0930, 0x092a, 0x0924, 0x091e, 0x0918, 0x0912
  .hword 0x090d, 0x0907, 0x0901, 0x08fb, 0x08f6, 0x08f0, 0x08eb, 0x08e5
  .hword 0x08e0, 0x08da, 0x08d5, 0x08cf, 0x08ca, 0x08c5, 0x08bf, 0x08ba
  .hword 0x08b5, 0x08b0, 0x08ab, 0x08a6, 0x08a1, 0x089c, 0x0897, 0x0892
  .hword 0x088d, 0x0888, 0x0883, 0x087e, 0x087a, 0x0875, 0x0870, 0x086b
  .hword 0x0867, 0x0862, 0x085e, 0x0859, 0x0855, 0x0850, 0x084c, 0x0847
  .hword 0x0843, 0x083e, 0x083a, 0x0836, 0x0831, 0x082d, 0x0829, 0x0824
  .hword 0x0820, 0x081c, 0x0818, 0x0814, 0x0810, 0x080c, 0x0808, 0x0804
