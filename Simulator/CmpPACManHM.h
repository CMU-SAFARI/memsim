// -----------------------------------------------------------------------------
// File: CmpPACManHM.h
// Description:
//    Implements a last-level cache with PACMan mechanism
// -----------------------------------------------------------------------------

#ifndef __CMP_PACMAN_H__
#define __CMP_PACMAN_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"
#include "GenericTagStore.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#define PACMAN_DUEL_PRIME 443

// -----------------------------------------------------------------------------
// Class: CmpPACMan
// Description:
//    Baseline lastlevel cache.
// -----------------------------------------------------------------------------

class CmpPACMan : public MemoryComponent {

protected:

  // -------------------------------------------------------------------------
  // Parameters
  // -------------------------------------------------------------------------

  uint32 _size;
  uint32 _blockSize;
  uint32 _associativity;
  string _policy;
  uint32 _policyVal;

  uint32 _tagStoreLatency;
  uint32 _dataStoreLatency;

  bool _pacmanH;
  bool _pacmanM;

  // -------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------
 
  enum PrefetchState {
    NOT_PREFETCHED,
    PREFETCHED_UNUSED,
    PREFETCHED_USED,
    PREFETCHED_REUSED,
  };
  
  struct TagEntry {
    bool dirty;
    addr_t vcla;
    addr_t pcla;
    uint32 appID;

    // prefetch related info
    PrefetchState prefState;
    bool lowPriority;

    // miss counter information
    uint64 prefetchMiss;
    uint64 useMiss;
    
    // cycle information
    cycles_t prefetchCycle;
    cycles_t useCycle;
    
    TagEntry() {
      dirty = false;
      lowPriority = false;
      prefState = NOT_PREFETCHED;
    }
   };

  // tag store
  uint32 _numSets;
  generic_tagstore_t <addr_t, TagEntry> _tags;
  policy_value_t _pval;

  // prefetch pollution predictor
  struct SetEntry {
    bool leader;
    bool pacman;
    SetEntry() { leader = false; }
  };
  vector <SetEntry> _duelInfo;
  saturating_counter _psel;
  uint32 _pselThreshold;

  vector <uint64> _missCounter;
  vector <uint64> _procMisses;


  // -------------------------------------------------------------------------
  // Declare Counters
  // -------------------------------------------------------------------------

  NEW_COUNTER(accesses);
  NEW_COUNTER(reads);
  NEW_COUNTER(writebacks);
  NEW_COUNTER(misses);
  NEW_COUNTER(evictions);
  NEW_COUNTER(dirty_evictions);

  NEW_COUNTER(prefetches);
  NEW_COUNTER(prefetch_misses);
  
  NEW_COUNTER(unused_prefetches);
  NEW_COUNTER(used_prefetches);
  NEW_COUNTER(unreused_prefetches);
  NEW_COUNTER(reused_prefetches);

  NEW_COUNTER(prefetch_use_cycle);
  NEW_COUNTER(prefetch_use_miss);

  NEW_COUNTER(prefetch_lifetime_cycle);
  NEW_COUNTER(prefetch_lifetime_miss);

public:

  // -------------------------------------------------------------------------
  // Constructor. It cannot take any arguments
  // -------------------------------------------------------------------------

  CmpPACMan() {
    _size = 1024;
    _blockSize = 64;
    _associativity = 16;
    _tagStoreLatency = 6;
    _dataStoreLatency = 15;
    _policy = "lru";
    _policyVal = 0;
    _pselThreshold = 1024;
    _pacmanH = true;
    _pacmanM = true;
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
      CMP_PARAMETER_UINT("policy-value", _policyVal)
      CMP_PARAMETER_UINT("tag-store-latency", _tagStoreLatency)
      CMP_PARAMETER_UINT("data-store-latency", _dataStoreLatency)
      CMP_PARAMETER_BOOLEAN("pacman-h", _pacmanH)
      CMP_PARAMETER_BOOLEAN("pacman-m", _pacmanM)
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

    INITIALIZE_COUNTER(prefetches, "Total prefetches")
    INITIALIZE_COUNTER(prefetch_misses, "Prefetch misses")

    INITIALIZE_COUNTER(unused_prefetches, "Unused prefetches")
    INITIALIZE_COUNTER(used_prefetches, "Used prefetches")
    INITIALIZE_COUNTER(unreused_prefetches, "Unreused prefetches")
    INITIALIZE_COUNTER(reused_prefetches, "Reused prefetches")

    INITIALIZE_COUNTER(prefetch_use_cycle, "Prefetch-to-use Cycles")
    INITIALIZE_COUNTER(prefetch_use_miss, "Prefetch-to-use Misses")

    INITIALIZE_COUNTER(prefetch_lifetime_cycle, "Prefetch-lifetime Cycles")
    INITIALIZE_COUNTER(prefetch_lifetime_miss, "Prefetch-lifetime Misses")

  }


  // -------------------------------------------------------------------------
  // Function called when simulation starts
  // -------------------------------------------------------------------------

  void StartSimulation() {

    // compute number of sets
    _numSets = (_size * 1024) / (_blockSize * _associativity);
    _tags.SetTagStoreParameters(_numSets, _associativity, _policy);
    _missCounter.resize(_numSets, 0);
    _procMisses.resize(_numCPUs, 0);

    switch (_policyVal) {
    case 0: _pval = POLICY_HIGH; break;
    case 1: _pval = POLICY_BIMODAL; break;
    case 2: _pval = POLICY_LOW; break;
    }

    // initialize the reuse predictor
    if (_pacmanM) {
      _psel = saturating_counter(_pselThreshold, _pselThreshold / 2);
      // dueling sets
      _duelInfo.resize(_numSets);
      cyclic_pointer current(_numSets, 0);
      for (int i = 0; i < 32; i ++) {
        _duelInfo[current].leader = true;
        _duelInfo[current].pacman = true;
        current.add(PACMAN_DUEL_PRIME);
        _duelInfo[current].leader = true;
        _duelInfo[current].pacman = false;
        current.add(PACMAN_DUEL_PRIME);
      }
    }
  }


  // -------------------------------------------------------------------------
  // Function called at a heart beat. Argument indicates cycles elapsed after
  // previous heartbeat
  // -------------------------------------------------------------------------

  void HeartBeat(cycles_t hbCount) {
  }

  void EndProcWarmUp(uint32 cpuID) {
    _procMisses[cpuID] = 0;
  }

  void EndSimulation() {
    DUMP_STATISTICS;
    for (uint32 i = 0; i < _numCPUs; i ++)
      CMP_LOG("misses-%u = %llu", i, _procMisses[i]);
    CLOSE_ALL_LOGS;
  }


protected:

  // -------------------------------------------------------------------------
  // Function to process a request. Return value indicates number of busy
  // cycles for the component.
  // -------------------------------------------------------------------------

  cycles_t ProcessRequest(MemoryRequest *request) {

    // update stats
    INCREMENT(accesses);

    cycles_t latency;

    // NO WRITES (Complete or partial)
    if (request -> type == MemoryRequest::WRITE || 
        request -> type == MemoryRequest::PARTIALWRITE) {
      fprintf(stderr, "LLC cannot handle direct writes (yet)\n");
      exit(0);
    }

    // compute the cache block tag
    addr_t ctag = VADDR(request) / _blockSize;
    uint32 index = _tags.index(ctag);

    // check if its a read or write back
    switch (request -> type) {

    // READ request
    case MemoryRequest::READ:
    case MemoryRequest::READ_FOR_WRITE:

      INCREMENT(reads);
          
      if (_tags.lookup(ctag)) {

        request -> serviced = true;
        request -> AddLatency(_tagStoreLatency + _dataStoreLatency);

        // read to update replacement policy
        _tags.read(ctag);
        
        // read to update state
        TagEntry &tagentry = _tags[ctag];

        tagentry.lowPriority = false;
        
        // check the prefetched state
        switch (tagentry.prefState) {
        case PREFETCHED_UNUSED:
          // update state
          tagentry.prefState = PREFETCHED_USED;
          tagentry.useMiss = _missCounter[index];
          tagentry.useCycle = request -> currentCycle;

          // update counters
          INCREMENT(used_prefetches);
          ADD_TO_COUNTER(prefetch_use_cycle,
                         tagentry.useCycle - tagentry.prefetchCycle);
          ADD_TO_COUNTER(prefetch_use_miss,
                         tagentry.useMiss - tagentry.prefetchMiss);
          break;
          
        case PREFETCHED_USED:
          tagentry.prefState = PREFETCHED_REUSED;
          INCREMENT(reused_prefetches);
          break;
          
        case NOT_PREFETCHED:
        case PREFETCHED_REUSED:
          // do nothing
          break;
        }
        
      }
      else {
        if (_pacmanM) {
          SetEntry sentry = _duelInfo[_tags.index(ctag)];
          if (sentry.leader) {
            if (sentry.pacman)
              _psel.decrement();
            else
              _psel.increment();
          }
        }
        
        INCREMENT(misses);
        request -> AddLatency(_tagStoreLatency);
        //_missCounter[index] ++;
        if (!_done.test(request -> cpuID)) _procMisses[request -> cpuID] ++;
      }
          
      return _tagStoreLatency;

    case MemoryRequest::PREFETCH:

      INCREMENT(prefetches);
      
      if (_tags.lookup(ctag)) {
        request -> serviced = true;
        request -> AddLatency(_tagStoreLatency + _dataStoreLatency);
        
        _tags[ctag].lowPriority = false;

        // read to update replacement policy
        if (!_pacmanH)
          _tags.read(ctag);
      }
      else {
        INCREMENT(prefetch_misses);
        request -> AddLatency(_tagStoreLatency);
        //_missCounter[index] ++;
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
    addr_t ctag = VADDR(request) / _blockSize;

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

    policy_value_t priority = _pval;

    if (_pacmanM && request -> type == MemoryRequest::PREFETCH) {
      SetEntry sentry = _duelInfo[_tags.index(ctag)];
      if ((sentry.leader && sentry.pacman) || (_psel > _pselThreshold / 2)) {
        priority = POLICY_LOW;
      }
      else {
        priority = POLICY_HIGH;
      }
    }
    
    // insert the block into the cache
    tagentry = _tags.insert(ctag, TagEntry(), priority);
    _tags[ctag].vcla = BLOCK_ADDRESS(VADDR(request), _blockSize);
    _tags[ctag].pcla = BLOCK_ADDRESS(PADDR(request), _blockSize);
    _tags[ctag].dirty = dirty;
    _tags[ctag].appID = request -> cpuID;
    _tags[ctag].prefState = NOT_PREFETCHED;

    if (priority == POLICY_LOW) {
      _tags[ctag].lowPriority = true;
    }

    uint32 index = _tags.index(ctag);

    // Handle prefetch
    if (request -> type == MemoryRequest::PREFETCH) {
      _tags[ctag].prefState = PREFETCHED_UNUSED;
      _tags[ctag].prefetchCycle = request -> currentCycle;
      _tags[ctag].prefetchMiss = _missCounter[index];
    }

    // if the evicted tag entry is valid
    if (tagentry.valid) {
      INCREMENT(evictions);

      // check prefetched state
      switch (tagentry.value.prefState) {
      case PREFETCHED_UNUSED:
        INCREMENT(unused_prefetches);
        ADD_TO_COUNTER(prefetch_lifetime_cycle,
                       request -> currentCycle - tagentry.value.prefetchCycle);
        ADD_TO_COUNTER(prefetch_lifetime_miss,
                       _missCounter[index] - tagentry.value.prefetchMiss);
        break;
        
      case PREFETCHED_USED:
        INCREMENT(unreused_prefetches);
        ADD_TO_COUNTER(prefetch_lifetime_cycle,
                       request -> currentCycle - tagentry.value.prefetchCycle);
        ADD_TO_COUNTER(prefetch_lifetime_miss,
                       _missCounter[index] - tagentry.value.prefetchMiss);
        break;
        
      case PREFETCHED_REUSED:
        ADD_TO_COUNTER(prefetch_lifetime_cycle,
                       tagentry.value.useCycle - tagentry.value.prefetchCycle);
        ADD_TO_COUNTER(prefetch_lifetime_miss,
                       tagentry.value.useMiss - tagentry.value.prefetchMiss);
        break;
        
      case NOT_PREFETCHED:
        // do nothing
        break;
      }

      if (!tagentry.value.lowPriority)
        _missCounter[index] ++;

      if (tagentry.value.dirty) {
        INCREMENT(dirty_evictions);
        MemoryRequest *writeback =
          new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this,
                            MemoryRequest::WRITEBACK, request -> cmpID, 
                            tagentry.value.vcla, tagentry.value.pcla, _blockSize,
                            request -> currentCycle);
        writeback -> icount = request -> icount;
        writeback -> ip = request -> ip;
        SendToNextComponent(writeback);
      }
    }
  }
};

#endif // __CMP_PACMAN_H__
