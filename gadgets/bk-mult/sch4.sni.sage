#order 3
#shares 4
#in a b
#out c
#randoms r00 r01 r02 r03 r10

s00 = a0 * b0
s01 = a0 * b1
s02 = a0 * b2
s03 = a0 * b3

s10 = a1 * b0
s11 = a1 * b1
s12 = a1 * b2
s13 = a1 * b3

s20 = a2 * b0
s21 = a2 * b1
s22 = a2 * b2
s23 = a2 * b3

s30 = a3 * b0
s31 = a3 * b1
s32 = a3 * b2
s33 = a3 * b3


t0 = s00 + r00
t0 = t0 + s01
t0 = t0 + s10
t0 = t0 + r01
t0 = t0 + s02
t0 = t0 + s20
c0 = t0 + r10

t1 = s11 + r01
t1 = t1 + s12
t1 = t1 + s21
t1 = t1 + r02
t1 = t1 + s13
t1 = t1 + s31
c1 = t1 + r10

t2 = s22 + r02
t2 = t2 + s23
t2 = t2 + s32
t2 = t2 + r03
c2 = t2 + r10

t3 = s33 + r03
t3 = t3 + s30
t3 = t3 + s03
t3 = t3 + r00
c3 = t3 + r10

