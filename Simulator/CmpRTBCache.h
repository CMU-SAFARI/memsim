// -----------------------------------------------------------------------------
// File: CmpRTBCache.h
// Description:
//    Implements a last-level cache
// -----------------------------------------------------------------------------

#ifndef __CMP_RTB_CACHE_H__
#define __CMP_RTB_CACHE_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"
#include "GenericTagStore.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Class: CmpRTBCache
// Description:
//    Baseline lastlevel cache.
// -----------------------------------------------------------------------------

class CmpRTBCache : public MemoryComponent {

protected:

  // -------------------------------------------------------------------------
  // Parameters
  // -------------------------------------------------------------------------

  uint32 _size;
  uint32 _blockSize;
  uint32 _associativity;
  string _policy;

  uint32 _tagStoreLatency;
  uint32 _dataStoreLatency;

  uint32 _MATSize;
  uint32 _MATmax;

  // -------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------

  struct TagEntry {
    bool dirty;
    addr_t vcla;
    addr_t pcla;
    uint32 appID;
    TagEntry() { dirty = false; }
  };

  // tag store
  uint32 _numSets;
  generic_tagstore_t <addr_t, TagEntry> _tags;

  // MAT
  map <addr_t, saturating_counter> _pMAT;
  generic_table_t <addr_t, saturating_counter> _MAT;
    
  // counters to keep track of occupancy
  vector <uint32> _occupancy;

  // per processor hit/miss counters
  vector <uint32> _hits;
  vector <uint32> _misses;
    
    

  // -------------------------------------------------------------------------
  // Declare Counters
  // -------------------------------------------------------------------------

  NEW_COUNTER(accesses);
  NEW_COUNTER(reads);
  NEW_COUNTER(writebacks);
  NEW_COUNTER(misses);
  NEW_COUNTER(evictions);
  NEW_COUNTER(dirty_evictions);


public:

  // -------------------------------------------------------------------------
  // Constructor. It cannot take any arguments
  // -------------------------------------------------------------------------

  CmpRTBCache() {
    _size = 1024;
    _blockSize = 64;
    _associativity = 16;
    _tagStoreLatency = 6;
    _dataStoreLatency = 15;
    _policy = "lru";
    _MATSize = 0;
    _MATmax = 256;
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
      CMP_PARAMETER_UINT("size", _size)
      CMP_PARAMETER_UINT("block-size", _blockSize)
      CMP_PARAMETER_UINT("associativity", _associativity)
      CMP_PARAMETER_STRING("policy", _policy)
      CMP_PARAMETER_UINT("tag-store-latency", _tagStoreLatency)
      CMP_PARAMETER_UINT("data-store-latency", _dataStoreLatency)
      CMP_PARAMETER_UINT("mat-size", _MATSize)
      CMP_PARAMETER_UINT("mat-max", _MATmax)

      CMP_PARAMETER_END
      }


  // -------------------------------------------------------------------------
  // Function to initialize statistics
  // -------------------------------------------------------------------------

  void InitializeStatistics() {

    INITIALIZE_COUNTER(accesses, "Total Accesses")
      INITIALIZE_COUNTER(reads, "Read Accesses")
      INITIALIZE_COUNTER(writebacks, "Writeback Accesses")
      INITIALIZE_COUNTER(misses, "Total Misses")
      INITIALIZE_COUNTER(evictions, "Evictions")
      INITIALIZE_COUNTER(dirty_evictions, "Dirty Evictions")
      }


  // -------------------------------------------------------------------------
  // Function called when simulation starts
  // -------------------------------------------------------------------------

  void StartSimulation() {

    // compute number of sets
    _numSets = (_size * 1024) / (_blockSize * _associativity);
    _tags.SetTagStoreParameters(_numSets, _associativity, _policy);
    _occupancy.resize(_numCPUs, 0);

    if (_MATSize != 0)
      _MAT.SetTableParameters(_MATSize, "lru");
    _pMAT.clear();

    // create the occupancy log file
    NEW_LOG_FILE("occupancy", "occupancy");
      
    _hits.resize(_numCPUs, 0);
    _misses.resize(_numCPUs, 0);
  }


  // -------------------------------------------------------------------------
  // Function called at a heart beat. Argument indicates cycles elapsed after
  // previous heartbeat
  // -------------------------------------------------------------------------

  void HeartBeat(cycles_t hbCount) {

    // if there are more than one apps, then print occupancy
    if (_numCPUs > 1) {
      LOG("occupancy", "%llu ", _currentCycle);
      for (uint32 i = 0; i < _numCPUs; i ++)
        LOG("occupancy", "%u ", _occupancy[i]);
      LOG("occupancy", "\n");
    }
  }


protected:

  // -------------------------------------------------------------------------
  // Function to process a request. Return value indicates number of busy
  // cycles for the component.
  // -------------------------------------------------------------------------

  cycles_t ProcessRequest(MemoryRequest *request) {

    // update stats
    INCREMENT(accesses);

    table_t <addr_t, TagEntry>::entry tagentry;
    cycles_t latency;

    // NO WRITES (Complete or partial)
    if (request -> type == MemoryRequest::WRITE || 
        request -> type == MemoryRequest::PARTIALWRITE) {
      fprintf(stderr, "LLC annot handle direct writes (yet)\n");
      exit(0);
    }

    // compute the cache block tag
    addr_t ctag = PADDR(request) / _blockSize;
    addr_t mtag = ctag >> 4;

    // check if its a read or write back
    switch (request -> type) {

      // READ request
    case MemoryRequest::READ: case MemoryRequest::READ_FOR_WRITE: case MemoryRequest::PREFETCH:

      INCREMENT(reads);

      if (_MATSize) {
        if (_MAT.lookup(mtag))
          _MAT[mtag].increment();
        else
          _MAT.insert(mtag, saturating_counter(_MATmax, 0));
      }
      else if(_pMAT.find(mtag) == _pMAT.end())
        _pMAT.insert(make_pair(mtag, saturating_counter(_MATmax, 0)));
      else
        _pMAT[mtag].increment();
          
      tagentry = _tags.read(ctag);
      if (tagentry.valid) {
        request -> serviced = true;
        request -> AddLatency(_tagStoreLatency + _dataStoreLatency);

        // update per processor counters
        _hits[request -> cpuID] ++;
      }
      else {
        INCREMENT(misses);
        request -> AddLatency(_tagStoreLatency);
        _misses[request -> cpuID] ++;
      }
          
      return _tagStoreLatency;


      // WRITEBACK request
    case MemoryRequest::WRITEBACK:

      INCREMENT(writebacks);

      if (_tags.lookup(ctag))
        _tags[ctag].dirty = true;
      else
        INSERT_BLOCK(ctag, true, request);

      request -> serviced = true;
      return _tagStoreLatency;
    }
  }


  // -------------------------------------------------------------------------
  // Function to process the return of a request. Return value indicates
  // number of busy cycles for the component.
  // -------------------------------------------------------------------------
    
  cycles_t ProcessReturn(MemoryRequest *request) { 

    // if its a writeback from this component, delete it
    if (request -> iniType == MemoryRequest::COMPONENT &&
        request -> iniPtr == this) {
      request -> destroy = true;
      return 0;
    }

    // get the cache block tag
    addr_t ctag = PADDR(request) / _blockSize;

    // if the block is already present, return
    if (_tags.lookup(ctag))
      return 0;

    INSERT_BLOCK(ctag, false, request);

    return 0;
  }


  // -------------------------------------------------------------------------
  // Function to insert a block into the cache
  // -------------------------------------------------------------------------

  void INSERT_BLOCK(addr_t ctag, bool dirty, MemoryRequest *request) {

    table_t <addr_t, TagEntry>::entry tagentry;

    uint32 index = _tags.index(ctag);
    bool freeSpace = false;
    uint32 mval, cand_mval;
    addr_t candidate;
    if (_tags.count(index) == _associativity) {
      candidate = _tags.to_be_evicted(_tags.index(ctag));
      addr_t mtag = ctag >> 4;
      addr_t cand_mtag = candidate >> 4;
      if (_MATSize) {
        mval = _MAT[mtag];
        if (_MAT.lookup(cand_mtag)) {
          _MAT[cand_mtag].decrement();
          cand_mval = _MAT[cand_mtag];
        }
        else
          cand_mval = 0;
      }
      else {
        mval = _pMAT[mtag];
        if (_pMAT.find(cand_mtag) != _pMAT.end()) {
          _pMAT[cand_mtag].decrement();
          cand_mval = _pMAT[cand_mtag];
        }
        else
          cand_mval = 0;
      }
    }
    else {
      freeSpace = true;
    }
      
    if (cand_mval < mval || freeSpace) {
      if (!freeSpace)
        tagentry = _tags.invalidate(candidate);

      // insert the block into the cache
      _tags.insert(ctag, TagEntry(), POLICY_HIGH);
      _tags[ctag].vcla = BLOCK_ADDRESS(VADDR(request), _blockSize);
      _tags[ctag].pcla = BLOCK_ADDRESS(PADDR(request), _blockSize);
      _tags[ctag].dirty = dirty;
      _tags[ctag].appID = request -> cpuID;
        
      // increment that apps occupancy
      _occupancy[request -> cpuID] ++;

      // if the evicted tag entry is valid
      if (tagentry.valid) {
        _occupancy[tagentry.value.appID] --;
        INCREMENT(evictions);
          
        if (tagentry.value.dirty) {
          INCREMENT(dirty_evictions);
          MemoryRequest *writeback =
            new MemoryRequest(
                              MemoryRequest::COMPONENT, request -> cpuID, this,
                              MemoryRequest::WRITEBACK, request -> cmpID, 
                              tagentry.value.vcla, tagentry.value.pcla, _blockSize,
                              request -> currentCycle);
          writeback -> icount = request -> icount;
          writeback -> ip = request -> ip;
          SendToNextComponent(writeback);
        }
      }
    }
    else {
      if (_MATSize)
        _MAT[mval].set(cand_mval);
      else
        _pMAT[mval].set(cand_mval);
    }
  }
};

#endif // __CMP_RTB_CACHE_H__
