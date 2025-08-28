in 0 0_0 #  a[0]
in 1 0_1 #  a[1]
in 2 0_2 #  a[2]
in 3 0_3 #  a[3]
in 4 0_4 #  a[4]
in 5 0_5 #  a[5]
in 6 1_0 #  b[0]
in 7 1_1 #  b[1]
in 8 1_2 #  b[2]
in 9 1_3 #  b[3]
in 10 1_4 #  b[4]
in 11 1_5 #  b[5]
ref 12 #  r01
ref 13 #  r02
ref 14 #  r03
ref 15 #  r04
ref 16 #  r05
ref 17 #  r12
ref 18 #  r13
ref 19 #  r14
ref 20 #  r15
ref 21 #  r23
ref 22 #  r24
ref 23 #  r25
ref 24 #  r34
ref 25 #  r35
ref 26 #  r45
and 0 7 #  tmp = a0 * b1 --> 27
xor 12 27 #  r10 = r01 + tmp --> 28
and 1 6 #  tmp = a1 * b0 --> 29
xor 28 29 #  r10 = r10 + tmp --> 30
and 0 8 #  tmp = a0 * b2 --> 31
xor 13 31 #  r20 = r02 + tmp --> 32
and 2 6 #  tmp = a2 * b0 --> 33
xor 32 33 #  r20 = r20 + tmp --> 34
and 0 9 #  tmp = a0 * b3 --> 35
xor 14 35 #  r30 = r03 + tmp --> 36
and 3 6 #  tmp = a3 * b0 --> 37
xor 36 37 #  r30 = r30 + tmp --> 38
and 0 10 #  tmp = a0 * b4 --> 39
xor 15 39 #  r40 = r04 + tmp --> 40
and 4 6 #  tmp = a4 * b0 --> 41
xor 40 41 #  r40 = r40 + tmp --> 42
and 0 11 #  tmp = a0 * b5 --> 43
xor 16 43 #  r50 = r05 + tmp --> 44
and 5 6 #  tmp = a5 * b0 --> 45
xor 44 45 #  r50 = r50 + tmp --> 46
and 1 8 #  tmp = a1 * b2 --> 47
xor 17 47 #  r21 = r12 + tmp --> 48
and 2 7 #  tmp = a2 * b1 --> 49
xor 48 49 #  r21 = r21 + tmp --> 50
and 1 9 #  tmp = a1 * b3 --> 51
xor 18 51 #  r31 = r13 + tmp --> 52
and 3 7 #  tmp = a3 * b1 --> 53
xor 52 53 #  r31 = r31 + tmp --> 54
and 1 10 #  tmp = a1 * b4 --> 55
xor 19 55 #  r41 = r14 + tmp --> 56
and 4 7 #  tmp = a4 * b1 --> 57
xor 56 57 #  r41 = r41 + tmp --> 58
and 1 11 #  tmp = a1 * b5 --> 59
xor 20 59 #  r51 = r15 + tmp --> 60
and 5 7 #  tmp = a5 * b1 --> 61
xor 60 61 #  r51 = r51 + tmp --> 62
and 2 9 #  tmp = a2 * b3 --> 63
xor 21 63 #  r32 = r23 + tmp --> 64
and 3 8 #  tmp = a3 * b2 --> 65
xor 64 65 #  r32 = r32 + tmp --> 66
and 2 10 #  tmp = a2 * b4 --> 67
xor 22 67 #  r42 = r24 + tmp --> 68
and 4 8 #  tmp = a4 * b2 --> 69
xor 68 69 #  r42 = r42 + tmp --> 70
and 2 11 #  tmp = a2 * b5 --> 71
xor 23 71 #  r52 = r25 + tmp --> 72
and 5 8 #  tmp = a5 * b2 --> 73
xor 72 73 #  r52 = r52 + tmp --> 74
and 3 10 #  tmp = a3 * b4 --> 75
xor 24 75 #  r43 = r34 + tmp --> 76
and 4 9 #  tmp = a4 * b3 --> 77
xor 76 77 #  r43 = r43 + tmp --> 78
and 3 11 #  tmp = a3 * b5 --> 79
xor 25 79 #  r53 = r35 + tmp --> 80
and 5 9 #  tmp = a5 * b3 --> 81
xor 80 81 #  r53 = r53 + tmp --> 82
and 4 11 #  tmp = a4 * b5 --> 83
xor 26 83 #  r54 = r45 + tmp --> 84
and 5 10 #  tmp = a5 * b4 --> 85
xor 84 85 #  r54 = r54 + tmp --> 86
and 0 6 #  tmp = a0 * b0 --> 87
xor 87 12 #  c0 = tmp + r01 --> 88
xor 88 13 #  c0 = c0 + r02 --> 89
xor 89 14 #  c0 = c0 + r03 --> 90
xor 90 15 #  c0 = c0 + r04 --> 91
xor 91 16 #  c0 = c0 + r05 --> 92
and 1 7 #  tmp = a1 * b1 --> 93
xor 93 30 #  c1 = tmp + r10 --> 94
xor 94 17 #  c1 = c1 + r12 --> 95
xor 95 18 #  c1 = c1 + r13 --> 96
xor 96 19 #  c1 = c1 + r14 --> 97
xor 97 20 #  c1 = c1 + r15 --> 98
and 2 8 #  tmp = a2 * b2 --> 99
xor 99 34 #  c2 = tmp + r20 --> 100
xor 100 50 #  c2 = c2 + r21 --> 101
xor 101 21 #  c2 = c2 + r23 --> 102
xor 102 22 #  c2 = c2 + r24 --> 103
xor 103 23 #  c2 = c2 + r25 --> 104
and 3 9 #  tmp = a3 * b3 --> 105
xor 105 38 #  c3 = tmp + r30 --> 106
xor 106 54 #  c3 = c3 + r31 --> 107
xor 107 66 #  c3 = c3 + r32 --> 108
xor 108 24 #  c3 = c3 + r34 --> 109
xor 109 25 #  c3 = c3 + r35 --> 110
and 4 10 #  tmp = a4 * b4 --> 111
xor 111 42 #  c4 = tmp + r40 --> 112
xor 112 58 #  c4 = c4 + r41 --> 113
xor 113 70 #  c4 = c4 + r42 --> 114
xor 114 78 #  c4 = c4 + r43 --> 115
xor 115 26 #  c4 = c4 + r45 --> 116
and 5 11 #  tmp = a5 * b5 --> 117
xor 117 46 #  c5 = tmp + r50 --> 118
xor 118 62 #  c5 = c5 + r51 --> 119
xor 119 74 #  c5 = c5 + r52 --> 120
xor 120 82 #  c5 = c5 + r53 --> 121
xor 121 86 #  c5 = c5 + r54 --> 122
out 92 0_0 #  c0
out 98 0_1 #  c1
out 104 0_2 #  c2
out 110 0_3 #  c3
out 116 0_4 #  c4
out 122 0_5 #  c5
