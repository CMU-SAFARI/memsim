#!/usr/bin/env python

from kvdata import KeyValueData
from plotlib import PlotLib
import sys
import os

#mechanisms = ['LRU', 'UCP', 'DIP', 'DRRIP',\
#                  'VTS', 'D-VTS']

#mechanisms = 'LRU DIP DRRIP SHIP RTB MCT EAF D-EAF'.split()
#mechanisms = 'DRRIP EAF D-EAF'.split()
#mechanisms = 'LRU EAF D-EAF'.split()
mechanisms = ['DRRIP', 'SHIP', 'MCT', 'EAF', 'D-EAF']
#mechanisms = 'LRU RRIP EAF-LRU EAF-RRIP'.split()
#mechanisms = 'EAF +B +N +NB +C +CB +NC +NCB'.split()
data = {}
for mech in mechanisms:
    data[mech] = {}

workload_order = []

fin = open(sys.argv[1])
for line in fin:
    fields = line.split()
    key = fields[0]
    values = [float(val) for val in fields[1:]]
    workload_order.append(key)

    for i,mech in enumerate(mechanisms):
        data[mech][key] = values[i]

kv = KeyValueData()
for name, values in data.iteritems():
    kv.add_data(name, values)
kv.extract_keys()

print (kv.keys())

#mechanisms = mechanisms[1:]

plot = PlotLib()
legend = mechanisms[:]
plot.set_y_tics_step(0.1)
plot.set_y_tics_shift(0)
plot.set_legend_shift(-50,-10)
plot.set_plot_dimensions(220,80)
plot.set_scale(0.5)
plot.set_y_tics_font_size("scriptsize")
plot.set_x_tics_font_size("scriptsize")
plot.set_y_label_font_size("small")
plot.set_x_label_font_size("small")
plot.set_ceiling(0.85)
plot.xlabel("Workloads", yshift = -10, fontsize = "small", options =
"")
plot.ylabel(sys.argv[2], xshift = -2, fontsize = "small", options = "")
plot.column_stacked_bars(workload_order, [kv.values(name, key_order = workload_order) for name in mechanisms], legend, key_padding = 0.3, value_base = 0)
plot.save_pdf(os.path.basename(sys.argv[1]))
plot.save_tikz(os.path.basename(sys.argv[1]) + ".tex")
