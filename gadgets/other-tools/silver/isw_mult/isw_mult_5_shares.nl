in 0 0_0 #  a[0]
in 1 0_1 #  a[1]
in 2 0_2 #  a[2]
in 3 0_3 #  a[3]
in 4 0_4 #  a[4]
in 5 1_0 #  b[0]
in 6 1_1 #  b[1]
in 7 1_2 #  b[2]
in 8 1_3 #  b[3]
in 9 1_4 #  b[4]
ref 10 #  r01
ref 11 #  r02
ref 12 #  r03
ref 13 #  r04
ref 14 #  r12
ref 15 #  r13
ref 16 #  r14
ref 17 #  r23
ref 18 #  r24
ref 19 #  r34
and 0 6 #  tmp = a0 * b1 --> 20
xor 10 20 #  r10 = r01 + tmp --> 21
and 1 5 #  tmp = a1 * b0 --> 22
xor 21 22 #  r10 = r10 + tmp --> 23
and 0 7 #  tmp = a0 * b2 --> 24
xor 11 24 #  r20 = r02 + tmp --> 25
and 2 5 #  tmp = a2 * b0 --> 26
xor 25 26 #  r20 = r20 + tmp --> 27
and 0 8 #  tmp = a0 * b3 --> 28
xor 12 28 #  r30 = r03 + tmp --> 29
and 3 5 #  tmp = a3 * b0 --> 30
xor 29 30 #  r30 = r30 + tmp --> 31
and 0 9 #  tmp = a0 * b4 --> 32
xor 13 32 #  r40 = r04 + tmp --> 33
and 4 5 #  tmp = a4 * b0 --> 34
xor 33 34 #  r40 = r40 + tmp --> 35
and 1 7 #  tmp = a1 * b2 --> 36
xor 14 36 #  r21 = r12 + tmp --> 37
and 2 6 #  tmp = a2 * b1 --> 38
xor 37 38 #  r21 = r21 + tmp --> 39
and 1 8 #  tmp = a1 * b3 --> 40
xor 15 40 #  r31 = r13 + tmp --> 41
and 3 6 #  tmp = a3 * b1 --> 42
xor 41 42 #  r31 = r31 + tmp --> 43
and 1 9 #  tmp = a1 * b4 --> 44
xor 16 44 #  r41 = r14 + tmp --> 45
and 4 6 #  tmp = a4 * b1 --> 46
xor 45 46 #  r41 = r41 + tmp --> 47
and 2 8 #  tmp = a2 * b3 --> 48
xor 17 48 #  r32 = r23 + tmp --> 49
and 3 7 #  tmp = a3 * b2 --> 50
xor 49 50 #  r32 = r32 + tmp --> 51
and 2 9 #  tmp = a2 * b4 --> 52
xor 18 52 #  r42 = r24 + tmp --> 53
and 4 7 #  tmp = a4 * b2 --> 54
xor 53 54 #  r42 = r42 + tmp --> 55
and 3 9 #  tmp = a3 * b4 --> 56
xor 19 56 #  r43 = r34 + tmp --> 57
and 4 8 #  tmp = a4 * b3 --> 58
xor 57 58 #  r43 = r43 + tmp --> 59
and 0 5 #  tmp = a0 * b0 --> 60
xor 60 10 #  c0 = tmp + r01 --> 61
xor 61 11 #  c0 = c0 + r02 --> 62
xor 62 12 #  c0 = c0 + r03 --> 63
xor 63 13 #  c0 = c0 + r04 --> 64
and 1 6 #  tmp = a1 * b1 --> 65
xor 65 23 #  c1 = tmp + r10 --> 66
xor 66 14 #  c1 = c1 + r12 --> 67
xor 67 15 #  c1 = c1 + r13 --> 68
xor 68 16 #  c1 = c1 + r14 --> 69
and 2 7 #  tmp = a2 * b2 --> 70
xor 70 27 #  c2 = tmp + r20 --> 71
xor 71 39 #  c2 = c2 + r21 --> 72
xor 72 17 #  c2 = c2 + r23 --> 73
xor 73 18 #  c2 = c2 + r24 --> 74
and 3 8 #  tmp = a3 * b3 --> 75
xor 75 31 #  c3 = tmp + r30 --> 76
xor 76 43 #  c3 = c3 + r31 --> 77
xor 77 51 #  c3 = c3 + r32 --> 78
xor 78 19 #  c3 = c3 + r34 --> 79
and 4 9 #  tmp = a4 * b4 --> 80
xor 80 35 #  c4 = tmp + r40 --> 81
xor 81 47 #  c4 = c4 + r41 --> 82
xor 82 55 #  c4 = c4 + r42 --> 83
xor 83 59 #  c4 = c4 + r43 --> 84
out 64 0_0 #  c0
out 69 0_1 #  c1
out 74 0_2 #  c2
out 79 0_3 #  c3
out 84 0_4 #  c4
