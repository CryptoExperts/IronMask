#SHARES 10
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 r15 r16 r17 r18 r19 r20 r21 r22 r23 r24 r25 
#OUT d

j0 = a0 + r0
j5 = a5 + r0
j1 = a1 + r1
j6 = a6 + r1
j2 = a2 + r2
j7 = a7 + r2
j3 = a3 + r3
j8 = a8 + r3
j4 = a4 + r4
j9 = a9 + r4


j0 = j0 + r5
j2 = j2 + r5
j1 = j1 + r6
j3 = j3 + r6
j4 = j4


k0 = j0 + r7
k1 = j1 + r7


j2 = j2 + r8
j3 = j3 + r8
j4 = j4


k2 = j2


k3 = j3 + r9
k4 = j4 + r9


k2 = k2 + r10
k3 = k3 + r10
k4 = k4


k0 = k0 + r11
k2 = k2 + r11
k1 = k1 + r12
k3 = k3 + r12
k4 = k4


j5 = j5 + r13
j7 = j7 + r13
j6 = j6 + r14
j8 = j8 + r14
j9 = j9


k5 = j5 + r15
k6 = j6 + r15


j7 = j7 + r16
j8 = j8 + r16
j9 = j9


k7 = j7


k8 = j8 + r17
k9 = j9 + r17


k7 = k7 + r18
k8 = k8 + r18
k9 = k9


k5 = k5 + r19
k7 = k7 + r19
k6 = k6 + r20
k8 = k8 + r20
k9 = k9


d0 = k0 + r21
d5 = k5 + r21
d1 = k1 + r22
d6 = k6 + r22
d2 = k2 + r23
d7 = k7 + r23
d3 = k3 + r24
d8 = k8 + r24
d4 = k4 + r25
d9 = k9 + r25
