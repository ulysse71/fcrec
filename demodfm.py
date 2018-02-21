import sys
import math

ll = []

filename=sys.argv[1]
print("reading data from file "+filename)
fd = open(filename, 'r')
ll = fd.readlines()
fd.close()

c=0
liq = []
llen = len(ll)
llen2 = int(llen/2)
for c in range(llen2):
  i = ll[2*c]
  q = ll[2*c+1]
  liq.append((i, q))

print("len "+str(llen))

twopi = 2. * math.pi

def regul(dtheta):
  if dtheta>math.pi:
    dtheta = dtheta - twopi
  elif dtheta<-math.pi:
    dtheta = dtheta + twopi
  return(dtheta)

freq=96000
dt=1./freq

lv = []
coeff = 0.05
theta0=0.
mx=0
mn=0
for c in range(llen2):
  i = ll[2*c]
  q = ll[2*c+1]
  theta1 = math.atan2(float(q), float(i))
  dtheta = theta1 - theta0
  dtheta = regul(dtheta)
  v = coeff * dtheta / dt
  if v<mn: mn=v
  if v>mx: mx=v
  lv.append(v)
  theta0 = theta1

print("mx "+str(mx))
print("mn "+str(mn))

# ascii dump
fd = open("demodfm.dat", 'w')
for v in lv: fd.write(str(int(v))+"\n")
fd.close()

# scipy wav dump
#import scipy.io.wavfile
#import numpy
#lv = map(lambda x: 0.5*x/mx, lv)
#scipy.io.wavfile.write("sample.wav", freq, numpy.array(lv))

