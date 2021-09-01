#ORDER 1
#SHARES 2
#IN a b
#RANDOMS r01 
#OUT c

tmp = a0 * b1
r10 = r01 + tmp
tmp = a1 * b0
r10 = r10 + tmp

tmp = a0 * b0
c0 = tmp + r01

tmp = a1 * b1
c1 = tmp + r10

