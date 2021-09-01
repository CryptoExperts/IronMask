#SHARES 7
#IN a b
#RANDOMS r01 r02 r03 r04 r05 r06 r12 r13 r14 r15 r16 r23 r24 r25 r26 r34 r35 r36 r45 r46 r56 rr01 rr02 rr03 rr04 rr05 rr06 rr12 rr13 rr14 rr15 rr16 rr23 rr24 rr25 rr26 rr34 rr35 rr36 rr45 rr46 rr56 
#OUT e

c0 = a0 + r01
c0 = c0 + r02
c0 = c0 + r03
c0 = c0 + r04
c0 = c0 + r05
c0 = c0 + r06

c1 = a1 + r01
c1 = c1 + r12
c1 = c1 + r13
c1 = c1 + r14
c1 = c1 + r15
c1 = c1 + r16

c2 = a2 + r02
c2 = c2 + r12
c2 = c2 + r23
c2 = c2 + r24
c2 = c2 + r25
c2 = c2 + r26

c3 = a3 + r03
c3 = c3 + r13
c3 = c3 + r23
c3 = c3 + r34
c3 = c3 + r35
c3 = c3 + r36

c4 = a4 + r04
c4 = c4 + r14
c4 = c4 + r24
c4 = c4 + r34
c4 = c4 + r45
c4 = c4 + r46

c5 = a5 + r05
c5 = c5 + r15
c5 = c5 + r25
c5 = c5 + r35
c5 = c5 + r45
c5 = c5 + r56

c6 = a6 + r06
c6 = c6 + r16
c6 = c6 + r26
c6 = c6 + r36
c6 = c6 + r46
c6 = c6 + r56

d0 = b0 + rr01
d0 = d0 + rr02
d0 = d0 + rr03
d0 = d0 + rr04
d0 = d0 + rr05
d0 = d0 + rr06

d1 = b1 + rr01
d1 = d1 + rr12
d1 = d1 + rr13
d1 = d1 + rr14
d1 = d1 + rr15
d1 = d1 + rr16

d2 = b2 + rr02
d2 = d2 + rr12
d2 = d2 + rr23
d2 = d2 + rr24
d2 = d2 + rr25
d2 = d2 + rr26

d3 = b3 + rr03
d3 = d3 + rr13
d3 = d3 + rr23
d3 = d3 + rr34
d3 = d3 + rr35
d3 = d3 + rr36

d4 = b4 + rr04
d4 = d4 + rr14
d4 = d4 + rr24
d4 = d4 + rr34
d4 = d4 + rr45
d4 = d4 + rr46

d5 = b5 + rr05
d5 = d5 + rr15
d5 = d5 + rr25
d5 = d5 + rr35
d5 = d5 + rr45
d5 = d5 + rr56

d6 = b6 + rr06
d6 = d6 + rr16
d6 = d6 + rr26
d6 = d6 + rr36
d6 = d6 + rr46
d6 = d6 + rr56



e0 = c0 + d0
e1 = c1 + d1
e2 = c2 + d2
e3 = c3 + d3
e4 = c4 + d4
e5 = c5 + d5
e6 = c6 + d6
