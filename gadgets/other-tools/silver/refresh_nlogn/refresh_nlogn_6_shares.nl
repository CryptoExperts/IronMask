in 0 0_0 #  a[0]
in 1 0_1 #  a[1]
in 2 0_2 #  a[2]
in 3 0_3 #  a[3]
in 4 0_4 #  a[4]
in 5 0_5 #  a[5]
ref 6 #  r0
ref 7 #  r1
ref 8 #  r2
ref 9 #  r3
ref 10 #  r4
ref 11 #  r5
ref 12 #  r6
ref 13 #  r7
ref 14 #  r8
ref 15 #  r9
ref 16 #  r10
ref 17 #  r11
xor 0 6 #  j0 = a0 + r0 --> 18
xor 3 6 #  j3 = a3 + r0 --> 19
xor 1 7 #  j1 = a1 + r1 --> 20
xor 4 7 #  j4 = a4 + r1 --> 21
xor 2 8 #  j2 = a2 + r2 --> 22
xor 5 8 #  j5 = a5 + r2 --> 23
xor 18 9 #  j0 = j0 + r3 --> 24
xor 20 9 #  j1 = j1 + r3 --> 25
reg 22 # j2 = j2 --> 26
reg 24 # k0 = j0 --> 27
xor 25 10 #  k1 = j1 + r4 --> 28
xor 26 10 #  k2 = j2 + r4 --> 29
xor 27 11 #  k0 = k0 + r5 --> 30
xor 28 11 #  k1 = k1 + r5 --> 31
reg 29 # k2 = k2 --> 32
xor 19 12 #  j3 = j3 + r6 --> 33
xor 21 12 #  j4 = j4 + r6 --> 34
reg 23 # j5 = j5 --> 35
reg 33 # k3 = j3 --> 36
xor 34 13 #  k4 = j4 + r7 --> 37
xor 35 13 #  k5 = j5 + r7 --> 38
xor 36 14 #  k3 = k3 + r8 --> 39
xor 37 14 #  k4 = k4 + r8 --> 40
reg 38 # k5 = k5 --> 41
xor 30 15 #  d0 = k0 + r9 --> 42
xor 39 15 #  d3 = k3 + r9 --> 43
xor 31 16 #  d1 = k1 + r10 --> 44
xor 40 16 #  d4 = k4 + r10 --> 45
xor 32 17 #  d2 = k2 + r11 --> 46
xor 41 17 #  d5 = k5 + r11 --> 47
out 42 0_0 #  d0
out 44 0_1 #  d1
out 46 0_2 #  d2
out 43 0_3 #  d3
out 45 0_4 #  d4
out 47 0_5 #  d5
