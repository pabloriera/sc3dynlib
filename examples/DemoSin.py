import time
from sc3dynlib import Dynlib
from sc3.all import *

s.boot()
time.sleep(8)

@synthdef
def demoSin(amp=1.0, freq=100):
    gen = Dynlib.ar('DemoSin', freq)*amp
    Out(0, gen)

time.sleep(3)

o = demoSin(amp=1, freq=220)
time.sleep(3)

o.set('amp',0.5)
o.set('freq',300)

time.sleep(3)
o.free()