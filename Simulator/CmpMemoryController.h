// -----------------------------------------------------------------------------
// File: CmpMemoryController.h
// Description:
//    Defines a memory controller which controls DRAM memory. For now, it
//    contains a simple model.
// -----------------------------------------------------------------------------

#ifndef __CMP_MEMORY_CONTROLLER_H__
#define __CMP_MEMORY_CONTROLLER_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Class: CmpMemoryController
// Description:
//    Simple DRAM memory controller model. For now, single channel, single rank.
//    Implements FCFS with drain-when-full.
// -----------------------------------------------------------------------------

class CmpMemoryController : public MemoryComponent {

protected:

  // -------------------------------------------------------------------------
  // Parameters
  // -------------------------------------------------------------------------

  uint32 _numBanks;
  uint32 _rowSize;

  string _schedAlgo;

  
  uint32 _rowHitLatency;
  uint32 _rowConflictLatency;
  uint32 _readToWriteLatency;
  uint32 _writeToReadLatency;

  uint32 _numWriteBufferEntries;
  uint32 _channelDelay;
  uint32 _busProcessorRatio;

  uint32 _dummy;

  // -------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------

  // memory request queue
  list <MemoryRequest *> _readQ;
  list <MemoryRequest *> _writeQ;
  vector <list <MemoryRequest *> > _writeQdwb;

  // last operation type
  MemoryRequest::Type _lastOp;

  // open row in each bank
  vector <addr_t> _openRow;

  // scheduling algorithm
  MemoryRequest * (CmpMemoryController::*NextRequest)();

  // for scheduling algorithms
  bool _drain;
  list <MemoryRequest *> _writeRowHits;
  list <MemoryRequest *> _readRowHits;

  int openWriteBuffer;
  // -------------------------------------------------------------------------
  // Declare counters
  // -------------------------------------------------------------------------

  NEW_COUNTER(accesses);
  NEW_COUNTER(reads);
  NEW_COUNTER(writes);
  NEW_COUNTER(rowhits);
  NEW_COUNTER(rowconflicts);
  NEW_COUNTER(readtowrites);
  NEW_COUNTER(writetoreads);

public:

  // -------------------------------------------------------------------------
  // Constructor. It cannot take any arguments
  // -------------------------------------------------------------------------

  CmpMemoryController() {

    _numBanks = 8;
    _rowSize = 8192;
    _rowHitLatency = 14;
    _rowConflictLatency = 34;
    _readToWriteLatency = 2;
    _writeToReadLatency = 6;
    _numWriteBufferEntries = 64;
    _channelDelay = 4;
    _busProcessorRatio = 6;
    _schedAlgo = "fcfs";
  }


  // -------------------------------------------------------------------------
  // Virtual functions to be implemented by the components
  // -------------------------------------------------------------------------

  // -------------------------------------------------------------------------
  // Function to add a parameter to the component
  // -------------------------------------------------------------------------

  void AddParameter(string pname, string pvalue) {
      
    CMP_PARAMETER_BEGIN

      // Add the list of parameters to the component here
      CMP_PARAMETER_UINT("num-banks", _numBanks)
      CMP_PARAMETER_UINT("row-size", _rowSize)
      CMP_PARAMETER_UINT("num-write-buffer-entries", _numWriteBufferEntries)
      CMP_PARAMETER_STRING("scheduling-algo", _schedAlgo)
      CMP_PARAMETER_UINT("row-hit-latency", _rowHitLatency)
      CMP_PARAMETER_UINT("row-conflict-latency", _rowConflictLatency)
      CMP_PARAMETER_UINT("read-to-write-latency", _readToWriteLatency)
      CMP_PARAMETER_UINT("write-to-read-latency", _readToWriteLatency)
        
      CMP_PARAMETER_UINT("channel-delay", _channelDelay)
      CMP_PARAMETER_UINT("bus-processor-ratio", _busProcessorRatio)

      CMP_PARAMETER_UINT("stall-count", _dummy)
      CMP_PARAMETER_UINT("cmp-stall-count", _dummy)

      CMP_PARAMETER_END
      }


  // -------------------------------------------------------------------------
  // Function to initialize statistics
  // -------------------------------------------------------------------------

  void InitializeStatistics() {

    INITIALIZE_COUNTER(accesses, "Total Accesses");
    INITIALIZE_COUNTER(reads, "Read Accesses");
    INITIALIZE_COUNTER(writes, "Write Accesses");
    INITIALIZE_COUNTER(rowhits, "Row Buffer Hits");
    INITIALIZE_COUNTER(rowconflicts, "Row Buffer Conflicts");
    INITIALIZE_COUNTER(readtowrites, "Read to Write Switches");
    INITIALIZE_COUNTER(writetoreads, "Write to Read Switches");
  }


  // -------------------------------------------------------------------------
  // Function called when simulation starts
  // -------------------------------------------------------------------------

  void StartSimulation() {
    _openRow.resize(_numBanks, 0);
    NextRequest = GetSchedulingAlgorithmFunction(_schedAlgo);
    _drain = false;
    _lastOp = MemoryRequest::READ;
    openWriteBuffer = 0;

    _rowHitLatency *= _busProcessorRatio;
    _rowConflictLatency *= _busProcessorRatio;
    _readToWriteLatency *= _busProcessorRatio;
    _writeToReadLatency *= _busProcessorRatio;
    _channelDelay *= _busProcessorRatio;
  }


  // -------------------------------------------------------------------------
  // Function called at a heart beat. Argument indicates cycles elapsed after
  // previous heartbeat
  // -------------------------------------------------------------------------
    
  void HeartBeat(cycles_t hbCount) {
  }


  void EndSimulation() {
    DUMP_STATISTICS;
    CLOSE_ALL_LOGS;
  }


protected:

  // -------------------------------------------------------------------------
  // Function to get the scheduling algorithm function
  // -------------------------------------------------------------------------

  MemoryRequest * (CmpMemoryController::*GetSchedulingAlgorithmFunction(
                                                                        string algo)) () {
    if (algo.compare("fcfs")) return &CmpMemoryController::FCFS;
  }


  // -------------------------------------------------------------------------
  // Function to process a request. Return value indicates number of busy
  // cycles for the component.
  // -------------------------------------------------------------------------

  cycles_t ProcessRequest(MemoryRequest *request) {

    INCREMENT(accesses);

    cycles_t latency = 0;
    cycles_t turnAround = 0;

    // determine if there is a switch penalty
    switch (request -> type) {

    case MemoryRequest::READ: case MemoryRequest::READ_FOR_WRITE: case MemoryRequest::PREFETCH:
      INCREMENT(reads);
      if (_lastOp == MemoryRequest::WRITEBACK) {
        INCREMENT(writetoreads);
        latency += _writeToReadLatency;
        turnAround = _writeToReadLatency;
      }
      break;

    case MemoryRequest::WRITEBACK:
      INCREMENT(writes);
      if (_lastOp == MemoryRequest::READ) {
        INCREMENT(readtowrites);
        latency += _readToWriteLatency;
        turnAround = _readToWriteLatency;
      }
      break;

    case MemoryRequest::WRITE:
    case MemoryRequest::PARTIALWRITE:
      fprintf(stderr, "Memory controller cannot get a write\n");
      exit(0);          
    }

    _lastOp = request -> type;

      
    // Get the row address of the request
    addr_t logicalRow = (request -> virtualAddress) / _rowSize;
    uint32 bankIndex = logicalRow % _numBanks;
    uint32 rowID = logicalRow / _numBanks;

    // check if the access is a row hit or conflict
    if (_openRow[bankIndex] == rowID) {
      INCREMENT(rowhits);
      latency += _rowHitLatency;
    }

    else {
      INCREMENT(rowconflicts);
      latency += _rowConflictLatency;
      _openRow[bankIndex] = rowID;
    }

    request -> AddLatency(latency);
    request -> serviced = true;
    return _channelDelay + turnAround;
  }


  // -------------------------------------------------------------------------
  // Function to process the return of a request. Return value indicates
  // number of busy cycles for the component.
  // -------------------------------------------------------------------------
    
  cycles_t ProcessReturn(MemoryRequest *request) { 
    return 0; 
  }


  // -------------------------------------------------------------------------
  // Overriding process pending requests. To do batch processing
  // -------------------------------------------------------------------------


  void ProcessPendingRequests() {

    // if processing return
    if (_processing) return;
    _processing = true;

    // if the request queue is empty return
    if (_queue.empty() && _readQ.empty() && _writeQ.empty() 
        && _readRowHits.empty() && _writeRowHits.empty()) {
      _processing = false;
      return;
    }

    MemoryRequest *request;

    // take all the requests in the queue till the simulator cycle and add
    // them to the read or write queue.
    if (!_queue.empty()) {

      request = _queue.top();

      while (request -> currentCycle <= (*_simulatorCycle)) {
        _queue.pop();

        // if the request is already serviced
        if (request -> serviced) {
          cycles_t busyCycles = ProcessReturn(request);
          _currentCycle += busyCycles;
          SendToNextComponent(request);
        }

        // else add the request to the corresponding queue 
        else {

          switch (request -> type) {
          case MemoryRequest::READ: case MemoryRequest::READ_FOR_WRITE: case MemoryRequest::PREFETCH: 
            _readQ.push_back(request);
            break;

          case MemoryRequest::WRITEBACK:
            _writeQ.push_back(request);
            break;

          case MemoryRequest::WRITE:
          case MemoryRequest::PARTIALWRITE:
            printf("Memory controller cannot receive a direct write\n");
            exit(0);
          }
        }

        if (_queue.empty())
          break;

        request = _queue.top();
      }
    }

    // process requests until there are none or the component time 
    // exceeds simulator time
    while (_currentCycle <= (*_simulatorCycle)) {

      // get the next request to schedule
      // TODO: request = (*this.*NextRequest)();
      request = FRFCFSDrainWhenFull();

      if (request == NULL)
        break;

      // process the request
      cycles_t now = max(request -> currentCycle, _currentCycle);
      _currentCycle = now;
      request -> currentCycle = now;
      cycles_t busyCycles = ProcessRequest(request);
      _currentCycle += busyCycles;
      SendToNextComponent(request);
    }

    _processing = false;
  }


  // -------------------------------------------------------------------------
  // Function to check if a request is a row buffer hit
  // -------------------------------------------------------------------------

  bool IsRowBufferHit(MemoryRequest *request) {
    addr_t logicalRow = (request -> virtualAddress) / _rowSize;
    uint32 bankIndex = logicalRow % _numBanks;
    addr_t rowID = logicalRow / _numBanks;
    if (_openRow[bankIndex] == rowID)
      return true;
    return false;
  }

#include "MemorySchedulers.h"

};

#endif // __CMP_MEMORY_CONTROLLER_H__
