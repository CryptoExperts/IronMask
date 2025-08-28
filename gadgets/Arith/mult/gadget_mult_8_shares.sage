#ORDER 7
#SHARES 8
#IN a b
#RANDOMS r01 r02 r03 r04 r05 r06 r07 r12 r13 r14 r15 r16 r17 r23 r24 r25 r26 r27 r34 r35 r36 r37 r45 r46 r47 r56 r57 r67 
#OUT c
#CAR 3329

tmp = a0 * b1
r10 = -1 r01 + tmp
tmp = a1 * b0
r10 = r10 + tmp

tmp = a0 * b2
r20 = -1 r02 + tmp
tmp = a2 * b0
r20 = r20 + tmp

tmp = a0 * b3
r30 = -1 r03 + tmp
tmp = a3 * b0
r30 = r30 + tmp

tmp = a0 * b4
r40 = -1 r04 + tmp
tmp = a4 * b0
r40 = r40 + tmp

tmp = a0 * b5
r50 = -1 r05 + tmp
tmp = a5 * b0
r50 = r50 + tmp

tmp = a0 * b6
r60 = -1 r06 + tmp
tmp = a6 * b0
r60 = r60 + tmp

tmp = a0 * b7
r70 = -1 r07 + tmp
tmp = a7 * b0
r70 = r70 + tmp

tmp = a1 * b2
r21 = -1 r12 + tmp
tmp = a2 * b1
r21 = r21 + tmp

tmp = a1 * b3
r31 = -1 r13 + tmp
tmp = a3 * b1
r31 = r31 + tmp

tmp = a1 * b4
r41 = -1 r14 + tmp
tmp = a4 * b1
r41 = r41 + tmp

tmp = a1 * b5
r51 = -1 r15 + tmp
tmp = a5 * b1
r51 = r51 + tmp

tmp = a1 * b6
r61 = -1 r16 + tmp
tmp = a6 * b1
r61 = r61 + tmp

tmp = a1 * b7
r71 = -1 r17 + tmp
tmp = a7 * b1
r71 = r71 + tmp

tmp = a2 * b3
r32 = -1 r23 + tmp
tmp = a3 * b2
r32 = r32 + tmp

tmp = a2 * b4
r42 = -1 r24 + tmp
tmp = a4 * b2
r42 = r42 + tmp

tmp = a2 * b5
r52 = -1 r25 + tmp
tmp = a5 * b2
r52 = r52 + tmp

tmp = a2 * b6
r62 = -1 r26 + tmp
tmp = a6 * b2
r62 = r62 + tmp

tmp = a2 * b7
r72 = -1 r27 + tmp
tmp = a7 * b2
r72 = r72 + tmp

tmp = a3 * b4
r43 = -1 r34 + tmp
tmp = a4 * b3
r43 = r43 + tmp

tmp = a3 * b5
r53 = -1 r35 + tmp
tmp = a5 * b3
r53 = r53 + tmp

tmp = a3 * b6
r63 = -1 r36 + tmp
tmp = a6 * b3
r63 = r63 + tmp

tmp = a3 * b7
r73 = -1 r37 + tmp
tmp = a7 * b3
r73 = r73 + tmp

tmp = a4 * b5
r54 = -1 r45 + tmp
tmp = a5 * b4
r54 = r54 + tmp

tmp = a4 * b6
r64 = -1 r46 + tmp
tmp = a6 * b4
r64 = r64 + tmp

tmp = a4 * b7
r74 = -1 r47 + tmp
tmp = a7 * b4
r74 = r74 + tmp

tmp = a5 * b6
r65 = -1 r56 + tmp
tmp = a6 * b5
r65 = r65 + tmp

tmp = a5 * b7
r75 = -1 r57 + tmp
tmp = a7 * b5
r75 = r75 + tmp

tmp = a6 * b7
r76 = -1 r67 + tmp
tmp = a7 * b6
r76 = r76 + tmp

tmp = a0 * b0
c0 = tmp + r01
c0 = c0 + r02
c0 = c0 + r03
c0 = c0 + r04
c0 = c0 + r05
c0 = c0 + r06
c0 = c0 + r07

tmp = a1 * b1
c1 = tmp + r10
c1 = c1 + r12
c1 = c1 + r13
c1 = c1 + r14
c1 = c1 + r15
c1 = c1 + r16
c1 = c1 + r17

tmp = a2 * b2
c2 = tmp + r20
c2 = c2 + r21
c2 = c2 + r23
c2 = c2 + r24
c2 = c2 + r25
c2 = c2 + r26
c2 = c2 + r27

tmp = a3 * b3
c3 = tmp + r30
c3 = c3 + r31
c3 = c3 + r32
c3 = c3 + r34
c3 = c3 + r35
c3 = c3 + r36
c3 = c3 + r37

tmp = a4 * b4
c4 = tmp + r40
c4 = c4 + r41
c4 = c4 + r42
c4 = c4 + r43
c4 = c4 + r45
c4 = c4 + r46
c4 = c4 + r47

tmp = a5 * b5
c5 = tmp + r50
c5 = c5 + r51
c5 = c5 + r52
c5 = c5 + r53
c5 = c5 + r54
c5 = c5 + r56
c5 = c5 + r57

tmp = a6 * b6
c6 = tmp + r60
c6 = c6 + r61
c6 = c6 + r62
c6 = c6 + r63
c6 = c6 + r64
c6 = c6 + r65
c6 = c6 + r67

tmp = a7 * b7
c7 = tmp + r70
c7 = c7 + r71
c7 = c7 + r72
c7 = c7 + r73
c7 = c7 + r74
c7 = c7 + r75
c7 = c7 + r76

