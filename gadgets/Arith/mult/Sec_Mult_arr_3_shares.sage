#SHARES 3
#IN x y
#RANDOMS r0 r1 r2
#OUT z
#CAR 3329

z0 = y0 * x0
z1 = x1 * y1
z2 = x2 * y2

tmp = x0 * y1
tmp = tmp + r0

tmp2 = x1 * y0
r00 = tmp + tmp2

z0 = z0 + -1 r0
z1 = z1 + r00

tmp = x0 * y2
tmp = tmp + r1

tmp2 = x2 * y0
r01 = tmp + tmp2

z0 = z0 + -1 r1
z2 = z2 + r01

tmp = x1 * y2
tmp = tmp + r2

tmp2 = x2 * y1
r02 = tmp + tmp2

z1 = z1 + -1 r2
z2 = z2 + r02

