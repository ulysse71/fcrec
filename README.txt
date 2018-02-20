Light Funcube Reader

For educational purpose

Beware, wont compile, needs HIDAPI to work
see //github.com/csete/fcdctl.git

example:
  acquire IQ data at 105.5MHz gain 20 during 8s with
  ./fcrec 105.5 20 8

  then process FM demodulation with
  ./try-pyfm fcrec.dat

  or process AM demodulation with
  ./try-pyam fcrec.dat

TODO
  correct build against HIDAPI

