#ORDER 2
#SHARES 3
#IN a b 
#RANDOMS r00 r01 r02 r10 r11 r12 r20 r21 r22 rr0 rr1 rr2 rr3 rr4 rr5 rr6 rr7 rr8 
#OUT c
#CAR 3329


tmp = -1 rr0 + rr1
h00 = tmp + a0

tmp = -1 rr1 + rr2
h01 = tmp + a1

tmp = -1 rr2 + rr0
h02 = tmp + a2


tmp = -1 rr3 + rr4
h10 = tmp + a0

tmp = -1 rr4 + rr5
h11 = tmp + a1

tmp = -1 rr5 + rr3
h12 = tmp + a2


tmp = -1 rr6 + rr7
h20 = tmp + a0

tmp = -1 rr7 + rr8
h21 = tmp + a1

tmp = -1 rr8 + rr6
h22 = tmp + a2


tmp = h00 * b0
p00 = tmp + r00

tmp = h01 * b0
p01 = tmp + r01

tmp = h02 * b0
p02 = tmp + r02

tmp = h10 * b1
p10 = tmp + r10

tmp = h11 * b1
p11 = tmp + r11

tmp = h12 * b1
p12 = tmp + r12

tmp = h20 * b2
p20 = tmp + r20

tmp = h21 * b2
p21 = tmp + r21

tmp = h22 * b2
p22 = tmp + r22

v0 = p00 + p10
v0 = v0 + p11
v1 = p01 + p21
v1 = v1 + p22
v2 = p02 + p20
v2 = v2 + p12

x0 = -1 r00 + -1 r10
x0 = x0 + -1 r11
x1 = -1 r01 + -1 r21
x1 = x1 + -1 r22
x2 = -1 r02 + -1 r20
x2 = x2 + -1 r12

c0 = v0 + x0
c1 = v1 + x1
c2 = v2 + x2
