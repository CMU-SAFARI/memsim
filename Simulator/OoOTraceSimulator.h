// -----------------------------------------------------------------------------
// File: OoOTraceSimulator.h
// Description:
//    Defines an trace simulator with a crude out-of-order model. It assumes all
//    instructions are independent. All non-memory instructions take 1 cycle to
//    complete and the latency of the memory instruction is determined by the
//    memory simulator.
// -----------------------------------------------------------------------------


#ifndef __OoO_TRACE_SIMULATOR_H__
#define __OoO_TRACE_SIMULATOR_H__


// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemorySimulator.h"
#include "TraceReader.h"
#include "Types.h"
#include "SyntheticTrace.h"


// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <algorithm>
#include <set>
#include <vector>
#include <string>
#include <bitset>
#include <queue>
#include <list>
#include <iostream>

#define WARM_UP 0
#define HEART_BEAT 1
#define END_SIMULATION 2


using namespace std;


// -----------------------------------------------------------------------------
// Class: OoOTraceSimulator
// Description:
//    Defines a trace simulator with crude out-of-order model. 
// Parameters:
//    - simulator definition (to pass to the memory simulator)
//    - simulator configuration (to pass to the memory simulator)
//    - number of cpus
//    - trace files
//    - out-of-order window
// -----------------------------------------------------------------------------

class OoOTraceSimulator {

  protected:

    // -------------------------------------------------------------------------
    // Parameters
    // -------------------------------------------------------------------------

    string _simulatorDefinition;
    string _simulatorConfiguration;
    string _simulationFolder;
    uint32 _numCPUs;
    vector <string> _traceFiles;
    uint32 _oooWindow;
  bool _synthetic;
  uint32 _workingSetSize;
  uint32 _memGap;

    // -------------------------------------------------------------------------
    // Private members
    // -------------------------------------------------------------------------

    // information for each processor
    struct ProcInfo {
      TraceReader *reader;
      SyntheticTrace *sreader;
      // current icount and cycle
      uint64 currentIcount;
      cycles_t currentCycle;
      // checkpoint at warmup
      uint64 checkpointIcount;
      cycles_t checkpointCycle;
      // checkpoint at finish
      uint64 finishIcount;
      cycles_t finishCycle;
      // list of outstanding requests
      list <MemoryRequest *> outstanding;
    };

    MemorySimulator _simulator;
    vector <ProcInfo> _procs;

    cycles_t _nextHeartBeatCycle;

    cycles_t _hbCount;
    vector <pair <cycles_t, uint32> > _milestones;
    vector <uint32> _mIndex;

    // queue of currently outstanding request
    priority_queue <MemoryRequest *, vector <MemoryRequest *>,
        MemoryRequest::ComparePointers> _queue;

    // ref count for garbage collection
    set <MemoryRequest *> _ref;

    // IPC file
    FILE *_ipcFile;

    // progress file
    FILE *_progress;

#define PROGRESS_LEAP 10000000


    // -------------------------------------------------------------------------
    // Simulate Function
    // -------------------------------------------------------------------------

    void Simulate() {

      bitset <128> finished;
      bitset <128> warmUp;
      finished.reset();
      warmUp.reset();

      MemoryRequest *request;
      static uint64 checkpoint[64];

      // until all processors have finished
      while (finished.count() < _numCPUs) {
	//if((_procs[0].currentIcount) % 1000 == 0)	cout << "Current cycle is " << _procs[0].currentCycle << endl;

        if (_queue.empty()) {
          printf("What the hell!");
          return;
        }
        assert(!_queue.empty());

        // check if a heart beat should be issued
        if (_hbCount > 0) {
          if (_simulator.CurrentCycle() > _nextHeartBeatCycle) {
            _simulator.HeartBeat(_hbCount);
            _nextHeartBeatCycle += _hbCount;
          }
        }

        // pop a request
        request = _queue.top();
        _queue.pop();

        // if the request is not stalling, then
        // advance simulation to request's current cycle
        if (!request -> stalling) {
          _simulator.AdvanceSimulation(request -> currentCycle);
	//cout << "Advance simulation to " << request -> currentCycle << endl;
        }
        else {
          _simulator.AutoAdvance();
	//cout << "Autoadvance called " << endl;
        }

        // if the request has not completed, push it back to the queue
        if (!(request -> finished)) {
          _queue.push(request);
        }

        // else check if the oldest instruction has finished
        else {
          uint32 cpuID = request -> cpuID;

          // check if the request is out of the outstanging queue
          if (_ref.find(request) != _ref.end()) {
            _ref.erase(request);
            delete request;
          }
          else {
            _ref.insert(request);
          }

          // until the oldest instruction has not finished
          while (_procs[cpuID].outstanding.front() -> finished) {

            MemoryRequest *oldest = _procs[cpuID].outstanding.front();
            _procs[cpuID].outstanding.pop_front();

            // compute the current cycle of the oldest request
            oldest -> currentCycle = max(oldest -> currentCycle,
                _procs[cpuID].currentCycle + oldest -> icount -
                _procs[cpuID].currentIcount);

            if (oldest -> icount > checkpoint[cpuID]) {
              fprintf(_progress, "P%u, %llu\n",
                  cpuID, checkpoint[cpuID]/PROGRESS_LEAP);
              fflush(_progress);
              checkpoint[cpuID] += PROGRESS_LEAP;
            }

            // update the current cycle and icount of the processor
            _procs[cpuID].currentIcount = oldest -> icount;
            _procs[cpuID].currentCycle = oldest -> currentCycle;

            // printf("%llu %llu\n", oldest -> icount, oldest -> currentCycle);

            // check if the request is out of the request queue
            if (_ref.find(oldest) != _ref.end()) {
              _ref.erase(oldest);
              delete oldest;
            }
            else if (_queue.top() == oldest) {
              _queue.pop();
              delete oldest;
            }
            else {
              _ref.insert(oldest);
            }

            // check if any more requests can be added to the queue
            while (((_procs[cpuID].outstanding.back() -> icount) - 
                  (_procs[cpuID].outstanding.front() -> icount)) < _oooWindow) {

              _procs[cpuID].outstanding.back() -> issueCycle =
                _procs[cpuID].currentCycle + 
                _procs[cpuID].outstanding.back() -> icount - 
                _procs[cpuID].currentIcount - _oooWindow;

              _procs[cpuID].outstanding.back() -> currentCycle = 
                _procs[cpuID].outstanding.back() -> issueCycle;

              // push it to the queue and send to the simulator
              _queue.push(_procs[cpuID].outstanding.back());
              _simulator.ProcessMemoryRequest(_procs[cpuID].outstanding.back());

              // get the next request for the processor
              if (_synthetic)
                request = _procs[cpuID].sreader -> NextRequest();
              else 
                request = _procs[cpuID].reader -> NextRequest();
              if (request == NULL) {
                fprintf(stderr, "No requests from processor %u\n", cpuID);
                exit(1);
              }

              // push the request to the outstanding queue
              _procs[cpuID].outstanding.push_back(request);
            }

            // check if the processor has completed run
            if (_procs[cpuID].currentIcount > _milestones[_mIndex[cpuID]].first) {
              bool warmUpMilestone = false;
              if (!finished.test(cpuID)) {

                switch (_milestones[_mIndex[cpuID]].second) {

                  case WARM_UP:
                    _procs[cpuID].checkpointIcount = _procs[cpuID].currentIcount;
                    _procs[cpuID].checkpointCycle = _procs[cpuID].currentCycle;
                    warmUpMilestone = true;
                    _mIndex[cpuID] ++;
                    warmUp.set(cpuID);
                    _simulator.EndProcWarmUp(cpuID);
                    if (warmUp.count() == _numCPUs) {
                      _simulator.EndWarmUp();
                    }
                    
                    break;

                  case END_SIMULATION:
                    if (!finished.test(cpuID)) {
                      _procs[cpuID].finishIcount = _procs[cpuID].currentIcount;
                      _procs[cpuID].finishCycle = _procs[cpuID].currentCycle;
                      finished.set(cpuID);
                      _simulator.EndProcSimulation(cpuID);
                      fprintf(_ipcFile, "%u %llu %llu\n", cpuID, 
                          _procs[cpuID].currentIcount - 
                          _procs[cpuID].checkpointIcount,
                          _procs[cpuID].currentCycle - 
                          _procs[cpuID].checkpointCycle);
                      fflush(_ipcFile);
                    }
                    break;
                }
              }
              if (!warmUpMilestone)
                break;
            }
          }
        }
      }
    }

  public:

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    OoOTraceSimulator(uint32 numCPUs, string simulatorDefinition, 
        string simulatorConfiguration, uint32 oooWindow, 
                      const vector <string> &traceFiles, string simulationFolder,
                      bool synthetic, uint32 workingSetSize, uint32 memGap) {

      _numCPUs = numCPUs;
      _simulatorDefinition = simulatorDefinition;
      _simulatorConfiguration = simulatorConfiguration;
      _oooWindow = oooWindow;
      _simulationFolder = simulationFolder;

      _synthetic = synthetic;
      _workingSetSize = workingSetSize;
      _memGap = memGap;

      if (!synthetic) {
        _traceFiles.resize(_numCPUs);
      }
      _procs.resize(_numCPUs);
      _mIndex.resize(_numCPUs, 0);

      if (!synthetic) {
        for (uint32 i = 0; i < _numCPUs; i ++)
          _traceFiles[i] = traceFiles[i];
      }

      _simulator.InitializeSimulator(numCPUs, simulationFolder,
          simulatorDefinition, simulatorConfiguration);

      string ipcFilename = _simulationFolder + "/sim.ipc";
      _ipcFile = fopen(ipcFilename.c_str(), "w");
      if (_ipcFile == NULL)
        _ipcFile = stdout;

      string progressFileName = _simulationFolder + "/progress";
      _progress = fopen(progressFileName.c_str(), "w");
      if (_progress == NULL)
        _progress = stdout;
    }


    // -------------------------------------------------------------------------
    // Function to start the simulation
    // -------------------------------------------------------------------------

    void StartSimulation() {

      // start the simulator
      _simulator.SetStartCycle(0);
      _simulator.StartSimulation();

      // open the trace readers
      if (!_synthetic) {
        for (uint32 i = 0; i < _numCPUs; i ++)
          _procs[i].reader = new TraceReader(_traceFiles[i], i, true);
      }
      else {
        for (uint32 i = 0; i < _numCPUs; i ++)
          _procs[i].sreader = new SyntheticTrace(_workingSetSize, _memGap, i);
      }


      // for each processor, fill its outstanding queue
      for (uint32 i = 0; i < _numCPUs; i ++) {

        MemoryRequest *request;

        // get the first request from the reader
        if (_synthetic)
          request = _procs[i].sreader -> NextRequest();
        else 
          request = _procs[i].reader -> NextRequest();
        if (request == NULL) {
          fprintf(stderr, "No requests from processor %u\n", i);
          exit(1);
        }

        // set the current cycle and icount of the processor
        _procs[i].currentIcount = 0;
        _procs[i].currentCycle = 0;

        request -> issueCycle = 0;
        request -> currentCycle = 0;

        // push the request to the outstanding queue
        _procs[i].outstanding.push_back(request);

        // while there is room in the out-of-order window
        while (((_procs[i].outstanding.back() -> icount) - 
            (_procs[i].outstanding.front() -> icount)) < _oooWindow) {

          _queue.push(_procs[i].outstanding.back());
          _simulator.ProcessMemoryRequest(_procs[i].outstanding.back());

	// **** Are we completing the simulation here?



          // get the next request 
          if (_synthetic)
            request = _procs[i].sreader -> NextRequest();
          else 
            request = _procs[i].reader -> NextRequest();
          if (request == NULL) {
            assert(false && "No requests from processor");
          }

          request -> issueCycle = request -> icount;
          request -> currentCycle = request -> issueCycle;
          _procs[i].outstanding.push_back(request);
        }
      }
    }


    // -------------------------------------------------------------------------
    // Function to run the simulation
    // -------------------------------------------------------------------------
    
    void RunSimulation(uint64 warmUp, uint64 mainRun, uint64 hbCount) {
    
      _hbCount = hbCount;
      _nextHeartBeatCycle = _hbCount;
      uint64 current;

      current = warmUp;
      _milestones.push_back(make_pair(current, WARM_UP));

      current = warmUp + mainRun;
      _milestones.push_back(make_pair(current, END_SIMULATION));

      Simulate();
      
      _simulator.EndSimulation();

      fclose(_ipcFile);
      fclose(_progress);
    }
    
};

#endif // __OoO_TRACE_SIMULATOR_H__
