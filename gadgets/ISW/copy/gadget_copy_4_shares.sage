#SHARES 4
#IN a
#RANDOMS r01 r02 r03 r12 r13 r23 rr01 rr02 rr03 rr12 rr13 rr23 
#OUT c d

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

d0 = a0 + rr01
d0 = d0 + rr02
d0 = d0 + rr03

d1 = a1 + rr01
d1 = d1 + rr12
d1 = d1 + rr13

d2 = a2 + rr02
d2 = d2 + rr12
d2 = d2 + rr23

d3 = a3 + rr03
d3 = d3 + rr13
d3 = d3 + rr23

