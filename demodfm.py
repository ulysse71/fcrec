import math

ll = []

f = open('fcrec.dat','r')
ll = f.readlines()
f.close()

c=0
liq = []
llen = len(ll)
llen2 = llen/2
for c in xrange(llen2):
  i = ll[2*c]
  q = ll[2*c+1]
  liq.append((i, q))

#print("len", llen)

def regul(dtheta):
  if dtheta>math.pi:
    dtheta = dtheta - 2.*math.pi
  elif dtheta<-math.pi:
    dtheta = dtheta + 2.*math.pi
  return(dtheta)

freq=96000
dt=1./freq

lv = []
coeff = 0.1
theta0=0.
mx=0
mn=0
for c in xrange(llen2):
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

# ascii dump
for v in lv: print(v)

# scipy wav dump
#import scipy.io.wavfile
#scipy.io.wavfile.write("fcrec.wav", freq, lv)

