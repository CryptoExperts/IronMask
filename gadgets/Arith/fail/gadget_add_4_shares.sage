#SHARES 4
#IN a b
#RANDOMS r01 r02 r03 r12 r13 r23 rr01 rr02 rr03 rr12 rr13 rr23 
#OUT e
#CAR 3329

c0 = a0 + r01
c0 = c0 + r02
c0 = c0 + r03

c1 = a1 + -1 r01
c1 = c1 + r12
c1 = c1 + r13

c2 = a2 + -1 r02
c2 = c2 + -1 r12
c2 = c2 + r23

c3 = a3 + -1 r03
c3 = c3 + -1 r13
c3 = c3 + -1 r23

d0 = b0 + rr01
d0 = d0 + rr02
d0 = d0 + rr03

d1 = b1 + -1 rr01
d1 = d1 + rr12
d1 = d1 + rr13

d2 = b2 + -1 rr02
d2 = d2 + -1 rr12
d2 = d2 + rr23

d3 = b3 + -1 rr03
d3 = d3 + -1 rr13
d3 = d3 + -1 rr23

tmp1 = a0 + a2
tmp2 = a1 + a3



e0 = c0 + d0
e1 = c1 + d1
e2 = c2 + d2
e3 = c3 + d3
