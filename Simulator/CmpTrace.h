// -----------------------------------------------------------------------------
// File: CmpTrace.h
// Description:
//    A sample component to dump traces.
// -----------------------------------------------------------------------------

#ifndef __CMP_TRACE_H__
#define __CMP_TRACE_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <zlib.h>


// -----------------------------------------------------------------------------
// Class: CmpTrace
// Description:
//    Defines a simple trace component that dumps traces
// -----------------------------------------------------------------------------

class CmpTrace : public MemoryComponent {

  protected:

    // -------------------------------------------------------------------------
    // Parameters
    // -------------------------------------------------------------------------

    string _traceFileName;

    // -------------------------------------------------------------------------
    // Private members
    // -------------------------------------------------------------------------

    gzFile _trace;


  public:

    // -------------------------------------------------------------------------
    // Constructor. It cannot take any arguments
    // -------------------------------------------------------------------------

    CmpTrace() {
      _traceFileName = "trace";
    }


    // -------------------------------------------------------------------------
    // Virtual functions to be implemented by the components
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    // Function to initialize statistics
    // -------------------------------------------------------------------------

    void InitializeStatistics() {
    }


    // -------------------------------------------------------------------------
    // Function to add a parameter to the component
    // -------------------------------------------------------------------------

    void AddParameter(string pname, string pvalue) {
      CMP_PARAMETER_BEGIN
      CMP_PARAMETER_STRING("trace-file-name", _traceFileName)
      CMP_PARAMETER_END
    }

    // -------------------------------------------------------------------------
    // Function called when simulation starts
    // -------------------------------------------------------------------------

    void StartSimulation() {
      string fileName = _simulationFolderName + "/" + _traceFileName + ".gz";
      _trace = gzopen64(fileName.c_str(), "w");
      assert (_trace != Z_NULL);
    }


    // -------------------------------------------------------------------------
    // Function called when simulation ends
    // -------------------------------------------------------------------------

    void EndSimulation() {
      gzclose(_trace);
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
      if (!_warmUp) {
        gzprintf(_trace, "%llu %llu %llu %llu %u %u\n", request -> icount,
            request -> ip, request -> virtualAddress, 
            request -> physicalAddress, request -> size, request -> type);
      }
      return 0; 
    }


    // -------------------------------------------------------------------------
    // Function to process the return of a request. Return value indicates
    // number of busy cycles for the component.
    // -------------------------------------------------------------------------
    
    cycles_t ProcessReturn(MemoryRequest *request) { 
      return 0; 
    }

};

#endif // __CMP_TRACE_H__
