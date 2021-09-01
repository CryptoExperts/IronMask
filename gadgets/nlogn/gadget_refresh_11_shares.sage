#SHARES 11
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 r15 r16 r17 r18 r19 r20 r21 r22 r23 r24 r25 r26 r27 r28 r29 
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
j10 = a10


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
j8 = j8 + r13
j6 = j6 + r14
j9 = j9 + r14
j7 = j7 + r15
j10 = j10 + r15


j5 = j5 + r16
j6 = j6 + r16
j7 = j7


k5 = j5


k6 = j6 + r17
k7 = j7 + r17


k5 = k5 + r18
k6 = k6 + r18
k7 = k7


j8 = j8 + r19
j9 = j9 + r19
j10 = j10


k8 = j8


k9 = j9 + r20
k10 = j10 + r20


k8 = k8 + r21
k9 = k9 + r21
k10 = k10


k5 = k5 + r22
k8 = k8 + r22
k6 = k6 + r23
k9 = k9 + r23
k7 = k7 + r24
k10 = k10 + r24


d0 = k0 + r25
d5 = k5 + r25
d1 = k1 + r26
d6 = k6 + r26
d2 = k2 + r27
d7 = k7 + r27
d3 = k3 + r28
d8 = k8 + r28
d4 = k4 + r29
d9 = k9 + r29
d10 = k10
