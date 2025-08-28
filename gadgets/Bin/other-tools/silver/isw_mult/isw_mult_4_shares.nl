in 0 0_0 #  a[0]
in 1 0_1 #  a[1]
in 2 0_2 #  a[2]
in 3 0_3 #  a[3]
in 4 1_0 #  b[0]
in 5 1_1 #  b[1]
in 6 1_2 #  b[2]
in 7 1_3 #  b[3]
ref 8 #  r01
ref 9 #  r02
ref 10 #  r03
ref 11 #  r12
ref 12 #  r13
ref 13 #  r23
and 0 5 #  tmp = a0 * b1 --> 14
xor 8 14 #  r10 = r01 + tmp --> 15
and 1 4 #  tmp = a1 * b0 --> 16
xor 15 16 #  r10 = r10 + tmp --> 17
and 0 6 #  tmp = a0 * b2 --> 18
xor 9 18 #  r20 = r02 + tmp --> 19
and 2 4 #  tmp = a2 * b0 --> 20
xor 19 20 #  r20 = r20 + tmp --> 21
and 0 7 #  tmp = a0 * b3 --> 22
xor 10 22 #  r30 = r03 + tmp --> 23
and 3 4 #  tmp = a3 * b0 --> 24
xor 23 24 #  r30 = r30 + tmp --> 25
and 1 6 #  tmp = a1 * b2 --> 26
xor 11 26 #  r21 = r12 + tmp --> 27
and 2 5 #  tmp = a2 * b1 --> 28
xor 27 28 #  r21 = r21 + tmp --> 29
and 1 7 #  tmp = a1 * b3 --> 30
xor 12 30 #  r31 = r13 + tmp --> 31
and 3 5 #  tmp = a3 * b1 --> 32
xor 31 32 #  r31 = r31 + tmp --> 33
and 2 7 #  tmp = a2 * b3 --> 34
xor 13 34 #  r32 = r23 + tmp --> 35
and 3 6 #  tmp = a3 * b2 --> 36
xor 35 36 #  r32 = r32 + tmp --> 37
and 0 4 #  tmp = a0 * b0 --> 38
xor 38 8 #  c0 = tmp + r01 --> 39
xor 39 9 #  c0 = c0 + r02 --> 40
xor 40 10 #  c0 = c0 + r03 --> 41
and 1 5 #  tmp = a1 * b1 --> 42
xor 42 17 #  c1 = tmp + r10 --> 43
xor 43 11 #  c1 = c1 + r12 --> 44
xor 44 12 #  c1 = c1 + r13 --> 45
and 2 6 #  tmp = a2 * b2 --> 46
xor 46 21 #  c2 = tmp + r20 --> 47
xor 47 29 #  c2 = c2 + r21 --> 48
xor 48 13 #  c2 = c2 + r23 --> 49
and 3 7 #  tmp = a3 * b3 --> 50
xor 50 25 #  c3 = tmp + r30 --> 51
xor 51 33 #  c3 = c3 + r31 --> 52
xor 52 37 #  c3 = c3 + r32 --> 53
out 41 0_0 #  c0
out 45 0_1 #  c1
out 49 0_2 #  c2
out 53 0_3 #  c3
