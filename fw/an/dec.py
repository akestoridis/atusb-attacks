#!/usr/bin/python

from tmc.wave import *
from tmc.dxplore import dxplore
from tmc.decode import d_usb_stream


#
# Clock recovery: we assume that each change in the wave is triggered by a
# clock edge. We know the clock's nominal period and resynchronize on each
# edge. Additionally, we can obtain a list of times when a timing violation
# has occurred.
#
# Note that the timing violations logic doesn't make much sense in its present
# form, since it mainly measures noise (particularly if we're digitizing slow
# edges) and not clock drift.
#
# A more useful metric would be accumulated error from some point of reference
# or at least the timing of same edges, to eliminate (generally harmless) time
# offsets introduced by digitizing.
#
# So it would probably make more sense for "recover" not to check for timing
# violations at all, and leave this to more specialized functions.
#
def recover(self, period, min = None, max = None, t0 = None):
    if t0 is None:
	t0 = self.data[0]
    v = not self.initial
    res = []
    violations = []
    for t in self.data:
	v = not v
	if t <= t0:
	    continue
	n = 0
	while t0 < t-period/2:
	    res.append(t0)
	    t0 += period
	    n += 1
	if min is not None:
	    if t0-t > n*min:
		violations.append(t)
	if max is not None:
	    if t-t0 > n*max:
		violations.append(t)
	t0 = t
    return res, violations


#
# Load the analog waves saved by get.py
#
wv = waves()
wv.load("_wv")

#
# Digitize the waves and save the result.
#
dp = wv[0].digitize(1.5, 1.8)
dm = wv[1].digitize(1.5, 1.8)
wv = waves(dp, dm, dp-dm)
wv.save("_dig")

#
# Also record the differential signal.
#
wd = wv[1]-wv[0]
dd = wd.digitize(-0.5, 0.5)
wd.save("_diff")

#
# Run clock recovery on D+/D-. We only need one, but check both to be sure.
#
#p = 1/1.5e6
p = 1/12e6
dp_t, viol = recover(dp, p, p*0.9, p*1.1)
print viol
dm_t, viol = recover(dm, p, p*.9, p*1.1, t0 = dp.data[0])
print viol

#
# Shift the clock by half a period, add a few periods to get steady state and
# SE0s (if any), and then sample the data lines.
#
clk = map(lambda t: t+p/2, dp_t)
clk.extend((clk[-1]+p, clk[-1]+2*p, clk[-1]+3*p))
dp_bv = dp.get(clk)
dm_bv = dm.get(clk)

#
# Save a wave with the recovered clock to make it easier to find the bits in
# analog graphs.
#
dd.data = dp_t;
dd.save("_clk")

#
# For decoding, we need a fake bit clock. We generate it by doubling each data
# bit and generating a L->H transition during this bit.
#
dpd = []
dmd = []
dck = []

# err, silly, seems that we've mixed up D+ and D- all over the place :-)
print d_usb_stream(dm_bv[:], dp_bv[:])

for v in dp_bv:
    dpd.append(v)
    dpd.append(v)
    dck.append(0)
    dck.append(1)

for v in dm_bv:
    dmd.append(v)
    dmd.append(v)

#
# Display the reconstructed digital signal. Note that the absolute time is only
# correct at the beginning and that relative time is only accurate over
# intervals in which no significant clock resynchronization has occurred.
#
# In fact, dxplore should probably have an option to either turn off time
# entirely or to display a user-provided time axis. The latter may be a bit
# tricky to implement.
#
dxplore((dmd, dpd, dck), 0, p/2, labels = ("D+", "D-", "CLK"))
