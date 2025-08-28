#SHARES 6
#IN a
#RANDOMS r01 r02 r03 r04 r05 r12 r13 r14 r15 r23 r24 r25 r34 r35 r45 
#OUT c

c0 = a0 + r01
c0 = c0 + r02
c0 = c0 + r03
c0 = c0 + r04
c0 = c0 + r05

c1 = a1 + r01
c1 = c1 + r12
c1 = c1 + r13
c1 = c1 + r14
c1 = c1 + r15

c2 = a2 + r02
c2 = c2 + r12
c2 = c2 + r23
c2 = c2 + r24
c2 = c2 + r25

c3 = a3 + r03
c3 = c3 + r13
c3 = c3 + r23
c3 = c3 + r34
c3 = c3 + r35

c4 = a4 + r04
c4 = c4 + r14
c4 = c4 + r24
c4 = c4 + r34
c4 = c4 + r45

c5 = a5 + r05
c5 = c5 + r15
c5 = c5 + r25
c5 = c5 + r35
c5 = c5 + r45

