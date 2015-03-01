// -----------------------------------------------------------------------------
// File: MemorySimulator.h
// Description:
//    Defines the memory simulator class. The interface is independent of the
//    front-end used to drive the simulator.
// -----------------------------------------------------------------------------

#ifndef __MEMORY_SIMULATOR_H__
#define __MEMORY_SIMULATOR_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"


// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <list>
#include <cstdio>
#include <cassert>
#include <sstream>
#include <cstring>

using namespace std;

// -----------------------------------------------------------------------------
// Class: MemorySimulator
// Description:
//    Defines a memory simulator
// -----------------------------------------------------------------------------

class MemorySimulator {

  protected:

    // list of memory components
    list <MemoryComponent *> _components;
    // number of cpus
    uint32 _numCPUs;
    // hierarchy of components for each processor
    vector <vector <MemoryComponent *> > _hier;

    // simulation folder name
    string _simulationFolderName;
    // simulation log file
    FILE *_simulationLog;

    // current time of the simulator
    cycles_t _currentCycle;


  public:

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    MemorySimulator() {
      // initialize the members
      _components.clear();
      _hier.clear();
      _numCPUs = 0;
      _currentCycle = 0;
    }


    // -------------------------------------------------------------------------
    // Function to initalize the simulator
    // -------------------------------------------------------------------------

    void InitializeSimulator(uint32 numCPUs, string simulationFolderName,
        string simulatorDefinition, string parameterValues) {
      // initialize members
      _numCPUs = numCPUs;
      _hier.resize(numCPUs);
      _simulationFolderName = simulationFolderName;
      // open the simulator log file
      string logFileName = simulationFolderName + "/SimulationLog";
      _simulationLog = fopen(logFileName.c_str(), "w");
      // Parse simulator configuration
      ParseSimulatorConfiguration(simulatorDefinition, parameterValues);
    }


    // -------------------------------------------------------------------------
    // Function to set the start cycle of the simulator
    // -------------------------------------------------------------------------

    void SetStartCycle(cycles_t now) {
      _currentCycle = now;
      // set the start cycle for all the components
      list <MemoryComponent *>::iterator cmp;
      for (cmp = _components.begin(); cmp != _components.end(); cmp ++)
        (*cmp) -> SetStartCycle(now);
    }


    // -------------------------------------------------------------------------
    // Function to start the simulation for all the components
    // -------------------------------------------------------------------------

    void StartSimulation() {
      // for each component, send the log folder, log file and pointer to the
      // hierarchy and current cycle
      list <MemoryComponent *>::iterator cmp;
      for (cmp = _components.begin(); cmp != _components.end(); cmp ++) {
        (*cmp) -> SetBackPointers(&_hier, &_currentCycle);
        (*cmp) -> SetLogDetails(_simulationFolderName, _simulationLog);
        (*cmp) -> InitializeStatistics();
        (*cmp) -> StartSimulation();
      }
    }


    // -------------------------------------------------------------------------
    // Advance simulation. Argument indicates current time
    // -------------------------------------------------------------------------

    void AdvanceSimulation(cycles_t now) {
      // update current time of the simulator
      if (now > _currentCycle)
        _currentCycle = now;
      else if (now <= _currentCycle) {
        // printf("Something wrong %llu %llu\n", now, _currentCycle);
        // list <MemoryComponent *>::iterator cmp;
        // for (cmp = _components.begin(); cmp != _components.end(); cmp ++) {
        //   (*cmp) -> PrintDebugInfo();
        // }
      }

      // Process pending requests of all the components
      list <MemoryComponent *>::iterator cmp;
      for (cmp = _components.begin(); cmp != _components.end(); cmp ++) {
        (*cmp) -> ProcessPendingRequests();
      }
    }


    // -------------------------------------------------------------------------
    // Auto advance simulation. 
    // -------------------------------------------------------------------------

    void AutoAdvance() {
      
      // For each component, find the earliest request that can be processed.
      // Take the min of all and advance simulation to that point.
      
      cycles_t min;
      bool flag = false;

      MemoryRequest *request;
      list <MemoryComponent *>::iterator cmp;
      for (cmp = _components.begin(); cmp != _components.end(); cmp ++) {
        request = (*cmp) -> EarliestRequest();
        if (request != NULL) {
          if (!flag) {
            flag = true;
            min = request -> currentCycle;
          }
          else {
            if (min > request -> currentCycle) {
              min = request -> currentCycle;
            }
          }
        }
      }

      if (!flag) {
        fprintf(stderr, "Request is waiting for nothing?\n");
        list <MemoryComponent *>::iterator cmp;
        for (cmp = _components.begin(); cmp != _components.end(); cmp ++) {
          (*cmp) -> PrintDebugInfo();
        }
        exit(0);
      }

      AdvanceSimulation(min);
    }

    // -------------------------------------------------------------------------
    // Function to end the simulation
    // -------------------------------------------------------------------------

    void EndSimulation() {
      // end the simulation for all the components
      list <MemoryComponent *>::iterator cmp;
      for (cmp = _components.begin(); cmp != _components.end(); cmp ++)
        (*cmp) -> EndSimulation();
      // close the simulation log
      fclose(_simulationLog);
    }


    // -------------------------------------------------------------------------
    // Function to end the warm up
    // -------------------------------------------------------------------------

    void EndWarmUp() {
      list <MemoryComponent *>::iterator cmp;
      for (cmp = _components.begin(); cmp != _components.end(); cmp ++)
        (*cmp) -> EndWarmUp();
    }

  void EndProcWarmUp(uint32 cpuID) {
    list <MemoryComponent *>::iterator cmp;
    for (cmp = _components.begin(); cmp != _components.end(); cmp ++)
      (*cmp) -> EndProcWarmUp(cpuID);
  }

  void EndProcSimulation(uint32 cpuID) {
    list <MemoryComponent *>::iterator cmp;
    for (cmp = _components.begin(); cmp != _components.end(); cmp ++)
      (*cmp) -> EndProcSimulation(cpuID);
  }


    // -------------------------------------------------------------------------
    // Current cycle
    // -------------------------------------------------------------------------

    cycles_t CurrentCycle() {
      return _currentCycle;
    }


    // -------------------------------------------------------------------------
    // Function to handle heart beat
    // -------------------------------------------------------------------------

    void HeartBeat(cycles_t hbCount) {
      list <MemoryComponent *>::iterator cmp;
      for (cmp = _components.begin(); cmp != _components.end(); cmp ++)
        (*cmp) -> HeartBeat(hbCount);
    }


    // -------------------------------------------------------------------------
    // Function to process a memory request. It initalizes some of the dynamic
    // parameters of the request and sends it to the first component of the cpu
    // hierarchy.
    // -------------------------------------------------------------------------

    void ProcessMemoryRequest(MemoryRequest *request) {

      // assert that the request is not issued
      assert(request -> issued == false);
      assert(request -> iniType == MemoryRequest::CPU);
      assert((uint32)request -> cpuID < _numCPUs);

      request -> issued = true;

      // check if there is some hierarchy
      if (_hier.size() == 0) {
        request -> finished = true;
        return;
      }

      // if the hierarchy for the cpu has no components
      if (_hier[request -> cpuID].size() == 0) {
        request -> finished = true;
        return;
      }

      // set the requests component ID and add it to the 
      // corresponding component's request
      request -> cmpID = 0;
      (_hier[request -> cpuID])[0] -> AddRequest(request);

      // advance simulation if needed
      if (request -> currentCycle > _currentCycle)
        AdvanceSimulation(request -> currentCycle);
    }


    // -------------------------------------------------------------------------
    // Function to parse the simulator configuration
    // -------------------------------------------------------------------------

    void ParseSimulatorConfiguration(string definition,
        string configuration) {


      // map of components
      map <string, MemoryComponent *> cmps;
      map <string, string> cmptype;

      FILE *file;

      // open the definition file
      file = fopen(definition.c_str(), "r");
      assert(file != NULL);

      char line[300];

      // parse the definition file
      while (fgets(line, 300, file)) {

        if (strlen(line) <= 1)
          continue;

        istringstream istr(line);
        char linetype[100], type[100], name[100];
        uint32 proc;
        istr >> linetype;

        if (strcmp(linetype, "component") == 0) {
          istr >> type >> name;
          assert(cmps.find(name) == cmps.end());
          cmptype[name] = type;
          cmps[name] = CreateComponent(type);
          cmps[name] -> SetName(name);
          _components.push_back(cmps[name]);
        }

        else if (strcmp(linetype, "all") == 0) {
          // add the components to all the queues
          while (istr >> name) {
            assert(cmps.find(name) != cmps.end());
            for (uint32 i = 0; i < _numCPUs; i ++)
              _hier[i].push_back(cmps[name]);
          }
        }

        else if (sscanf(linetype, "%u", &proc) == 1) {
          assert(proc < _numCPUs);
          while (istr >> name) {
            if (cmps.find(name) == cmps.end()) {
              fprintf(stderr, "Unknown component %s\n", name);
            }
            assert(cmps.find(name) != cmps.end());
            _hier[proc].push_back(cmps[name]);
          }
        }
      }

      // open the configuration file
      file = fopen(configuration.c_str(), "r");
      assert(file != NULL);

      // parse the configuration file
      while (fgets(line, 300, file)) {
        istringstream istr(line);
        char name[100], field[100], value[100], fname[100];

        istr >> name;

        if (strcmp(name, "override") == 0) {
          istr >> name >> field >> value;
          assert(cmps.find(name) != cmps.end());
          cmps[name] -> AddParameter(field, value);
        }

        else if (strlen(name) != 0) {
          assert(cmps.find(name) != cmps.end());
          istr >> fname;

          // open the component config file
          string cmpfilename = COMPONENTS_FOLDER + cmptype[name] + "/" + 
            (string)fname;
          FILE *cmpfile = fopen(cmpfilename.c_str(), "r");
          if (cmpfile == NULL) {
            fprintf(stderr, "Error: Component file `%s' not found\n",
                cmpfilename.c_str());
            exit(0);
          }

          char fieldline[300];
          while (fgets(fieldline, 300, cmpfile)) {
            sscanf(fieldline, "%s %s", field, value);
            if (strlen(field) > 0 && strlen(value) > 0) {
              cmps[name] -> AddParameter(field, value);
            }
          }
        }
      }
    }

};

#endif // __MEMORY_SIMULATOR_H__
