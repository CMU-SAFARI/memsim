#!/usr/bin/env python

# ------------------------------------------------------------------------------
# This script runs the memory simulator with multi programmed workloads. The
# workloads are specified through a workload file. The number of processors are
# inferred from the workload file. So, it can be used to run single programmed
# workloads also.
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Import necessary libraries
# ------------------------------------------------------------------------------

import sys
import os
import argparse
import commands
import random
import re


# ------------------------------------------------------------------------------
# Some global parameters
# ------------------------------------------------------------------------------

MOD_FOLDER = "Simulator/"
DEF_FOLDER = MOD_FOLDER + "Definitions/"
SIM_BINARY= MOD_FOLDER + "bin/OoOTraceSimulator"
SIM_DEBUG_BINARY = MOD_FOLDER + "bin/Debug.OoOTraceSimulator"
WORKLOAD_FOLDER = MOD_FOLDER + "Workloads/"
SHARED_TRACE_FOLDER = "." # shared file system if running simulations on multiple machines
LOCAL_TRACE_FOLDER = "." # folder that contains all the trace files
CONDOR_BASE_FOLDER = "."

# ------------------------------------------------------------------------------
# Create the command line parser
# ------------------------------------------------------------------------------

parser = argparse.ArgumentParser("Multi-programmed simulation script")

parser.add_argument("--definition", action = "store", required = True, \
    help = "simulator definition filename")
parser.add_argument("--configurations", action = "store", required = True, \
    help = "configurations to use for simulation")
parser.add_argument("--folder", action = "store", required = True, \
    help = "folder to store the results")
parser.add_argument("--warm-up", action = "store",  \
    default = "500000000", help = "warm-up duration")
parser.add_argument("--run-time", action = "store",  \
    default = "500000000", help = "number of instructions to use for statistics")
parser.add_argument("--heart-beat", action = "store",  \
    default = "0", help = "duration of periodic heart-beat [0 for none]")
parser.add_argument("--trace-selector", action = "store", \
    help = "folder containing the trace files")
parser.add_argument("--workload-file", action = "store", \
    help = "name of file containing the workload information")
parser.add_argument("--workload", action = "append", \
    help = "a specific workload to run")
parser.add_argument("--ooo-window", action = "store",  default = "1", \
    help = "out-of-order window to use for simulation")
parser.add_argument("--machine", action = "append", \
    help = "list of machines to use for condor submission")
parser.add_argument("--condor", action = "store_true", default = False, \
    help = "use condor to run the simulation rather than local machine")
parser.add_argument("--list", action = "store_true", default = False, \
    help = "print the simulation parameters and exit")
parser.add_argument("--debug", action = "store_true", default = False, \
    help = "run in debug mode")
parser.add_argument("--no-nice", action = "store_true", default = False, \
    help = "run jobs as not nice user")
parser.add_argument("--no-workload-print", action = "store_true", default = False, \
    help = "do not print the workload information")
parser.add_argument("--synthetic", action = "store_true", default = False, \
    help = "use a synthetic trace")

args = parser.parse_args()


# ------------------------------------------------------------------------------
# Get the required definition and configurations
# ------------------------------------------------------------------------------

def_file = DEF_FOLDER + args.definition
if not os.path.isfile(def_file):
    print "Error: Simulator definition file is not present"
    quit()

config_folder = MOD_FOLDER + "Configs/" + args.definition + "/"
if not os.path.exists(config_folder):
    print "Error: No configuration folder for the simulator exists"
    quit()

configs = commands.getoutput("ls " + config_folder + \
        args.configurations).split()

if len(configs) == 0:
    print "Error: No configurations chosen"
    quit()


# ------------------------------------------------------------------------------
# Check if the workload file is present
# ------------------------------------------------------------------------------

if args.workload_file is not None:
    workload_file = WORKLOAD_FOLDER + args.workload_file
    if not os.path.isfile(workload_file):
        print "Error: Specified workload file does not exist"
        quit()

#make some changes here
# ------------------------------------------------------------------------------
# Check if the trace selector is present or can be infered
# ------------------------------------------------------------------------------

if args.trace_selector is None and not args.synthetic:
    if os.path.exists(LOCAL_TRACE_FOLDER):
        args.trace_selector = LOCAL_TRACE_FOLDER
    else:
        args.trace_selector = SHARED_TRACE_FOLDER

# ------------------------------------------------------------------------------
# Print the simulation details
# ------------------------------------------------------------------------------

print
print "Simulator definition : ", def_file
print "       Result folder : ", args.folder
print "    Warm-up duration : ", args.warm_up
print "            Run-time : ", args.run_time
print "          Heart-beat : ", args.heart_beat
print "      Trace Selector : ", args.trace_selector
print "          OoO Window : ", args.ooo_window
print "           Nice user : ", not args.no_nice
print
print "List of configuration files: "
for config in configs:
    print config
print
print
if not args.no_workload_print:
    print "Workloads: "
    if args.workload_file is not None:
        os.system("cat " + workload_file)
    if args.workload is not None:
        for wl in args.workload:
            print wl
    print
    print

if args.list:
    quit()

# ------------------------------------------------------------------------------
# Copy details
# ------------------------------------------------------------------------------

def_name = args.definition
results_folder = args.folder + "/"
if not args.synthetic:
    trace_selector = args.trace_selector + "/"
run_time = args.run_time
warm_up = args.warm_up
heart_beat = args.heart_beat
ooo_window = args.ooo_window

# ------------------------------------------------------------------------------
# For each workload, check if all the benchmarks are available. If yes, then
# run the workload for all configurations
# ------------------------------------------------------------------------------

if args.workload_file is not None:
    workloads = open(workload_file, "r")
else:
    workloads = args.workload


for workload in workloads:

    flag = False
    
    # --------------------------------------------------------------------------
    # Get the list of benchmarks and check if the trace files are present
    # --------------------------------------------------------------------------

    benchmarks = workload.strip().split("-")
    if not args.synthetic:
        for benchmark in benchmarks:
            if not os.path.exists(trace_selector + benchmark + ".gz") and not args.condor:
                print "Error: No trace file for benchmark `" + benchmark + "'"
                flag = True
                break

            if flag:
                print "Discarding workload `" + workload + "'"
                print
                continue

            trace_file_string = ",".join([trace_selector + benchmark + ".gz" \
                                              for benchmark in benchmarks])
    

    # --------------------------------------------------------------------------
    # For each configuration
    # --------------------------------------------------------------------------

    for config in configs:
        
        # ----------------------------------------------------------------------
        # Create the run folder
        # ----------------------------------------------------------------------

        run_folder = results_folder + config[len(MOD_FOLDER + "Configs/"):] + "/" + "-".join(benchmarks)
        if not os.path.exists(run_folder):
            os.makedirs(run_folder)


        # ----------------------------------------------------------------------
        # Invoke the script to run the program
        # ----------------------------------------------------------------------

        if args.debug:
            executable = SIM_DEBUG_BINARY
        else:
            executable = SIM_BINARY

        arguments = " --definition " + def_file + " --configuration " + \
            config + " --folder " + run_folder + " --num-cpus " + \
            str(len(benchmarks)) + \
            " --warm-up " + warm_up + " --run-time " + run_time + \
            " --heart-beat " + heart_beat + " --ooo-window " + ooo_window

        if not args.synthetic:
            arguments += " --trace-files " + trace_file_string
        else:
            arguments += " --synthetic " + benchmarks[0]

        if args.debug:
            print "Running: " + executable + arguments

        # ----------------------------------------------------------------------
        # If we don't have to use condor, just run it 
        # ----------------------------------------------------------------------

        if not args.condor:
            print "BEGIN: Workload " + ",".join(benchmarks) + " " + config
            os.system(executable + arguments)
            print "  END: Workload " + ",".join(benchmarks) + " " + config

        # ----------------------------------------------------------------------
        # If we have to use condor, use it
        # ----------------------------------------------------------------------

        else:

            condor_folder = CONDOR_BASE_FOLDER + ".condor4/" + \
                commands.getoutput("date +%m.%d-%a-%H.%M.%S").strip() + "-" + \
                "-".join(benchmarks) + "-" + str(random.randint(1,10000))

            while os.path.exists(condor_folder):
                condor_folder = condor_folder + "0"
            os.makedirs(condor_folder)


            compute_machines = [
            ]

            if args.machine is None:
                condor_machines = compute_machines
            else:
                condor_machines = args.machine

            condor_script = open(condor_folder + "/script", "w")
            condor_script.write("universe = vanilla\n")
            condor_script.write("output = " + condor_folder + "/output\n")
            condor_script.write("error = " + condor_folder + "/error\n")
            condor_script.write("log = " + condor_folder + "/log\n")
            if not args.no_nice:
                condor_script.write("niceuser = true\n")
            condor_script.write('requirements = ' + \
                " || ".join(['(Machine == "' + machine + '")' \
                for machine in condor_machines]) + "\n")
            condor_script.write("executable = " + executable + "\n")
            condor_script.write("arguments = " + arguments + "\n")
            condor_script.write("queue\n")
            condor_script.close()

            os.system("condor_submit " + condor_folder + "/script > /dev/null")
            print "Condor job submitted for " + ",".join(benchmarks) \
                + ", " + config


if args.workload_file is not None:
    workloads.close()
