#ORDER 1
#SHARES 2
#IN a b 
#RANDOMS r00 r01 r10 r11 rr0 rr1 rr2 rr3 
#OUT c
#CAR 3329


tmp = -1 rr0 + rr1
h00 = tmp + a0

tmp = -1 rr1 + rr0
h01 = tmp + a1


tmp = -1 rr2 + rr3
h10 = tmp + a0

tmp = -1 rr3 + rr2
h11 = tmp + a1


tmp = h00 * b0
p00 = tmp + r00

tmp = h01 * b0
p01 = tmp + r01

tmp = h10 * b1
p10 = tmp + r10

tmp = h11 * b1
p11 = tmp + r11

v0 = p00 + p01
v1 = p10 + p11

x0 = -1 r00 + -1 r01
x1 = -1 r10 + -1 r11

c0 = v0 + x0
c1 = v1 + x1
