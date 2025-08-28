#SHARES 7
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8
#OUT d
#CAR 3329

c2 = -1 r0 + -1 r1

c0 = a0 + r1
c1 = a1 + r0
c2 = a2 + c2

c3 = a3 + r2
c4 = a4 + -1 r2

c5 = a5 + r3
c6 = a6 + -1 r3

c3 = c3 + r4
c5 = c5 + -1 r4
c4 = c4 + r5
d6 = c6 + -1 r5

d0 = c0 + r6
d3 = c3 + -1 r6
d1 = c1 + r7
d4 = c4 + -1 r7
d2 = c2 + r8
d5 = c5 + -1 r8
