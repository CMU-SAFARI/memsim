// -----------------------------------------------------------------------------
// File: CmpStall.h
// Description:
//    This component simply stalls the memory request for a specified number of
//    cycles.
// -----------------------------------------------------------------------------

#ifndef __CMP_STALL_H__
#define __CMP_STALL_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Class: CmpStall
// Description:
//    Simple stalling component
// -----------------------------------------------------------------------------

class CmpStall : public MemoryComponent {

protected:

  // -------------------------------------------------------------------------
  // Parameters
  // -------------------------------------------------------------------------

  uint32 _stallCount;
  uint32 _cmpStallCount;

  // -------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------


  NEW_COUNTER(reads);
  NEW_COUNTER(prefetches);
  NEW_COUNTER(writes);

public:

  // -------------------------------------------------------------------------
  // Constructor. It cannot take any arguments
  // -------------------------------------------------------------------------

  CmpStall() {
    _stallCount = 300;
    _cmpStallCount = 0;
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
      CMP_PARAMETER_UINT("stall-count", _stallCount)
      CMP_PARAMETER_UINT("cmp-stall-count", _cmpStallCount)

    CMP_PARAMETER_END
  }


  // -------------------------------------------------------------------------
  // Function to initialize statistics
  // -------------------------------------------------------------------------

  void InitializeStatistics() {
    INITIALIZE_COUNTER(reads, "reads")
    INITIALIZE_COUNTER(prefetches, "prefetches")
    INITIALIZE_COUNTER(writes, "writes")
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
    switch(request -> type) {
    case MemoryRequest::READ:
    case MemoryRequest::READ_FOR_WRITE:
      INCREMENT(reads);
      break;
    case MemoryRequest::PREFETCH:
      INCREMENT(prefetches);
      break;
    case MemoryRequest::WRITEBACK:
      INCREMENT(writes);
      break;
    }
    request -> AddLatency(_stallCount);
    return _cmpStallCount; 
  }


  // -------------------------------------------------------------------------
  // Function to process the return of a request. Return value indicates
  // number of busy cycles for the component.
  // -------------------------------------------------------------------------
    
  cycles_t ProcessReturn(MemoryRequest *request) { 
    return 0; 
  }

};

#endif // __CMP_STALL_H__
