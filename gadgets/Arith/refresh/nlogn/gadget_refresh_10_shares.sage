#SHARES 10
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14
#OUT d
#CAR 3329

c0 = a0 + r0
c1 = a1 + -1 r0

c4 = -1 r1 + -1 r2

c2 = a2 + r2
c3 = a3 + r1
c4 = a4 + c4

c0 = c0 + r3
c2 = c2 + -1 r3
c1 = c1 + r4
c3 = c3 + -1 r4
c4 = c4

c5 = a5 + r5
c6 = a6 + -1 r5

c9 = -1 r6 + -1 r7

c7 = a7 + r7
c8 = a8 + r6
c9 = a9 + c9

c5 = c5 + r8
c7 = c7 + -1 r8
c6 = c6 + r9
c8 = c8 + -1 r9
c9 = c9

d0 = c0 + r10
d5 = c5 + -1 r10
d1 = c1 + r11
d6 = c6 + -1 r11
d2 = c2 + r12
d7 = c7 + -1 r12
d3 = c3 + r13
d8 = c8 + -1 r13
d4 = c4 + r14
d9 = c9 + -1 r14
