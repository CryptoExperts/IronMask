#SHARES 6
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6
#OUT d
#CAR 3329

c2 = -1 r0 + -1 r1

c0 = a0 + r1
c1 = a1 + r0
c2 = a2 + c2

c5 = -1 r2 + -1 r3

c3 = a3 + r3
c4 = a4 + r2
c5 = a5 + c5

d0 = c0 + r4
d3 = c3 + -1 r4
d1 = c1 + r5
d4 = c4 + -1 r5
d2 = c2 + r6
d5 = c5 + -1 r6
