#!/usr/bin/env python3

from math import *

def generate_lines_circ1(n, inp, out, lines, r) :
  for i in range (n - 1) :
    lines.append( "t"+ str(i) + " =  r"+str(i) + " + -1 r"+str(i + 1)+"\n")
    r[0] += 1
  lines.append("t" + str(n - 1) + " =  r"+str(n - 1) + " + -1 r"+str(0)+"\n")
  r[0] += 1

  lines.append("\n")

  for i in range (n) :
    lines.append(out + str(i) + " = " + inp + str(i) + " + t"+str(i) + "\n")

def generate_lines_circ2(n, inp, out, lines, r) :
  for i in range (n - 1) :
    lines.append( "t"+ str(i) + " =  r"+str(i) + " + -1 r"+str(i + 1)+"\n")
    r[0] += 1
  lines.append("t" + str(n - 1) + " =  r"+str(n - 1) + " + -1 r"+str(0)+"\n")
  r[0] += 1

  lines.append("\n")
  rd_used = r[0]

  for i in range (n - 2) :
    lines.append( "u"+ str(i) + " =  r"+str(i + rd_used) + " + -1 r"+str(rd_used + i + 2)+"\n")
    r[0] += 1
  lines.append("u" + str(n - 2) + " =  r"+str(rd_used + n - 2) + " + -1 r"+str(rd_used)+"\n")
  r[0] += 1
  lines.append("u" + str(n - 1) + " =  r"+str(rd_used + n - 1) + " + -1 r"+str(rd_used + 1)+"\n")
  r[0] += 1

  lines.append("\n")

  for i in range (n) :
    lines.append("v" +str(i) + " = " + "t" + str(i) + " + u" +str(i) + "\n")
  lines.append("\n")

  for i in range (n) :
    lines.append(out + str(i) + " = " + inp + str(i) + " + v"+str(i) + "\n")



def create_circ_solo_iteration (n):
  r = [0]
  lines = []
  generate_lines_circ1(n, "a", "d", lines, r)

  lines.insert(0, "#CAR 3329\n\n")
  lines.insert(0, "#OUT d\n")
  lines.insert(0, "#RANDOMS " + " ".join(f"r{i}" for i in range(r[0])) + "\n")
  lines.insert(0, "#IN a\n")    
  lines.insert(0, f"#SHARES {n}\n")

  with open(f"gadget_refresh_circ1_{n}_shares.sage", "w") as f:
    f.writelines(lines)

def create_circ_two_iteration (n):
  r = [0]
  lines = []
  generate_lines_circ2(n, "a", "d", lines, r)

  lines.insert(0, "#CAR 3329\n\n")
  lines.insert(0, "#OUT d\n")
  lines.insert(0, "#RANDOMS " + " ".join(f"r{i}" for i in range(r[0])) + "\n")
  lines.insert(0, "#IN a\n")    
  lines.insert(0, f"#SHARES {n}\n")

  with open(f"gadget_refresh_circ2_{n}_shares.sage", "w") as f:
    f.writelines(lines)

create_circ_solo_iteration (3)
create_circ_solo_iteration (4)
create_circ_solo_iteration (5)
create_circ_solo_iteration (6)
create_circ_solo_iteration (7)
create_circ_solo_iteration (8)
create_circ_solo_iteration (9)
create_circ_solo_iteration (10)

create_circ_two_iteration (3)
create_circ_two_iteration (4)
create_circ_two_iteration (5)
create_circ_two_iteration (6)
create_circ_two_iteration (7)
create_circ_two_iteration (8)
create_circ_two_iteration (9)
create_circ_two_iteration (10)


