// -----------------------------------------------------------------------------
// File: TraceReader.h
// Description:
//    Defines a reader for trace files. It can handle trace I generated.
// -----------------------------------------------------------------------------

#ifndef __TRACE_READER_H__
#define __TRACE_READER_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"
#include "MemoryRequest.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <zlib.h>
#include <stdio.h>
#include <string>

// -----------------------------------------------------------------------------
// Class: TraceReader
// Description:
//    Defines a reader for trace files.
// -----------------------------------------------------------------------------

class TraceReader {

  protected:

    // -------------------------------------------------------------------------
    // Parameters
    // -------------------------------------------------------------------------

    string _traceFileName;
    uint32 _cpuID;
    bool _wrapAround;

    // -------------------------------------------------------------------------
    // Private members
    // -------------------------------------------------------------------------

    bool _noTrace;
    gzFile _trace;
    uint64 _startIcount;
    uint64 _lastIcount;
    uint64 _icountShift;
    uint64 _cycleShift;
    bool _first;

    // -------------------------------------------------------------------------
    // Normalize the address
    // -------------------------------------------------------------------------

  uint64 Normalize(uint64 val, uint32 shift = 48) {
      return (val + ((addr_t)(_cpuID) << shift));
    }

  public:


    // -------------------------------------------------------------------------
    // Constructor with options
    // -------------------------------------------------------------------------

    TraceReader(string traceFileName, uint32 cpuID, bool wrapAround) {
      // update members
      _traceFileName = traceFileName;
      _cpuID = cpuID;
      _wrapAround = wrapAround;

      _icountShift = 0;
      _cycleShift = 0;
      _noTrace = false;
      _first = true;

      // open the trace file
      _trace = gzopen64(_traceFileName.c_str(), "r");
      if (_trace == Z_NULL) {
        _noTrace = true;
        // TODO: Error message
      }
    }


    // -------------------------------------------------------------------------
    // Function to return the next request in the trace
    // -------------------------------------------------------------------------

    MemoryRequest *NextRequest() {

      // if there is no trace, return NULL
      if (_noTrace)
        return NULL;

      char line[300];
      char *ret;

      // read a line from the trace
      ret = gzgets(_trace, line, 300);

      // if there is a valid entry
      if (ret != Z_NULL) {
        MemoryRequest *request;
        // create a new request and obtain the details
        request = new MemoryRequest;

        // scan the line and fill the request
        /* sscanf(line, "%u %llu %llu %llu %llu %llu %u %c",
            &(request -> cpuID), &(request -> currentCycle), 
            &(request -> icount), &(request -> ip), &(request -> virtualAddress),
            &(request -> physicalAddress), &(request -> size), &type); */
        uint32 type;
        sscanf(line, "%llu %llu %llu %llu %u %u", &(request -> icount),
            &(request -> ip), &(request -> virtualAddress), 
            &(request -> physicalAddress), &(request -> size), 
            &(type));
        
        // make initial updates
        request -> iniType = MemoryRequest::CPU;
        request -> cpuID = _cpuID;
        request -> iniPtr = NULL;
        request -> type = (MemoryRequest::Type)(type);

        // normalize the addresses
        request -> ip = Normalize(request -> ip);
        request -> virtualAddress = Normalize(request -> virtualAddress);
        request -> physicalAddress = Normalize(request -> physicalAddress, 32);

        // check if the request is the first request
        if (_first) {
          _first = false;
          _startIcount = request -> icount;
          request -> icount = 1;
          _lastIcount = 0;
        }
        else {
          request -> icount -= _startIcount;
          if (request -> icount == _lastIcount)
            request -> icount ++;
        }

        request -> icount += _icountShift;

        while (_lastIcount >= request -> icount) {
          request -> icount ++;
        }

        _lastIcount = request -> icount;
        return request;
      }

      // nothing in trace
      else if (_first) {
        _noTrace = true;
        return NULL;
      }

      // if trace ended
      else if (_wrapAround) {
        _icountShift = _lastIcount + 1;
        // close and reopen the file
        gzclose(_trace);
        _trace = gzopen64(_traceFileName.c_str(), "r");
        // return the next request
        return NextRequest();
      }

      // return NULL
      return NULL;
    }
};

#endif // __TRACE_READER_H__
