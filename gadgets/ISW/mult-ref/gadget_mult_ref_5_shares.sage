#ORDER 4
#SHARES 5
#IN a b 
#RANDOMS r01 r02 r03 r04 r12 r13 r14 r23 r24 r34 rr0 rr1 rr2 rr3 rr4 rr5 rr6 rr7 rr8 rr9 rr10 rr11 rr12 rr13 rr14 rr15 rr16 rr17 rr18 rr19 rr20 rr21 rr22 rr23 rr24 
#OUT c


tmp = rr0 + rr1
h00 = tmp + a0

tmp = rr1 + rr2
h01 = tmp + a1

tmp = rr2 + rr3
h02 = tmp + a2

tmp = rr3 + rr4
h03 = tmp + a3

tmp = rr4 + rr0
h04 = tmp + a4


tmp = rr5 + rr6
h10 = tmp + a0

tmp = rr6 + rr7
h11 = tmp + a1

tmp = rr7 + rr8
h12 = tmp + a2

tmp = rr8 + rr9
h13 = tmp + a3

tmp = rr9 + rr5
h14 = tmp + a4


tmp = rr10 + rr11
h20 = tmp + a0

tmp = rr11 + rr12
h21 = tmp + a1

tmp = rr12 + rr13
h22 = tmp + a2

tmp = rr13 + rr14
h23 = tmp + a3

tmp = rr14 + rr10
h24 = tmp + a4


tmp = rr15 + rr16
h30 = tmp + a0

tmp = rr16 + rr17
h31 = tmp + a1

tmp = rr17 + rr18
h32 = tmp + a2

tmp = rr18 + rr19
h33 = tmp + a3

tmp = rr19 + rr15
h34 = tmp + a4


tmp = rr20 + rr21
h40 = tmp + a0

tmp = rr21 + rr22
h41 = tmp + a1

tmp = rr22 + rr23
h42 = tmp + a2

tmp = rr23 + rr24
h43 = tmp + a3

tmp = rr24 + rr20
h44 = tmp + a4


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

tmp = h30 * b4
r40 = r04 + tmp
tmp = h04 * b0
r40 = r40 + tmp

tmp = h11 * b2
r21 = r12 + tmp
tmp = h12 * b1
r21 = r21 + tmp

tmp = h21 * b3
r31 = r13 + tmp
tmp = h13 * b1
r31 = r31 + tmp

tmp = h31 * b4
r41 = r14 + tmp
tmp = h14 * b1
r41 = r41 + tmp

tmp = h22 * b3
r32 = r23 + tmp
tmp = h23 * b2
r32 = r32 + tmp

tmp = h32 * b4
r42 = r24 + tmp
tmp = h24 * b2
r42 = r42 + tmp

tmp = h33 * b4
r43 = r34 + tmp
tmp = h34 * b3
r43 = r43 + tmp

tmp = h40 * b0
c0 = tmp + r01
c0 = c0 + r02
c0 = c0 + r03
c0 = c0 + r04

tmp = h41 * b1
c1 = tmp + r12
c1 = c1 + r13
c1 = c1 + r14
c1 = c1 + r10

tmp = h42 * b2
c2 = tmp + r23
c2 = c2 + r24
c2 = c2 + r20
c2 = c2 + r21

tmp = h43 * b3
c3 = tmp + r34
c3 = c3 + r30
c3 = c3 + r31
c3 = c3 + r32

tmp = h44 * b4
c4 = tmp + r40
c4 = c4 + r41
c4 = c4 + r42
c4 = c4 + r43

