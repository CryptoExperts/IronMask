#order 9
#shares 10
#in a b
#out c
#randoms r00 r01 r02 r03 r04 r05 r06 r07 r08 r09 r10 r11 r12 r13 r14 r15 r16 r17 r18 r19 r20 r21 r22 r23 r24 r25 r30 r31 r32 r33

s00 = a0 * b0
s01 = a0 * b1
s02 = a0 * b2
s03 = a0 * b3
s04 = a0 * b4
s05 = a0 * b5
s06 = a0 * b6
s07 = a0 * b7
s08 = a0 * b8
s09 = a0 * b9

s10 = a1 * b0
s11 = a1 * b1
s12 = a1 * b2
s13 = a1 * b3
s14 = a1 * b4
s15 = a1 * b5
s16 = a1 * b6
s17 = a1 * b7
s18 = a1 * b8
s19 = a1 * b9

s20 = a2 * b0
s21 = a2 * b1
s22 = a2 * b2
s23 = a2 * b3
s24 = a2 * b4
s25 = a2 * b5
s26 = a2 * b6
s27 = a2 * b7
s28 = a2 * b8
s29 = a2 * b9

s30 = a3 * b0
s31 = a3 * b1
s32 = a3 * b2
s33 = a3 * b3
s34 = a3 * b4
s35 = a3 * b5
s36 = a3 * b6
s37 = a3 * b7
s38 = a3 * b8
s39 = a3 * b9

s40 = a4 * b0
s41 = a4 * b1
s42 = a4 * b2
s43 = a4 * b3
s44 = a4 * b4
s45 = a4 * b5
s46 = a4 * b6
s47 = a4 * b7
s48 = a4 * b8
s49 = a4 * b9

s50 = a5 * b0
s51 = a5 * b1
s52 = a5 * b2
s53 = a5 * b3
s54 = a5 * b4
s55 = a5 * b5
s56 = a5 * b6
s57 = a5 * b7
s58 = a5 * b8
s59 = a5 * b9

s60 = a6 * b0
s61 = a6 * b1
s62 = a6 * b2
s63 = a6 * b3
s64 = a6 * b4
s65 = a6 * b5
s66 = a6 * b6
s67 = a6 * b7
s68 = a6 * b8
s69 = a6 * b9

s70 = a7 * b0
s71 = a7 * b1
s72 = a7 * b2
s73 = a7 * b3
s74 = a7 * b4
s75 = a7 * b5
s76 = a7 * b6
s77 = a7 * b7
s78 = a7 * b8
s79 = a7 * b9

s80 = a8 * b0
s81 = a8 * b1
s82 = a8 * b2
s83 = a8 * b3
s84 = a8 * b4
s85 = a8 * b5
s86 = a8 * b6
s87 = a8 * b7
s88 = a8 * b8
s89 = a8 * b9

s90 = a9 * b0
s91 = a9 * b1
s92 = a9 * b2
s93 = a9 * b3
s94 = a9 * b4
s95 = a9 * b5
s96 = a9 * b6
s97 = a9 * b7
s98 = a9 * b8
s99 = a9 * b9


t0 = s00 + r00
t0 = t0 + s01
t0 = t0 + s10
t0 = t0 + r01
t0 = t0 + s02
t0 = t0 + s20
t0 = t0 + r10
t0 = t0 + s03
t0 = t0 + s30
t0 = t0 + r11
t0 = t0 + s04
t0 = t0 + s40
t0 = t0 + r20
t0 = t0 + s05
t0 = t0 + s50
t0 = t0 + r21
c0 = t0 + r30

t1 = s11 + r01
t1 = t1 + s12
t1 = t1 + s21
t1 = t1 + r02
t1 = t1 + s13
t1 = t1 + s31
t1 = t1 + r11
t1 = t1 + s14
t1 = t1 + s41
t1 = t1 + r12
t1 = t1 + s15
t1 = t1 + s51
t1 = t1 + r21
t1 = t1 + s16
t1 = t1 + s61
t1 = t1 + r22
c1 = t1 + r31

t2 = s22 + r02
t2 = t2 + s23
t2 = t2 + s32
t2 = t2 + r03
t2 = t2 + s24
t2 = t2 + s42
t2 = t2 + r12
t2 = t2 + s25
t2 = t2 + s52
t2 = t2 + r13
t2 = t2 + s26
t2 = t2 + s62
t2 = t2 + r22
t2 = t2 + s27
t2 = t2 + s72
t2 = t2 + r23
c2 = t2 + r32

t3 = s33 + r03
t3 = t3 + s34
t3 = t3 + s43
t3 = t3 + r04
t3 = t3 + s35
t3 = t3 + s53
t3 = t3 + r13
t3 = t3 + s36
t3 = t3 + s63
t3 = t3 + r14
t3 = t3 + s37
t3 = t3 + s73
t3 = t3 + r23
t3 = t3 + s38
t3 = t3 + s83
t3 = t3 + r24
c3 = t3 + r33

t4 = s44 + r04
t4 = t4 + s45
t4 = t4 + s54
t4 = t4 + r05
t4 = t4 + s46
t4 = t4 + s64
t4 = t4 + r14
t4 = t4 + s47
t4 = t4 + s74
t4 = t4 + r15
t4 = t4 + s48
t4 = t4 + s84
t4 = t4 + r24
t4 = t4 + s49
t4 = t4 + s94
c4 = t4 + r25

t5 = s55 + r05
t5 = t5 + s56
t5 = t5 + s65
t5 = t5 + r06
t5 = t5 + s57
t5 = t5 + s75
t5 = t5 + r15
t5 = t5 + s58
t5 = t5 + s85
t5 = t5 + r16
t5 = t5 + s59
t5 = t5 + s95
t5 = t5 + r25
c5 = t5 + r20

t6 = s66 + r06
t6 = t6 + s67
t6 = t6 + s76
t6 = t6 + r07
t6 = t6 + s68
t6 = t6 + s86
t6 = t6 + r16
t6 = t6 + s69
t6 = t6 + s96
t6 = t6 + r17
t6 = t6 + s60
t6 = t6 + s06
c6 = t6 + r30

t7 = s77 + r07
t7 = t7 + s78
t7 = t7 + s87
t7 = t7 + r08
t7 = t7 + s79
t7 = t7 + s97
t7 = t7 + r17
t7 = t7 + s70
t7 = t7 + s07
t7 = t7 + r18
t7 = t7 + s71
t7 = t7 + s17
c7 = t7 + r31

t8 = s88 + r08
t8 = t8 + s89
t8 = t8 + s98
t8 = t8 + r09
t8 = t8 + s80
t8 = t8 + s08
t8 = t8 + r18
t8 = t8 + s81
t8 = t8 + s18
t8 = t8 + r19
t8 = t8 + s82
t8 = t8 + s28
c8 = t8 + r32

t9 = s99 + r09
t9 = t9 + s90
t9 = t9 + s09
t9 = t9 + r00
t9 = t9 + s91
t9 = t9 + s19
t9 = t9 + r19
t9 = t9 + s92
t9 = t9 + s29
t9 = t9 + r10
t9 = t9 + s93
t9 = t9 + s39
c9 = t9 + r33

