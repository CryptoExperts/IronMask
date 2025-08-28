in 0 0_0 #  a[0]
in 1 0_1 #  a[1]
in 2 0_2 #  a[2]
in 3 0_3 #  a[3]
in 4 0_4 #  a[4]
in 5 0_5 #  a[5]
in 6 0_6 #  a[6]
ref 7 #  r0
ref 8 #  r1
ref 9 #  r2
ref 10 #  r3
ref 11 #  r4
ref 12 #  r5
ref 13 #  r6
ref 14 #  r7
ref 15 #  r8
ref 16 #  r9
ref 17 #  r10
ref 18 #  r11
ref 19 #  r12
ref 20 #  r13
ref 21 #  r14
xor 0 7 #  j0 = a0 + r0 --> 22
xor 3 7 #  j3 = a3 + r0 --> 23
xor 1 8 #  j1 = a1 + r1 --> 24
xor 4 8 #  j4 = a4 + r1 --> 25
xor 2 9 #  j2 = a2 + r2 --> 26
xor 5 9 #  j5 = a5 + r2 --> 27
reg 6 # j6 = a6 --> 28
xor 22 10 #  j0 = j0 + r3 --> 29
xor 24 10 #  j1 = j1 + r3 --> 30
reg 26 # j2 = j2 --> 31
reg 29 # k0 = j0 --> 32
xor 30 11 #  k1 = j1 + r4 --> 33
xor 31 11 #  k2 = j2 + r4 --> 34
xor 32 12 #  k0 = k0 + r5 --> 35
xor 33 12 #  k1 = k1 + r5 --> 36
reg 34 # k2 = k2 --> 37
xor 23 13 #  j3 = j3 + r6 --> 38
xor 27 13 #  j5 = j5 + r6 --> 39
xor 25 14 #  j4 = j4 + r7 --> 40
xor 28 14 #  j6 = j6 + r7 --> 41
xor 38 15 #  k3 = j3 + r8 --> 42
xor 40 15 #  k4 = j4 + r8 --> 43
xor 39 16 #  k5 = j5 + r9 --> 44
xor 41 16 #  k6 = j6 + r9 --> 45
xor 42 17 #  k3 = k3 + r10 --> 46
xor 44 17 #  k5 = k5 + r10 --> 47
xor 43 18 #  k4 = k4 + r11 --> 48
xor 45 18 #  k6 = k6 + r11 --> 49
xor 35 19 #  d0 = k0 + r12 --> 50
xor 46 19 #  d3 = k3 + r12 --> 51
xor 36 20 #  d1 = k1 + r13 --> 52
xor 48 20 #  d4 = k4 + r13 --> 53
xor 37 21 #  d2 = k2 + r14 --> 54
xor 47 21 #  d5 = k5 + r14 --> 55
reg 49 # d6 = k6 --> 56
out 50 0_0 #  d0
out 52 0_1 #  d1
out 54 0_2 #  d2
out 51 0_3 #  d3
out 53 0_4 #  d4
out 55 0_5 #  d5
out 56 0_6 #  d6
