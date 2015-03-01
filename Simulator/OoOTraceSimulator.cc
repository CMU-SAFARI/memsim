// -----------------------------------------------------------------------------
// File: OoOTraceSimulator.cc
// Description:
//    This file defines the trace based simulator. Traces can be generated
//    through the trace module for simics.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "OoOTraceSimulator.h"


// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <string>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <getopt.h>

using namespace std;

// -----------------------------------------------------------------------------
// Function: main
// -----------------------------------------------------------------------------

int main(int argc, char **argv) {

  // ---------------------------------------------------------------------------
  // Get the command line arguments
  // ---------------------------------------------------------------------------
  
  string simulatorDefinition("");
  string simulatorConfiguration("");
  string folder("");
  uint32 numCPUs = 0;
  uint32 oooWindow = 1;
  uint64 warmUp = 0;
  uint64 runTime = 0;
  uint64 heartBeat = 0;
  vector <string> traceFiles;
  bool synthetic = false;
  uint32 workingSetSize = 0;
  uint32 memGap = 50;
  

  struct option cmd_options[] = {
    {"definition", required_argument, 0, 'a'},
    {"configuration", required_argument, 0, 'b'},
    {"folder", required_argument, 0, 'c'},
    {"num-cpus", required_argument, 0, 'd'},
    {"trace-files", required_argument, 0, 'e'},
    {"warm-up", required_argument, 0, 'f'},
    {"run-time", required_argument, 0, 'g'},
    {"heart-beat", required_argument, 0, 'h'},
    {"ooo-window", required_argument, 0, 'i'},
    {"synthetic", required_argument, 0, 'k'},
    {"mem-gap", required_argument, 0, 'm'},
  };

  int c = 0;
  int optindex = 0;

  string trString;
  string::size_type index;
  string::size_type next;

  c = getopt_long(argc, argv, "a:b:c:d:e:f:g:h:i:", cmd_options, &optindex);

  while (c != -1) {

    switch(c) {

      // -----------------------------------------------------------------------
      // simulator file
      // -----------------------------------------------------------------------
      case 'a':
        simulatorDefinition = optarg;
        break;

      // -----------------------------------------------------------------------
      // parameter file
      // -----------------------------------------------------------------------
      case 'b':
        simulatorConfiguration = optarg;
        break;

      // -----------------------------------------------------------------------
      // simulation folder
      // -----------------------------------------------------------------------
      case 'c':
        folder = optarg;
        break;

      // -----------------------------------------------------------------------
      // number of cpus
      // -----------------------------------------------------------------------
      case 'd':
        numCPUs = atoi(optarg);
        break;

      // -----------------------------------------------------------------------
      // trace files
      // -----------------------------------------------------------------------
      case 'e':
        trString = optarg;
        index = 0;
        next = trString.find_first_of(",", index);
        while (next != string::npos) {
          traceFiles.push_back(trString.substr(index, next-index));
          index = next + 1;
          next = trString.find_first_of(",", index);
        }
        traceFiles.push_back(trString.substr(index));
        break;

      // -----------------------------------------------------------------------
      // warm up
      // -----------------------------------------------------------------------
      case 'f':
        warmUp = atoll(optarg);
        break;

      // -----------------------------------------------------------------------
      // run time
      // -----------------------------------------------------------------------
      case 'g':
        runTime = atoll(optarg);
        break;

      // -----------------------------------------------------------------------
      // heart beat
      // -----------------------------------------------------------------------
      case 'h':
        heartBeat = atoll(optarg);
        break;

      // -----------------------------------------------------------------------
      // ooo window
      // -----------------------------------------------------------------------
      case 'i':
        oooWindow = atoi(optarg);
        break;

    case 'k':
      synthetic = true;
      workingSetSize = atoi(optarg);
      break;

    case 'm':
      memGap = atoi(optarg);
      break;

      // -----------------------------------------------------------------------
      // wrong option
      // -----------------------------------------------------------------------
      default:
        cerr << "Invalid command line option" << endl;
        return 1;
        
    }

    c = getopt_long(argc, argv, "a:b:c:d:e:", cmd_options, &optindex);
  }

  OoOTraceSimulator traceSim(numCPUs, simulatorDefinition, 
                             simulatorConfiguration, oooWindow, traceFiles,
                             folder, synthetic, workingSetSize, memGap);

  traceSim.StartSimulation();
  traceSim.RunSimulation(warmUp, runTime, heartBeat);
  return 0;
}
