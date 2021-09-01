#SHARES 8
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 r15 r16 r17 r18 r19 
#OUT d

j0 = a0 + r0
j4 = a4 + r0
j1 = a1 + r1
j5 = a5 + r1
j2 = a2 + r2
j6 = a6 + r2
j3 = a3 + r3
j7 = a7 + r3


j0 = j0 + r4
j2 = j2 + r4
j1 = j1 + r5
j3 = j3 + r5


k0 = j0 + r6
k1 = j1 + r6


k2 = j2 + r7
k3 = j3 + r7


k0 = k0 + r8
k2 = k2 + r8
k1 = k1 + r9
k3 = k3 + r9


j4 = j4 + r10
j6 = j6 + r10
j5 = j5 + r11
j7 = j7 + r11


k4 = j4 + r12
k5 = j5 + r12


k6 = j6 + r13
k7 = j7 + r13


k4 = k4 + r14
k6 = k6 + r14
k5 = k5 + r15
k7 = k7 + r15


d0 = k0 + r16
d4 = k4 + r16
d1 = k1 + r17
d5 = k5 + r17
d2 = k2 + r18
d6 = k6 + r18
d3 = k3 + r19
d7 = k7 + r19
