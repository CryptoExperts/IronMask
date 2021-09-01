#!python

from math import *

def nlogn_rec(n, start_index, rand_index, inp, out):
    if(n == 1):
        return [out+str(start_index-1)+ " = "+inp+str(start_index-1)+"\n"]

    if(n == 2):
        lines = []
        lines.append(out+str(start_index-1)+ " = "+inp+str(start_index-1)+ " + r"+str(rand_index[0])+"\n")
        lines.append(out+str(start_index+1-1)+ " = "+inp+str(start_index+1-1)+ " + r"+str(rand_index[0])+"\n")
        rand_index[0] += 1
        return lines

    lines = []
    for i in range(0,floor(n/2)):
        lines.append("j"+str(start_index+i-1) + " = "+inp+str(start_index+i-1)+ " + r"+str(rand_index[0])+"\n")
        lines.append("j"+str(start_index+i+floor(n/2)-1) + " = "+inp+str(start_index+i+floor(n/2)-1)+ " + r"+str(rand_index[0])+"\n")
        rand_index[0] += 1

    if((n%2) == 1):
        lines.append("j"+str(start_index+n-2) + " = "+inp+str(start_index+n-2) + "\n")

    lines.append("\n\n")

    ## Recursive calls
    lines = lines + nlogn_rec(floor(n/2), start_index, rand_index, "j", "k")
    lines.append("\n\n")
    lines = lines + nlogn_rec(ceil(n/2), start_index+floor(n/2), rand_index, "j", "k")
    lines.append("\n\n")

    for i in range(0,floor(n/2)):
        lines.append(out+str(start_index+i-1) + " = k"+str(start_index+i-1)+ " + r"+str(rand_index[0])+"\n")
        lines.append(out+str(start_index+i+floor(n/2)-1) + " = k"+str(start_index+i+floor(n/2)-1)+ " + r"+str(rand_index[0])+"\n")
        rand_index[0] += 1

    if(n%2 == 1):
        lines.append(out+str(start_index+n-2) + " = k"+str(start_index+n-2)+"\n")

    return lines


def create_nlogn_refresh(nb_shares):
    n = nb_shares
    r = [0]
    lines = nlogn_rec(n,1, r, "a", "d")
    f = open("gadget_refresh_"+str(n)+"_shares.sage", "w")
    f.write("#SHARES "+str(n)+"\n")
    f.write("#IN a\n")
    l = ""
    for i in range(0,r[0]):
        l += "r"+str(i)+ " "
    f.write("#RANDOMS "+ l)
    f.write("\n")
    f.write("#OUT d\n\n")
    f.writelines(lines)
    f.close()




create_nlogn_refresh(6)
create_nlogn_refresh(7)
create_nlogn_refresh(8)
create_nlogn_refresh(9)
create_nlogn_refresh(10)
create_nlogn_refresh(11)
create_nlogn_refresh(12)
