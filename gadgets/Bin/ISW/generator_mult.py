#!python

def isw(d):
    randoms = ""
    for i in range(d):
        for j in range(i+1, d):
            randoms += "r"+str(i)+str(j)+" "

    lines = []

    for i in range(d):
        for j in range(i+1, d):

            lines.append("tmp = a" +str(i) +" * b"+str(j)+"\n")
            lines.append("r"+str(j)+str(i)+" = r"+str(i)+str(j)+ " + tmp\n")

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
    f.write("#OUT c\n\n")
    f.writelines(lines)

    f.close()

isw(2)
isw(3)
isw(4)
isw(5)
isw(6)
isw(7)
isw(8)
isw(9)
isw(10)
