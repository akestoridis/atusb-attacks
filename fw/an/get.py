#!/usr/bin/python

from tmc.scope import rigol_ds1000c

#-800, +1600
s = rigol_ds1000c()
#s.debug = False

pos = s.hor.pos
scale = s.hor.scale
t0 = pos-scale*s.div_hor/2
t1 = pos+scale*s.div_hor/2
print t0, t1

#zoom = 10
#step = scale/s.samples_per_div/zoom
#print step
step = 4e-9
step = 2e-9

w = s.wave((s.ch[0], s.ch[1]), start = t0, end = t1, step = step)
w[0] = 3.3-w[0]
w[1] = 3.3-w[1]

s.hor.pos = pos
s.hor.scale = scale

w[0].label = "D+";
w[1].label = "D-";

w.save("_wv")
