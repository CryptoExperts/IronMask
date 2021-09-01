in 0 0_0 #  a[0]
in 1 0_1 #  a[1]
in 2 0_2 #  a[2]
in 3 0_3 #  a[3]
in 4 0_4 #  a[4]
ref 5 #  r0
ref 6 #  r1
ref 7 #  r2
ref 8 #  r3
ref 9 #  r4
ref 10 #  r5
ref 11 #  r6
ref 12 #  r7
xor 0 5 #  j0 = a0 + r0 --> 13
xor 2 5 #  j2 = a2 + r0 --> 14
xor 1 6 #  j1 = a1 + r1 --> 15
xor 3 6 #  j3 = a3 + r1 --> 16
reg 4 # j4 = a4 --> 17
xor 13 7 #  k0 = j0 + r2 --> 18
xor 15 7 #  k1 = j1 + r2 --> 19
xor 14 8 #  j2 = j2 + r3 --> 20
xor 16 8 #  j3 = j3 + r3 --> 21
reg 17 # j4 = j4 --> 22
reg 20 # k2 = j2 --> 23
xor 21 9 #  k3 = j3 + r4 --> 24
xor 22 9 #  k4 = j4 + r4 --> 25
xor 23 10 #  k2 = k2 + r5 --> 26
xor 24 10 #  k3 = k3 + r5 --> 27
reg 25 # k4 = k4 --> 28
xor 18 11 #  d0 = k0 + r6 --> 29
xor 26 11 #  d2 = k2 + r6 --> 30
xor 19 12 #  d1 = k1 + r7 --> 31
xor 27 12 #  d3 = k3 + r7 --> 32
reg 28 # d4 = k4 --> 33
out 29 0_0 #  d0
out 31 0_1 #  d1
out 30 0_2 #  d2
out 32 0_3 #  d3
out 33 0_4 #  d4
