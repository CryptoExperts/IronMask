#order 1
#shares 2
#in a b
#out c
#randoms r

a0b0 = a0 * b0
a0b1 = a0 * b1
a1b0 = a1 * b0
a1b1 = a1 * b1

aux0 = r + a0b1
aux1 = r + a1b0
x1 = ![a0b0]
x2 = ![a1b1]
x3 = ![aux0]
x4 = ![aux1]
t0 = x1 + x3
t1 = x2 + x4

c0 = t0
c1 = t1
