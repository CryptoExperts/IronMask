#SHARES 3
#IN a b
#RANDOMS r0 r1 r2
#OUT c


k0 = b0 + r0
k0 = k0 + r1

k1 = b1 + r1
k1 = k1 + r2

k2 = b2 + r2
k2 = k2 + r0


m00 = a0 * k0
m11 = a1 * k1
m22 = a2 * k2

m01 = a0 * k1
m10 = a1 * k0

m02 = a0 * k2
m20 = a2 * k0

m12 = a1 * k2
m21 = a2 * k1



c0 = m00 + r4
c0 = c0 + m01
c0 = c0 + r5
c0 = c0 + m11

c1 = m10 + r5
c1 = c1 + m20
c1 = c1 + r6
c1 = c1 + m21

c2 = m02 + r6
c2 = c2 + m12
c2 = c2 + r4
c2 = c2 + m22
