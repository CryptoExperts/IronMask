#ORDER 1
#SHARES 2
#IN a b
#RANDOMS r0
#OUT c
unused = a0 ^ r0
t = a0 ^ a1
m1 = r0 & b0
m2 = t & b0
nonleaky = m1 ^ m2
c0 = nonleaky
c1 = r0
