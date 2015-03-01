#!/usr/bin/env python

# ------------------------------------------------------------------------------
# File: parse_results.py
# Description:
# 		This script parses the simulation result files and generates data and
# 		plots. Works only for single programmed experiments
# ------------------------------------------------------------------------------


# ------------------------------------------------------------------------------
# Import necessary libraries
# ------------------------------------------------------------------------------

import sys
import os
import argparse
import commands
import re
from kvdata import KeyValueData
from plotlib import PlotLib

# ------------------------------------------------------------------------------
# DEFINITIONS
# ------------------------------------------------------------------------------

WORKLOAD_FOLDER = "Simulator/Workloads/"

# ------------------------------------------------------------------------------
# Function to parse results file
# ------------------------------------------------------------------------------

def parse_simulation_log(bench_folder):

    # --------------------------------------------------------------------------
    # Open the main simulation log
    # --------------------------------------------------------------------------

    fin = open(bench_folder + "/SimulationLog", "r")
    data = {}

    # --------------------------------------------------------------------------
    # for each line in the file, read the data into the dictionary
    # --------------------------------------------------------------------------
    
    for line in fin:
        (component, field, value) = re.split("[:= ]+", line.strip())
        if component not in data:
            data[component] = {}
        data[component][field] = float(value)

    fin.close()

    # --------------------------------------------------------------------------
    # Open the ipc file
    # --------------------------------------------------------------------------

    data["sim"] = {"insts":{}, "cycles":{}, "ipc":{}}
    data["sim"]["throughput"] = 0

    if 'llc' not in data:
        print bench_folder
        exit
    
    #data["llc"]["ins_prefetches"] = data["llc"]["used_prefetches"] + data["llc"]["unused_prefetches"] + 1
    
    #data["llc"]["used_prefetch_frac"] = (data["llc"]["used_prefetches"] * 100) / data["llc"]["ins_prefetches"]
    #data["llc"]["unused_prefetch_frac"] = (data["llc"]["unused_prefetches"] * 100) / data["llc"]["ins_prefetches"]

   # if "predicted_accurate" in data["llc"]:
   #     data["llc"]["predicted_accurate_frac"] = (data["llc"]["predicted_accurate"] * 100) / data["llc"]["ins_prefetches"]
   #     data["llc"]["accurate_predicted_inaccurate_frac"] = (data["llc"]["accurate_predicted_inaccurate"] * 100) / data["llc"]["ins_prefetches"]
   #     data["llc"]["inaccurate_predicted_accurate_frac"] = (data["llc"]["accurate_predicted_inaccurate"] * 100) / data["llc"]["ins_prefetches"]
   #     data["llc"]["incorrect_frac"] = ((data["llc"]["accurate_predicted_inaccurate"] + data["llc"]["inaccurate_predicted_accurate"]) * 100) / data["llc"]["ins_prefetches"]
    
   # data["llc"]["used_prefetches"] += 1
   # data["llc"]["reused_prefetch_frac"] = (data["llc"]["reused_prefetches"] * 100) / data["llc"]["used_prefetches"]
   # data["llc"]["unreused_prefetch_frac"] = (data["llc"]["unreused_prefetches"] * 100) / data["llc"]["used_prefetches"]

   # data["llc"]["prefetch_use_cycle"] = (data["llc"]["prefetch_use_cycle"]) /data["llc"]["used_prefetches"]
   # data["llc"]["prefetch_use_miss"] = (data["llc"]["prefetch_use_miss"]) /data["llc"]["used_prefetches"]
    
    
   # data["llc"]["prefetch_lifetime_cycle"] = (data["llc"]["prefetch_lifetime_cycle"]) /data["llc"]["ins_prefetches"]
   # data["llc"]["prefetch_lifetime_miss"] = (data["llc"]["prefetch_lifetime_miss"]) /data["llc"]["ins_prefetches"]


    fin = open(bench_folder + "/sim.ipc", "r")

    max_cycles = 0
    for line in fin:
        (cpuid, icount, cycles) = line.split()
        max_cycles = max(max_cycles, cycles)
        cpuid = int(cpuid)
        data["sim"]["insts"][cpuid] = int(icount)
        data["sim"]["cycles"][cpuid] = int(cycles)
        ipc = float(icount) / float(cycles)
        data["sim"]["ipc"][cpuid] = ipc
        data["sim"]["throughput"] += ipc
   
    fin.close()

    return data


# ------------------------------------------------------------------------------
# Function to compute the speed ups 
# ------------------------------------------------------------------------------

def compute_speedup(sets, alone_run):

    # --------------------------------------------------------------------------
    # For each set, benchmark within the set
    # --------------------------------------------------------------------------

    for name, value in sets.iteritems():
        for workload, data in value.iteritems():

            # ------------------------------------------------------------------
            # Initialize ws and hs
            # ------------------------------------------------------------------
            
            data["sim"]["ws"] = 0
            data["sim"]["hs"] = 0
            data["sim"]["ms"] = 0
            data["sim"]["speedup"] = {}

            # ------------------------------------------------------------------
            # Get the list of benchmarks in the workload
            # ------------------------------------------------------------------
            
            benchmarks = workload.split("-")
            for i, benchmark in enumerate(benchmarks):
                speedup = data["sim"]["ipc"][i] / \
                    alone_run[benchmark]["sim"]["ipc"][0]

                data["sim"]["speedup"][i] = speedup
                data["sim"]["ws"] += speedup
                data["sim"]["hs"] += 1/speedup
                data["sim"]["ms"] = max(data["sim"]["ms"] , 1/speedup)

            data["sim"]["hs"] = len(benchmarks) / data["sim"]["hs"]



# ------------------------------------------------------------------------------
# Function to get metric
# ------------------------------------------------------------------------------

def get_metric(name):

    # --------------------------------------------------------------------------
    # Return metric depending on the name
    # --------------------------------------------------------------------------

    if name == "mpki":
        return ("llc:misses", 1000)
    elif name == "read_intensity":
        return ("llc:reads", 1000)
    elif name == "write_intensity":
        return ("llc:writes", 1000)
    elif name == "throughput":
        return ("sim:throughput", None)
    elif name == "ws":
        return ("sim:ws", None)
    elif name == "hs":
        return ("sim:hs", None)
    elif name == "ms":
        return ("sim:ms", None)
    elif name == "prefetch_accuracy":
        return ("llc:used_prefetch_frac", None)
    elif name == "upp_rate":
        return ("llc:unreused_prefetch_frac", None)
    elif name == "pref_lifetime":
        return ("llc:prefetch_lifetime_cycle", None)
    elif name == "pref_lifetime_miss":
        return ("llc:prefetch_lifetime_miss", None)
    elif name == "pref_use_cycle":
        return ("llc:prefetch_use_cycle", None)
    elif name == "pref_use_miss":
        return ("llc:prefetch_use_miss", None)
    elif name == "pref_pred_acc":
        return ("llc:predicted_accurate_frac", None)
    elif name == "incorrect_pred":
        return ("llc:incorrect_frac", None)
    
    print "Error: Undefined metric name"
    quit()


# ------------------------------------------------------------------------------
# Create the command line parser
# ------------------------------------------------------------------------------

parser = argparse.ArgumentParser(description = "Simulation results parser")
parser.add_argument("--compare", action = "append", default = [],
    metavar = "NAME:FOLDER", help = "A comparison point")
parser.add_argument("--workload-file", action = "store", metavar = "FILE", 
    help = "Path to the workload file containing the list of benchmarks")
parser.add_argument("--compare-folder", action = "store", metavar = "FOLDER",
    help = "Multiple comparison points using wild cards")
parser.add_argument("--metric", action = "store", 
    help = "A predefined metric like ipc, mpki, throughput, etc")
parser.add_argument("--metric-name", action = "store", 
    metavar = "COMPONENT:FIELD", 
    help = "Metric to be extracted from the logs llc:misses")
parser.add_argument("--per-n-insts", action = "store", type = int,
    metavar = "N", help = "Compute the value per N instructions")
parser.add_argument("--alone-run", action = "store", metavar = "FOLDER",
    help = "Folder containing the alone run results")
parser.add_argument("--plot", action = "store_true", default = False,
    help = "Generate a plot with the gathered data")
parser.add_argument("--plot-name", action = "store", default = "a",
    help = "Name of the plot. To be used for the file")
parser.add_argument("--plot-line", action = "store_true", default = False,
    help = "Plot a line graph instead of bars")
parser.add_argument("--output", action = "store", default = "pdf",
    help = "Output type: pdf, tex, tikz")
parser.add_argument("--normalize", action = "store", metavar = "NAME",
    help = "Name of the data set to use for normalizing")
parser.add_argument("--ignore", action = "append", default = [],
    help = "List of keys to ignore")
parser.add_argument("--no-mean", action = "store_true", default = False,
    help = "Do not compute the mean")
parser.add_argument("--amean", action = "store_true", default = False,
    help = "arith mean")
parser.add_argument("--value-base", action = "store", type = float,
    default = 0, help = "Minimum y-axis value")
parser.add_argument("--sort-workload", action = "store", 
    help = "Sort by value for particular set")
parser.add_argument("--title", action = "store",
    help = "Title for the plot")
parser.add_argument("--y-label", action = "store",
    help = "Y-Label for the plot")
parser.add_argument("--x-label", action = "store", default = "Workloads",
    help = "X-Label for the plot")
parser.add_argument("--y-tics-step", action = "store", type = float,
    help = "Y-Tics step to be used for the plot")
parser.add_argument("--no-legend", action = "store_true", 
    help = "Plot no legends")
parser.add_argument("--print-table", action = "store_true", default = False,
    help = "Print a table of gathered data")
parser.add_argument("--only-mean", action = "store_true", default = False,
    help = "Print only the geometric mean")
parser.add_argument("--no-header", action = "store_true", default = False,
    help = "Do not print the header")
parser.add_argument("--workload-name", action = "store", 
    help = "Name for the workload")
parser.add_argument("--justify-width", action = "store", type = int, 
    default = 0, help = "Justification for workload name")
parser.add_argument("--legend-x-shift", action = "store", type = int, 
    default = 0, help = "Legend x shift")
parser.add_argument("--legend-y-shift", action = "store", type = int,
    default = 0, help = "Legned y shifg")
parser.add_argument("--x-padding", action = "store", type = int,
    default = 5, help = "x padding for continuous line plots")
parser.add_argument("--round", action = "store", type = int,
    default = 2, help = "Round to as many digits")
parser.add_argument("--ceiling", action = "store", type = float,
    default = 0.8, help = "Ceiling for the plot")
parser.add_argument("--x-tics-step", action = "store", type = int,
    default = 10, help = "X-tics step")
parser.add_argument("--plot-width", action = "store", type = int,
    default = 200, help = "plot width")
parser.add_argument("--plot-height", action = "store", type = int,
    default = 100, help = "plot height")

args = parser.parse_args()


sets = {}
set_order = []
workload_order = []

# ------------------------------------------------------------------------------
# Check if multiple comparison points are given
# ------------------------------------------------------------------------------

if args.compare_folder is not None:
    compare_list = commands.getoutput("ls " + args.compare_folder + "| sort -n")
    for compare_point in compare_list.split():
        args.compare.append(compare_point + ":" + args.compare_folder + "/" + \
            compare_point)


# ------------------------------------------------------------------------------
# For all the comparison points, get the data for all available benchmarks
# ------------------------------------------------------------------------------

for point in args.compare:

    # --------------------------------------------------------------------------
    # Get the key and folder
    # --------------------------------------------------------------------------

    (set_name, folder) = point.split(":")
    if set_name in sets:
        print "Cannot have multiple sets with the same name"
        quit()

    if not os.path.exists(folder):
        print "Target folder `" + folder + "' does not exist"
        quit()

    sets[set_name] = {}
    set_order.append(set_name)

    
    # --------------------------------------------------------------------------
    # Read the data from the folder
    # --------------------------------------------------------------------------

    if args.workload_file is None:
        benchlist = commands.getoutput("ls " + folder)
    else:
        benchlist = commands.getoutput("cat " +  \
            args.workload_file)

    workload_order = benchlist.split()
    for benchmark in benchlist.split():
        sets[set_name][benchmark] = parse_simulation_log(folder + "/" + \
            benchmark)


# ------------------------------------------------------------------------------
# Compute the speedups if alone run is specified
# ------------------------------------------------------------------------------

if args.alone_run is not None:

    # --------------------------------------------------------------------------
    # Check if the folder is present
    # --------------------------------------------------------------------------

    folder = args.alone_run
    if not os.path.exists(folder):
        print "Target folder `" + folder + "' does not exist"
        quit()

    # --------------------------------------------------------------------------
    # Parse alone run details
    # --------------------------------------------------------------------------
    
    alone_run = {}
    benchlist = commands.getoutput("ls " + folder)
    for benchmark in benchlist.split():
        alone_run[benchmark] = parse_simulation_log(folder + "/" + benchmark)

    # --------------------------------------------------------------------------
    # Compute speed ups for the comparison points
    # --------------------------------------------------------------------------
    
    compute_speedup(sets, alone_run)


# ------------------------------------------------------------------------------
# Identify the metric to be computed
# ------------------------------------------------------------------------------

if args.metric_name is None:
    if args.metric is None:
        print "Error: Cannot identify metric to be computed"
        quit()
    else:
        (metric_name, per_n_insts) = get_metric(args.metric)
else:
    metric_name = args.metric_name
    per_n_insts = args.per_n_insts

(component, field) = metric_name.split(":")


data = {}

# ------------------------------------------------------------------------------
# Get the required data from each set
# ------------------------------------------------------------------------------

for set_name in sets:
    data[set_name] = {}
    for benchmark, values in sets[set_name].iteritems():
        if component not in values:
            print component + " not present for benchmark " + benchmark, set_name
            quit()
        elif field not in values[component]:
            print field, "not present in component", component, benchmark, set_name
            quit()
        data[set_name][benchmark] = values[component][field]
        if per_n_insts is not None:
            data[set_name][benchmark] = float(data[set_name][benchmark]) * \
                per_n_insts / values["sim"]["insts"][0]
        

# ------------------------------------------------------------------------------
# Use kv data to polish the data
# ------------------------------------------------------------------------------

kv = KeyValueData()
for name, values in data.iteritems():
    kv.add_data(name, values)
kv.extract_keys()

copy_wo = workload_order[:]
avail_keys = kv.keys()
workload_order = []
for key in copy_wo:
    if key in avail_keys:
        workload_order.append(key)

if args.normalize is not None:
    kv.normalize(args.normalize)


# ------------------------------------------------------------------------------
# Change the workload order if some sort condition is mentioned
# ------------------------------------------------------------------------------

if args.sort_workload is not None:
    sort_set = args.sort_workload
    if sort_set not in data:
        print "Set specified for sorting is not available"
        quit()
    else:
        new_workload_order = sorted(workload_order, 
            key = lambda workload: data[sort_set][workload])
        workload_order = new_workload_order

if not args.no_mean:
    kv.append_gmean()
    workload_order.append("gmean")

if args.amean:
    kv.append_amean()
    workload_order.append("amean")

# ------------------------------------------------------------------------------
# Check if we need to print
# ------------------------------------------------------------------------------

just = args.justify_width
if args.only_mean:
    print_order = ["gmean"]
    if args.workload_name is not None:
        output = [args.workload_name.rjust(just)]
    elif args.workload_file is not None:
        output = [os.path.basename(args.workload_file).rjust(just)]
    else:
        output = ["gmean".rjust(just)]
else:
    print_order = workload_order
    output = [workload.rjust(just) for workload in print_order]

for name in set_order:
    if name not in args.ignore:
        output = [cur + "," + str(round(val,args.round)).ljust(args.round+2,'0') for cur, val in \
            zip(output, kv.values(name, key_order = print_order))]

if args.print_table:
    if not args.no_header: print ','.join(['%s' % (set_name) for set_name in set_order])
    for line in output:
        print line


# ------------------------------------------------------------------------------
# Plot data if requested
# ------------------------------------------------------------------------------

if args.plot:
    plot = PlotLib()

    plot.set_plot_dimensions(args.plot_width, args.plot_height)

    if args.y_tics_step is not None:
        plot.set_y_tics_step(args.y_tics_step)
    elif args.normalize is not None:
        plot.set_y_tics_step(0.1)
    
    plot.set_ceiling(args.ceiling)

    legend = [name for name in set_order if name not in args.ignore]
    if args.no_legend:
        legend = []
    plot.set_legend_shift(args.legend_x_shift, args.legend_y_shift)

    if args.plot_line:
        xtics = range(0, (len(workload_order)/args.x_tics_step)*args.x_tics_step + args.x_tics_step - 1,\
            args.x_tics_step)
        lines = [[(i,val) for i,val in enumerate(kv.values(name, key_order = \
            workload_order))] for name in set_order if name not in args.ignore]
        plot.continuous_lines(lines, legend, value_base = args.value_base, \
            xtics = xtics, x_padding = args.x_padding, use_markers = 0)

    else:
        plot.column_stacked_bars(workload_order, [kv.values(name, key_order = \
            workload_order)  for name in set_order if name not in args.ignore],\
            legend, value_base = args.value_base)


    if args.title is not None:
        plot.title(args.title)
    if args.y_label is not None:
        plot.ylabel(args.y_label)
    plot.xlabel(args.x_label)
    if args.output == "pdf":
        plot.save_pdf(args.plot_name)
        plot.save_tikz(args.plot_name + "_tikz.tex")
        plot.save_tex(args.plot_name)
    elif args.output == "tex":
        plot.save_tex(args.plot_name)
    elif args.output == "tikz":
        plot.save_tikz(args.plot_name + ".tex")
