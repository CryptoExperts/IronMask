#ORDER 4
#SHARES 5
#IN a b
#RANDOMS r01 r02 r03 r04 r12 r13 r14 r23 r24 r34 
#OUT c

tmp = a0 * b1
r10 = r01 + tmp
tmp = a1 * b0
r10 = r10 + tmp

tmp = a0 * b2
r20 = r02 + tmp
tmp = a2 * b0
r20 = r20 + tmp

tmp = a0 * b3
r30 = r03 + tmp
tmp = a3 * b0
r30 = r30 + tmp

tmp = a0 * b4
r40 = r04 + tmp
tmp = a4 * b0
r40 = r40 + tmp

tmp = a1 * b2
r21 = r12 + tmp
tmp = a2 * b1
r21 = r21 + tmp

tmp = a1 * b3
r31 = r13 + tmp
tmp = a3 * b1
r31 = r31 + tmp

tmp = a1 * b4
r41 = r14 + tmp
tmp = a4 * b1
r41 = r41 + tmp

tmp = a2 * b3
r32 = r23 + tmp
tmp = a3 * b2
r32 = r32 + tmp

tmp = a2 * b4
r42 = r24 + tmp
tmp = a4 * b2
r42 = r42 + tmp

tmp = a3 * b4
r43 = r34 + tmp
tmp = a4 * b3
r43 = r43 + tmp

tmp = a0 * b0
c0 = tmp + r01
c0 = c0 + r02
c0 = c0 + r03
c0 = c0 + r04

tmp = a1 * b1
c1 = tmp + r10
c1 = c1 + r12
c1 = c1 + r13
c1 = c1 + r14

tmp = a2 * b2
c2 = tmp + r20
c2 = c2 + r21
c2 = c2 + r23
c2 = c2 + r24

tmp = a3 * b3
c3 = tmp + r30
c3 = c3 + r31
c3 = c3 + r32
c3 = c3 + r34

tmp = a4 * b4
c4 = tmp + r40
c4 = c4 + r41
c4 = c4 + r42
c4 = c4 + r43

