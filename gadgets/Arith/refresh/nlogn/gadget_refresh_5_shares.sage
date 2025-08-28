#SHARES 5
#IN a
#RANDOMS r0 r1 r2 r3 r4
#OUT d
#CAR 3329

c0 = a0 + r0
c1 = a1 + -1 r0

c4 = -1 r1 + -1 r2

c2 = a2 + r2
c3 = a3 + r1
d4 = a4 + c4

d0 = c0 + r3
d2 = c2 + -1 r3
d1 = c1 + r4
d3 = c3 + -1 r4
