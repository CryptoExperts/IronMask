#!/usr/bin/env python3

import sys



def print_test_NI (str1, filename) :
  f = open(filename, 'r')
  str2 = ""
  line = f.readline()
  str2 += line[:-1]
  
  if (str1 == str2) :
    print("*** Passed ***")
    sys.exit(0)
  else :
    print("*** Failed ***")
    sys.exit(1)

def print_test_RPE (str1, str2, filename) :
  if (print_test_RPE1(str1, filename) and print_test_RPE2(str2, filename)) :
    print("*** Passed ***")
    sys.exit(0)
    
  else :
    print("*** Failed ***")
    sys.exit(1)
  
def print_test_RPE1 (str1, filename) :
  ind = 0
  f = open(filename, 'r')
  for _ in range(3) :
    str_prop = ""
    while (str1[ind] != '[') :
      ind += 1
    while (str1[ind] != ']') :
      str_prop += str1[ind]
      ind += 1
    str_prop += str1[ind]
    
    str2 = ""
    ind2 = 0
    line = f.readline()
    cpt = 0
    while(line[:11] != "Coeffs RPE1" and cpt < 15) :
      cpt += 1
      line = f.readline()
    line = f.readline()
    
    while (line[ind2] != '['):
      ind2 += 1
      
    str2 += line[ind2:-1]
    
    if (str_prop != str2) :
      return False
    
    return True
    
def print_test_RPE2 (str1, filename) :
  ind = 0
  f = open(filename, 'r')
  for _ in range(3) :
    str_prop = ""
    while (str1[ind] != '[') :
      ind += 1
    while (str1[ind] != ']') :
      str_prop += str1[ind]
      ind += 1
    str_prop += str1[ind]
    
    str2 = ""
    ind2 = 0
    line = f.readline()
    while(line[:11] != "Coeffs RPE2") :
      line = f.readline()
    line = f.readline()
    
    while (line[ind2] != '['):
      ind2 += 1
      
    str2 += line[ind2:-1]
    
    if (str_prop != str2) :
      return False
    
    return True 
    
def print_test_RPE_COPY (str1, filename) :
  ind = 0
  fail = False
  f = open(filename, 'r')
  for _ in range(4) :
    str_prop = ""
    while (str1[ind] != '[') :
      ind += 1
    while (str1[ind] != ']') :
      str_prop += str1[ind]
      ind += 1
    str_prop += str1[ind]
    
    str2 = ""
    ind2 = 0
    line = f.readline()
    while(line[0] != 'I') :
      line = f.readline()
    
    while (line[ind2] != '['):
      ind2 += 1
      
    str2 += line[ind2:-1]
    
    if (str_prop != str2) :
      print("*** Failed ***")
      sys.exit(1)
    
  
  if (not fail) :
    print("*** Passed ***")    


def print_test_CRPC (str1, filename) :
  ind = 0
  f = open(filename, 'r')
  line = f.readline()
  ind_str1 = 0
  tin = 0
  tout = 0
  is_equal = True
  while (line != "") :
    if (line[:3] == "tin") :
      tin = line[6]
      tout = line[15]
    
    if (line[:7] == " f(p) =") :
      coeff_f = line[8:]
      coeff_str1 = ""
      while (str1[ind_str1] != '[') :
        ind_str1 += 1
      while (str1[ind_str1] != '\n') :
        coeff_str1 += str1[ind_str1]
        ind_str1 += 1
      coeff_str1 += str1[ind_str1]

      is_equal = (is_equal and (coeff_f == coeff_str1))

    line = f.readline()


  if (is_equal) :
    print("*** Passed ***")   

  else :
    print("*** Failed ***")
    sys.exit(1)


  
if __name__ == '__main__' :
  if (len(sys.argv) == 5) :
    globals()[sys.argv[1]](sys.argv[2], sys.argv[3], sys.argv[4])
    
  else : 
    globals()[sys.argv[1]](sys.argv[2], sys.argv[3])
