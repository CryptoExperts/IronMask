#ORDER 3
#SHARES 4
#IN a b 
#RANDOMS r00 r01 r02 r03 r10 r11 r12 r13 r20 r21 r22 r23 r30 r31 r32 r33 rr0 rr1 rr2 rr3 rr4 rr5 rr6 rr7 rr8 rr9 rr10 rr11 rr12 rr13 rr14 rr15 
#OUT c
#CAR 3329


tmp = -1 rr0 + rr1
h00 = tmp + a0

tmp = -1 rr1 + rr2
h01 = tmp + a1

tmp = -1 rr2 + rr3
h02 = tmp + a2

tmp = -1 rr3 + rr0
h03 = tmp + a3


tmp = -1 rr4 + rr5
h10 = tmp + a0

tmp = -1 rr5 + rr6
h11 = tmp + a1

tmp = -1 rr6 + rr7
h12 = tmp + a2

tmp = -1 rr7 + rr4
h13 = tmp + a3


tmp = -1 rr8 + rr9
h20 = tmp + a0

tmp = -1 rr9 + rr10
h21 = tmp + a1

tmp = -1 rr10 + rr11
h22 = tmp + a2

tmp = -1 rr11 + rr8
h23 = tmp + a3


tmp = -1 rr12 + rr13
h30 = tmp + a0

tmp = -1 rr13 + rr14
h31 = tmp + a1

tmp = -1 rr14 + rr15
h32 = tmp + a2

tmp = -1 rr15 + rr12
h33 = tmp + a3


tmp = h00 * b0
p00 = tmp + r00

tmp = h01 * b0
p01 = tmp + r01

tmp = h02 * b0
p02 = tmp + r02

tmp = h03 * b0
p03 = tmp + r03

tmp = h10 * b1
p10 = tmp + r10

tmp = h11 * b1
p11 = tmp + r11

tmp = h12 * b1
p12 = tmp + r12

tmp = h13 * b1
p13 = tmp + r13

tmp = h20 * b2
p20 = tmp + r20

tmp = h21 * b2
p21 = tmp + r21

tmp = h22 * b2
p22 = tmp + r22

tmp = h23 * b2
p23 = tmp + r23

tmp = h30 * b3
p30 = tmp + r30

tmp = h31 * b3
p31 = tmp + r31

tmp = h32 * b3
p32 = tmp + r32

tmp = h33 * b3
p33 = tmp + r33

v0 = p00 + p01
v0 = v0 + p02
v0 = v0 + p03
v1 = p10 + p11
v1 = v1 + p12
v1 = v1 + p13
v2 = p20 + p21
v2 = v2 + p22
v2 = v2 + p23
v3 = p30 + p31
v3 = v3 + p32
v3 = v3 + p33

x0 = -1 r00 + -1 r01
x0 = x0 + -1 r02
x0 = x0 + -1 r03
x1 = -1 r10 + -1 r11
x1 = x1 + -1 r12
x1 = x1 + -1 r13
x2 = -1 r20 + -1 r21
x2 = x2 + -1 r22
x2 = x2 + -1 r23
x3 = -1 r30 + -1 r31
x3 = x3 + -1 r32
x3 = x3 + -1 r33

c0 = v0 + x0
c1 = v1 + x1
c2 = v2 + x2
c3 = v3 + x3
