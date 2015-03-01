// -----------------------------------------------------------------------------
// File: CmpNextLinePrefetcher.h
// Description:
//
// -----------------------------------------------------------------------------

#ifndef __CMP_NEXT_LINE_PREFETCHER_H__
#define __CMP_NEXT_LINE_PREFETCHER_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Class: CmpNextLinePrefetcher
// Description:
// This class implements a simple next line prefetcher. The number
// of lines to prefetch can be configured
// -----------------------------------------------------------------------------

class CmpNextLinePrefetcher : public MemoryComponent {

protected:

  // -------------------------------------------------------------------------
  // Parameters
  // -------------------------------------------------------------------------

  uint32 _degree;
  uint32 _blockSize;
  bool _prefetchOnWrite;
  
  
  // -------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------

  // Next line prefetcher requires no state
  
  // -------------------------------------------------------------------------
  // Declare Counters
  // -------------------------------------------------------------------------

  // No counters for now. May need later.

public:

  // -------------------------------------------------------------------------
  // Constructor. It cannot take any arguments
  // -------------------------------------------------------------------------

  CmpNextLinePrefetcher() {
    _degree = 4;
    _prefetchOnWrite = false;
    _blockSize = 64;
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

    CMP_PARAMETER_END
 }


  // -------------------------------------------------------------------------
  // Function to initialize statistics
  // -------------------------------------------------------------------------

  void InitializeStatistics() {
  }


  // -------------------------------------------------------------------------
  // Function called when simulation starts
  // -------------------------------------------------------------------------

  void StartSimulation() {
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

    // Prefetch the next "degree" cachelines
    // TODO: take care of across page prefetching!

    addr_t vcla = VBLOCK_ADDRESS(request, _blockSize);
    addr_t pcla = PBLOCK_ADDRESS(request, _blockSize);
    
    for (int i = 0; i < _degree; i ++) {
      vcla += _blockSize;
      pcla += _blockSize;
      MemoryRequest *prefetch =
        new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this,
                          MemoryRequest::PREFETCH, request -> cmpID, 
                          vcla, pcla, _blockSize, request -> currentCycle);
      prefetch -> icount = request -> icount;
      prefetch -> ip = request -> ip;
      SendToNextComponent(prefetch);
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

#endif // __CMP_NEXT_LINE_PREFETCHER_H__
