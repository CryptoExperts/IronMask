#SHARES 8
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 r15
#OUT d
#CAR 3329

z0 = r0 + r3
z1 = -1 r0 + r2
z2 = r1 + -1 r2
z3 = -1 r1 + -1 r3

z4 = r4 + r7
z5 = -1 r4 + r6
z6 = r5 + -1 r6
z7 = -1 r5 + -1 r7


c0 = a0 + z0
c1 = a1 + z1
c2 = a2 + z2
c3 = a3 + z3

c4 = a4 + z4
c5 = a5 + z5
c6 = a6 + z6
c7 = a7 + z7

t0 = r8 + r11
t1 = -1 r8 + r10
t2 = r9 + -1 r10
t3 = -1 r9 + -1 r11

t4 = r12 + r15
t5 = -1 r12 + r14
t6 = r13 + -1 r14
t7 = -1 r13 + -1 r15

d0 = c0 + t0
d1 = c1 + t1
d2 = c2 + t4
d3 = c3 + t5

d4 = c4 + t2
d5 = c5 + t3
d6 = c6 + t6
d7 = c7 + t7

