#!python

def nlogn_rec(n, start_index, rand_index, inp, out):
    if(n == 2):
        lines = []
        lines.append(out+str(start_index-1)+ " = "+inp+str(start_index-1)+ " + r"+str(rand_index[0])+"\n")
        lines.append(out+str(start_index+1-1)+ " = "+inp+str(start_index+1-1)+ " + r"+str(rand_index[0])+"\n")
        rand_index[0] += 1
        return lines

    lines = []
    for i in range(0,n//2):
        lines.append("j"+str(start_index+i-1) + " = "+inp+str(start_index+i-1)+ " + r"+str(rand_index[0])+"\n")
        lines.append("j"+str(start_index+i+n//2-1) + " = "+inp+str(start_index+i+n//2-1)+ " + r"+str(rand_index[0])+"\n")
        rand_index[0] += 1

    lines.append("\n\n")

    ## Recursive calls
    lines = lines + nlogn_rec(n//2, start_index, rand_index, "j", "k")
    lines.append("\n\n")
    lines = lines + nlogn_rec(n//2, start_index+n//2, rand_index, "j", "k")
    lines.append("\n\n")

    for i in range(0,n//2):
        lines.append(out+str(start_index+i-1) + " = k"+str(start_index+i-1)+ " + r"+str(rand_index[0])+"\n")
        lines.append(out+str(start_index+i+n//2-1) + " = k"+str(start_index+i+n//2-1)+ " + r"+str(rand_index[0])+"\n")
        rand_index[0] += 1


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


def create_nlogn_copy(nb_shares):
    n = nb_shares
    r = [0]
    lines = nlogn_rec(n,1, r, "a", "d")
    lines2 = nlogn_rec(n,1, r, "a", "e")

    f = open("gadget_copy_"+str(n)+"_shares.sage", "w")
    f.write("#SHARES "+str(n)+"\n")
    f.write("#IN a\n")
    l = ""
    for i in range(0,r[0]):
        l += "r"+str(i)+ " "
    f.write("#RANDOMS "+ l)
    f.write("\n")
    f.write("#OUT d e\n\n")
    f.writelines(lines)
    f.write("\n\n")
    f.writelines(lines2)
    f.close()

def create_nlogn_add(nb_shares):
    n = nb_shares
    r = [0]
    lines = nlogn_rec(n,1, r, "a", "e")
    lines2 = nlogn_rec(n,1, r, "b", "f")

    f = open("gadget_add_"+str(n)+"_shares.sage", "w")
    f.write("#SHARES "+str(n)+"\n")
    f.write("#IN a b\n")
    l = ""
    for i in range(0,r[0]):
        l += "r"+str(i)+ " "
    f.write("#RANDOMS "+ l)
    f.write("\n")
    f.write("#OUT d\n\n")
    f.writelines(lines)
    f.write("\n\n")
    f.writelines(lines2)
    f.write("\n\n")

    for i in range(n):
        f.write("d"+str(i)+" = e"+str(i)+" + f"+str(i)+"\n")
    f.close()


create_nlogn_refresh(2)
create_nlogn_refresh(4)
create_nlogn_refresh(8)
create_nlogn_refresh(16)

create_nlogn_copy(2)
create_nlogn_copy(4)
create_nlogn_copy(8)
create_nlogn_copy(16)

create_nlogn_add(2)
create_nlogn_add(4)
create_nlogn_add(8)
create_nlogn_add(16)
