#ORDER 5
#IN a b
#SHARES 6
#OUT c
#RANDOMS r00 r01 r02 r03 r04 r05 r06 r07 r08 r09

s00 = a0 * b0
s01 = a0 * b1
s02 = a0 * b2
s03 = a0 * b3
s04 = a0 * b4
s05 = a0 * b5

s10 = a1 * b0
s11 = a1 * b1
s12 = a1 * b2
s13 = a1 * b3
s14 = a1 * b4
s15 = a1 * b5

s20 = a2 * b0
s21 = a2 * b1
s22 = a2 * b2
s23 = a2 * b3
s24 = a2 * b4
s25 = a2 * b5

s30 = a3 * b0
s31 = a3 * b1
s32 = a3 * b2
s33 = a3 * b3
s34 = a3 * b4
s35 = a3 * b5

s40 = a4 * b0
s41 = a4 * b1
s42 = a4 * b2
s43 = a4 * b3
s44 = a4 * b4
s45 = a4 * b5

s50 = a5 * b0
s51 = a5 * b1
s52 = a5 * b2
s53 = a5 * b3
s54 = a5 * b4
s55 = a5 * b5

t = s00 ^ r00
t = t ^ s01
t = t ^ s10
t = t ^ r01
t = t ^ s02
t = t ^ s20
t = t ^ r06
t = t ^ s03
t = t ^ s30
c0 = t ^ r07

t = s11 ^ r01
t = t ^ s12
t = t ^ s21
t = t ^ r02
t = t ^ s13
t = t ^ s31
t = t ^ r07
t = t ^ s14
t = t ^ s41
c1 = t ^ r08

t = s22 ^ r02
t = t ^ s23
t = t ^ s32
t = t ^ r03
t = t ^ s24
t = t ^ s42
t = t ^ r08
t = t ^ s25
t = t ^ s52
c2 = t ^ r09

t = s33 ^ r03
t = t ^ s34
t = t ^ s43
t = t ^ r04
t = t ^ s35
t = t ^ s53
t = t ^ r09
c3 = t ^ r06

t = s44 ^ r04
t = t ^ s45
t = t ^ s54
t = t ^ r05
t = t ^ s40
c4 = t ^ s04

t = s55 ^ r05
t = t ^ s50
t = t ^ s05
t = t ^ r00
t = t ^ s51
c5 = t ^ s15
