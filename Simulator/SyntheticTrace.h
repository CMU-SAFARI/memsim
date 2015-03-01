#ifndef __SYNTHETIC_TRACE_H__
#define __SYNTHETIC_TRACE_H__

#include "Types.h"
#include "MemoryRequest.h"

class SyntheticTrace {

protected:

  uint32 _blockSize;
  uint32 _workingSetSize;
  uint32 _memInstGap;
  uint32 _cpuID;


  uint64 _icount;
  addr_t _vaddr;
  addr_t _paddr;

  cyclic_pointer _index;

  
  uint64 Normalize(uint64 val) {
    return (val + ((addr_t)(_cpuID) << 48));
  }


public:

  SyntheticTrace(uint32 workingSetSize = 128,
                 uint32 memInstGap = 50, uint32 cpuID = 0, uint32 blockSize = 64)
    : _index(workingSetSize * 1024 / blockSize) {
    _blockSize = blockSize;
    _workingSetSize = workingSetSize;
    _memInstGap = memInstGap;
    _cpuID = cpuID;

    _icount = 1;
    _vaddr = 0xdead0000;
    _paddr = 0xbeef0000;
  }


  MemoryRequest *NextRequest() {
    MemoryRequest *request = new MemoryRequest;

    request -> type = MemoryRequest::READ;
    request -> iniType = MemoryRequest::CPU;
    request -> cpuID = _cpuID;

    request -> virtualAddress = Normalize(_vaddr + _index * _blockSize);
    request -> physicalAddress = Normalize(_paddr + _index * _blockSize);
    request -> ip = Normalize(0xdeadbeef);
    request -> icount = _icount;
    request -> size = 8;
    request -> iniPtr = NULL;

    _icount += _memInstGap;
    _index.increment();

    return request;
  }
  
};

#endif // __SYNTHETIC_TRACE_H__
