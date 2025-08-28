#SHARES 6
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 
#OUT d

j0 = a0 + r0
j3 = a3 + r0
j1 = a1 + r1
j4 = a4 + r1
j2 = a2 + r2
j5 = a5 + r2


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
j4 = j4 + r6
j5 = j5


k3 = j3


k4 = j4 + r7
k5 = j5 + r7


k3 = k3 + r8
k4 = k4 + r8
k5 = k5


d0 = k0 + r9
d3 = k3 + r9
d1 = k1 + r10
d4 = k4 + r10
d2 = k2 + r11
d5 = k5 + r11
