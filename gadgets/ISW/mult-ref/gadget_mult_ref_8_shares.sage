#ORDER 7
#SHARES 8
#IN a b 
#RANDOMS r01 r02 r03 r04 r05 r06 r07 r12 r13 r14 r15 r16 r17 r23 r24 r25 r26 r27 r34 r35 r36 r37 r45 r46 r47 r56 r57 r67 rr0 rr1 rr2 rr3 rr4 rr5 rr6 rr7 rr8 rr9 rr10 rr11 rr12 rr13 rr14 rr15 rr16 rr17 rr18 rr19 rr20 rr21 rr22 rr23 rr24 rr25 rr26 rr27 rr28 rr29 rr30 rr31 rr32 rr33 rr34 rr35 rr36 rr37 rr38 rr39 rr40 rr41 rr42 rr43 rr44 rr45 rr46 rr47 rr48 rr49 rr50 rr51 rr52 rr53 rr54 rr55 rr56 rr57 rr58 rr59 rr60 rr61 rr62 rr63 
#OUT c


tmp = rr0 + rr1
h00 = tmp + a0

tmp = rr1 + rr2
h01 = tmp + a1

tmp = rr2 + rr3
h02 = tmp + a2

tmp = rr3 + rr4
h03 = tmp + a3

tmp = rr4 + rr5
h04 = tmp + a4

tmp = rr5 + rr6
h05 = tmp + a5

tmp = rr6 + rr7
h06 = tmp + a6

tmp = rr7 + rr0
h07 = tmp + a7


tmp = rr8 + rr9
h10 = tmp + a0

tmp = rr9 + rr10
h11 = tmp + a1

tmp = rr10 + rr11
h12 = tmp + a2

tmp = rr11 + rr12
h13 = tmp + a3

tmp = rr12 + rr13
h14 = tmp + a4

tmp = rr13 + rr14
h15 = tmp + a5

tmp = rr14 + rr15
h16 = tmp + a6

tmp = rr15 + rr8
h17 = tmp + a7


tmp = rr16 + rr17
h20 = tmp + a0

tmp = rr17 + rr18
h21 = tmp + a1

tmp = rr18 + rr19
h22 = tmp + a2

tmp = rr19 + rr20
h23 = tmp + a3

tmp = rr20 + rr21
h24 = tmp + a4

tmp = rr21 + rr22
h25 = tmp + a5

tmp = rr22 + rr23
h26 = tmp + a6

tmp = rr23 + rr16
h27 = tmp + a7


tmp = rr24 + rr25
h30 = tmp + a0

tmp = rr25 + rr26
h31 = tmp + a1

tmp = rr26 + rr27
h32 = tmp + a2

tmp = rr27 + rr28
h33 = tmp + a3

tmp = rr28 + rr29
h34 = tmp + a4

tmp = rr29 + rr30
h35 = tmp + a5

tmp = rr30 + rr31
h36 = tmp + a6

tmp = rr31 + rr24
h37 = tmp + a7


tmp = rr32 + rr33
h40 = tmp + a0

tmp = rr33 + rr34
h41 = tmp + a1

tmp = rr34 + rr35
h42 = tmp + a2

tmp = rr35 + rr36
h43 = tmp + a3

tmp = rr36 + rr37
h44 = tmp + a4

tmp = rr37 + rr38
h45 = tmp + a5

tmp = rr38 + rr39
h46 = tmp + a6

tmp = rr39 + rr32
h47 = tmp + a7


tmp = rr40 + rr41
h50 = tmp + a0

tmp = rr41 + rr42
h51 = tmp + a1

tmp = rr42 + rr43
h52 = tmp + a2

tmp = rr43 + rr44
h53 = tmp + a3

tmp = rr44 + rr45
h54 = tmp + a4

tmp = rr45 + rr46
h55 = tmp + a5

tmp = rr46 + rr47
h56 = tmp + a6

tmp = rr47 + rr40
h57 = tmp + a7


tmp = rr48 + rr49
h60 = tmp + a0

tmp = rr49 + rr50
h61 = tmp + a1

tmp = rr50 + rr51
h62 = tmp + a2

tmp = rr51 + rr52
h63 = tmp + a3

tmp = rr52 + rr53
h64 = tmp + a4

tmp = rr53 + rr54
h65 = tmp + a5

tmp = rr54 + rr55
h66 = tmp + a6

tmp = rr55 + rr48
h67 = tmp + a7


tmp = rr56 + rr57
h70 = tmp + a0

tmp = rr57 + rr58
h71 = tmp + a1

tmp = rr58 + rr59
h72 = tmp + a2

tmp = rr59 + rr60
h73 = tmp + a3

tmp = rr60 + rr61
h74 = tmp + a4

tmp = rr61 + rr62
h75 = tmp + a5

tmp = rr62 + rr63
h76 = tmp + a6

tmp = rr63 + rr56
h77 = tmp + a7


tmp = h00 * b1
r10 = r01 + tmp
tmp = h01 * b0
r10 = r10 + tmp

tmp = h10 * b2
r20 = r02 + tmp
tmp = h02 * b0
r20 = r20 + tmp

tmp = h20 * b3
r30 = r03 + tmp
tmp = h03 * b0
r30 = r30 + tmp

tmp = h30 * b4
r40 = r04 + tmp
tmp = h04 * b0
r40 = r40 + tmp

tmp = h40 * b5
r50 = r05 + tmp
tmp = h05 * b0
r50 = r50 + tmp

tmp = h50 * b6
r60 = r06 + tmp
tmp = h06 * b0
r60 = r60 + tmp

tmp = h60 * b7
r70 = r07 + tmp
tmp = h07 * b0
r70 = r70 + tmp

tmp = h11 * b2
r21 = r12 + tmp
tmp = h12 * b1
r21 = r21 + tmp

tmp = h21 * b3
r31 = r13 + tmp
tmp = h13 * b1
r31 = r31 + tmp

tmp = h31 * b4
r41 = r14 + tmp
tmp = h14 * b1
r41 = r41 + tmp

tmp = h41 * b5
r51 = r15 + tmp
tmp = h15 * b1
r51 = r51 + tmp

tmp = h51 * b6
r61 = r16 + tmp
tmp = h16 * b1
r61 = r61 + tmp

tmp = h61 * b7
r71 = r17 + tmp
tmp = h17 * b1
r71 = r71 + tmp

tmp = h22 * b3
r32 = r23 + tmp
tmp = h23 * b2
r32 = r32 + tmp

tmp = h32 * b4
r42 = r24 + tmp
tmp = h24 * b2
r42 = r42 + tmp

tmp = h42 * b5
r52 = r25 + tmp
tmp = h25 * b2
r52 = r52 + tmp

tmp = h52 * b6
r62 = r26 + tmp
tmp = h26 * b2
r62 = r62 + tmp

tmp = h62 * b7
r72 = r27 + tmp
tmp = h27 * b2
r72 = r72 + tmp

tmp = h33 * b4
r43 = r34 + tmp
tmp = h34 * b3
r43 = r43 + tmp

tmp = h43 * b5
r53 = r35 + tmp
tmp = h35 * b3
r53 = r53 + tmp

tmp = h53 * b6
r63 = r36 + tmp
tmp = h36 * b3
r63 = r63 + tmp

tmp = h63 * b7
r73 = r37 + tmp
tmp = h37 * b3
r73 = r73 + tmp

tmp = h44 * b5
r54 = r45 + tmp
tmp = h45 * b4
r54 = r54 + tmp

tmp = h54 * b6
r64 = r46 + tmp
tmp = h46 * b4
r64 = r64 + tmp

tmp = h64 * b7
r74 = r47 + tmp
tmp = h47 * b4
r74 = r74 + tmp

tmp = h55 * b6
r65 = r56 + tmp
tmp = h56 * b5
r65 = r65 + tmp

tmp = h65 * b7
r75 = r57 + tmp
tmp = h57 * b5
r75 = r75 + tmp

tmp = h66 * b7
r76 = r67 + tmp
tmp = h67 * b6
r76 = r76 + tmp

tmp = h70 * b0
c0 = tmp + r01
c0 = c0 + r02
c0 = c0 + r03
c0 = c0 + r04
c0 = c0 + r05
c0 = c0 + r06
c0 = c0 + r07

tmp = h71 * b1
c1 = tmp + r12
c1 = c1 + r13
c1 = c1 + r14
c1 = c1 + r15
c1 = c1 + r16
c1 = c1 + r17
c1 = c1 + r10

tmp = h72 * b2
c2 = tmp + r23
c2 = c2 + r24
c2 = c2 + r25
c2 = c2 + r26
c2 = c2 + r27
c2 = c2 + r20
c2 = c2 + r21

tmp = h73 * b3
c3 = tmp + r34
c3 = c3 + r35
c3 = c3 + r36
c3 = c3 + r37
c3 = c3 + r30
c3 = c3 + r31
c3 = c3 + r32

tmp = h74 * b4
c4 = tmp + r45
c4 = c4 + r46
c4 = c4 + r47
c4 = c4 + r40
c4 = c4 + r41
c4 = c4 + r42
c4 = c4 + r43

tmp = h75 * b5
c5 = tmp + r56
c5 = c5 + r57
c5 = c5 + r50
c5 = c5 + r51
c5 = c5 + r52
c5 = c5 + r53
c5 = c5 + r54

tmp = h76 * b6
c6 = tmp + r67
c6 = c6 + r60
c6 = c6 + r61
c6 = c6 + r62
c6 = c6 + r63
c6 = c6 + r64
c6 = c6 + r65

tmp = h77 * b7
c7 = tmp + r70
c7 = c7 + r71
c7 = c7 + r72
c7 = c7 + r73
c7 = c7 + r74
c7 = c7 + r75
c7 = c7 + r76

