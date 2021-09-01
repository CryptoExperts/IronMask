#SHARES 4
#IN a b
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 
#OUT d

j0 = a0 + r0
j2 = a2 + r0
j1 = a1 + r1
j3 = a3 + r1


k0 = j0 + r2
k1 = j1 + r2


k2 = j2 + r3
k3 = j3 + r3


e0 = k0 + r4
e2 = k2 + r4
e1 = k1 + r5
e3 = k3 + r5


j0 = b0 + r6
j2 = b2 + r6
j1 = b1 + r7
j3 = b3 + r7


k0 = j0 + r8
k1 = j1 + r8


k2 = j2 + r9
k3 = j3 + r9


f0 = k0 + r10
f2 = k2 + r10
f1 = k1 + r11
f3 = k3 + r11


d0 = e0 + f0
d1 = e1 + f1
d2 = e2 + f2
d3 = e3 + f3
