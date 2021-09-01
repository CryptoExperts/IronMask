#SHARES 4
#IN a
#RANDOMS r01 r02 r03 r12 r13 r23 
#OUT c

c0 = a0 + r01
c0 = c0 + r02
c0 = c0 + r03

c1 = a1 + r01
c1 = c1 + r12
c1 = c1 + r13

c2 = a2 + r02
c2 = c2 + r12
c2 = c2 + r23

c3 = a3 + r03
c3 = c3 + r13
c3 = c3 + r23

