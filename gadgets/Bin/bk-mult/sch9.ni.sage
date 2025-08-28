#order 8
#shares 9
#in a b
#out c
#randoms r00 r01 r02 r03 r04 r05 r06 r07 r08 r09 r10 r11 r12 r13 r14 r15 r16 r17

s00 = a0 * b0
s01 = a0 * b1
s02 = a0 * b2
s03 = a0 * b3
s04 = a0 * b4
s05 = a0 * b5
s06 = a0 * b6
s07 = a0 * b7
s08 = a0 * b8

s10 = a1 * b0
s11 = a1 * b1
s12 = a1 * b2
s13 = a1 * b3
s14 = a1 * b4
s15 = a1 * b5
s16 = a1 * b6
s17 = a1 * b7
s18 = a1 * b8

s20 = a2 * b0
s21 = a2 * b1
s22 = a2 * b2
s23 = a2 * b3
s24 = a2 * b4
s25 = a2 * b5
s26 = a2 * b6
s27 = a2 * b7
s28 = a2 * b8

s30 = a3 * b0
s31 = a3 * b1
s32 = a3 * b2
s33 = a3 * b3
s34 = a3 * b4
s35 = a3 * b5
s36 = a3 * b6
s37 = a3 * b7
s38 = a3 * b8

s40 = a4 * b0
s41 = a4 * b1
s42 = a4 * b2
s43 = a4 * b3
s44 = a4 * b4
s45 = a4 * b5
s46 = a4 * b6
s47 = a4 * b7
s48 = a4 * b8

s50 = a5 * b0
s51 = a5 * b1
s52 = a5 * b2
s53 = a5 * b3
s54 = a5 * b4
s55 = a5 * b5
s56 = a5 * b6
s57 = a5 * b7
s58 = a5 * b8

s60 = a6 * b0
s61 = a6 * b1
s62 = a6 * b2
s63 = a6 * b3
s64 = a6 * b4
s65 = a6 * b5
s66 = a6 * b6
s67 = a6 * b7
s68 = a6 * b8

s70 = a7 * b0
s71 = a7 * b1
s72 = a7 * b2
s73 = a7 * b3
s74 = a7 * b4
s75 = a7 * b5
s76 = a7 * b6
s77 = a7 * b7
s78 = a7 * b8

s80 = a8 * b0
s81 = a8 * b1
s82 = a8 * b2
s83 = a8 * b3
s84 = a8 * b4
s85 = a8 * b5
s86 = a8 * b6
s87 = a8 * b7
s88 = a8 * b8


t0 = s00 + r00
t0 = t0 + s01
t0 = t0 + s10
t0 = t0 + r01
t0 = t0 + s02
t0 = t0 + s20
t0 = t0 + r09
t0 = t0 + s03
t0 = t0 + s30
t0 = t0 + r10
t0 = t0 + s04
c0 = t0 + s40

t1 = s11 + r01
t1 = t1 + s12
t1 = t1 + s21
t1 = t1 + r02
t1 = t1 + s13
t1 = t1 + s31
t1 = t1 + r10
t1 = t1 + s14
t1 = t1 + s41
t1 = t1 + r11
t1 = t1 + s15
c1 = t1 + s51

t2 = s22 + r02
t2 = t2 + s23
t2 = t2 + s32
t2 = t2 + r03
t2 = t2 + s24
t2 = t2 + s42
t2 = t2 + r11
t2 = t2 + s25
t2 = t2 + s52
t2 = t2 + r12
t2 = t2 + s26
c2 = t2 + s62

t3 = s33 + r03
t3 = t3 + s34
t3 = t3 + s43
t3 = t3 + r04
t3 = t3 + s35
t3 = t3 + s53
t3 = t3 + r12
t3 = t3 + s36
t3 = t3 + s63
t3 = t3 + r13
t3 = t3 + s37
c3 = t3 + s73

t4 = s44 + r04
t4 = t4 + s45
t4 = t4 + s54
t4 = t4 + r05
t4 = t4 + s46
t4 = t4 + s64
t4 = t4 + r13
t4 = t4 + s47
t4 = t4 + s74
t4 = t4 + r14
t4 = t4 + s48
c4 = t4 + s84

t5 = s55 + r05
t5 = t5 + s56
t5 = t5 + s65
t5 = t5 + r06
t5 = t5 + s57
t5 = t5 + s75
t5 = t5 + r14
t5 = t5 + s58
t5 = t5 + s85
t5 = t5 + r15
t5 = t5 + s50
c5 = t5 + s05

t6 = s66 + r06
t6 = t6 + s67
t6 = t6 + s76
t6 = t6 + r07
t6 = t6 + s68
t6 = t6 + s86
t6 = t6 + r15
t6 = t6 + s60
t6 = t6 + s06
t6 = t6 + r16
t6 = t6 + s61
c6 = t6 + s16

t7 = s77 + r07
t7 = t7 + s78
t7 = t7 + s87
t7 = t7 + r08
t7 = t7 + s70
t7 = t7 + s07
t7 = t7 + r16
t7 = t7 + s71
t7 = t7 + s17
t7 = t7 + r17
t7 = t7 + s72
c7 = t7 + s27

t8 = s88 + r08
t8 = t8 + s80
t8 = t8 + s08
t8 = t8 + r00
t8 = t8 + s81
t8 = t8 + s18
t8 = t8 + r17
t8 = t8 + s82
t8 = t8 + s28
t8 = t8 + r09
t8 = t8 + s83
c8 = t8 + s38

