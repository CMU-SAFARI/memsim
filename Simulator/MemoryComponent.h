// -----------------------------------------------------------------------------
// File: MemoryComponent.h
// Description:
//    Defines an abstract memory component. All other components have to
//    inherit this class.
// -----------------------------------------------------------------------------

#ifndef __MEMORY_COMPONENT_H__
#define __MEMORY_COMPONENT_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryRequest.h"
#include "Types.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <algorithm>
#include <vector>
#include <list>
#include <cstdio>
#include <string>
#include <bitset>
#include <queue>
#include <map>

using namespace std;

#define COMPONENTS_FOLDER "Simulator/Components/"

// -----------------------------------------------------------------------------
// Functions and macros to add a new component to the component list and create
// a new component
// -----------------------------------------------------------------------------

#define COMPONENT_LIST_BEGIN \
  MemoryComponent *cmp;\
  if (false) { }

#define COMPONENT_LIST_END \
  else {\
    fprintf(stderr, "Error: Component type `%s' not found\n", type.c_str());\
    exit(-1);\
  }\
  return cmp;

#define COMPONENT(name,classname) \
  else if (type.compare(name) == 0) {\
    cmp = new classname;\
  }



// -----------------------------------------------------------------------------
// Macros to add parameters to the component
// -----------------------------------------------------------------------------

#define CMP_PARAMETER_BEGIN \
  if (false) {}

#define CMP_PARAMETER_END \
  else {\
    fprintf(stderr, "Error: Unknown parameter `%s' for component `%s'\n",\
        pname.c_str(), _name.c_str());\
    exit(-1);\
  }

#define CMP_PARAMETER_INT(name,var) \
  else if (pname.compare(name) == 0) {\
    int32 ret;\
    ret = sscanf(pvalue.c_str(), "%d", &var);\
    assert(ret == 1);\
  }

#define CMP_PARAMETER_UINT(name,var) \
  else if (pname.compare(name) == 0) {\
    int32 ret;\
    ret = sscanf(pvalue.c_str(), "%u", &var);\
    assert(ret == 1);\
  }

#define CMP_PARAMETER_DOUBLE(name,var) \
  else if (pname.compare(name) == 0) {\
    int32 ret;\
    ret = sscanf(pvalue.c_str(), "%lf", &var);\
    assert(ret == 1);\
  }

#define CMP_PARAMETER_STRING(name,var) \
  else if (pname.compare(name) == 0) {\
    var = pvalue;\
  }

#define CMP_PARAMETER_BOOLEAN(name,var) \
  else if (pname.compare(name) == 0) {\
    uint32 temp;\
    int32 ret;\
    ret = sscanf(pvalue.c_str(), "%u", &temp);\
    assert(ret == 1);\
    var = (temp != 0);\
  }


// -----------------------------------------------------------------------------
// Macros to add and manipulate statistics
// -----------------------------------------------------------------------------

#define NEW_COUNTER(var) uint64 c_##var

#define INITIALIZE_COUNTER(var, lname) {\
  _statsOrder.push_back(#var);\
  _stats[#var].ptr = &c_##var;\
  _stats[#var].longname = (string)lname;\
}

#define INCREMENT(var) {\
  c_##var ++;\
}

#define DECREMENT(var) {\
  c_##var --;\
}

#define ADD_TO_COUNTER(var,value) {\
  c_##var += (value);\
}

#define RESET_ALL_COUNTERS {\
  map <string, Stats>::iterator it;\
  for (it = _stats.begin(); it != _stats.end(); it ++) {\
    *((*it).second.ptr) = 0;\
  }\
}


// -----------------------------------------------------------------------------
// Macro to write into the simulation log
// -----------------------------------------------------------------------------

#define CMP_LOG(...) {\
  fprintf(_simulationLog, "%s:", _name.c_str());\
  fprintf(_simulationLog, __VA_ARGS__);\
  fprintf(_simulationLog, "\n");\
}


// -----------------------------------------------------------------------------
// Macro to dump all statistics
// -----------------------------------------------------------------------------

#define DUMP_STATISTICS {\
  list <string>::iterator it;\
  for (it = _statsOrder.begin(); it != _statsOrder.end(); it ++) {\
    CMP_LOG("%s = %llu", (*it).c_str(),\
        *(_stats[(*it)].ptr));\
  }\
}


// -----------------------------------------------------------------------------
// Macro to create and use log files
// -----------------------------------------------------------------------------

#define NEW_LOG_FILE(name,fname) {\
  assert(_logs.find(name) == _logs.end());\
  string _temp_fname = _simulationFolderName + "/" + _name + "." + fname;\
  _logs[name] = fopen(_temp_fname.c_str(), "w");\
  assert(_logs[name] != NULL);\
}

#define LOG(name,...) {\
  assert(_logs.find(name) != _logs.end());\
  fprintf(_logs[name], __VA_ARGS__);\
}

#define LOG_W(name,...) {\
  assert(_logs.find(name) != _logs.end());\
  if (!_warmUp) {\
    fprintf(_logs[name], __VA_ARGS__);\
  }\
}

#define CLOSE_ALL_LOGS {\
  map <string, FILE *>::iterator it;\
  for (it = _logs.begin(); it != _logs.end(); it ++) {\
    fclose((*it).second);\
  }\
}


// -----------------------------------------------------------------------------
// Priority queue definition
// -----------------------------------------------------------------------------

typedef priority_queue <MemoryRequest *, vector <MemoryRequest *>,
          MemoryRequest::ComparePointers> RequestPriorityQueue;

// priority_queue is a pre-existing type, present in the std namespace

// -----------------------------------------------------------------------------
// Class: MemoryComponent
// Description:
//    Defines an abstract memory component.
// -----------------------------------------------------------------------------

class MemoryComponent {

  protected:

    // -------------------------------------------------------------------------
    // Static fields
    // -------------------------------------------------------------------------

    // name of the component. used for log files and such
    string _name;

    // -------------------------------------------------------------------------
    // Dynamic fields
    // -------------------------------------------------------------------------


    // current cycle of the component
    cycles_t _currentCycle;
    // processing? true, if processing function is active
    bool _processing;
    // warm up flag, true if in warm up phase
    bool _warmUp;
    // pointer to the simulator cycle
    cycles_t *_simulatorCycle;
    // pointer to the memory hierarchy
    vector <vector <MemoryComponent *> > *_hier;
    // number of cpus
    uint32 _numCPUs;

  bitset <128> _done;

    // simulation folder
    string _simulationFolderName;
    // simulation log
    FILE *_simulationLog;

    // priority queue of requests
    RequestPriorityQueue _queue;

    // statistics
    struct Stats {
      string longname;
      uint64 *ptr;
    };
    map <string, Stats> _stats;
    list <string> _statsOrder;

    // log files
    map <string, FILE *> _logs;


  public:

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    MemoryComponent() {
      // initialize variables
      _name = (string)("No-name");
      _currentCycle = 0;
      _processing = false;
      _warmUp = true;
      _stats.clear();
      _statsOrder.clear();
      _logs.clear();
      _done.reset();
    }


    // -------------------------------------------------------------------------
    // Function to set the name of the component
    // -------------------------------------------------------------------------

    void SetName(string name) {
      _name = name;
    }


    // return size

    unsigned Size(){
	return _queue.size();
   }

    // update with new value of top
    void UpdateQueue(){
	MemoryRequest* request = _queue.top();
	_queue.pop();
	(request -> currentCycle) += 10;
	_queue.push(request);
    }


    // -------------------------------------------------------------------------
    // Function to return the name of the component
    // -------------------------------------------------------------------------

    string Name() {
      return _name;
    }


    // -------------------------------------------------------------------------
    // Function to set the start cycle of the component
    // -------------------------------------------------------------------------

    void SetStartCycle(cycles_t now) {
      _currentCycle = now;
    }


    // -------------------------------------------------------------------------
    // Function to set back pointers for the simulator time and hierarchy
    // -------------------------------------------------------------------------

// This function sets the simulator cycle and hierarchy pointers to the global simcycle and hierarchy

    void SetBackPointers(vector <vector <MemoryComponent *> > *hier,
        cycles_t *simCycle) {
      // set the pointers for the simulator cycle and hierarchy
      _hier = hier;
      _simulatorCycle = simCycle;
      _numCPUs = (*hier).size();
      _done.reset();
    }


    // -------------------------------------------------------------------------
    // Function to set the log details of the request
    // -------------------------------------------------------------------------

    void SetLogDetails(string simulationFolderName, FILE *simulationLog) {
      // initialize members
      _simulationFolderName = simulationFolderName;
      _simulationLog = simulationLog;
    }


    // -------------------------------------------------------------------------
    // Function to add a request to the queue
    // -------------------------------------------------------------------------

    void AddRequest(MemoryRequest *request) {
      _queue.push(request);
      if (!_processing)
        ProcessPendingRequests();

    }


    // ------------------------------------------------------------------------
    // Function to add request without doing progress
    // ------------------------------------------------------------------------
    void SimpleAddRequest(MemoryRequest *request) {
      _queue.push(request);

    }


    // -------------------------------------------------------------------------
    // Function to get the head of queue
    // This function has been modified to return the earliest request that is
    // not stalling for DRAMSim
    // -------------------------------------------------------------------------

    virtual MemoryRequest * EarliestRequest() {
      if (_queue.empty())
        return NULL;

      return _queue.top();
    }


    // -------------------------------------------------------------------------
    // Virtual functions to be implemented by the components
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    // Function to initialize statistics
    // -------------------------------------------------------------------------

    virtual void InitializeStatistics() {}


    // -------------------------------------------------------------------------
    // Function to add a parameter to the component
    // -------------------------------------------------------------------------

    virtual void AddParameter(string pname, string pvalue) {}


    // -------------------------------------------------------------------------
    // Function called when simulation starts
    // -------------------------------------------------------------------------

    virtual void StartSimulation() {}


    // -------------------------------------------------------------------------
    // Function called when simulation ends
    // -------------------------------------------------------------------------

    virtual void EndSimulation() {
      DUMP_STATISTICS;
      CLOSE_ALL_LOGS;
    }


    // -------------------------------------------------------------------------
    // Function called when warmup ends
    // -------------------------------------------------------------------------

    virtual void EndWarmUp() {
      _warmUp = false;
      RESET_ALL_COUNTERS;
    }

    virtual void EndProcWarmUp(uint32 cpuID) {
    }

    virtual void EndProcSimulation(uint32 cpuID) {
      _done.set(cpuID);
    }
    
    virtual void PrintDebugInfo() {
      printf("%s\n", _name.c_str());
      printf("Queue size is %u\n", _queue.size());
    }


    // -------------------------------------------------------------------------
    // Function called at a heart beat. Argument indicates cycles elapsed after
    // previous heartbeat
    // -------------------------------------------------------------------------

    virtual void HeartBeat(cycles_t hbCount) {}


    // -------------------------------------------------------------------------
    // Function to process pending requests. Different components can choose to
    // override this function. The default implementation processes one request
    // at a time.
    // -------------------------------------------------------------------------

    virtual void ProcessPendingRequests() {

      // if already processing, return
      if (_processing) return;
      _processing = true;

      // if the queue is empty, return
      if (_queue.empty()) {
        _processing = false;
        return;
      }

      // get the request on the top of the queue
      MemoryRequest *request = _queue.top();

/*
Process till the currentcycle of request reaches global simulator cycles
and do this for all the request that are in the queue.
Access the request according to age.
*/
      // process till component (or request ??) reaches simulator cycles

	// What if the currentCycle of request is more than _simulatorCycle ?, as is the case when a request is returned by a cb
      while (request -> currentCycle <= (*_simulatorCycle)) {

        _queue.pop();

        // if component is past the simulator (When can this scenario arise?)
	// this if block ensures that request is in sync with whichever component it arrives at
        if (_currentCycle > (*_simulatorCycle)) {
          request -> currentCycle = _currentCycle;
          _queue.push(request);
        }

        // else process the request
        else {
          // get actual current time
          cycles_t now = max(request -> currentCycle, _currentCycle);
          _currentCycle = now;
          // if request is serviced, process return of request
          if (request -> serviced) {
            // CHECK
            cycles_t busyCycles = ProcessReturn(request);
            _currentCycle += busyCycles;
            SendToNextComponent(request);
          }
	// request is yet to be serviced
          else {
            request -> currentCycle = now;
            cycles_t busyCycles = ProcessRequest(request);
            _currentCycle += busyCycles;
            SendToNextComponent(request);
          }

          // if queue is empty, return
          if (_queue.empty()) {
            _processing = false;
            return;
          }

          // get the request on top of the queue
          request = _queue.top();
        }
      }

      _processing = false;
    }

  protected:

    // -------------------------------------------------------------------------
    // Function to process a request. Return value indicates number of busy
    // cycles for the component.
    // -------------------------------------------------------------------------

    virtual cycles_t ProcessRequest(MemoryRequest *request) { return 0; }


    // -------------------------------------------------------------------------
    // Function to process the return of a request. Return value indicates
    // number of busy cycles for the component.
    // -------------------------------------------------------------------------

    virtual cycles_t ProcessReturn(MemoryRequest *request) { return 0; }


    // -------------------------------------------------------------------------
    // Function to send the request to the next component. It uses the serviced
    // flag in the request to determine the direction of the request.
    // -------------------------------------------------------------------------

    void SendToNextComponent(MemoryRequest *request) {

      // if the request should be destroyed, delete it
      if (request -> destroy) {
        delete request;
        return;
      }

      if(request -> type == MemoryRequest::CLEAN){
         this -> AddRequest(request);
         return;
      }

      // if the request is stalling, set its current cycle to
      // one past the component's currenct cycle (ensure progress)
      // add it back to the component's queue. (This doesn't seem to have been done)
      if (request -> stalling) {
        return;
      }

      // else if request is serviced, send it to previous component
      if (request -> serviced) {
        if (request -> cmpID == 0) {
          request -> finished = true;
          return;
        }
        request -> cmpID --;
      }

      // else if request is not serviced send it to next component
      else {
	// check if request has reached end of component hierarchy
        if ((uint32)(request -> cmpID + 1) == ((*_hier)[request -> cpuID]).size())
          request -> serviced = true;
        else
          request -> cmpID ++;
      }
      ((*_hier)[request -> cpuID])[request -> cmpID] -> AddRequest(request);
    }
};

// Function to create a new memory component
MemoryComponent *CreateComponent(string type);

#endif // __MEMORY_COMPONENT_H__
