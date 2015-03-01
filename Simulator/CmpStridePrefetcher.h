// -----------------------------------------------------------------------------
// File: CmpStridePrefetcher.h
// Description:
//
// -----------------------------------------------------------------------------

#ifndef __CMP_STRIDE_PREFETCHER_H__
#define __CMP_STRIDE_PREFETCHER_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"
#include "GenericTable.h"


// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Class: CmpStridePrefetcher
// Description:
// This class implements a simple stride prefetcher. The number
// of strides to prefetch can be configured
// -----------------------------------------------------------------------------


class CmpStridePrefetcher : public MemoryComponent {

protected:

  // -------------------------------------------------------------------------
  // Parameters
  // -------------------------------------------------------------------------

  uint32 _degree;
  uint32 _blockSize;
  bool _prefetchOnWrite;

  uint32 _tableSize;
  string _tablePolicy;
  uint32 _numTrains;
  uint32 _trainDistance;
  uint32 _distance;
  
  // -------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------

  // Stride table entry
  
  // last seen address
  // last stride
  // trained?
  
  struct StrideEntry{

    // last recorded request address
    addr_t vaddr;
    addr_t paddr;

    // last sent prefetch
    addr_t vpref;
    addr_t ppref;

    // stride length
    int stride;

    // training state
    int trainHits;
    bool trained;
    
  };

  generic_table_t <addr_t, StrideEntry> _strideTable;


  // -------------------------------------------------------------------------
  // Declare Counters
  // -------------------------------------------------------------------------

  NEW_COUNTER(num_prefetches);


public:

  // -------------------------------------------------------------------------
  // Constructor. It cannot take any arguments
  // -------------------------------------------------------------------------

  CmpStridePrefetcher() {
    _degree = 4;
    _prefetchOnWrite = false;
    _blockSize = 64;
		
    _tableSize = 16;
    _tablePolicy = "lru";

    _numTrains = 2;
    _distance = 24;
    _trainDistance = 16;
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
      CMP_PARAMETER_UINT("degree", _degree)
      CMP_PARAMETER_UINT("block-size", _blockSize)
      CMP_PARAMETER_BOOLEAN("prefetch-on-write", _prefetchOnWrite)	  
      CMP_PARAMETER_UINT("table-size", _tableSize)
      CMP_PARAMETER_STRING("table-policy", _tablePolicy)
      CMP_PARAMETER_UINT("train-distance", _trainDistance)
      CMP_PARAMETER_UINT("num-trains", _numTrains)
      CMP_PARAMETER_UINT("distance", _distance)

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
    _strideTable.SetTableParameters(_tableSize, _tablePolicy);
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

    addr_t vcla = VBLOCK_ADDRESS(request, _blockSize);
    addr_t pcla = PBLOCK_ADDRESS(request, _blockSize);
    addr_t ip = request -> ip;

    table_t <addr_t, StrideEntry>::entry row;

    row = _strideTable.read(ip);
    
    // if no IP entry found, add new entry.
    if (!row.valid) {
      StrideEntry entry;
      entry.vaddr = VBLOCK_ADDRESS(request, _blockSize); 
      entry.paddr = PBLOCK_ADDRESS(request, _blockSize);
      entry.trained = false;
      entry.trainHits = 0;
      entry.stride = 0;
      
      // insert the new entry into the table
      _strideTable.insert(request->ip, entry);
      
      return 0;
    }
		

    // Stride table hit. Actual read entry
    StrideEntry &entry = _strideTable[ip];
		
    // compute stride
    int vstride = vcla - entry.vaddr;

    // if stride mismatch, retrain
    if (entry.stride != vstride) {
      entry.trainHits = 0;
      entry.trained = false;
      entry.stride = vstride;
    }
    
    entry.vaddr = vcla;
    entry.paddr = pcla;

    // if entry is not trained, then update vpref and ppref
    if (!entry.trained) {
      entry.trainHits ++;
      entry.vpref = vcla;
      entry.ppref = pcla;
    }

    // check if entry is trained
    if (entry.trainHits >= _numTrains)
      entry.trained = true;

    // if stride is 0, no point prefetching
    if (entry.stride == 0)
      return 0;


    // If entry is trained, issue prefetches.
    if (entry.trained == true) {
		  
      // figure out how many prefetches to send
      addr_t maxAddress =
        entry.vaddr + ((_distance + 1) * entry.stride * _blockSize);
      int maxPrefetches = (maxAddress - entry.vpref)/_blockSize;
      int numPrefetches = (maxPrefetches > _degree) ? _degree : maxPrefetches;

      // issue prefetches
      for (int i = 0; i < numPrefetches; i ++) {

        //virtual and physical addresses of next prefetch
        entry.vpref += _blockSize * entry.stride;
        entry.ppref += _blockSize * entry.stride;

        // generate memory request
        MemoryRequest *prefetch =
          new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this,
                            MemoryRequest::PREFETCH, request -> cmpID, 
                            entry.vpref, entry.ppref, _blockSize,
                            request -> currentCycle);
        prefetch -> icount = request -> icount;
        prefetch -> ip = request -> ip;

        // send the prefetch request downstream
        SendToNextComponent(prefetch);
      }
		
      ADD_TO_COUNTER(num_prefetches, numPrefetches);
    }

    return 0;
  }


  // -------------------------------------------------------------------------
  // Function to process the return of a request. Return value indicates
  // number of busy cycles for the component.
  // -------------------------------------------------------------------------
    
  cycles_t ProcessReturn(MemoryRequest *request) {

    // if its a prefetch from this component, delete it
    if (request -> iniType == MemoryRequest::COMPONENT &&
        request -> iniPtr == this) {
      request -> destroy = true;
    }
    
    return 0; 
  }

};

#endif // __CMP_STRIDE_PREFETCHER_H__
