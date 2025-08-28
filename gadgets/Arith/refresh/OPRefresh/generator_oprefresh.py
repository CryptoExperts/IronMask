#!/usr/bin/env python3

def new_refresh (n, q) :
    nh = n // 2
    randoms = ""
    for i in range(n):
            randoms += "r"+str(i)+" "
    
    lines = []
    #for l in range (nh) : 
    #  lines.append("z"+str(2 * l)+" =  r"+str(l)+"\n")
    #  lines.append("z"+str(2 * l + 1)+" = -1 r"+str(l)+"\n")

    #lines.append("\n")
    
    for l in range (nh - 1) :
      lines.append("z"+str(2 * l + 1)+" = -1 r"+str(l)+" + r"+str(nh + l)+"\n")
      lines.append("z"+str(2 * l + 2)+" = r"+str(l + 1)+" + -1 r"+str(nh + l)+"\n")

    lines.append("z0 = r0 + r"+str(n - 1)+"\n")
    lines.append("z"+str(n - 1)+" = -1 r"+str(nh - 1)+" + -1 r"+str(n - 1)+"\n")
    
    lines.append("\n")
    for i in range(n):
        lines.append("c"+str(i)+" = a"+str(i)+" + z"+str(i)+"\n")
        lines.append("\n")

    
    
    f = open("gadget_OPRefresh_"+str(n)+"_shares.sage", "w")
    f.write("#SHARES "+str(n)+"\n")
    f.write("#IN a\n")
    f.write("#RANDOMS ")
    f.write(randoms)
    f.write("\n")
    f.write("#OUT c\n")
    f.write("#CAR " + str(q)+"\n\n")
    f.writelines(lines)

    f.close()

def new_refresh_1layer (n, q) :
    nh = n // 2
    randoms = ""
    for i in range(nh):
            randoms += "r"+str(i)+" "
    
    lines = []
    for l in range (nh) : 
      lines.append("z"+str(2 * l)+" =  r"+str(l)+"\n")
      lines.append("z"+str(2 * l + 1)+" = -1 r"+str(l)+"\n")

    lines.append("\n")
    for i in range(n):
        lines.append("c"+str(i)+" = a"+str(i)+" + z"+str(i)+"\n")
        lines.append("\n")


    f = open("gadget_OPRefreshL1_"+str(n)+"_shares.sage", "w")
    f.write("#SHARES "+str(n)+"\n")
    f.write("#IN a\n")
    f.write("#RANDOMS ")
    f.write(randoms)
    f.write("\n")
    f.write("#OUT c\n")
    f.write("#CAR " + str(q)+"\n\n")
    f.writelines(lines)

    f.close()



    
q = 3329
for n in range (2, 17, 2) : 
  #new_refresh_1layer(n, q)
  new_refresh(n, q)
