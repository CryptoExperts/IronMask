#ORDER 10
#SHARES 11
#IN a
#OUT c
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 s0 s1 s2 s3 s4 s5

t0 = r0 + r1
t1 = r1 + r2
t2 = r2 + r3
t3 = r3 + r4
t4 = r4 + r5
t5 = r5 + r6
t6 = r6 + r7
t7 = r7 + r8
t8 = r8 + r9
t9 = r9 + r10
t10 = r10 + r0

t0 = t0 + s0
t1 = t1 + s1
t2 = t2 + s2
t3 = t3 + s3
t4 = t4 + s4
t5 = t5 + s0
t6 = t6 + s1
t7 = t7 + s2
t7 = t7 + s5
t8 = t8 + s3
t9 = t9 + s4
t10 = t10 + s5

c0 = a0 + t0
c1 = a1 + t1
c2 = a2 + t2
c3 = a3 + t3
c4 = a4 + t4
c5 = a5 + t5
c6 = a6 + t6
c7 = a7 + t7
c8 = a8 + t8
c9 = a9 + t9
c10 = a10 + t10