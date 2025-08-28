#SHARES 3
#IN x
#RANDOMS r0 r1 r2
#OUT a
#CAR 3329


b0 = x0 + -1 r0

tmp0 = b0 * x1
tmp1 = 2 tmp0
a0 = b0 + -1 tmp1

tmp0 = r0 * x1
tmp1 = 2 tmp0
a1 = r0 + -1 tmp1

a0 = a0 + x1

b0 = a0 + -1 r1
b1 = a1 + -1 r2
b2 = r1 + r2

tmp0 = b0 * x2
tmp1 = 2 tmp0
a0 = b0 + -1 tmp1

tmp0 = b1 * x2
tmp1 = 2 tmp0
a1 = b1 + -1 tmp1

tmp0 = b2 * x2
tmp1 = 2 tmp0
a2 = b2 + -1 tmp1

a0 = a0 + x2
