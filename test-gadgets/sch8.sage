#ORDER 7
#SHARES 8
#IN a b
#RANDOMS r00 r01 r02 r03 r04 r05 r06 r07 r08 r09 r10 r11 r12 r13 r14 r15
#OUT c

s00 = a0*b0
s01 = a0*b1
s02 = a0*b2
s03 = a0*b3
s04 = a0*b4
s05 = a0*b5
s06 = a0*b6
s07 = a0*b7
s10 = a1*b0
s11 = a1*b1
s12 = a1*b2
s13 = a1*b3
s14 = a1*b4
s15 = a1*b5
s16 = a1*b6
s17 = a1*b7
s20 = a2*b0
s21 = a2*b1
s22 = a2*b2
s23 = a2*b3
s24 = a2*b4
s25 = a2*b5
s26 = a2*b6
s27 = a2*b7
s30 = a3*b0
s31 = a3*b1
s32 = a3*b2
s33 = a3*b3
s34 = a3*b4
s35 = a3*b5
s36 = a3*b6
s37 = a3*b7
s40 = a4*b0
s41 = a4*b1
s42 = a4*b2
s43 = a4*b3
s44 = a4*b4
s45 = a4*b5
s46 = a4*b6
s47 = a4*b7
s50 = a5*b0
s51 = a5*b1
s52 = a5*b2
s53 = a5*b3
s54 = a5*b4
s55 = a5*b5
s56 = a5*b6
s57 = a5*b7
s60 = a6*b0
s61 = a6*b1
s62 = a6*b2
s63 = a6*b3
s64 = a6*b4
s65 = a6*b5
s66 = a6*b6
s67 = a6*b7
s70 = a7*b0
s71 = a7*b1
s72 = a7*b2
s73 = a7*b3
s74 = a7*b4
s75 = a7*b5
s76 = a7*b6
s77 = a7*b7

aux000 = s00 + r00
aux001 = aux000 + s01
aux002 = aux001 + s10
aux003 = aux002 + r01
aux004 = aux003 + s02
aux005 = aux004 + s20
aux006 = aux005 + r08
aux007 = aux006 + s03
aux008 = aux007 + s30
aux009 = aux008 + r09
aux010 = aux009 + s04
c0  = aux010 + s40

aux100 = s11 + r01
aux101 = aux100 + s12
aux102 = aux101 + s21
aux103 = aux102 + r02
aux104 = aux103 + s13
aux105 = aux104 + s31
aux106 = aux105 + r09
aux107 = aux106 + s14
aux108 = aux107 + s41
aux109 = aux108 + r10
aux110 = aux109 + s15
c1  = aux110 + s51

aux200 = s22 + r02
aux201 = aux200 + s23
aux202 = aux201 + s32
aux203 = aux202 + r03
aux204 = aux203 + s24
aux205 = aux204 + s42
aux206 = aux205 + r10
aux207 = aux206 + s25
aux208 = aux207 + s52
aux209 = aux208 + r11
aux210 = aux209 + s26
c2  = aux210 + s62

aux300 = s33 + r03
aux301 = aux300 + s34
aux302 = aux301 + s43
aux303 = aux302 + r04
aux304 = aux303 + s35
aux305 = aux304 + s53
aux306 = aux305 + r11
aux307 = aux306 + s36
aux308 = aux307 + s63
aux309 = aux308 + r12
aux310 = aux309 + s37
c3  = aux310 + s73

aux400 = s44 + r04
aux401 = aux400 + s45
aux402 = aux401 + s54
aux403 = aux402 + r05
aux404 = aux403 + s46
aux405 = aux404 + s64
aux406 = aux405 + r12
aux407 = aux406 + s47
aux408 = aux407 + s74
c4  = aux408 + r13

aux500 = s55 + r05
aux501 = aux500 + s56
aux502 = aux501 + s65
aux503 = aux502 + r06
aux504 = aux503 + s57
aux505 = aux504 + s75
aux506 = aux505 + r13
aux507 = aux506 + s50
aux508 = aux507 + s05
c5  = aux508 + r14

aux600 = s66 + r06
aux601 = aux600 + s67
aux602 = aux601 + s76
aux603 = aux602 + r07
aux604 = aux603 + s60
aux605 = aux604 + s06
aux606 = aux605 + r14
aux607 = aux606 + s61
aux608 = aux607 + s16
c6  = aux608 + r15

aux700 = s77 + r07
aux701 = aux700 + s70
aux702 = aux701 + s07
aux703 = aux702 + r00
aux704 = aux703 + s71
aux705 = aux704 + s17
aux706 = aux705 + r15
aux707 = aux706 + s72
aux708 = aux707 + s27
c7  = aux708 + r08
