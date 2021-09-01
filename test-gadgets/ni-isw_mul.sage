#ORDER 6
#SHARES 7
#INPUT a b
#OUTPUT c
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 r15 r16 r17 r18 r19 r20


c0 = a0 * b0
c1 = a1 * b1
c2 = a2 * b2
c3 = a3 * b3
c4 = a4 * b4
c5 = a5 * b5
c6 = a6 * b6

c0 = c0 + r0
aibj = a0 * b1
ajbi = a1 * b0
tmp0 = aibj + r0
tmp1 = tmp0 + ajbi
c1 = c1 + tmp1

c0 = c0 + r1
aibj = a0 * b2
ajbi = a2 * b0
tmp0 = aibj + r1
tmp1 = tmp0 + ajbi
c2 = c2 + tmp1

c0 = c0 + r2
aibj = a0 * b3
ajbi = a3 * b0
tmp0 = aibj + r2
tmp1 = tmp0 + ajbi
c3 = c3 + tmp1

c0 = c0 + r3
aibj = a0 * b4
ajbi = a4 * b0
tmp0 = aibj + r3
tmp1 = tmp0 + ajbi
c4 = c4 + tmp1

c0 = c0 + r4
aibj = a0 * b5
ajbi = a5 * b0
tmp0 = aibj + r4
tmp1 = tmp0 + ajbi
c5 = c5 + tmp1

c0 = c0 + r5
aibj = a0 * b6
ajbi = a6 * b0
tmp0 = aibj + r5
tmp1 = tmp0 + ajbi
c6 = c6 + tmp1

c1 = c1 + r6
aibj = a1 * b2
ajbi = a2 * b1
tmp0 = aibj + r6
tmp1 = tmp0 + ajbi
c2 = c2 + tmp1

c1 = c1 + r7
aibj = a1 * b3
ajbi = a3 * b1
tmp0 = aibj + r7
tmp1 = tmp0 + ajbi
c3 = c3 + tmp1

c1 = c1 + r8
aibj = a1 * b4
ajbi = a4 * b1
tmp0 = aibj + r8
tmp1 = tmp0 + ajbi
c4 = c4 + tmp1

c1 = c1 + r9
aibj = a1 * b5
ajbi = a5 * b1
tmp0 = aibj + r9
tmp1 = tmp0 + ajbi
c5 = c5 + tmp1

c1 = c1 + r10
aibj = a1 * b6
ajbi = a6 * b1
tmp0 = aibj + r10
tmp1 = tmp0 + ajbi
c6 = c6 + tmp1

c2 = c2 + r11
aibj = a2 * b3
ajbi = a3 * b2
tmp0 = aibj + r11
tmp1 = tmp0 + ajbi
c3 = c3 + tmp1

c2 = c2 + r12
aibj = a2 * b4
ajbi = a4 * b2
tmp0 = aibj + r12
tmp1 = tmp0 + ajbi
c4 = c4 + tmp1

c2 = c2 + r13
aibj = a2 * b5
ajbi = a5 * b2
tmp0 = aibj + r13
tmp1 = tmp0 + ajbi
c5 = c5 + tmp1

c2 = c2 + r14
aibj = a2 * b6
ajbi = a6 * b2
tmp0 = aibj + r14
tmp1 = tmp0 + ajbi
c6 = c6 + tmp1

c3 = c3 + r15
aibj = a3 * b4
ajbi = a4 * b3
tmp0 = aibj + r15
tmp1 = tmp0 + ajbi
c4 = c4 + tmp1

c3 = c3 + r16
aibj = a3 * b5
ajbi = a5 * b3
tmp0 = aibj + r16
tmp1 = tmp0 + ajbi
c5 = c5 + tmp1

c3 = c3 + r17
aibj = a3 * b6
ajbi = a6 * b3
tmp0 = aibj + r17
tmp1 = tmp0 + ajbi
c6 = c6 + tmp1

c4 = c4 + r18
aibj = a4 * b5
ajbi = a5 * b4
tmp0 = aibj + r18
tmp1 = tmp0 + ajbi
c5 = c5 + tmp1

c4 = c4 + r19
aibj = a4 * b6
ajbi = a6 * b4
tmp0 = aibj + r19
tmp1 = tmp0 + ajbi
c6 = c6 + tmp1

c5 = c5 + r20
aibj = a5 * b6
ajbi = a6 * b5
tmp0 = aibj + r20
tmp1 = tmp0 + ajbi
c6 = c6 + tmp1
