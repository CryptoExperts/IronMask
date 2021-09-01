#ORDER 1
#SHARES 3
#IN a b 
#RANDOMS rr01 rr02 rr10 rr12 rr20 rr21 rr00 rr11 rr22 r0 r1 r2 r3 r4 r5 r6 r7
#OUT g


k0 = a0 + r6
k1 = a1 + r7
k2 = r6 + r7
k2 = k2 + a2

h0 = b0 + r0
h1 = b1 + r1
tmp = r0 + r1
h2 = tmp + b2

c00 = k0 * h0
c01 = k0 * h1
c02 = k0 * h2

h0 = b0 + r2
h1 = b1 + r3
tmp = r2 + r3
h2 = tmp + b2

c10 = k1 * h0
c11 = k1 * h1
c12 = k1 * h2

h0 = b0 + r4
h1 = b1 + r5
tmp = r4 + r5
h2 = tmp + b2

c20 = k2 * h0
c21 = k2 * h1
c22 = k2 * h2

d00 = c00 + rr00
d01 = c01 + rr01
d02 = c02 + rr02

d10 = c10 + rr10
d11 = c11 + rr11
d12 = c12 + rr12

d20 = c20 + rr20
d21 = c21 + rr21
d22 = c22 + rr22

e0 = d00 + d01
e0 = e0 + d02

e1 = d10 + d11
e1 = e1 + d12

e2 = d20 + d21
e2 = e2 + d22

f0 = rr00 + rr10
f0 = f0 + rr20

f1 = rr01 + rr11
f1 = f1 + rr21

f2 = rr02 + rr12
f2 = f2 + rr22

g0 = e0 + f0
g1 = e1 + f1
g2 = e2 + f2

