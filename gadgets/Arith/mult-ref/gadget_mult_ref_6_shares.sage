#ORDER 5
#SHARES 6
#IN a b 
#RANDOMS r00 r01 r02 r03 r04 r05 r10 r11 r12 r13 r14 r15 r20 r21 r22 r23 r24 r25 r30 r31 r32 r33 r34 r35 r40 r41 r42 r43 r44 r45 r50 r51 r52 r53 r54 r55 rr0 rr1 rr2 rr3 rr4 rr5 rr6 rr7 rr8 rr9 rr10 rr11 rr12 rr13 rr14 rr15 rr16 rr17 rr18 rr19 rr20 rr21 rr22 rr23 rr24 rr25 rr26 rr27 rr28 rr29 rr30 rr31 rr32 rr33 rr34 rr35 
#OUT c
#CAR 3329


tmp = -1 rr0 + rr1
h00 = tmp + a0

tmp = -1 rr1 + rr2
h01 = tmp + a1

tmp = -1 rr2 + rr3
h02 = tmp + a2

tmp = -1 rr3 + rr4
h03 = tmp + a3

tmp = -1 rr4 + rr5
h04 = tmp + a4

tmp = -1 rr5 + rr0
h05 = tmp + a5


tmp = -1 rr6 + rr7
h10 = tmp + a0

tmp = -1 rr7 + rr8
h11 = tmp + a1

tmp = -1 rr8 + rr9
h12 = tmp + a2

tmp = -1 rr9 + rr10
h13 = tmp + a3

tmp = -1 rr10 + rr11
h14 = tmp + a4

tmp = -1 rr11 + rr6
h15 = tmp + a5


tmp = -1 rr12 + rr13
h20 = tmp + a0

tmp = -1 rr13 + rr14
h21 = tmp + a1

tmp = -1 rr14 + rr15
h22 = tmp + a2

tmp = -1 rr15 + rr16
h23 = tmp + a3

tmp = -1 rr16 + rr17
h24 = tmp + a4

tmp = -1 rr17 + rr12
h25 = tmp + a5


tmp = -1 rr18 + rr19
h30 = tmp + a0

tmp = -1 rr19 + rr20
h31 = tmp + a1

tmp = -1 rr20 + rr21
h32 = tmp + a2

tmp = -1 rr21 + rr22
h33 = tmp + a3

tmp = -1 rr22 + rr23
h34 = tmp + a4

tmp = -1 rr23 + rr18
h35 = tmp + a5


tmp = -1 rr24 + rr25
h40 = tmp + a0

tmp = -1 rr25 + rr26
h41 = tmp + a1

tmp = -1 rr26 + rr27
h42 = tmp + a2

tmp = -1 rr27 + rr28
h43 = tmp + a3

tmp = -1 rr28 + rr29
h44 = tmp + a4

tmp = -1 rr29 + rr24
h45 = tmp + a5


tmp = -1 rr30 + rr31
h50 = tmp + a0

tmp = -1 rr31 + rr32
h51 = tmp + a1

tmp = -1 rr32 + rr33
h52 = tmp + a2

tmp = -1 rr33 + rr34
h53 = tmp + a3

tmp = -1 rr34 + rr35
h54 = tmp + a4

tmp = -1 rr35 + rr30
h55 = tmp + a5


tmp = h00 * b0
p00 = tmp + r00

tmp = h01 * b0
p01 = tmp + r01

tmp = h02 * b0
p02 = tmp + r02

tmp = h03 * b0
p03 = tmp + r03

tmp = h04 * b0
p04 = tmp + r04

tmp = h05 * b0
p05 = tmp + r05

tmp = h10 * b1
p10 = tmp + r10

tmp = h11 * b1
p11 = tmp + r11

tmp = h12 * b1
p12 = tmp + r12

tmp = h13 * b1
p13 = tmp + r13

tmp = h14 * b1
p14 = tmp + r14

tmp = h15 * b1
p15 = tmp + r15

tmp = h20 * b2
p20 = tmp + r20

tmp = h21 * b2
p21 = tmp + r21

tmp = h22 * b2
p22 = tmp + r22

tmp = h23 * b2
p23 = tmp + r23

tmp = h24 * b2
p24 = tmp + r24

tmp = h25 * b2
p25 = tmp + r25

tmp = h30 * b3
p30 = tmp + r30

tmp = h31 * b3
p31 = tmp + r31

tmp = h32 * b3
p32 = tmp + r32

tmp = h33 * b3
p33 = tmp + r33

tmp = h34 * b3
p34 = tmp + r34

tmp = h35 * b3
p35 = tmp + r35

tmp = h40 * b4
p40 = tmp + r40

tmp = h41 * b4
p41 = tmp + r41

tmp = h42 * b4
p42 = tmp + r42

tmp = h43 * b4
p43 = tmp + r43

tmp = h44 * b4
p44 = tmp + r44

tmp = h45 * b4
p45 = tmp + r45

tmp = h50 * b5
p50 = tmp + r50

tmp = h51 * b5
p51 = tmp + r51

tmp = h52 * b5
p52 = tmp + r52

tmp = h53 * b5
p53 = tmp + r53

tmp = h54 * b5
p54 = tmp + r54

tmp = h55 * b5
p55 = tmp + r55

v0 = p00 + p01
v0 = v0 + p02
v0 = v0 + p03
v0 = v0 + p04
v0 = v0 + p05
v1 = p10 + p11
v1 = v1 + p12
v1 = v1 + p13
v1 = v1 + p14
v1 = v1 + p15
v2 = p20 + p21
v2 = v2 + p22
v2 = v2 + p23
v2 = v2 + p24
v2 = v2 + p25
v3 = p30 + p31
v3 = v3 + p32
v3 = v3 + p33
v3 = v3 + p34
v3 = v3 + p35
v4 = p40 + p41
v4 = v4 + p42
v4 = v4 + p43
v4 = v4 + p44
v4 = v4 + p45
v5 = p50 + p51
v5 = v5 + p52
v5 = v5 + p53
v5 = v5 + p54
v5 = v5 + p55

x0 = -1 r00 + -1 r01
x0 = x0 + -1 r02
x0 = x0 + -1 r03
x0 = x0 + -1 r04
x0 = x0 + -1 r05
x1 = -1 r10 + -1 r11
x1 = x1 + -1 r12
x1 = x1 + -1 r13
x1 = x1 + -1 r14
x1 = x1 + -1 r15
x2 = -1 r20 + -1 r21
x2 = x2 + -1 r22
x2 = x2 + -1 r23
x2 = x2 + -1 r24
x2 = x2 + -1 r25
x3 = -1 r30 + -1 r31
x3 = x3 + -1 r32
x3 = x3 + -1 r33
x3 = x3 + -1 r34
x3 = x3 + -1 r35
x4 = -1 r40 + -1 r41
x4 = x4 + -1 r42
x4 = x4 + -1 r43
x4 = x4 + -1 r44
x4 = x4 + -1 r45
x5 = -1 r50 + -1 r51
x5 = x5 + -1 r52
x5 = x5 + -1 r53
x5 = x5 + -1 r54
x5 = x5 + -1 r55

c0 = v0 + x0
c1 = v1 + x1
c2 = v2 + x2
c3 = v3 + x3
c4 = v4 + x4
c5 = v5 + x5
