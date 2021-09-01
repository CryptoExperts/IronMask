#!python

def isw_refresh(d):
    randoms = ""
    for i in range(d):
        for j in range(i+1, d):
            randoms += "r"+str(i)+str(j)+" "

    lines = []
    lines.append("c0 = a0 + r01\n")
    for j in range(2,d):
        lines.append("c0 = c0 + r0"+str(j)+"\n")

    lines.append("\n")
    for i in range(1,d):
        lines.append("c"+str(i)+" = a"+str(i)+" + r0"+str(i)+"\n")
        for j in range(1,i):
            lines.append("c"+str(i)+" = c"+str(i)+" + r"+str(j)+str(i)+"\n")
        for j in range(i+1,d):
            lines.append("c"+str(i)+" = c"+str(i)+" + r"+str(i)+str(j)+"\n")
        lines.append("\n")


    f = open("gadget_refresh_"+str(d)+"_shares.sage", "w")
    f.write("#SHARES "+str(d)+"\n")
    f.write("#IN a\n")
    f.write("#RANDOMS ")
    f.write(randoms)
    f.write("\n")
    f.write("#OUT c\n\n")
    f.writelines(lines)

    f.close()



def isw_copy(d):
    randoms = ""
    for i in range(d):
        for j in range(i+1, d):
            randoms += "r"+str(i)+str(j)+" "
    lines = []
    lines.append("c0 = a0 + r01\n")
    for j in range(2,d):
        lines.append("c0 = c0 + r0"+str(j)+"\n")

    lines.append("\n")
    for i in range(1,d):
        lines.append("c"+str(i)+" = a"+str(i)+" + r0"+str(i)+"\n")
        for j in range(1,i):
            lines.append("c"+str(i)+" = c"+str(i)+" + r"+str(j)+str(i)+"\n")
        for j in range(i+1,d):
            lines.append("c"+str(i)+" = c"+str(i)+" + r"+str(i)+str(j)+"\n")
        lines.append("\n")


    for i in range(d):
        for j in range(i+1, d):
            randoms += "rr"+str(i)+str(j)+" "
    lines.append("d0 = a0 + rr01\n")
    for j in range(2,d):
        lines.append("d0 = d0 + rr0"+str(j)+"\n")

    lines.append("\n")
    for i in range(1,d):
        lines.append("d"+str(i)+" = a"+str(i)+" + rr0"+str(i)+"\n")
        for j in range(1,i):
            lines.append("d"+str(i)+" = d"+str(i)+" + rr"+str(j)+str(i)+"\n")
        for j in range(i+1,d):
            lines.append("d"+str(i)+" = d"+str(i)+" + rr"+str(i)+str(j)+"\n")
        lines.append("\n")


    f = open("gadget_copy_"+str(d)+"_shares.sage", "w")
    f.write("#SHARES "+str(d)+"\n")
    f.write("#IN a\n")
    f.write("#RANDOMS ")
    f.write(randoms)
    f.write("\n")
    f.write("#OUT c d\n\n")
    f.writelines(lines)

    f.close()


def isw_add(d):
    randoms = ""
    for i in range(d):
        for j in range(i+1, d):
            randoms += "r"+str(i)+str(j)+" "
    lines = []
    lines.append("c0 = a0 + r01\n")
    for j in range(2,d):
        lines.append("c0 = c0 + r0"+str(j)+"\n")

    lines.append("\n")
    for i in range(1,d):
        lines.append("c"+str(i)+" = a"+str(i)+" + r0"+str(i)+"\n")
        for j in range(1,i):
            lines.append("c"+str(i)+" = c"+str(i)+" + r"+str(j)+str(i)+"\n")
        for j in range(i+1,d):
            lines.append("c"+str(i)+" = c"+str(i)+" + r"+str(i)+str(j)+"\n")
        lines.append("\n")


    for i in range(d):
        for j in range(i+1, d):
            randoms += "rr"+str(i)+str(j)+" "
    lines.append("d0 = b0 + rr01\n")
    for j in range(2,d):
        lines.append("d0 = d0 + rr0"+str(j)+"\n")

    lines.append("\n")
    for i in range(1,d):
        lines.append("d"+str(i)+" = b"+str(i)+" + rr0"+str(i)+"\n")
        for j in range(1,i):
            lines.append("d"+str(i)+" = d"+str(i)+" + rr"+str(j)+str(i)+"\n")
        for j in range(i+1,d):
            lines.append("d"+str(i)+" = d"+str(i)+" + rr"+str(i)+str(j)+"\n")
        lines.append("\n")

    lines.append("\n\n")

    for i in range(d):
        lines.append("e"+str(i)+" = c"+str(i)+" + d"+str(i)+"\n")


    f = open("gadget_add_"+str(d)+"_shares.sage", "w")
    f.write("#SHARES "+str(d)+"\n")
    f.write("#IN a b\n")
    f.write("#RANDOMS ")
    f.write(randoms)
    f.write("\n")
    f.write("#OUT e\n\n")
    f.writelines(lines)
    f.close()


isw_refresh(2)
isw_refresh(3)
isw_refresh(4)
isw_refresh(5)
isw_refresh(6)
isw_refresh(7)
isw_refresh(8)
isw_refresh(9)
isw_refresh(10)

isw_copy(2)
isw_copy(3)
isw_copy(4)
isw_copy(5)
isw_copy(6)
isw_copy(7)
isw_copy(8)
isw_copy(9)
isw_copy(10)


isw_add(2)
isw_add(3)
isw_add(4)
isw_add(5)
isw_add(6)
isw_add(7)
isw_add(8)
isw_add(9)
isw_add(10)
