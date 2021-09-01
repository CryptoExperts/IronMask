#!python

def mult(d, rotation):
    lines = []
    randoms = ""
    for i in range(d):
        for j in range(i+1, d):
            randoms += "r"+str(i)+str(j)+" "

    #Refresh both inputs
    init = 0
    cpt = 0
    lines.append("\n")
    for i in range(d):

        for j in range(d):
            lines.append("tmp = rr" +str(cpt + init) + " + rr" + str((cpt+rotation)%d + init) +"\n")
            lines.append("h"+str(i)+str(j)+ " = tmp + a" +str(j)+"\n")

            lines.append("\n")

            cpt += 1

        init += d
        cpt = 0

        lines.append("\n")


    table = [0 for i in range(d)]
    for i in range(d):
        for j in range(i+1, d):

            lines.append("tmp = h" +str(table[i])+str(i) +" * b"+str(j)+"\n")
            lines.append("r"+str(j)+str(i)+" = r"+str(i)+str(j)+ " + tmp\n")
            table[i] += 1

            lines.append("tmp = h" +str(table[j])+str(j) +" * b"+str(i)+"\n")
            lines.append("r"+str(j)+str(i)+" = r"+str(j)+str(i)+ " + tmp\n\n")
            table[j] += 1


    for i in range(d):
        first = True
        lines.append("tmp = h" +str(table[i])+str(i) +" * b"+str(i)+"\n")
        table[i] += 1

        for j in range(i+1,d):
            if(first):
                lines.append("c"+str(i)+ " = tmp + r"+str(i)+str(j)+"\n")
                first = False
            else:
                lines.append("c"+str(i)+ " = c"+str(i)+ " + r"+str(i)+str(j)+"\n")

        for j in range(i):
            if(first):
                lines.append("c"+str(i)+ " = tmp + r"+str(i)+str(j)+"\n")
                first = False
            else:
                lines.append("c"+str(i)+ " = c"+str(i)+ " + r"+str(i)+str(j)+"\n")

        lines.append("\n")

    f = open("gadget_mult_ref_"+str(d)+"_shares.sage", "w")
    f.write("#ORDER "+str(d-1)+"\n")
    f.write("#SHARES "+str(d)+"\n")
    f.write("#IN a b \n")
    f.write("#RANDOMS ")
    for i in range(d):
        for j in range(i+1, d):
            f.write("r"+str(i)+str(j)+" ")

    for i in range(init):
        f.write("rr"+str(i)+" ")

    f.write("\n")
    f.write("#OUT c\n\n")
    f.writelines(lines)

    f.close()


mult(3, 1)
mult(4, 1)
mult(5, 1)
mult(6, 1)
mult(7, 1)
mult(8, 1)
