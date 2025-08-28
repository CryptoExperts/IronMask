#!/usr/bin/env python3

def mult(d, rotation, q):
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
            lines.append("tmp = -1 rr" +str(cpt + init) + " + rr" + str((cpt+rotation)%d + init) +"\n")
            lines.append("h"+str(i)+str(j)+ " = tmp + a" +str(j)+"\n")

            lines.append("\n")

            cpt += 1

        init += d
        cpt = 0

        lines.append("\n")


    for i in range(d):        
        for j in range(d):
            lines.append("tmp = h"+str(i)+str(j) + " * b"+str(i)+"\n")
            lines.append("p"+str(i)+str(j)+ " = tmp + r"+str(i)+str(j)+"\n")
            lines.append("\n")
            
    for i in range (d) :
        lines.append("v" +str(i)+" = p"+str(i)+"0 + p"+str(i)+"1\n")
        for j in range(2,d) :
          lines.append("v" + str(i) + " = v"+str(i)+ " + p"+str(i)+str(j)+"\n") 
    
    lines.append("\n")
            
    for i in range (d) :
        lines.append("x" +str(i)+" = -1 r"+str(i)+"0 + -1 r"+str(i)+"1\n")
        for j in range(2,d) :
          lines.append("x" + str(i) + " = x"+str(i)+ " + -1 r"+str(i)+str(j)+"\n") 
    
    lines.append("\n")        

    for i in range(d) :
      lines.append("c"+str(i) + " = v"+str(i) + " + x"+str(i)+"\n")

    f = open("gadget_mult_ref_"+str(d)+"_shares.sage", "w")
    f.write("#ORDER "+str(d-1)+"\n")
    f.write("#SHARES "+str(d)+"\n")
    f.write("#IN a b \n")
    f.write("#RANDOMS ")
    for i in range(d):
        for j in range(d):
            f.write("r"+str(i)+str(j)+" ")

    for i in range(init):
        f.write("rr"+str(i)+" ")

    f.write("\n")
    f.write("#OUT c\n")
    f.write("#CAR "+ str(q)+"\n\n")
    f.writelines(lines)

    f.close()

q = 3329

mult(2, 1, q)
mult(3, 1, q)
mult(4, 1, q)
mult(5, 1, q)
mult(6, 1, q)
mult(7, 1, q)
mult(8, 1, q)
