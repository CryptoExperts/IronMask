#ORDER 3
#SHARES 4
#IN a b
#RANDOMS r01 r02 r03 r12 r13 r23 
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

tmp = a1 * b2
r21 = r12 + tmp
tmp = a2 * b1
r21 = r21 + tmp

tmp = a1 * b3
r31 = r13 + tmp
tmp = a3 * b1
r31 = r31 + tmp

tmp = a2 * b3
r32 = r23 + tmp
tmp = a3 * b2
r32 = r32 + tmp

tmp = a0 * b0
c0 = tmp + r01
c0 = c0 + r02
c0 = c0 + r03

tmp = a1 * b1
c1 = tmp + r10
c1 = c1 + r12
c1 = c1 + r13

tmp = a2 * b2
c2 = tmp + r20
c2 = c2 + r21
c2 = c2 + r23

tmp = a3 * b3
c3 = tmp + r30
c3 = c3 + r31
c3 = c3 + r32

