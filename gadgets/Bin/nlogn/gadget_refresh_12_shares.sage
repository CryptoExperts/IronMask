#SHARES 12
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 r15 r16 r17 r18 r19 r20 r21 r22 r23 r24 r25 r26 r27 r28 r29 r30 r31 r32 r33 r34 r35 
#OUT d

j0 = a0 + r0
j6 = a6 + r0
j1 = a1 + r1
j7 = a7 + r1
j2 = a2 + r2
j8 = a8 + r2
j3 = a3 + r3
j9 = a9 + r3
j4 = a4 + r4
j10 = a10 + r4
j5 = a5 + r5
j11 = a11 + r5


j0 = j0 + r6
j3 = j3 + r6
j1 = j1 + r7
j4 = j4 + r7
j2 = j2 + r8
j5 = j5 + r8


j0 = j0 + r9
j1 = j1 + r9
j2 = j2


k0 = j0


k1 = j1 + r10
k2 = j2 + r10


k0 = k0 + r11
k1 = k1 + r11
k2 = k2


j3 = j3 + r12
j4 = j4 + r12
j5 = j5


k3 = j3


k4 = j4 + r13
k5 = j5 + r13


k3 = k3 + r14
k4 = k4 + r14
k5 = k5


k0 = k0 + r15
k3 = k3 + r15
k1 = k1 + r16
k4 = k4 + r16
k2 = k2 + r17
k5 = k5 + r17


j6 = j6 + r18
j9 = j9 + r18
j7 = j7 + r19
j10 = j10 + r19
j8 = j8 + r20
j11 = j11 + r20


j6 = j6 + r21
j7 = j7 + r21
j8 = j8


k6 = j6


k7 = j7 + r22
k8 = j8 + r22


k6 = k6 + r23
k7 = k7 + r23
k8 = k8


j9 = j9 + r24
j10 = j10 + r24
j11 = j11


k9 = j9


k10 = j10 + r25
k11 = j11 + r25


k9 = k9 + r26
k10 = k10 + r26
k11 = k11


k6 = k6 + r27
k9 = k9 + r27
k7 = k7 + r28
k10 = k10 + r28
k8 = k8 + r29
k11 = k11 + r29


d0 = k0 + r30
d6 = k6 + r30
d1 = k1 + r31
d7 = k7 + r31
d2 = k2 + r32
d8 = k8 + r32
d3 = k3 + r33
d9 = k9 + r33
d4 = k4 + r34
d10 = k10 + r34
d5 = k5 + r35
d11 = k11 + r35
