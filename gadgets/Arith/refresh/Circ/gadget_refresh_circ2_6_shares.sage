#SHARES 6
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11
#OUT d
#CAR 3329

t0 =  r0 + -1 r1
t1 =  r1 + -1 r2
t2 =  r2 + -1 r3
t3 =  r3 + -1 r4
t4 =  r4 + -1 r5
t5 =  r5 + -1 r0

u0 =  r6 + -1 r8
u1 =  r7 + -1 r9
u2 =  r8 + -1 r10
u3 =  r9 + -1 r11
u4 =  r10 + -1 r6
u5 =  r11 + -1 r7

v0 = t0 + u0
v1 = t1 + u1
v2 = t2 + u2
v3 = t3 + u3
v4 = t4 + u4
v5 = t5 + u5

d0 = a0 + v0
d1 = a1 + v1
d2 = a2 + v2
d3 = a3 + v3
d4 = a4 + v4
d5 = a5 + v5
