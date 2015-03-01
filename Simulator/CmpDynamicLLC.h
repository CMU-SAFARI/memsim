// -----------------------------------------------------------------------------
// File: CmpDynamicLLC.h
// Description:
//    Implements a LLC with Dynamic policy. 
// -----------------------------------------------------------------------------

#ifndef __CMP_DYNAMIC_LLC_H__
#define __CMP_DYNAMIC_LLC_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"
#include "SetDuelingTagStore.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Class: CmpDynamicLLC
// Description:
//    LLC with Dynamic policy
// -----------------------------------------------------------------------------

class CmpDynamicLLC : public MemoryComponent {

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

    uint32 _numDuelingSets;
    uint32 _maxPSELValue;


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
    set_dueling_tagstore_t <addr_t, TagEntry> _tags;

    // counters to keep track of occupancy
    vector <uint32> _occupancy;

    // per processor hit/miss counters
    vector <uint32> _hits;
    vector <uint32> _misses;

    // per processor victim hit/miss counters
    vector <vector <uint32> > _victimHits;
    vector <uint32> _victimMisses;
    

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

    CmpDynamicLLC() {
      _size = 1024;
      _blockSize = 64;
      _associativity = 16;
      _tagStoreLatency = 8;
      _dataStoreLatency = 20;
      _numDuelingSets = 32;
      _maxPSELValue = 1024;
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
      CMP_PARAMETER_UINT("max-psel-value", _maxPSELValue)

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
      _tags.SetTagStoreParameters(_numCPUs, _numSets, _associativity, _policy,
          _numDuelingSets, _maxPSELValue);
      _occupancy.resize(_numCPUs, 0);

      // create the occupancy log file
      NEW_LOG_FILE("occupancy", "occupancy");

      // policy log file
      NEW_LOG_FILE("policy", "policy");
      
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

      // for each cpu log the policy
      for (uint32 i = 0; i < _numCPUs; i ++) {
        LOG("policy", "%u ", _tags.policy(i));
      }
      LOG("policy", "\n");
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
    // Function to insert a block
    // -------------------------------------------------------------------------

    void INSERT_BLOCK(addr_t ctag, bool dirty, MemoryRequest *request) {

      table_t <addr_t, TagEntry>::entry tagentry;

      // if the block is dirty, don't update PSEL
      tagentry = _tags.insert(request -> cpuID, ctag, TagEntry(), ~dirty);
      _tags[ctag].vcla = BLOCK_ADDRESS(VADDR(request), _blockSize);
      _tags[ctag].pcla = BLOCK_ADDRESS(PADDR(request), _blockSize);
      _tags[ctag].dirty = dirty;
      _tags[ctag].appID = request -> cpuID;

      // increment occupancy
      _occupancy[request -> cpuID] ++;

      // if the evicted tag entry is valid
      if (tagentry.valid) {
        _occupancy[tagentry.value.appID] --;
        INCREMENT(evictions);

        if (tagentry.value.dirty) {
          INCREMENT(dirty_evictions);
          MemoryRequest *writeback = new MemoryRequest(
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

#endif // __CMP_DYNAMIC_LLC_H__
