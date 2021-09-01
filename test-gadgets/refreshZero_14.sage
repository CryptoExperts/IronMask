#ORDER 14
#SHARES 15
#INPUT a
#OUTPUT c
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 rr0 rr1 rr2 rr3 rr4 rr5 rr6 rr7 rr8 rr9 rr10 rr11 rr12 rr13 rr14

t0 = r0 ^ r1
t1 = r1 ^ r2
t2 = r2 ^ r3
t3 = r3 ^ r4
t4 = r4 ^ r5
t5 = r5 ^ r6
t6 = r6 ^ r7
t7 = r7 ^ r8
t8 = r8 ^ r9
t9 = r9 ^ r10
t10 = r10 ^ r11
t11 = r11 ^ r12
t12 = r12 ^ r13
t13 = r13 ^ r14
t14 = r14 ^ r0

u0 = rr0 ^ rr4
u1 = rr1 ^ rr5
u2 = rr2 ^ rr6
u3 = rr3 ^ rr7
u4 = rr4 ^ rr8
u5 = rr5 ^ rr9
u6 = rr6 ^ rr10
u7 = rr7 ^ rr11
u8 = rr8 ^ rr12
u9 = rr9 ^ rr13
u10 = rr10 ^ rr14
u11 = rr11 ^ rr0
u12 = rr12 ^ rr1
u13 = rr13 ^ rr2
u14 = rr14 ^ rr3

v0 = t0 ^ u0
v1 = t1 ^ u1
v2 = t2 ^ u2
v3 = t3 ^ u3
v4 = t4 ^ u4
v5 = t5 ^ u5
v6 = t6 ^ u6
v7 = t7 ^ u7
v8 = t8 ^ u8
v9 = t9 ^ u9
v10 = t10 ^ u10
v11 = t11 ^ u11
v12 = t12 ^ u12
v13 = t13 ^ u13
v14 = t14 ^ u14


c0 = v0 ^ a0
c1 = v1 ^ a1
c2 = v2 ^ a2
c3 = v3 ^ a3
c4 = v4 ^ a4
c5 = v5 ^ a5
c6 = v6 ^ a6
c7 = v7 ^ a7
c8 = v8 ^ a8
c9 = v9 ^ a9
c10 = v10 ^ a10
c11 = v11 ^ a11
c12 = v12 ^ a12
c13 = v13 ^ a13
c14 = v14 ^ a14