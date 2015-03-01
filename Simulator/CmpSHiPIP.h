// -----------------------------------------------------------------------------
// File: CmpSHIPIP.h
// Description:
//    Implements a last-level cache
// -----------------------------------------------------------------------------

#ifndef __CMP_SHIPIP_H__
#define __CMP_SHIPIP_H__

#define SET_DUELING_PRIME 443

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
// Class: CmpSHIPIP
// Description:
//    Baseline lastlevel cache.
// -----------------------------------------------------------------------------

class CmpSHIPIP : public MemoryComponent {

protected:
  
  // -------------------------------------------------------------------------
  // Parameters
  // -------------------------------------------------------------------------
  
  uint32 _size;
  uint32 _blockSize;
  uint32 _associativity;
  string _policy;
  uint32 _shctMax;
  bool _useBimodal;
  bool _noIncrement;
  
  uint32 _tagStoreLatency;
  uint32 _dataStoreLatency;

  bool _useDueling;
  uint32 _numDuelingSets;
  uint32 _pselMax;

  // -------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------
  
  struct TagEntry {
    bool dirty;
    addr_t vcla;
    addr_t pcla;
    addr_t ip;
    uint32 appID;
    bool reused;
    TagEntry() { dirty = false; reused = false; }
  };

  // tag store
  uint32 _numSets;
  generic_tagstore_t <addr_t, TagEntry> _tags;

  // map from instruction pointer to saturating counter
  map <addr_t, saturating_counter> _ipTable;

  // counters to keep track of occupancy
  vector <uint32> _occupancy;

  struct SetInfo {
    bool leader;
    bool ship;
    uint32 appID;
    SetInfo() {
      leader = false;
    }
  };

  vector <SetInfo> _sets;
  vector <saturating_counter> _psel;


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

  CmpSHIPIP() {
    _size = 1024;
    _blockSize = 64;
    _associativity = 16;
    _tagStoreLatency = 6;
    _dataStoreLatency = 15;
    _policy = "drrip";
    _shctMax = 3;
    _useBimodal = false;
    _numDuelingSets = 32;
    _noIncrement = false;
    _pselMax = 1024;
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
      CMP_PARAMETER_UINT("shct-max", _shctMax)
      CMP_PARAMETER_BOOLEAN("use-bimodal", _useBimodal)
      CMP_PARAMETER_BOOLEAN("use-dueling", _useDueling)
      CMP_PARAMETER_BOOLEAN("no-increment", _noIncrement)

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

    _sets.resize(_numSets);
    _psel.resize(_numCPUs, saturating_counter(_pselMax, _pselMax/2));
      
    cyclic_pointer current(_numSets, 0);

    // create the dueling sets for all the apps
    for (uint32 id = 0; id < _numCPUs; id ++) {
      for (uint32 sid = 0; sid < _numDuelingSets; sid ++) {
        _sets[current].leader = true;
        _sets[current].appID = id;
        _sets[current].ship = true;
        current.add(SET_DUELING_PRIME);
        _sets[current].leader = true;
        _sets[current].appID = id;
        _sets[current].ship = false;
        current.add(SET_DUELING_PRIME);
      }
    }
      
    // create the occupancy log file
    NEW_LOG_FILE("occupancy", "occupancy");
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
      fprintf(stderr, "SHIPIP annot handle direct writes (yet)\n");
      exit(0);
    }

    // compute the cache block tag
    addr_t ctag = PADDR(request) / _blockSize;

    // check if the instruction pointer is in the ip table
    if (_ipTable.find(request -> ip) == _ipTable.end()) {
      _ipTable.insert(make_pair(request -> ip, saturating_counter(_shctMax, 0)));
    }

    // check if its a read or write back
    switch (request -> type) {

      // READ request
    case MemoryRequest::READ: case MemoryRequest::READ_FOR_WRITE: case MemoryRequest::PREFETCH:

      INCREMENT(reads);
          
      tagentry = _tags.read(ctag);
      if (tagentry.valid) {
        _tags[ctag].reused = true;
        if (_noIncrement)
          _ipTable[request -> ip].set(_shctMax);
        else
          _ipTable[request -> ip].increment();
        request -> serviced = true;
        request -> AddLatency(_tagStoreLatency + _dataStoreLatency);
      }
      else {
        INCREMENT(misses);
        request -> AddLatency(_tagStoreLatency);
            
        if (_useDueling) {
          uint32 index = _tags.index(ctag);
          if (_sets[index].leader && _sets[index].appID == request -> cpuID) {
            if (_sets[index].ship)
              _psel[request -> cpuID].decrement();
            else
              _psel[request -> cpuID].increment();
          }
        }
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

    policy_value_t priority = POLICY_HIGH;

    // check the ip table to find out priority
    assert(_ipTable.find(request -> ip) != _ipTable.end());
    if (_ipTable[request -> ip] == 0) {
      priority = _useBimodal ? POLICY_BIMODAL : POLICY_LOW;
    }

    uint32 index = _tags.index(ctag);
    if (_useDueling) {
      if (_sets[index].leader && _sets[index].appID == request -> cpuID) {
        if (!_sets[index].ship)
          priority = POLICY_BIMODAL;
      }
      else {
        if (_psel[request -> cpuID] <= _pselMax/2)
          priority = POLICY_BIMODAL;
      }
    }
        
    // insert the block into the cache
    tagentry = _tags.insert(ctag, TagEntry(), priority);
    _tags[ctag].vcla = BLOCK_ADDRESS(VADDR(request), _blockSize);
    _tags[ctag].pcla = BLOCK_ADDRESS(PADDR(request), _blockSize);
    _tags[ctag].ip = request -> ip;
    _tags[ctag].dirty = dirty;
    _tags[ctag].appID = request -> cpuID;
    _tags[ctag].reused = false;

    // increment that apps occupancy
    _occupancy[request -> cpuID] ++;

    // if the evicted tag entry is valid
    if (tagentry.valid) {
      _occupancy[tagentry.value.appID] --;
      INCREMENT(evictions);

      assert(_ipTable.find(tagentry.value.ip) != _ipTable.end());
      if (!tagentry.value.reused) 
        _ipTable[tagentry.value.ip].decrement();        

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

#endif // __CMP_SHIPIP_H__
