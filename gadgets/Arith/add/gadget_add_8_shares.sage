#SHARES 8
#IN a b
#RANDOMS r01 r02 r03 r04 r05 r06 r07 r12 r13 r14 r15 r16 r17 r23 r24 r25 r26 r27 r34 r35 r36 r37 r45 r46 r47 r56 r57 r67 rr01 rr02 rr03 rr04 rr05 rr06 rr07 rr12 rr13 rr14 rr15 rr16 rr17 rr23 rr24 rr25 rr26 rr27 rr34 rr35 rr36 rr37 rr45 rr46 rr47 rr56 rr57 rr67 
#OUT e
#CAR 3329

c0 = a0 + r01
c0 = c0 + r02
c0 = c0 + r03
c0 = c0 + r04
c0 = c0 + r05
c0 = c0 + r06
c0 = c0 + r07

c1 = a1 + -1 r01
c1 = c1 + r12
c1 = c1 + r13
c1 = c1 + r14
c1 = c1 + r15
c1 = c1 + r16
c1 = c1 + r17

c2 = a2 + -1 r02
c2 = c2 + -1 r12
c2 = c2 + r23
c2 = c2 + r24
c2 = c2 + r25
c2 = c2 + r26
c2 = c2 + r27

c3 = a3 + -1 r03
c3 = c3 + -1 r13
c3 = c3 + -1 r23
c3 = c3 + r34
c3 = c3 + r35
c3 = c3 + r36
c3 = c3 + r37

c4 = a4 + -1 r04
c4 = c4 + -1 r14
c4 = c4 + -1 r24
c4 = c4 + -1 r34
c4 = c4 + r45
c4 = c4 + r46
c4 = c4 + r47

c5 = a5 + -1 r05
c5 = c5 + -1 r15
c5 = c5 + -1 r25
c5 = c5 + -1 r35
c5 = c5 + -1 r45
c5 = c5 + r56
c5 = c5 + r57

c6 = a6 + -1 r06
c6 = c6 + -1 r16
c6 = c6 + -1 r26
c6 = c6 + -1 r36
c6 = c6 + -1 r46
c6 = c6 + -1 r56
c6 = c6 + r67

c7 = a7 + -1 r07
c7 = c7 + -1 r17
c7 = c7 + -1 r27
c7 = c7 + -1 r37
c7 = c7 + -1 r47
c7 = c7 + -1 r57
c7 = c7 + -1 r67

d0 = b0 + rr01
d0 = d0 + rr02
d0 = d0 + rr03
d0 = d0 + rr04
d0 = d0 + rr05
d0 = d0 + rr06
d0 = d0 + rr07

d1 = b1 + -1 rr01
d1 = d1 + rr12
d1 = d1 + rr13
d1 = d1 + rr14
d1 = d1 + rr15
d1 = d1 + rr16
d1 = d1 + rr17

d2 = b2 + -1 rr02
d2 = d2 + -1 rr12
d2 = d2 + rr23
d2 = d2 + rr24
d2 = d2 + rr25
d2 = d2 + rr26
d2 = d2 + rr27

d3 = b3 + -1 rr03
d3 = d3 + -1 rr13
d3 = d3 + -1 rr23
d3 = d3 + rr34
d3 = d3 + rr35
d3 = d3 + rr36
d3 = d3 + rr37

d4 = b4 + -1 rr04
d4 = d4 + -1 rr14
d4 = d4 + -1 rr24
d4 = d4 + -1 rr34
d4 = d4 + rr45
d4 = d4 + rr46
d4 = d4 + rr47

d5 = b5 + -1 rr05
d5 = d5 + -1 rr15
d5 = d5 + -1 rr25
d5 = d5 + -1 rr35
d5 = d5 + -1 rr45
d5 = d5 + rr56
d5 = d5 + rr57

d6 = b6 + -1 rr06
d6 = d6 + -1 rr16
d6 = d6 + -1 rr26
d6 = d6 + -1 rr36
d6 = d6 + -1 rr46
d6 = d6 + -1 rr56
d6 = d6 + rr67

d7 = b7 + -1 rr07
d7 = d7 + -1 rr17
d7 = d7 + -1 rr27
d7 = d7 + -1 rr37
d7 = d7 + -1 rr47
d7 = d7 + -1 rr57
d7 = d7 + -1 rr67



e0 = c0 + d0
e1 = c1 + d1
e2 = c2 + d2
e3 = c3 + d3
e4 = c4 + d4
e5 = c5 + d5
e6 = c6 + d6
e7 = c7 + d7
