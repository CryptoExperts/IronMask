#SHARES 8
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11
#OUT d
#CAR 3329

c0 = a0 + r0
c1 = a1 + -1 r0

c2 = a2 + r1
c3 = a3 + -1 r1

c0 = c0 + r2
c2 = c2 + -1 r2
c1 = c1 + r3
c3 = c3 + -1 r3

c4 = a4 + r4
c5 = a5 + -1 r4

c6 = a6 + r5
c7 = a7 + -1 r5

c4 = c4 + r6
c6 = c6 + -1 r6
c5 = c5 + r7
c7 = c7 + -1 r7

d0 = c0 + r8
d4 = c4 + -1 r8
d1 = c1 + r9
d5 = c5 + -1 r9
d2 = c2 + r10
d6 = c6 + -1 r10
d3 = c3 + r11
d7 = c7 + -1 r11
