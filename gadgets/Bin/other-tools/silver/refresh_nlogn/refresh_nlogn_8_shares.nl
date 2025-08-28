in 0 0_0 #  a[0]
in 1 0_1 #  a[1]
in 2 0_2 #  a[2]
in 3 0_3 #  a[3]
in 4 0_4 #  a[4]
in 5 0_5 #  a[5]
in 6 0_6 #  a[6]
in 7 0_7 #  a[7]
ref 8 #  r0
ref 9 #  r1
ref 10 #  r2
ref 11 #  r3
ref 12 #  r4
ref 13 #  r5
ref 14 #  r6
ref 15 #  r7
ref 16 #  r8
ref 17 #  r9
ref 18 #  r10
ref 19 #  r11
ref 20 #  r12
ref 21 #  r13
ref 22 #  r14
ref 23 #  r15
ref 24 #  r16
ref 25 #  r17
ref 26 #  r18
ref 27 #  r19
xor 0 8 #  j0 = a0 + r0 --> 28
xor 4 8 #  j4 = a4 + r0 --> 29
xor 1 9 #  j1 = a1 + r1 --> 30
xor 5 9 #  j5 = a5 + r1 --> 31
xor 2 10 #  j2 = a2 + r2 --> 32
xor 6 10 #  j6 = a6 + r2 --> 33
xor 3 11 #  j3 = a3 + r3 --> 34
xor 7 11 #  j7 = a7 + r3 --> 35
xor 28 12 #  j0 = j0 + r4 --> 36
xor 32 12 #  j2 = j2 + r4 --> 37
xor 30 13 #  j1 = j1 + r5 --> 38
xor 34 13 #  j3 = j3 + r5 --> 39
xor 36 14 #  k0 = j0 + r6 --> 40
xor 38 14 #  k1 = j1 + r6 --> 41
xor 37 15 #  k2 = j2 + r7 --> 42
xor 39 15 #  k3 = j3 + r7 --> 43
xor 40 16 #  k0 = k0 + r8 --> 44
xor 42 16 #  k2 = k2 + r8 --> 45
xor 41 17 #  k1 = k1 + r9 --> 46
xor 43 17 #  k3 = k3 + r9 --> 47
xor 29 18 #  j4 = j4 + r10 --> 48
xor 33 18 #  j6 = j6 + r10 --> 49
xor 31 19 #  j5 = j5 + r11 --> 50
xor 35 19 #  j7 = j7 + r11 --> 51
xor 48 20 #  k4 = j4 + r12 --> 52
xor 50 20 #  k5 = j5 + r12 --> 53
xor 49 21 #  k6 = j6 + r13 --> 54
xor 51 21 #  k7 = j7 + r13 --> 55
xor 52 22 #  k4 = k4 + r14 --> 56
xor 54 22 #  k6 = k6 + r14 --> 57
xor 53 23 #  k5 = k5 + r15 --> 58
xor 55 23 #  k7 = k7 + r15 --> 59
xor 44 24 #  d0 = k0 + r16 --> 60
xor 56 24 #  d4 = k4 + r16 --> 61
xor 46 25 #  d1 = k1 + r17 --> 62
xor 58 25 #  d5 = k5 + r17 --> 63
xor 45 26 #  d2 = k2 + r18 --> 64
xor 57 26 #  d6 = k6 + r18 --> 65
xor 47 27 #  d3 = k3 + r19 --> 66
xor 59 27 #  d7 = k7 + r19 --> 67
out 60 0_0 #  d0
out 62 0_1 #  d1
out 64 0_2 #  d2
out 66 0_3 #  d3
out 61 0_4 #  d4
out 63 0_5 #  d5
out 65 0_6 #  d6
out 67 0_7 #  d7
