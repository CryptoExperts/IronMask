#SHARES 2
#IN a b
#RANDOMS r0 r1 rr0 rr1 r2 r3 r4 r5 r6 r7 r8 r9
#OUT f
#CAR 3329

c00 = r0 + a0
c01 = r1 + a1
c02 = c00 + c01

c10 = rr0 + b0
c11 = rr1 + b1
c12 = c10 + c11

c0 = c02 * c12

j0 = 3 rr0 + b0
j1 = 3 rr1 + b1
j2 = j0 + j1

c1 = -1 r0 * j2
c2 = -1 r1 * j2 

k0 = -2 r0 + a0
k1 = -2 r1 + a1
k2 = k0 + k1

c3 = -1 rr0 * k2
c4 = -1 rr1 * k2

d0 = c0 + r2
d2 = c2 + -1 r2
d1 = c1 + r3
d3 = c3 + -1 r3

e0 = d0 + r4
e1 = d1 + -1 r4

e2 = d2 + r5
e3 = d3 + -1 r5
e3 = e3 + r6
e4 = c4 + -1 r6

e2 = e2 + r7
e3 = e3 + -1 r7

e0 = e0 + r8
e2 = e2 + -1 r8
e1 = e1 + r9 
e3 = e3 + -1 r9

f0 = e0 + e2
f1 = e1 + e3
f0 = f0 + e4


