import sys, os
sys.path.append('/data/sanchez/results/pacemaker/zsim-scripts/graph-scripts')

import powerModel


pm = powerModel.HWPowerModelOld()
freqs = range(800, 3500, 100)

for f in freqs:
    p = pm.corePower(f, 1.0, 1.0/4.0, 0.0)
    print '%u : %.2f' % (f, p)


