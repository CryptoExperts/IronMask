#ORDER 5
#SHARES 6
#IN a b
#RANDOMS r01 r02 r03 r04 r05 r12 r13 r14 r15 r23 r24 r25 r34 r35 r45 rr0 rr1 rr2 rr3 rr4 rr5
#OUT c

h0 = rr0 + rr1
h0 = h0 + a0

h1 = rr1 + rr2
h1 = h1 + a1

h2 = rr2 + rr3
h2 = h2 + a2

h3 = rr3 + rr4
h3 = h3 + a3

h4 = rr4 + rr5
h4 = h4 + a4

h5 = rr5 + rr0
h5 = h5 + a5

tmp = h0 * b1
r10 = r01 + tmp
tmp = h1 * b0
r10 = r10 + tmp

tmp = h0 * b2
r20 = r02 + tmp
tmp = h2 * b0
r20 = r20 + tmp

tmp = h0 * b3
r30 = r03 + tmp
tmp = h3 * b0
r30 = r30 + tmp

tmp = h0 * b4
r40 = r04 + tmp
tmp = h4 * b0
r40 = r40 + tmp

tmp = h0 * b5
r50 = r05 + tmp
tmp = h5 * b0
r50 = r50 + tmp

tmp = h1 * b2
r21 = r12 + tmp
tmp = h2 * b1
r21 = r21 + tmp

tmp = h1 * b3
r31 = r13 + tmp
tmp = h3 * b1
r31 = r31 + tmp

tmp = h1 * b4
r41 = r14 + tmp
tmp = h4 * b1
r41 = r41 + tmp

tmp = h1 * b5
r51 = r15 + tmp
tmp = h5 * b1
r51 = r51 + tmp

tmp = h2 * b3
r32 = r23 + tmp
tmp = h3 * b2
r32 = r32 + tmp

tmp = h2 * b4
r42 = r24 + tmp
tmp = h4 * b2
r42 = r42 + tmp

tmp = h2 * b5
r52 = r25 + tmp
tmp = h5 * b2
r52 = r52 + tmp

tmp = h3 * b4
r43 = r34 + tmp
tmp = h4 * b3
r43 = r43 + tmp

tmp = h3 * b5
r53 = r35 + tmp
tmp = h5 * b3
r53 = r53 + tmp

tmp = h4 * b5
r54 = r45 + tmp
tmp = h5 * b4
r54 = r54 + tmp

tmp = h0 * b0
c0 = tmp + r01
c0 = c0 + r02
c0 = c0 + r03
c0 = c0 + r04
c0 = c0 + r05

tmp = h1 * b1
c1 = tmp + r12
c1 = c1 + r13
c1 = c1 + r14
c1 = c1 + r15
c1 = c1 + r10

tmp = h2 * b2
c2 = tmp + r23
c2 = c2 + r24
c2 = c2 + r25
c2 = c2 + r20
c2 = c2 + r21

tmp = h3 * b3
c3 = tmp + r34
c3 = c3 + r35
c3 = c3 + r30
c3 = c3 + r31
c3 = c3 + r32

tmp = h4 * b4
c4 = tmp + r45
c4 = c4 + r40
c4 = c4 + r41
c4 = c4 + r42
c4 = c4 + r43

tmp = h5 * b5
c5 = tmp + r50
c5 = c5 + r51
c5 = c5 + r52
c5 = c5 + r53
c5 = c5 + r54
