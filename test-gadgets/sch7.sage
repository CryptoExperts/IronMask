#ORDER 6
#SHARES 7
#IN a b
#RANDOMS r00 r01 r02 r03 r04 r05 r06 r07 r08 r09 r10 r11 r12 r13
#OUT c

s00 = a0*b0
s01 = a0*b1
s02 = a0*b2
s03 = a0*b3
s04 = a0*b4
s05 = a0*b5
s06 = a0*b6
s10 = a1*b0
s11 = a1*b1
s12 = a1*b2
s13 = a1*b3
s14 = a1*b4
s15 = a1*b5
s16 = a1*b6
s20 = a2*b0
s21 = a2*b1
s22 = a2*b2
s23 = a2*b3
s24 = a2*b4
s25 = a2*b5
s26 = a2*b6
s30 = a3*b0
s31 = a3*b1
s32 = a3*b2
s33 = a3*b3
s34 = a3*b4
s35 = a3*b5
s36 = a3*b6
s40 = a4*b0
s41 = a4*b1
s42 = a4*b2
s43 = a4*b3
s44 = a4*b4
s45 = a4*b5
s46 = a4*b6
s50 = a5*b0
s51 = a5*b1
s52 = a5*b2
s53 = a5*b3
s54 = a5*b4
s55 = a5*b5
s56 = a5*b6
s60 = a6*b0
s61 = a6*b1
s62 = a6*b2
s63 = a6*b3
s64 = a6*b4
s65 = a6*b5
s66 = a6*b6

aux00 = s00 + r00
aux01 = aux00 + s01
aux02 = aux01 + s10
aux03 = aux02 + r01
aux04 = aux03 + s02
aux05 = aux04 + s20
aux06 = aux05 + r07
aux07 = aux06 + s03
aux08 = aux07 + s30
c0  = aux08 + r08

aux10 = s11 + r01
aux11 = aux10 + s12
aux12 = aux11 + s21
aux13 = aux12 + r02
aux14 = aux13 + s13
aux15 = aux14 + s31
aux16 = aux15 + r08
aux17 = aux16 + s14
aux18 = aux17 + s41
c1  = aux18 + r09

aux20 = s22 + r02
aux21 = aux20 + s23
aux22 = aux21 + s32
aux23 = aux22 + r03
aux24 = aux23 + s24
aux25 = aux24 + s42
aux26 = aux25 + r09
aux27 = aux26 + s25
aux28 = aux27 + s52
c2  = aux28 + r10

aux30 = s33 + r03
aux31 = aux30 + s34
aux32 = aux31 + s43
aux33 = aux32 + r04
aux34 = aux33 + s35
aux35 = aux34 + s53
aux36 = aux35 + r10
aux37 = aux36 + s36
aux38 = aux37 + s63
c3  = aux38 + r11

aux40 = s44 + r04
aux41 = aux40 + s45
aux42 = aux41 + s54
aux43 = aux42 + r05
aux44 = aux43 + s46
aux45 = aux44 + s64
aux46 = aux45 + r11
aux47 = aux46 + s40
aux48 = aux47 + s04
c4  = aux48 + r12

aux50 = s55 + r05
aux51 = aux50 + s56
aux52 = aux51 + s65
aux53 = aux52 + r06
aux54 = aux53 + s50
aux55 = aux54 + s05
aux56 = aux55 + r12
aux57 = aux56 + s51
aux58 = aux57 + s15
c5  = aux58 + r13

aux60 = s66 + r06
aux61 = aux60 + s60
aux62 = aux61 + s06
aux63 = aux62 + r00
aux64 = aux63 + s61
aux65 = aux64 + s16
aux66 = aux65 + r13
aux67 = aux66 + s62
aux68 = aux67 + s26
c6  = aux68 + r07