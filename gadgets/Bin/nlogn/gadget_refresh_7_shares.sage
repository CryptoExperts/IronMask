#SHARES 7
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 
#OUT d

j0 = a0 + r0
j3 = a3 + r0
j1 = a1 + r1
j4 = a4 + r1
j2 = a2 + r2
j5 = a5 + r2
j6 = a6


j0 = j0 + r3
j1 = j1 + r3
j2 = j2


k0 = j0


k1 = j1 + r4
k2 = j2 + r4


k0 = k0 + r5
k1 = k1 + r5
k2 = k2


j3 = j3 + r6
j5 = j5 + r6
j4 = j4 + r7
j6 = j6 + r7


k3 = j3 + r8
k4 = j4 + r8


k5 = j5 + r9
k6 = j6 + r9


k3 = k3 + r10
k5 = k5 + r10
k4 = k4 + r11
k6 = k6 + r11


d0 = k0 + r12
d3 = k3 + r12
d1 = k1 + r13
d4 = k4 + r13
d2 = k2 + r14
d5 = k5 + r14
d6 = k6
