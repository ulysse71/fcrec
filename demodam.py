import sys
import math

ll = []

# reading IQ data in ASCII file
filename=sys.argv[1]
print("reading data from file "+filename)
fd = open(filename, 'r')
ll = fd.readlines()
fd.close()

llen = len(ll)
llen2 = int(llen/2)
print("len "+str(llen))

freq=96000
dt=1./freq

# real demodulation 
lv = []
coeff = 10.
mx=0
mn=0
for c in range(llen2):
  i = float(ll[2*c])
  q = float(ll[2*c+1])
  # calculates complex modulus
  v = coeff * math.sqrt(i*i + q*q)
  # calculates min and max
  if v<mn: mn=v
  if v>mx: mx=v
  lv.append(v)

print("mx "+str(mx))
print("mn "+str(mn))
mid=0.5*(mx+mn)

# ascii dump
fd = open("demodam.dat", 'w')
for v in lv: fd.write(str(int(v-mid))+"\n")
fd.close()

# scipy wav dump
#import scipy.io.wavfile
#import numpy
#lv = map(lambda x: x/(mx-mid), lv)
#scipy.io.wavfile.write("sample.wav", freq, numpy.array(lv))

