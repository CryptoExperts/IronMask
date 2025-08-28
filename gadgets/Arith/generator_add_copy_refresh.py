#!/usr/bin/env python3

def isw_refresh(d, q):
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
        lines.append("c"+str(i)+" = a"+str(i)+" + -1 r0"+str(i)+"\n")
        for j in range(1,i):
            lines.append("c"+str(i)+" = c"+str(i)+" + -1 r"+str(j)+str(i)+"\n")
        for j in range(i+1,d):
            lines.append("c"+str(i)+" = c"+str(i)+" + r"+str(i)+str(j)+"\n")
        lines.append("\n")


    f = open("gadget_refresh_"+str(d)+"_shares.sage", "w")
    f.write("#SHARES "+str(d)+"\n")
    f.write("#IN a\n")
    f.write("#RANDOMS ")
    f.write(randoms)
    f.write("\n")
    f.write("#OUT c\n")
    f.write("#CAR " + str(q)+"\n\n")
    f.writelines(lines)

    f.close()



def isw_copy(d, q):
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
        lines.append("c"+str(i)+" = a"+str(i)+" + -1 r0"+str(i)+"\n")
        for j in range(1,i):
            lines.append("c"+str(i)+" = c"+str(i)+" + -1 r"+str(j)+str(i)+"\n")
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
        lines.append("d"+str(i)+" = a"+str(i)+" + -1 rr0"+str(i)+"\n")
        for j in range(1,i):
            lines.append("d"+str(i)+" = d"+str(i)+" + -1 rr"+str(j)+str(i)+"\n")
        for j in range(i+1,d):
            lines.append("d"+str(i)+" = d"+str(i)+" + rr"+str(i)+str(j)+"\n")
        lines.append("\n")


    f = open("gadget_copy_"+str(d)+"_shares.sage", "w")
    f.write("#SHARES "+str(d)+"\n")
    f.write("#IN a\n")
    f.write("#RANDOMS ")
    f.write(randoms)
    f.write("\n")
    f.write("#OUT c d\n")
    f.write("#CAR "+str(q)+"\n\n")
    f.writelines(lines)

    f.close()


def isw_add(d, q):
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
        lines.append("c"+str(i)+" = a"+str(i)+" + -1 r0"+str(i)+"\n")
        for j in range(1,i):
            lines.append("c"+str(i)+" = c"+str(i)+" + -1 r"+str(j)+str(i)+"\n")
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
        lines.append("d"+str(i)+" = b"+str(i)+" + -1 rr0"+str(i)+"\n")
        for j in range(1,i):
            lines.append("d"+str(i)+" = d"+str(i)+" + -1 rr"+str(j)+str(i)+"\n")
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
    f.write("#OUT e\n")
    f.write("#CAR " + str(q) + "\n\n")
    f.writelines(lines)
    f.close()

q = 3329 #For Kyber studies

isw_refresh(2, q)
isw_refresh(3, q)
isw_refresh(4, q)
isw_refresh(5, q)
isw_refresh(6, q)
isw_refresh(7, q)
isw_refresh(8, q)
isw_refresh(9, q)
isw_refresh(10, q)

isw_copy(2, q)
isw_copy(3, q)
isw_copy(4, q)
isw_copy(5, q)
isw_copy(6, q)
isw_copy(7, q)
isw_copy(8, q)
isw_copy(9, q)
isw_copy(10, q)


isw_add(2, q)
isw_add(3, q)
isw_add(4, q)
isw_add(5, q)
isw_add(6, q)
isw_add(7, q)
isw_add(8, q)
isw_add(9, q)
isw_add(10, q)
