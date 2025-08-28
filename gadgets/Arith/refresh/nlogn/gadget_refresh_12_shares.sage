#SHARES 12
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 r15 r16 r17 r18 r19
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

c0 = c0 + r4
c3 = c3 + -1 r4
c1 = c1 + r5
c4 = c4 + -1 r5
c2 = c2 + r6
c5 = c5 + -1 r6

c8 = -1 r7 + -1 r8

c6 = a6 + r8
c7 = a7 + r7
c8 = a8 + c8

c11 = -1 r9 + -1 r10

c9 = a9 + r10
c10 = a10 + r9
c11 = a11 + c11

c6 = c6 + r11
c9 = c9 + -1 r11
c7 = c7 + r12
c10 = c10 + -1 r12
c8 = c8 + r13
c11 = c11 + -1 r13

d0 = c0 + r14
d6 = c6 + -1 r14
d1 = c1 + r15
d7 = c7 + -1 r15
d2 = c2 + r16
d8 = c8 + -1 r16
d3 = c3 + r17
d9 = c9 + -1 r17
d4 = c4 + r18
d10 = c10 + -1 r18
d5 = c5 + r19
d11 = c11 + -1 r19
