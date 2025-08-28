#ORDER 2
#SHARES 3
#IN a b
#RANDOMS r01 r02 r12 
#OUT c
#CAR 3329

tmp = a0 * b1
r10 = -1 r01 + tmp
tmp = a1 * b0
r10 = r10 + tmp

tmp = a0 * b2
r20 = -1 r02 + tmp
tmp = a2 * b0
r20 = r20 + tmp

tmp = a1 * b2
r21 = -1 r12 + tmp
tmp = a2 * b1
r21 = r21 + tmp

tmp = a0 * b0
c0 = tmp + r01
c0 = c0 + r02

tmp = a1 * b1
c1 = tmp + r10
c1 = c1 + r12

tmp = a2 * b2
c2 = tmp + r20
c2 = c2 + r21

