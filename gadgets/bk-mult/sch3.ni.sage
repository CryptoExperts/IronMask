#order 2
#shares 3
#in a b
#out c
#randoms r00 r01 r02

s00 = a0 * b0
s01 = a0 * b1
s02 = a0 * b2

s10 = a1 * b0
s11 = a1 * b1
s12 = a1 * b2

s20 = a2 * b0
s21 = a2 * b1
s22 = a2 * b2


t0 = s00 + r00
t0 = t0 + s01
t0 = t0 + s10
c0 = t0 + r01

t1 = s11 + r01
t1 = t1 + s12
t1 = t1 + s21
c1 = t1 + r02

t2 = s22 + r02
t2 = t2 + s20
t2 = t2 + s02
c2 = t2 + r00

