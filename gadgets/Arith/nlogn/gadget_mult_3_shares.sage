#SHARES 3
#IN a b
#RANDOMS r0 r1 r2 rr0 rr1 rr2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 r15 r16 r17
#OUT f
#CAR 3329

c00 = r0 + a0
c01 = r1 + a1
c02 = r2 + a2
tmp = c00 + c01
c03 = tmp + c02

c10 = rr0 + b0
c11 = rr1 + b1
c12 = rr2 + b2
tmp = c10 + c11
c13 = tmp + c12

c0 = c03 * c13

j0 = 3 rr0 + b0
j1 = 3 rr1 + b1
j2 = 3 rr2 + b2
tmp = j0 + j1 
j3 = tmp + j2

c1 = -1 r0 * j3
c2 = -1 r1 * j3 
c3 = -1 r2 * j3

k0 = -2 r0 + a0
k1 = -2 r1 + a1
k2 = -2 r2 + a2
tmp = k0 + k1
k3 = tmp + k2

c4 = -1 rr0 * k3
c5 = -1 rr1 * k3
c6 = -1 rr2 * k3

d0 = c0 + r3
d3 = c3 + -1 r3
d1 = c1 + r4
d4 = c4 + -1 r4
d2 = c0 + r5
d5 = c5 + -1 r5

e0 = d0 + r6
e1 = d1 + -1 r6
e1 = e1 + r7
e2 = d2 + -1 r7

e0 = e0 + r8
e1 = e1 + -1 r8


e3 = d3 + r9
e5 = d5 + -1 r9
e4 = d4 + r10
e6 = c6 + -1 r10

e3 = e3 + r11
e4 = e4 + -1 r11
e5 = e5 + r12
e6 = e6 + -1 r12

e3 = e3 + r13
e5 = e5 + -1 r13
e4 = e4 + r14 
e6 = e6 + -1 r14

e0 = e0 + r15
e3 = e3 + -1 r15
e1 = e1 + r16
e4 = e4 + -1 r16
e2 = e2 + r17
e5 = e5 + -1 r17


f0 = e0 + e3
f1 = e1 + e4
f2 = e2 + e5

f0 = f0 + e6


