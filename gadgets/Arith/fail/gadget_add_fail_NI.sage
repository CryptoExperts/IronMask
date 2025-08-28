#SHARES 3
#IN a b
#RANDOMS r01 r02 r12 rr01 rr02 rr12 
#OUT e
#CAR 3329

c0 = a0 + 7 r01
c0 = c0 + -7 r02

c1 = a1 + -7 r01
c1 = c1 + 7 r12

tmp = c1 + c0
tmp = tmp + 7 r02
tmp = tmp + -7 r12

c2 = a2 + 7 r02
c2 = c2 + -7 r12

d0 = b0 + 7 rr01
d0 = d0 + -7 rr02

d1 = b1 + -7 rr01
d1 = d1 + 7 rr12

d2 = b2 + 7 rr02
d2 = d2 + -7 rr12

e0 = c0 + d0
e1 = c1 + d1
e2 = c2 + d2
