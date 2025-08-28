#!/usr/bin/env python3

from math import *

"""
def nlogn_rec(n, start_index, rand_index, inp, out):
    if(n == 2):
        lines = []
        lines.append(out+str(start_index-1)+ " = r"+str(rand_index[0])+"\n")
        lines.append(out+str(start_index+1-1)+ " = -1 r"+str(rand_index[0])+"\n")
        rand_index[0] += 1
        return lines
        
    if (n == 3) :
      print("ok")
      lines = []
      lines.append(out+str(start_index+1-1)+ " = r"+str(rand_index[0])+"\n")
      lines.append(out+str(start_index+2-1)+ " = -1 r"+str(rand_index[0])+"\n")
      rand_index[0] += 1
      lines.append(out+str(start_index-1)+ " = r"+str(rand_index[0])+"\n")
      lines.append(out+str(start_index+2-1)+ " = " +out +str(start_index+2-1)+ " + -1 r"+str(rand_index[0])+"\n")
      rand_index[0] += 1
      return lines

    #lines = []
    #for i in range(0,floor(n/2)):
    #    lines.append("j"+str(start_index+i-1) + " = "+inp+str(start_index+i-1)+ " + r"+str(rand_index[0])+"\n")
    #    lines.append("j"+str(start_index+i+floor(n/2)-1) + " = "+inp+str(start_index+i+floor(n/2)-1)+ " + -1 r"+str(rand_index[0])+"\n")
    #    rand_index[0] += 1

    #if((n%2) == 1):
    #    lines.append("j"+str(start_index+n-2) + " = "+inp+str(start_index+n-2) + "\n")

    #lines.append("\n\n")

    ## Recursive calls
    lines = nlogn_rec(floor(n/2), start_index, rand_index, "k", "k")
    lines.append("\n\n")
    lines = lines + nlogn_rec(ceil(n/2), start_index+floor(n/2), rand_index, "k", "k")
    lines.append("\n\n")

    for i in range(0,floor(n/2)):
        lines.append(out+str(start_index+i-1) + " = " + inp +str(start_index+i-1)+ " + r"+str(rand_index[0])+"\n")
        lines.append(out+str(start_index+i+floor(n/2)-1) + " = " + inp +str(start_index+i+floor(n/2)-1)+ " + -1 r"+str(rand_index[0])+"\n")
        rand_index[0] += 1

    if(n%2 == 1):
        lines.append(out+str(start_index+n-2) + " = "+ inp +str(start_index+n-2)+"\n")

    return lines
"""
"""
def create_nlogn_refresh(nb_shares):
    n = nb_shares
    r = [0]
    lines = nlogn_rec(n,1, r, "k", "c")
    inp = "a"
    out = "d"
    lines.append("\n\n")    
    for i in range (n) :
      lines.append(out + str(i) + " = " + inp + str(i) + " + c"+str(i) +"\n")
    
    
    f = open("gadget_refresh_"+str(n)+"_shares.sage", "w")
    f.write("#SHARES "+str(n)+"\n")
    f.write("#IN a\n")
    l = ""
    for i in range(0,r[0]):
        l += "r"+str(i)+ " "
    f.write("#RANDOMS "+ l)
    f.write("\n")
    f.write("#OUT d\n")
    f.write("#CAR 3329\n\n")
    f.writelines(lines)
    f.close()
"""



def nlogn_rec(n, start_index, rand_index, out_lines, inp, out):
    if n == 2:
        # d0 = a0 + r0
        # d1 = a1 + -1 r0
        out_lines.append(out + str(start_index + 0) + " = a" + str(start_index + 0) + " + r" + str(rand_index[0]) + "\n")
        out_lines.append(out + str(start_index + 1) + " = a" + str(start_index + 1) + " + -1 r" + str(rand_index[0]) + "\n")
        rand_index[0] += 1
        return

    if n == 3:
        r0 = rand_index[0]
        r1 = rand_index[0] + 1
        rand_index[0] += 2

        # c2 = -1 r0 + -1 r1
        out_lines.append("c" + str(start_index + 2) + " = -1 r" + str(r0) + " + -1 r" + str(r1) + "\n\n")

        # d0 = a0 + r1
        # d1 = a1 + r0
        # d2 = a2 + c2
        out_lines.append(out + str(start_index + 0) + " = a" + str(start_index + 0) + " + r" + str(r1) + "\n")
        out_lines.append(out + str(start_index + 1) + " = a" + str(start_index + 1) + " + r" + str(r0) + "\n")
        out_lines.append(out + str(start_index + 2) + " = a" + str(start_index + 2) + " + c" + str(start_index + 2) + "\n")
        return

    # Recursive
    nlogn_rec(floor(n / 2), start_index, rand_index, out_lines, "c", "c")
    out_lines.append("\n")
    nlogn_rec(ceil(n / 2), start_index + floor(n / 2), rand_index, out_lines, "c", "c")
    out_lines.append("\n")

    # Final mix
    for i in range(floor(n / 2)):
        i1 = start_index + i
        i2 = start_index + i + floor(n / 2)

        r = rand_index[0]
        rand_index[0] += 1

        out_lines.append(out + str(i1) + " = " + inp + str(i1) + " + r" + str(r) + "\n")
        out_lines.append(out + str(i2) + " = " + inp + str(i2) + " + -1 r" + str(r) + "\n")

    if n % 2 == 1 and not(out == 'd'):
        i = start_index + n - 1
        out_lines.append(out + str(i) + " = "+ inp + str(i) + "\n")
    
    if n % 2 == 1 and (out == 'd'):
      for i in range (len(out_lines) - 1, 0, -1) :
        if (out_lines[i][:2] == "c" +str(n - 1)) :
          out_lines[i] = 'd' + out_lines[i][1:]
          break
    #    for (int i = 0; 
    #    out_lines[i] = 'd'     

def create_nlogn_refresh(nb_shares):
    n = nb_shares
    r = [0]
    lines = []

    # Generate core logic
    nlogn_rec(n, 0, r, lines, "c", "d")

    lines.insert(0, "#CAR 3329\n\n")
    lines.insert(0, "#OUT d\n")
    lines.insert(0, "#RANDOMS " + " ".join(f"r{i}" for i in range(r[0])) + "\n")
    lines.insert(0, "#IN a\n")    
    lines.insert(0, f"#SHARES {n}\n")

 



    with open(f"gadget_refresh_{n}_shares.sage", "w") as f:
        f.writelines(lines)



create_nlogn_refresh(3)
create_nlogn_refresh(4)
create_nlogn_refresh(5)
create_nlogn_refresh(6)
create_nlogn_refresh(7)
create_nlogn_refresh(8)
create_nlogn_refresh(9)
create_nlogn_refresh(10)
create_nlogn_refresh(11)
create_nlogn_refresh(12)
create_nlogn_refresh(13)
create_nlogn_refresh(14)
create_nlogn_refresh(15)
create_nlogn_refresh(16)
