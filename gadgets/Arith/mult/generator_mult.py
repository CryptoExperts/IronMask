#!/usr/bin/env python3

def isw(d, q):
    randoms = ""
    for i in range(d):
        for j in range(i+1, d):
            randoms += "r"+str(i)+str(j)+" "

    lines = []

    for i in range(d):
        for j in range(i+1, d):

            lines.append("tmp = a" +str(i) +" * b"+str(j)+"\n")
            lines.append("r"+str(j)+str(i)+" = -1 r"+str(i)+str(j)+ " + tmp\n")

            lines.append("tmp = a" +str(j) +" * b"+str(i)+"\n")
            lines.append("r"+str(j)+str(i)+" = r"+str(j)+str(i)+ " + tmp\n")

            lines.append("\n")

    for i in range(d):
        first = True
        lines.append("tmp = a" +str(i) +" * b"+str(i)+"\n")

        for j in range(d):
            if(i != j):
                if(first):
                    lines.append("c"+str(i)+ " = tmp + r"+str(i)+str(j)+"\n")
                    first = False
                else:
                    lines.append("c"+str(i)+ " = c"+str(i)+ " + r"+str(i)+str(j)+"\n")

        lines.append("\n")


    f = open("gadget_mult_"+str(d)+"_shares.sage", "w")
    f.write("#ORDER "+str(d-1)+"\n")
    f.write("#SHARES "+str(d)+"\n")
    f.write("#IN a b\n")
    f.write("#RANDOMS ")
    f.write(randoms)
    f.write("\n")
    f.write("#OUT c\n")
    f.write("#CAR " + str(q) + "\n\n")
    f.writelines(lines)

    f.close()

q = 3329

isw(2,q)
isw(3,q)
isw(4,q)
isw(5,q)
isw(6,q)
isw(7,q)
isw(8,q)
isw(9,q)
isw(10,q)
