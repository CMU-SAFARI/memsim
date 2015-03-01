// -----------------------------------------------------------------------------
// File: CmpStreamPrefetcher.h
// Description:
//
// -----------------------------------------------------------------------------

#ifndef __CMP_STREAM_PREFETCHER_H__
#define __CMP_STREAM_PREFETCHER_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "GenericTable.h"
#include "Types.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <cstdlib>

// -----------------------------------------------------------------------------
// Class: CmpStreamPrefetcher
// Description:
// This class implements a stream prefetcher. Similar to the IBM
// Power prefetchers. Imported primarily from the stream
// prefetcher in scarab/ringo
// -----------------------------------------------------------------------------

class CmpStreamPrefetcher : public MemoryComponent {

protected:

  // -------------------------------------------------------------------------
  // Parameters
  // -------------------------------------------------------------------------

  uint32 _blockSize;
  bool _prefetchOnWrite;
  
  uint32 _tableSize;
  string _tablePolicy;
  uint32 _numTrains;
  uint32 _trainDistance;
  uint32 _distance;
  uint32 _degree;
  uint32 _maxFakeCounter;
  bool _fake;


  // -------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------

  enum StreamDirection {
    FORWARD = 1,
    BACKWARD = -1,
    NONE = 0
  };
  
  struct StreamEntry {
    // the miss address that allocated the stream entry and the
    // instruction pointer that caused the miss
    addr_t allocMissAddress;
    addr_t ip;

    // app ID
    uint32 appID;
    uint64 counterVal;
    bool faked;

    // start and end pointers of the stream
    addr_t sp;
    addr_t ep;

    // start and end physical addresses
    addr_t psp;
    addr_t pep;

    // last fake
    addr_t last_demand_v;
    addr_t last_demand_p;
    addr_t fake_vp;
    addr_t fake_pp;

    // is the prefetcher trained
    int trainHits;
    bool trained;
    StreamDirection direction;
  };

  vector <uint64> _appCounter;
  
  // Prefetcher table
  generic_table_t <uint32, StreamEntry> _streamTable;

  // Running index, primarily to reuse the generic table
  // implementation
  uint32 _runningIndex;

  // Frequently used values
  addr_t _trainAddrDistance;
  addr_t _prefetchAddrDistance;

  
  // -------------------------------------------------------------------------
  // Declare Counters
  // -------------------------------------------------------------------------

  NEW_COUNTER(num_prefetches);
  
public:

  // -------------------------------------------------------------------------
  // Constructor. It cannot take any arguments
  // -------------------------------------------------------------------------

  CmpStreamPrefetcher() {
    _blockSize = 64;
    _prefetchOnWrite = false;

    _tableSize = 16;
    _tablePolicy = "lru";
    _trainDistance = 16;
    _numTrains = 2;
    _distance = 24;
    _degree = 4;
    _maxFakeCounter = 16;
    _fake = false;
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
      CMP_PARAMETER_UINT("block-size", _blockSize)
      CMP_PARAMETER_BOOLEAN("prefetch-on-write", _prefetchOnWrite)
      CMP_PARAMETER_BOOLEAN("fake", _fake)

      CMP_PARAMETER_UINT("table-size", _tableSize)
      CMP_PARAMETER_STRING("table-policy", _tablePolicy)
      CMP_PARAMETER_UINT("train-distance", _trainDistance)
      CMP_PARAMETER_UINT("num-trains", _numTrains)
      CMP_PARAMETER_UINT("distance", _distance)
      CMP_PARAMETER_UINT("degree", _degree)
      CMP_PARAMETER_UINT("max-fake-counter", _maxFakeCounter)

    CMP_PARAMETER_END
 }


  // -------------------------------------------------------------------------
  // Function to initialize statistics
  // -------------------------------------------------------------------------

  void InitializeStatistics() {
    INITIALIZE_COUNTER(num_prefetches, "Number of prefetches issued")
  }


  // -------------------------------------------------------------------------
  // Function called when simulation starts
  // -------------------------------------------------------------------------

  void StartSimulation() {
    _streamTable.SetTableParameters(_tableSize, _tablePolicy);
    _runningIndex = 0;

    _appCounter.resize(_numCPUs, 0);

    _trainAddrDistance = _trainDistance * _blockSize;
    _prefetchAddrDistance = _distance * _blockSize;
  }


  // -------------------------------------------------------------------------
  // Function called at a heart beat. Argument indicates cycles elapsed after
  // previous heartbeat
  // -------------------------------------------------------------------------

  void HeartBeat(cycles_t hbCount) {
  }


protected:

  // -------------------------------------------------------------------------
  // Function to process a request. Return value indicates number of busy
  // cycles for the component.
  // -------------------------------------------------------------------------

  cycles_t ProcessRequest(MemoryRequest *request) {

    if (request -> type == MemoryRequest::WRITE ||
        request -> type == MemoryRequest::WRITEBACK ||
        request -> type == MemoryRequest::PREFETCH) {
      // do nothing
      return 0;
    }

    if (!_prefetchOnWrite &&
        (request -> type == MemoryRequest::READ_FOR_WRITE)) {
      // do nothing
      return 0;
    }

    _appCounter[request -> cpuID] ++;

    addr_t vcla = VBLOCK_ADDRESS(request, _blockSize);
    addr_t pcla = PBLOCK_ADDRESS(request, _blockSize);
    
    table_t <uint32, StreamEntry>::entry row;

    bool hit = false;
    uint32 key;
    
    // Check if there is a stream entry matching the address
    for (uint32 i = 0; i < _tableSize; i ++) {

      // get row i from the stream table
      row = _streamTable.entry_at_index(i);

      // if row is invalid, continue
      if (!row.valid) continue;

      // get the stream entry information
      StreamEntry entry = row.value;

      // if entry is in the training phase
      if (!row.value.trained) {
        if (llabs(entry.allocMissAddress - vcla) < _trainAddrDistance) {
          // HIT! entry within training scope
          hit = true;
          key = row.key;
          break;
        }
      }

      // not training phase
      else {
        if (entry.sp <= vcla && entry.ep >= vcla) {
          // HIT! entry within monitor scope
          hit = true;
          key = row.key;
          break;
        }
      }
    }


    // If there is a stream entry, then update the entry based on
    // the current phase and issue prefetches if necessary
    if (hit) {
      // dummy read to update replacement state
      _streamTable.read(key);
      
      // real read to modify stream entry state
      StreamEntry &entry = _streamTable[key];
      entry.counterVal = _appCounter[entry.appID];
      entry.faked = false;

      // entry not trained yet
      if (!entry.trained) {
        // forward direction
        if (entry.allocMissAddress < vcla) {
          switch (entry.direction) {
          case FORWARD:
            // same direction.
            entry.trainHits ++;
            if (vcla > entry.ep) {
              entry.ep = vcla;
              entry.pep = pcla;
            }
            break;
          case BACKWARD:
          case NONE:
            // new direction
            entry.trainHits = 1;
            entry.direction = FORWARD;
            entry.ep = vcla;
            entry.pep = pcla;
            break;
          }
        }

        // backward direction
        else {
          switch (entry.direction) {
          case BACKWARD:
            // same direction.
            entry.trainHits ++;
            if (vcla < entry.ep) {
              entry.ep = vcla;
              entry.pep = pcla;
            }
            break;
          case FORWARD:
          case NONE:
            // new direction
            entry.trainHits = 1;
            entry.direction = BACKWARD;
            entry.ep = vcla;
            entry.pep = pcla;
            break;
          }
        }

        // Upgrade to trained?
        if (entry.trainHits >= _numTrains)
          entry.trained = true;
      }

      // entry trained
      if (entry.trained) {
        // Issue prefetches

        // update the request entry
        request -> d_prefetched = true;
        request -> d_prefID = row.index;

        int32 numPrefetches = 0;

        // start points to current demand
        entry.sp = vcla;
        entry.psp = pcla;
        
        // determine number of prefetches to issue
        int32 maxPrefetches = 0;
        if (entry.direction == FORWARD) {
          addr_t maxAddress = entry.sp + (_prefetchAddrDistance + _blockSize);
          maxPrefetches = (maxAddress - entry.ep) / _blockSize;
        }
        else {
          addr_t minAddress = entry.sp - (_prefetchAddrDistance + _blockSize);
          maxPrefetches = (entry.ep - minAddress) / _blockSize;
        }
        numPrefetches = (maxPrefetches < _degree ? maxPrefetches : _degree);

        for (int32 i = 0; i < numPrefetches; i ++) {
          entry.ep += (entry.direction * _blockSize);
          entry.pep += (entry.direction * _blockSize);
          MemoryRequest *prefetch =
            new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this,
                              MemoryRequest::PREFETCH, request -> cmpID, 
                              entry.ep, entry.pep, _blockSize,
                              request -> currentCycle);
          prefetch -> icount = request -> icount;
          prefetch -> ip = request -> ip;
          prefetch -> prefetcherID = row.index;
          SendToNextComponent(prefetch);
        }

        ADD_TO_COUNTER(num_prefetches, numPrefetches);

        // issue fake reads
        int32 numFakes;

        addr_t vcurrent, pcurrent;

        if (_fake) {
          if (entry.direction == FORWARD) {
            vcurrent = entry.last_demand_v + _blockSize;
            pcurrent = entry.last_demand_p + _blockSize;
            numFakes = (int32)((int64)vcla - vcurrent) / _blockSize;
            if (numFakes <= _distance) {
              entry.faked = true;
              for (int32 i = 0; i < numFakes; i ++) {
                MemoryRequest *fake =
                  new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this,
                                    MemoryRequest::FAKE_READ, request -> cmpID,
                                    vcurrent, pcurrent, _blockSize,
                                    request -> currentCycle);
                fake -> icount = request -> icount;
                fake -> ip = request -> ip;
                fake -> prefetcherID = row.index;
                SendToNextComponent(fake);
                entry.fake_vp = vcurrent;
                entry.fake_pp = pcurrent;
                vcurrent += _blockSize;
                pcurrent += _blockSize;
              }
            }
          }
          else if (entry.direction == BACKWARD) {
            vcurrent = entry.last_demand_v - _blockSize;
            pcurrent = entry.last_demand_p - _blockSize;
            numFakes = (int32)((int64)vcurrent - vcla) / _blockSize;
            if (numFakes <= _distance) {
              entry.faked = true;
              for (int32 i = 0; i < numFakes; i ++) {
                MemoryRequest *fake =
                  new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this,
                                    MemoryRequest::FAKE_READ, request -> cmpID,
                                    vcurrent, pcurrent, _blockSize,
                                    request -> currentCycle);
                fake -> icount = request -> icount;
                fake -> ip = request -> ip;
                fake -> prefetcherID = row.index;
                entry.fake_vp = vcurrent;
                entry.fake_pp = pcurrent;
                SendToNextComponent(fake);
                vcurrent -= _blockSize;
                pcurrent -= _blockSize;
              }
            }
          }
        }

        //        printf("-- fakes = %d\n", numFakes);

        entry.last_demand_v = vcla;
        entry.last_demand_p = pcla;
        
        if (entry.direction == FORWARD &&
            (entry.ep - entry.sp) > _prefetchAddrDistance) {
          entry.sp = entry.ep - _prefetchAddrDistance;
        }
        else if (entry.direction == BACKWARD &&
                 (entry.sp - entry.ep) > _prefetchAddrDistance) {
          entry.sp = entry.ep + _prefetchAddrDistance;
        }
      }

      // Remove redundant stream entry
      for (uint32 i = 0; i < _tableSize; i ++) {
        row = _streamTable.entry_at_index(i);
        if (!row.valid) continue;
        if (row.key == key) continue;

        if (((entry.direction == FORWARD) &&
             ((row.value.sp <= entry.ep && row.value.sp >= entry.sp) ||
              (row.value.ep <= entry.ep && row.value.ep >= entry.sp))) ||
            ((entry.direction == BACKWARD) &&
             ((row.value.sp <= entry.sp && row.value.sp >= entry.ep) ||
              (row.value.ep <= entry.sp && row.value.ep >= entry.ep)))) {
          _streamTable.invalidate(row.key);
        }
      }
    }
    
    // If there is no stream entry, allocate a new stream entry
    else {
      // Create a new stream entry
      StreamEntry entry;
      table_t <uint32, StreamEntry>::entry evicted;
      entry.allocMissAddress = vcla;
      entry.ip = request -> ip;
      entry.appID = request -> cpuID;
      entry.counterVal = _appCounter[entry.appID];
      entry.sp = vcla;
      entry.ep = vcla;
      entry.last_demand_v = vcla;
      entry.fake_vp = vcla;
      entry.psp = pcla;
      entry.pep = pcla;
      entry.last_demand_p = pcla;
      entry.fake_pp = pcla;
      entry.trainHits = 0;
      entry.trained = false;
      entry.faked = false;
      entry.direction = NONE;
      evicted = _streamTable.insert(_runningIndex, entry);
      _runningIndex ++;
      
      if (_fake && evicted.valid && evicted.value.trained) {
        // issue fake reads
        addr_t vcurrent = evicted.value.sp;
        addr_t pcurrent = evicted.value.psp;
        int32 numFakes;
        if (evicted.value.direction == FORWARD && (evicted.value.sp < evicted.value.ep)) {
          numFakes = (int32)(evicted.value.ep - evicted.value.sp)/_blockSize;
          if (numFakes < _distance) {
            
            for (int32 i = 0; i < numFakes; i ++) {
              MemoryRequest *fake =
                new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this,
                                  MemoryRequest::FAKE_READ, request -> cmpID,
                                  vcurrent, pcurrent, _blockSize,
                                  request -> currentCycle);
              fake -> icount = request -> icount;
              fake -> ip = request -> ip;
              fake -> prefetcherID = row.index;
              SendToNextComponent(fake);
              vcurrent += _blockSize;
              pcurrent += _blockSize;
            }
          }
        }
        else if (evicted.value.direction == BACKWARD && (evicted.value.sp > evicted.value.ep)) {
          numFakes = (int32)(evicted.value.sp - evicted.value.ep)/_blockSize;
          if (numFakes <= _distance) {
            for (int32 i = 0; i < numFakes; i ++) {
              MemoryRequest *fake =
                new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this,
                                  MemoryRequest::FAKE_READ, request -> cmpID,
                                  vcurrent, pcurrent, _blockSize,
                                  request -> currentCycle);
              fake -> icount = request -> icount;
              fake -> ip = request -> ip;
              fake -> prefetcherID = row.index;
              SendToNextComponent(fake);
              vcurrent -= _blockSize;
              pcurrent -= _blockSize;
            }
          }
        }
        // printf("-- Enum fakes = %d\n", numFakes);
      }
    }
      
    return 0; 
  }


  // -------------------------------------------------------------------------
  // Function to process the return of a request. Return value indicates
  // number of busy cycles for the component.
  // -------------------------------------------------------------------------
    
  cycles_t ProcessReturn(MemoryRequest *request) {

    // if its a prefetch/fake from this component, delete it
    if (request -> iniType == MemoryRequest::COMPONENT &&
        request -> iniPtr == this) {
      request -> destroy = true;
    }
    
    return 0; 
  }

};

#endif // __CMP_STREAM_PREFETCHER_H__
