#order 1
#shares 2
#in a b
#out c
#randoms r00 r01

s00 = a0*b0
s01 = a0*b1

s10 = a1*b0
s11 = a1*b1

t00 = s00 + r00
t01 = t00 + s01
t02 = t01 + s10
c0 = t02 + r01

t1 = s11 + r01
c1 = t1 + r00
