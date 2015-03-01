// -----------------------------------------------------------------------------
// File: CmpLLCVTS.h
// Description:
//    Implements a last-level cache
// -----------------------------------------------------------------------------

#ifndef __CMP_LLCVTS_H__
#define __CMP_LLCVTS_H__

#define DUEL_PRIME 443

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"
#include "GenericTagStore.h"
#include "VictimTagStore.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Class: CmpLLCVTS
// Description:
//    Baseline lastlevel cache.
// -----------------------------------------------------------------------------

class CmpLLCVTS : public MemoryComponent {

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

  bool _useDueling;
  uint32 _numDuelingSets;
  uint32 _maxPSEL;

  bool _ideal;
  bool _noClear;
  bool _decoupleClear;
  bool _segmented;

  bool _useBloomFilter;
  uint32 _alpha;

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

  struct SetInfo {
    bool leader;
    bool vts;
    SetInfo() { leader = false; }
  };

  saturating_counter _psel;
    
  // leader sets
  vector <SetInfo> _sets;
    
  // counters to keep track of occupancy
  vector <uint32> _occupancy;

  // per processor hit/miss counters
  vector <uint32> _hits;
  vector <uint32> _misses;

  victim_tag_store_t _vts;

  // -------------------------------------------------------------------------
  // Declare Counters
  // -------------------------------------------------------------------------

  NEW_COUNTER(accesses);
  NEW_COUNTER(reads);
  NEW_COUNTER(writebacks);
  NEW_COUNTER(misses);
  NEW_COUNTER(vts_hits);
  NEW_COUNTER(evictions);
  NEW_COUNTER(dirty_evictions);


public:

  // -------------------------------------------------------------------------
  // Constructor. It cannot take any arguments
  // -------------------------------------------------------------------------

  CmpLLCVTS():_psel(1024, 512) {
    _size = 1024;
    _blockSize = 64;
    _associativity = 16;
    _tagStoreLatency = 6;
    _dataStoreLatency = 15;
    _policy = "drrip";
    _ideal = false;
    _noClear = false;
    _decoupleClear = false;
    _segmented = false;
    _numDuelingSets = 32;
    _useDueling = false;
    _maxPSEL = 1024;
    _useBloomFilter = false;
    _alpha = 8;
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
      CMP_PARAMETER_UINT("num-dueling-sets", _numDuelingSets)
      CMP_PARAMETER_UINT("max-psel-value", _maxPSEL)
      CMP_PARAMETER_BOOLEAN("use-dueling", _useDueling)
      CMP_PARAMETER_BOOLEAN("ideal", _ideal)
      CMP_PARAMETER_BOOLEAN("no-clear", _noClear)
      CMP_PARAMETER_BOOLEAN("decouple-clear", _decoupleClear)
      CMP_PARAMETER_BOOLEAN("segmented", _segmented)
      CMP_PARAMETER_BOOLEAN("use-bloom", _useBloomFilter)
      CMP_PARAMETER_UINT("alpha", _alpha)

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
      INITIALIZE_COUNTER(vts_hits, "VTS hits")
      }


  // -------------------------------------------------------------------------
  // Function called when simulation starts
  // -------------------------------------------------------------------------

  void StartSimulation() {

    // compute number of sets
    _numSets = (_size * 1024) / (_blockSize * _associativity);
    _tags.SetTagStoreParameters(_numSets, _associativity, _policy);
    _occupancy.resize(_numCPUs, 0);
    _vts.initialize(_numSets * _associativity, _useBloomFilter,
                    _ideal, _noClear, _decoupleClear, _segmented, _alpha);

    // create the occupancy log file
    NEW_LOG_FILE("occupancy", "occupancy");

    // set dueling
    if (_useDueling) {
      _sets.resize(_numSets);
      cyclic_pointer current(_numSets, 0);
      for (int i = 0; i < _numDuelingSets; i ++) {
        _sets[current].leader = true;
        _sets[current].vts = true;
        current.add(DUEL_PRIME);
        _sets[current].leader = true;
        _sets[current].vts = false;
        current.add(DUEL_PRIME);
      }

      _psel = saturating_counter(_maxPSEL, _maxPSEL / 2);
    }
      
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


  void EndSimulation() {
    CMP_LOG("false_positives = %lf", _vts.false_positive_rate());
    DUMP_STATISTICS;
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

    table_t <addr_t, TagEntry>::entry tagentry;
    cycles_t latency;

    // NO WRITES (Complete or partial)
    if (request -> type == MemoryRequest::WRITE || 
        request -> type == MemoryRequest::PARTIALWRITE) {
      fprintf(stderr, "LLCVTS annot handle direct writes (yet)\n");
      exit(0);
    }

    // compute the cache block tag
    addr_t ctag = PADDR(request) / _blockSize;

    // check if its a read or write back
    switch (request -> type) {

      // READ request
    case MemoryRequest::READ: case MemoryRequest::READ_FOR_WRITE: case MemoryRequest::PREFETCH:

      INCREMENT(reads);
          
      tagentry = _tags.read(ctag);
      if (tagentry.valid) {
        request -> serviced = true;
        request -> AddLatency(_tagStoreLatency + _dataStoreLatency);

        // update per processor counters
        _hits[request -> cpuID] ++;
      }
      else {
        uint32 index = _tags.index(ctag);
        if (_useDueling) {
          if (_sets[index].leader) {
            if (_sets[index].vts)
              _psel.increment();
            else
              _psel.decrement();
          }
        }
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

    policy_value_t priority = POLICY_BIMODAL;
    uint32 index = _tags.index(ctag);
    policy_value_t vts_priority = POLICY_BIMODAL;

    // VTS MAGIC
    if (_vts.test(ctag)) {
      vts_priority = POLICY_HIGH;
      INCREMENT(vts_hits);
    }

    if (_useDueling) {
      priority = _sets[index].leader ?
        (_sets[index].vts ? vts_priority : POLICY_HIGH)
        : ((_psel > (_maxPSEL / 2)) ? POLICY_HIGH : vts_priority);
    }
    else {
      priority = vts_priority;
    }
      
    // insert the block into the cache
    tagentry = _tags.insert(ctag, TagEntry(), priority);
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

      // VTS INSERT
      _vts.insert(tagentry.key);
        
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
};

#endif // __CMP_LLCVTS_H__
