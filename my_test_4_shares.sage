#SHARES 4
#IN a b
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7
#OUT c


t0 = a0 + r0
t0 = t0 + r1

t1 = a1 + r1
t1 = t1 + r2

t2 = a2 + r2
t2 = t2 + r3

t3 = a3 + r3
t3 = t3 + r0


k0 = b0 + r4
k0 = k0 + r5

k1 = b1 + r5
k1 = k1 + r6

k2 = b2 + r6
k2 = k2 + r7

k3 = b3 + r7
k3 = k3 + r4


m00 = t0 * k0
m11 = t1 * k1
m22 = t2 * k2
m33 = t3 * k3

m01 = t0 * k1
m10 = t1 * k0

m02 = t0 * k2
m20 = t2 * k0

m03 = t0 * k3
m30 = t3 * k0

m12 = t1 * k2
m21 = t2 * k1

m13 = t1 * k3
m31 = t3 * k1

m23 = t2 * k3
m32 = t3 * k2



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
