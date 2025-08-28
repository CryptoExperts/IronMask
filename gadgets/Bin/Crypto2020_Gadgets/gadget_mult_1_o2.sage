#ORDER 2
#SHARES 3
#IN a b
#RANDOMS r0 r1 r2 r3 r4 r00 r11 r22 r33 r44 r55
#OUT d

tmp = r00 + r11
h0 = tmp + a0

tmp = r11 + r22
h1 = tmp + a1

tmp = r22 + r00
h2 = tmp + a2

tmp = r33 + r44
i0 = tmp + b0

tmp = r44 + r55
i1 = tmp + b1

tmp = r55 + r33
i2 = tmp + b2

tmp = h0 * i0	
c0 = tmp + r0
tmp = h0 * i1
tmp2 = tmp + r1
c0 = c0 + tmp2
tmp = h0 * i2
tmp2 = tmp + r2
d0 = c0 + tmp2

tmp = h1 * i0	
c1 = tmp + r1
tmp = h1 * i1
tmp2 = tmp + r4
c1 = c1 + tmp2
tmp = h1 * i2
tmp2 = tmp + r3
d1 = c1 + tmp2

tmp = h2 * i0	
c2 = tmp + r2
tmp = h2 * i1
tmp2 = tmp + r3
c2 = c2 + tmp2
tmp = h2 * i2
tmp2 = tmp + r0
c2 = c2 + tmp2
d2 = c2 + r4