#order 1
#shares 2
#in a b
#out c
#randoms r0 r1

a0b0 = a0 * b0
a0b1 = a0 * b1
a1b0 = a1 * b0
a1b1 = a1 * b1

t0 = ![a0b0 + r0]
t1 = ![a1b1 + r1]

t2 = t0 + a0b1
t3 = t1 + a1b0

t4 = ![t2 + r1]
t5 = ![t3 + r0]

c0 = t4  # a0b0 + r0 + a0b1 + r1
c1 = t5  # a1b1 + r1 + a1b0 + r0
