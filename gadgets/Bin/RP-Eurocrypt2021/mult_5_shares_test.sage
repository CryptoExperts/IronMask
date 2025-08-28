#ORDER 1
#SHARES 5
#IN a b 
#RANDOMS rr00 rr01 rr02 rr03 rr04 rr10 rr11 rr12 rr13 rr14 rr20 rr21 rr22 rr23 rr24 rr30 rr31 rr32 rr33 rr34 rr40 rr41 rr42 rr43 rr44 r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 r15 r16 r17 r18 r19 r20 r21 r22 r23 r24 r25 r26 r27 r28 r29
#OUT g


tmp = r25 + r26
k0 = tmp + a0
tmp = r26 + r27
k1 = tmp + a1
tmp = r27 + r28
k2 = tmp + a2
tmp = r28 + r29
k3 = tmp + a3
tmp = r29 + r25
k4 = tmp + a4

tmp = r0 + r1
h0 = tmp + b0
tmp = r1 + r2
h1 = tmp + b1
tmp = r2 + r3
h2 = tmp + b2
tmp = r3 + r4
h3 = tmp + b3
tmp = r4 + r0
h4 = tmp + b4

c00 = k0 * h0
c01 = k0 * h1
c02 = k0 * h2
c03 = k0 * h3
c04 = k0 * h4

tmp = r5 + r6
h0 = tmp + b0
tmp = r6 + r7
h1 = tmp + b1
tmp = r7 + r8
h2 = tmp + b2
tmp = r8 + r9
h3 = tmp + b3
tmp = r9 + r5
h4 = tmp + b4

c10 = k1 * h0
c11 = k1 * h1
c12 = k1 * h2
c13 = k1 * h3
c14 = k1 * h4

tmp = r10 + r11
h0 = tmp + b0
tmp = r11 + r12
h1 = tmp + b1
tmp = r12 + r13
h2 = tmp + b2
tmp = r13 + r14
h3 = tmp + b3
tmp = r14 + r10
h4 = tmp + b4

c20 = k2 * h0
c21 = k2 * h1
c22 = k2 * h2
c23 = k2 * h3
c24 = k2 * h4

tmp = r15 + r16
h0 = tmp + b0
tmp = r16 + r17
h1 = tmp + b1
tmp = r17 + r18
h2 = tmp + b2
tmp = r18 + r19
h3 = tmp + b3
tmp = r19 + r15
h4 = tmp + b4

c30 = k3 * h0
c31 = k3 * h1
c32 = k3 * h2
c33 = k3 * h3
c34 = k3 * h4

tmp = r20 + r21
h0 = tmp + b0
tmp = r21 + r22
h1 = tmp + b1
tmp = r22 + r23
h2 = tmp + b2
tmp = r23 + r24
h3 = tmp + b3
tmp = r24 + r20
h4 = tmp + b4

c40 = k4 * h0
c41 = k4 * h1
c42 = k4 * h2
c43 = k4 * h3
c44 = k4 * h4

d00 = c00 + rr00
d01 = c01 + rr01
d02 = c02 + rr02
d03 = c03 + rr03
d04 = c04 + rr04
d10 = c10 + rr10
d11 = c11 + rr11
d12 = c12 + rr12
d13 = c13 + rr13
d14 = c14 + rr14
d20 = c20 + rr20
d21 = c21 + rr21
d22 = c22 + rr22
d23 = c23 + rr23
d24 = c24 + rr24
d30 = c30 + rr30
d31 = c31 + rr31
d32 = c32 + rr32
d33 = c33 + rr33
d34 = c34 + rr34
d40 = c40 + rr40
d41 = c41 + rr41
d42 = c42 + rr42
d43 = c43 + rr43
d44 = c44 + rr44

e0 = d00 + d01
e0 = e0 + d02
e0 = e0 + d03
e0 = e0 + d04
e1 = d10 + d11
e1 = e1 + d12
e1 = e1 + d13
e1 = e1 + d14
e2 = d20 + d21
e2 = e2 + d22
e2 = e2 + d23
e2 = e2 + d24
e3 = d30 + d31
e3 = e3 + d32
e3 = e3 + d33
e3 = e3 + d34
e4 = d40 + d41
e4 = e4 + d42
e4 = e4 + d43
e4 = e4 + d44

f0 = rr00 + rr10
f0 = f0 + rr20
f0 = f0 + rr30
f0 = f0 + rr40
f1 = rr01 + rr11
f1 = f1 + rr21
f1 = f1 + rr31
f1 = f1 + rr41
f2 = rr02 + rr12
f2 = f2 + rr22
f2 = f2 + rr32
f2 = f2 + rr42
f3 = rr03 + rr13
f3 = f3 + rr23
f3 = f3 + rr33
f3 = f3 + rr43
f4 = rr04 + rr14
f4 = f4 + rr24
f4 = f4 + rr34
f4 = f4 + rr44

g0 = e0 + f0
g1 = e1 + f1
g2 = e2 + f2
g3 = e3 + f3
g4 = e4 + f4

