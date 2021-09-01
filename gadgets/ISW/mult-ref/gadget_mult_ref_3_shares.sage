#ORDER 2
#SHARES 3
#IN a b 
#RANDOMS r01 r02 r12 rr0 rr1 rr2 rr3 rr4 rr5 rr6 rr7 rr8 
#OUT c


tmp = rr0 + rr1
h00 = tmp + a0

tmp = rr1 + rr2
h01 = tmp + a1

tmp = rr2 + rr0
h02 = tmp + a2


tmp = rr3 + rr4
h10 = tmp + a0

tmp = rr4 + rr5
h11 = tmp + a1

tmp = rr5 + rr3
h12 = tmp + a2


tmp = rr6 + rr7
h20 = tmp + a0

tmp = rr7 + rr8
h21 = tmp + a1

tmp = rr8 + rr6
h22 = tmp + a2


tmp = h00 * b1
r10 = r01 + tmp
tmp = h01 * b0
r10 = r10 + tmp

tmp = h10 * b2
r20 = r02 + tmp
tmp = h02 * b0
r20 = r20 + tmp

tmp = h11 * b2
r21 = r12 + tmp
tmp = h12 * b1
r21 = r21 + tmp

tmp = h20 * b0
c0 = tmp + r01
c0 = c0 + r02

tmp = h21 * b1
c1 = tmp + r12
c1 = c1 + r10

tmp = h22 * b2
c2 = tmp + r20
c2 = c2 + r21

