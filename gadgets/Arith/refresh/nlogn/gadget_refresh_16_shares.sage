#SHARES 16
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 r15 r16 r17 r18 r19 r20 r21 r22 r23 r24 r25 r26 r27 r28 r29 r30 r31
#OUT d
#CAR 3329

c0 = a0 + r0
c1 = a1 + -1 r0

c2 = a2 + r1
c3 = a3 + -1 r1

c0 = c0 + r2
c2 = c2 + -1 r2
c1 = c1 + r3
c3 = c3 + -1 r3

c4 = a4 + r4
c5 = a5 + -1 r4

c6 = a6 + r5
c7 = a7 + -1 r5

c4 = c4 + r6
c6 = c6 + -1 r6
c5 = c5 + r7
c7 = c7 + -1 r7

c0 = c0 + r8
c4 = c4 + -1 r8
c1 = c1 + r9
c5 = c5 + -1 r9
c2 = c2 + r10
c6 = c6 + -1 r10
c3 = c3 + r11
c7 = c7 + -1 r11

c8 = a8 + r12
c9 = a9 + -1 r12

c10 = a10 + r13
c11 = a11 + -1 r13

c8 = c8 + r14
c10 = c10 + -1 r14
c9 = c9 + r15
c11 = c11 + -1 r15

c12 = a12 + r16
c13 = a13 + -1 r16

c14 = a14 + r17
c15 = a15 + -1 r17

c12 = c12 + r18
c14 = c14 + -1 r18
c13 = c13 + r19
c15 = c15 + -1 r19

c8 = c8 + r20
c12 = c12 + -1 r20
c9 = c9 + r21
c13 = c13 + -1 r21
c10 = c10 + r22
c14 = c14 + -1 r22
c11 = c11 + r23
c15 = c15 + -1 r23

d0 = c0 + r24
d8 = c8 + -1 r24
d1 = c1 + r25
d9 = c9 + -1 r25
d2 = c2 + r26
d10 = c10 + -1 r26
d3 = c3 + r27
d11 = c11 + -1 r27
d4 = c4 + r28
d12 = c12 + -1 r28
d5 = c5 + r29
d13 = c13 + -1 r29
d6 = c6 + r30
d14 = c14 + -1 r30
d7 = c7 + r31
d15 = c15 + -1 r31
