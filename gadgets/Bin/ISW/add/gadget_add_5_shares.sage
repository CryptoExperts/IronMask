#SHARES 5
#IN a b
#RANDOMS r01 r02 r03 r04 r12 r13 r14 r23 r24 r34 rr01 rr02 rr03 rr04 rr12 rr13 rr14 rr23 rr24 rr34 
#OUT e

c0 = a0 + r01
c0 = c0 + r02
c0 = c0 + r03
c0 = c0 + r04

c1 = a1 + r01
c1 = c1 + r12
c1 = c1 + r13
c1 = c1 + r14

c2 = a2 + r02
c2 = c2 + r12
c2 = c2 + r23
c2 = c2 + r24

c3 = a3 + r03
c3 = c3 + r13
c3 = c3 + r23
c3 = c3 + r34

c4 = a4 + r04
c4 = c4 + r14
c4 = c4 + r24
c4 = c4 + r34

d0 = b0 + rr01
d0 = d0 + rr02
d0 = d0 + rr03
d0 = d0 + rr04

d1 = b1 + rr01
d1 = d1 + rr12
d1 = d1 + rr13
d1 = d1 + rr14

d2 = b2 + rr02
d2 = d2 + rr12
d2 = d2 + rr23
d2 = d2 + rr24

d3 = b3 + rr03
d3 = d3 + rr13
d3 = d3 + rr23
d3 = d3 + rr34

d4 = b4 + rr04
d4 = d4 + rr14
d4 = d4 + rr24
d4 = d4 + rr34



e0 = c0 + d0
e1 = c1 + d1
e2 = c2 + d2
e3 = c3 + d3
e4 = c4 + d4
