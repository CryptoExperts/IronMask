#ORDER 3
#SHARES 4
#IN a b 
#RANDOMS r01 r02 r03 r12 r13 r23 rr0 rr1 rr2 rr3 rr4 rr5 rr6 rr7 rr8 rr9 rr10 rr11 rr12 rr13 rr14 rr15 
#OUT c


tmp = rr0 + rr1
h00 = tmp + a0

tmp = rr1 + rr2
h01 = tmp + a1

tmp = rr2 + rr3
h02 = tmp + a2

tmp = rr3 + rr0
h03 = tmp + a3


tmp = rr4 + rr5
h10 = tmp + a0

tmp = rr5 + rr6
h11 = tmp + a1

tmp = rr6 + rr7
h12 = tmp + a2

tmp = rr7 + rr4
h13 = tmp + a3


tmp = rr8 + rr9
h20 = tmp + a0

tmp = rr9 + rr10
h21 = tmp + a1

tmp = rr10 + rr11
h22 = tmp + a2

tmp = rr11 + rr8
h23 = tmp + a3


tmp = rr12 + rr13
h30 = tmp + a0

tmp = rr13 + rr14
h31 = tmp + a1

tmp = rr14 + rr15
h32 = tmp + a2

tmp = rr15 + rr12
h33 = tmp + a3


tmp = h00 * b1
r10 = r01 + tmp
tmp = h01 * b0
r10 = r10 + tmp

tmp = h10 * b2
r20 = r02 + tmp
tmp = h02 * b0
r20 = r20 + tmp

tmp = h20 * b3
r30 = r03 + tmp
tmp = h03 * b0
r30 = r30 + tmp

tmp = h11 * b2
r21 = r12 + tmp
tmp = h12 * b1
r21 = r21 + tmp

tmp = h21 * b3
r31 = r13 + tmp
tmp = h13 * b1
r31 = r31 + tmp

tmp = h22 * b3
r32 = r23 + tmp
tmp = h23 * b2
r32 = r32 + tmp

tmp = h30 * b0
c0 = tmp + r01
c0 = c0 + r02
c0 = c0 + r03

tmp = h31 * b1
c1 = tmp + r12
c1 = c1 + r13
c1 = c1 + r10

tmp = h32 * b2
c2 = tmp + r23
c2 = c2 + r20
c2 = c2 + r21

tmp = h33 * b3
c3 = tmp + r30
c3 = c3 + r31
c3 = c3 + r32

