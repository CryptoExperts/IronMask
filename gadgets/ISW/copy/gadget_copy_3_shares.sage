#SHARES 3
#IN a
#RANDOMS r01 r02 r12 rr01 rr02 rr12 
#OUT c d

c0 = a0 + r01
c0 = c0 + r02

c1 = a1 + r01
c1 = c1 + r12

c2 = a2 + r02
c2 = c2 + r12

d0 = a0 + rr01
d0 = d0 + rr02

d1 = a1 + rr01
d1 = d1 + rr12

d2 = a2 + rr02
d2 = d2 + rr12

