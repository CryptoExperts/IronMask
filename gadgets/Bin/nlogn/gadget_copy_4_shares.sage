#SHARES 4
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 
#OUT d e

j0 = a0 + r0
j2 = a2 + r0
j1 = a1 + r1
j3 = a3 + r1


k0 = j0 + r2
k1 = j1 + r2


k2 = j2 + r3
k3 = j3 + r3


d0 = k0 + r4
d2 = k2 + r4
d1 = k1 + r5
d3 = k3 + r5


j0 = a0 + r6
j2 = a2 + r6
j1 = a1 + r7
j3 = a3 + r7


k0 = j0 + r8
k1 = j1 + r8


k2 = j2 + r9
k3 = j3 + r9


e0 = k0 + r10
e2 = k2 + r10
e1 = k1 + r11
e3 = k3 + r11
