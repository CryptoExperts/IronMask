#order 5
#shares 6
#in a b
#out c
#randoms r00 r01 r02 r03 r04 r05 r06 r07 r08 r09

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


t0 = s00 + r00
t0 = t0 + s01
t0 = t0 + s10
t0 = t0 + r01
t0 = t0 + s02
t0 = t0 + s20
t0 = t0 + r06
t0 = t0 + s03
t0 = t0 + s30
c0 = t0 + r07

t1 = s11 + r01
t1 = t1 + s12
t1 = t1 + s21
t1 = t1 + r02
t1 = t1 + s13
t1 = t1 + s31
t1 = t1 + r07
t1 = t1 + s14
t1 = t1 + s41
c1 = t1 + r08

t2 = s22 + r02
t2 = t2 + s23
t2 = t2 + s32
t2 = t2 + r03
t2 = t2 + s24
t2 = t2 + s42
t2 = t2 + r08
t2 = t2 + s25
t2 = t2 + s52
c2 = t2 + r09

t3 = s33 + r03
t3 = t3 + s34
t3 = t3 + s43
t3 = t3 + r04
t3 = t3 + s35
t3 = t3 + s53
t3 = t3 + r09
c3 = t3 + r06

t4 = s44 + r04
t4 = t4 + s45
t4 = t4 + s54
t4 = t4 + r05
t4 = t4 + s40
c4 = t4 + s04

t5 = s55 + r05
t5 = t5 + s50
t5 = t5 + s05
t5 = t5 + r00
t5 = t5 + s51
c5 = t5 + s15

