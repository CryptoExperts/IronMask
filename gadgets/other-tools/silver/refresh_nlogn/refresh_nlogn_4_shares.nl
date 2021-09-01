in 0 0_0 #  a[0]
in 1 0_1 #  a[1]
in 2 0_2 #  a[2]
in 3 0_3 #  a[3]
ref 4 #  r0
ref 5 #  r1
ref 6 #  r2
ref 7 #  r3
ref 8 #  r4
ref 9 #  r5
xor 0 4 #  j0 = a0 + r0 --> 10
xor 2 4 #  j2 = a2 + r0 --> 11
xor 1 5 #  j1 = a1 + r1 --> 12
xor 3 5 #  j3 = a3 + r1 --> 13
xor 10 6 #  k0 = j0 + r2 --> 14
xor 12 6 #  k1 = j1 + r2 --> 15
xor 11 7 #  k2 = j2 + r3 --> 16
xor 13 7 #  k3 = j3 + r3 --> 17
xor 14 8 #  d0 = k0 + r4 --> 18
xor 16 8 #  d2 = k2 + r4 --> 19
xor 15 9 #  d1 = k1 + r5 --> 20
xor 17 9 #  d3 = k3 + r5 --> 21
out 18 0_0 #  d0
out 20 0_1 #  d1
out 19 0_2 #  d2
out 21 0_3 #  d3
