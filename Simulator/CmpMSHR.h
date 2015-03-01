// -----------------------------------------------------------------------------
// File: CmpMSHR.h
// Description:
// -----------------------------------------------------------------------------

#ifndef __CMP_MSHR_H__
#define __CMP_MSHR_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <map>
#include <list>

#define MSHR_STALL_PENALTY 10

// -----------------------------------------------------------------------------
// Class: CmpMSHR
// Description:
// -----------------------------------------------------------------------------

class CmpMSHR : public MemoryComponent {

  protected:

    // -------------------------------------------------------------------------
    // Parameters
    // -------------------------------------------------------------------------

    uint32 _count;
    uint32 _blockSize;

    // -------------------------------------------------------------------------
    // Private members
    // -------------------------------------------------------------------------

    map <addr_t, list <MemoryRequest *> > _missed;
    list <MemoryRequest *> _waitQ;
    map <addr_t, MemoryRequest *> _outstanding;
    map <MemoryRequest *, MemoryRequest *> _handler;


  public:

    // -------------------------------------------------------------------------
    // Constructor. It cannot take any arguments
    // -------------------------------------------------------------------------

    CmpMSHR() {
      _count = 32;
      _blockSize = 64;
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

      // Add the list of parameters to the component here
      CMP_PARAMETER_UINT("count", _count)
      CMP_PARAMETER_UINT("block-size", _blockSize)

      CMP_PARAMETER_END
    }

    // -------------------------------------------------------------------------
    // Function called when simulation starts
    // -------------------------------------------------------------------------

    void StartSimulation() {
      // nothing much to do
      _missed.clear();
      _outstanding.clear();
      _handler.clear();
    }


    // -------------------------------------------------------------------------
    // Function called at a heart beat. Argument indicates cycles elapsed after
    // previous heartbeat
    // -------------------------------------------------------------------------

    void HeartBeat(cycles_t hbCount) {
    }


  // override earliest request
  MemoryRequest *EarliestRequest() {

    if (_queue.empty()) return NULL;
    
    list <MemoryRequest *> stalling;
    MemoryRequest *earliest, *request;
    earliest = NULL;

    request = _queue.top();

    while (request -> stalling) {
      _queue.pop();
      stalling.push_back(request);
      if (_queue.empty()) break;
      request = _queue.top();
    }

    if (!request -> stalling) {
      earliest = request;
    }

    for (list <MemoryRequest *>::iterator req = stalling.begin(); req != stalling.end();
         req ++) {
      request = *req;
      _queue.push(request);
    }

    return earliest;
  }


  void PrintDebugInfo() {
    printf("%s\n", _name.c_str());
    printf("Queue size = %u\n", _queue.size());
    list <MemoryRequest *> reqs;
    MemoryRequest *request;

    if (_queue.empty()) return;
    
    do {
      request = _queue.top();
      printf("%llu %X %X %d\n", request -> currentCycle, request, request -> virtualAddress, request -> stalling);
      _queue.pop();
      reqs.push_back(request);
    } while (!_queue.empty());

    for (list <MemoryRequest *>::iterator req = reqs.begin(); req != reqs.end();
         req ++) {
      request = *req;
      _queue.push(request);
    }
  }

  protected:

    // -------------------------------------------------------------------------
    // Function to process a request. Return value indicates number of busy
    // cycles for the component.
    // -------------------------------------------------------------------------

    cycles_t ProcessRequest(MemoryRequest *request) {

      // writebacks bypass the MSHR
      if (request -> type == MemoryRequest::WRITEBACK)
        return 0;

      // get the block address of the request
      addr_t blockAddr = ((request -> physicalAddress)/_blockSize)*_blockSize;


      // if there is already a miss for the block, then insert it at the end of
      // that block's request list
      if (_missed.find(blockAddr) != _missed.end()) {
        // write requests don't stall the processor
        if (request -> type == MemoryRequest::WRITE) {
          request -> serviced = true;
          return 0;
        }

        if (request -> type == MemoryRequest::READ)
          _outstanding[blockAddr] -> type = MemoryRequest::READ;

        request -> stalling = true;
        _missed[blockAddr].push_back(request);
        return 0;
      }

      // if there are no free MSHRs, stall the request
      if (_count != 0) {
        if (_missed.size() == _count) {
          request -> stalling = true;
          _waitQ.push_back(request);
          return 0;
        }
      }

      // assign a new MSHR to the request      
      _missed[blockAddr] = list <MemoryRequest *> ();

      MemoryRequest *miss = new MemoryRequest(MemoryRequest::COMPONENT,
          request -> cpuID, this, MemoryRequest::READ, request -> cmpID,
          request -> virtualAddress, blockAddr, _blockSize, 
          request -> currentCycle);

      miss -> type = request -> type;
      if (request -> type == MemoryRequest::WRITE)
        miss -> type = MemoryRequest::READ_FOR_WRITE;
      
      // set icount
      miss -> icount = request -> icount;
      
      _outstanding[blockAddr] = miss;


      if (request -> type == MemoryRequest::WRITE) {
        request -> serviced = true;
      }
      else {
        request -> stalling = true;
        _missed[blockAddr].push_back(request);
      }

      SendToNextComponent(miss);
      return 0;
    }


    // -------------------------------------------------------------------------
    // Function to process the return of a request. Return value indicates
    // number of busy cycles for the component.
    // -------------------------------------------------------------------------
    
    cycles_t ProcessReturn(MemoryRequest *request) {

      // if the request is not generated by this component,
      // just return
      if (request -> iniType != MemoryRequest::COMPONENT ||
          request -> iniPtr != this)
        return 0;

      // else mark all the requests waiting for this miss as serviced
      addr_t blockAddr = request -> physicalAddress;

      assert(_missed.find(blockAddr) != _missed.end());
      list <MemoryRequest *>::iterator it;
      for (it = _missed[blockAddr].begin(); it != _missed[blockAddr].end();
          it ++) {
        (*it) -> stalling = false;
        (*it) -> serviced = true;
        (*it) -> currentCycle = request -> currentCycle;
        if (request -> dirtyReply) 
          (*it) -> dirtyReply = true;
        AddRequest((*it));
      }

      // remove the entry for the miss
      _missed.erase(blockAddr);
      _outstanding.erase(blockAddr);
      if (!_waitQ.empty()) {
        MemoryRequest *front = _waitQ.front();
        _waitQ.pop_front();
        front -> stalling = false;
        AddRequest(front);
      }

      // destroy the request
      request -> destroy = true;
     
      return 0; 
    }


};

#endif // __CMP_MSHR_H__
