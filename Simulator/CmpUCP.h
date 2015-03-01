// -----------------------------------------------------------------------------
// File: CmpUCP.h
// Description:
//
// -----------------------------------------------------------------------------

#ifndef __CMP_UCP_H__
#define __CMP_UCP_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Class: CmpUCP
// Description:
//
// -----------------------------------------------------------------------------

class CmpUCP : public MemoryComponent {

  protected:

    // -------------------------------------------------------------------------
    // Parameters
    // -------------------------------------------------------------------------

    uint32 _size;
    uint32 _blockSize;
    uint32 _associativity;
    uint32 _tagStoreLatency;
    uint32 _dataStoreLatency;
    uint32 _partitionPeriod; 

    // -------------------------------------------------------------------------
    // Private members
    // -------------------------------------------------------------------------

    struct TagEntry {
      bool valid;
      bool dirty;
      addr_t ctag;
      addr_t vcla;
      addr_t pcla;
      TagEntry() { valid = false; dirty = false; }
    };

    uint32 _numSets;
    vector <uint32> _target;
    vector <vector <uint32> > _current;
    vector <vector <uint32> > _hits;
    vector <uint32> _misses;
    vector <uint32> _free;
    vector <vector <vector <TagEntry> > > _tags;
    vector <vector <uint32> > _utility;

    cycles_t _previousPartitionCycle;

    vector <uint32> _occupancy;

    // -------------------------------------------------------------------------
    // Declare Counters
    // -------------------------------------------------------------------------

    NEW_COUNTER(accesses);
    NEW_COUNTER(reads);
    NEW_COUNTER(writes);
    NEW_COUNTER(partialwrites);
    NEW_COUNTER(writebacks);
    NEW_COUNTER(misses);
    NEW_COUNTER(readmisses);
    NEW_COUNTER(writemisses);
    NEW_COUNTER(evictions);
    NEW_COUNTER(dirtyevictions);

  public:

    // -------------------------------------------------------------------------
    // Constructor. It cannot take any arguments
    // -------------------------------------------------------------------------

    CmpUCP() {
      _size = 32;
      _blockSize = 64;
      _associativity = 2;
      _tagStoreLatency = 1;
      _dataStoreLatency = 2;
      _partitionPeriod = 5000000;
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
      CMP_PARAMETER_UINT("tag-store-latency", _tagStoreLatency)
      CMP_PARAMETER_UINT("data-store-latency", _dataStoreLatency)
      CMP_PARAMETER_UINT("partition-period", _partitionPeriod)

      CMP_PARAMETER_END
    }


    // -------------------------------------------------------------------------
    // Function to initialize statistics
    // -------------------------------------------------------------------------

    void InitializeStatistics() {
      INITIALIZE_COUNTER(accesses, "Total Accesses")
      INITIALIZE_COUNTER(reads, "Read Accesses")
      INITIALIZE_COUNTER(writes, "Write Accesses")
      INITIALIZE_COUNTER(partialwrites, "Partial Write Accesses")
      INITIALIZE_COUNTER(writebacks, "Writeback Accesses")
      INITIALIZE_COUNTER(misses, "Total Misses")
      INITIALIZE_COUNTER(readmisses, "Read Misses")
      INITIALIZE_COUNTER(writemisses, "Write Misses")
      INITIALIZE_COUNTER(evictions, "Evictions")
      INITIALIZE_COUNTER(dirtyevictions, "Dirty Evictions")
    }


    // -------------------------------------------------------------------------
    // Function called when simulation starts
    // -------------------------------------------------------------------------

    void StartSimulation() {
      _numSets = (_size * 1024) / (_blockSize * _associativity);
      _target.resize(_numCPUs, _associativity/_numCPUs);
      _free.resize(_numSets, _associativity);
      
      _current.resize(_numSets);
      for (uint32 i = 0; i < _numSets; i ++) 
        _current[i].resize(_numCPUs, 0);

      _tags.resize(_numCPUs);
      _hits.resize(_numCPUs);
      _utility.resize(_numCPUs);
      _misses.resize(_numCPUs, 0);
      for (uint32 i = 0; i < _numCPUs; i ++) {
        _hits[i].resize(_associativity, 0);
        _utility[i].resize(_associativity);
        _tags[i].resize(_numSets);
        for (uint32 j = 0; j < _numSets; j ++) {
          _tags[i][j].resize(_associativity);
        }
      }

      _previousPartitionCycle = 0;

      _occupancy.resize(_numCPUs, 0);

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

      // update statistics
      INCREMENT(accesses);

      // check if we need to repartition
      if (_currentCycle > _previousPartitionCycle + _partitionPeriod) {
        RepartitionCache();
        _previousPartitionCycle += _partitionPeriod;
      }
      
      // compute the cache block tag
      addr_t ctag = (request -> physicalAddress) / _blockSize;
      
      cycles_t latency;
      uint32 cpuID = request -> cpuID;

      // if its a partial write and the size is same as block size, then convert
      // it into a writeback
      if (request -> type == MemoryRequest::PARTIALWRITE &&
          request -> size == _blockSize) {
        request -> type = MemoryRequest::WRITEBACK;
      }

      // else if the request is a writeback and the size is smaller than the
      // block size, then convert it into a partial write
      else if (request -> type == MemoryRequest::WRITEBACK &&
          request -> size < _blockSize) {
        request -> type = MemoryRequest::PARTIALWRITE;
      }

      // action taken based on the request type
      switch (request -> type) {

        // READ request
        case MemoryRequest::READ: case MemoryRequest::READ_FOR_WRITE: case MemoryRequest::PREFETCH:

          INCREMENT(reads);

          // if the block is present, return the data to the requestor
          //     latency = serial-lookup?tag + data
          // if the block is not present, then request should be sent to the 
          //     latency = tag
          // cache stalls for the tag

          if (CheckBlock(cpuID, ctag)) {
            latency = _tagStoreLatency + _dataStoreLatency;
            request -> serviced = true;
          }

          else {
            INCREMENT(misses);
            INCREMENT(readmisses);
            latency = _tagStoreLatency;
          }

          request -> AddLatency(latency);
          return _tagStoreLatency;


        // WRITE request
        case MemoryRequest::WRITE:
          fprintf(stderr, "Writed not yet handled for UCP\n");
          exit(0);          

        // PARTIAL WRITE request
        case MemoryRequest::PARTIALWRITE:
          fprintf(stderr, "Writed not yet handled for UCP\n");
          exit(0);          


        // WRITEBACK request
        case MemoryRequest::WRITEBACK:

          INCREMENT(writebacks);

          // if the block is present, mark it as dirty
          // if the block is not present, evict a block and insert this into the
          // cache
          // cache stalls for the tag

          addr_t vcla = BLOCK_ADDRESS(request -> virtualAddress, _blockSize);
          addr_t pcla = BLOCK_ADDRESS(request -> physicalAddress, _blockSize);
          if (!MarkDirty(cpuID, ctag)) {
            InsertBlock(cpuID, ctag, true, vcla, pcla, request);
          }

          request -> serviced = true;
          return _tagStoreLatency;
      }
      

      return 0; 
    }


    // -------------------------------------------------------------------------
    // Function to process the return of a request. Return value indicates
    // number of busy cycles for the component.
    // -------------------------------------------------------------------------
    
    cycles_t ProcessReturn(MemoryRequest *request) { 

      // if the request is generated by this component, then it must have been a
      // writeback, so delete it
      if (request -> iniType == MemoryRequest::COMPONENT &&
          request -> iniPtr == this) {
        request -> destroy = true;
        return 0;
      }

      addr_t ctag = (request -> physicalAddress) / _blockSize;
      bool dirty = (request -> type == MemoryRequest::WRITE) ||
                   (request -> type == MemoryRequest::PARTIALWRITE);
      
      addr_t vcla = BLOCK_ADDRESS(request -> virtualAddress, _blockSize);
      addr_t pcla = BLOCK_ADDRESS(request -> physicalAddress, _blockSize);
      InsertBlock(request -> cpuID, ctag, dirty, vcla, pcla, request);

      return 0; 
    }


    // -------------------------------------------------------------------------
    // Function to compute the index of a cache block
    // -------------------------------------------------------------------------

    uint32 Index(addr_t ctag) {
      return ctag % _numSets;
    }

    // -------------------------------------------------------------------------
    // Function to check if a block is present. On a hit update the replacement
    // policy and the hit counters
    // -------------------------------------------------------------------------
    
    bool CheckBlock(uint32 cpuID, addr_t ctag) {
      
      uint32 index = Index(ctag);
      for (uint32 way = 0; way < _associativity; way ++) {

        if (_tags[cpuID][index][way].ctag == ctag) {

          // Update hit counters
          _hits[cpuID][way] ++;

          // TRUE HIT
          if (_tags[cpuID][index][way].valid) {
            TagEntry temp = _tags[cpuID][index][way];
            for (int32 i = way;  i > 0; i --) {
              _tags[cpuID][index][i] = _tags[cpuID][index][i - 1];
            }
            _tags[cpuID][index][0] = temp;
            return true;
          }

          // FALSE HIT
          _misses[cpuID] ++;
          return false;
        }
      }

      _misses[cpuID] ++;
      return false;
    }


    // -------------------------------------------------------------------------
    // Function to mark a block as dirty if its present
    // -------------------------------------------------------------------------

    bool MarkDirty(uint32 cpuID, addr_t ctag) {

      uint32 index = Index(ctag);
      for (uint32 way = 0; way < _associativity; way ++) {

        if (_tags[cpuID][index][way].ctag == ctag) {

          // Update hit counters
          _hits[cpuID][way] ++;

          // TRUE HIT
          if (_tags[cpuID][index][way].valid) {
            _tags[cpuID][index][way].dirty = true;
            return true;
          }

          // FALSE HIT
          return false;
        }
      }

      return false;
    }


    // -------------------------------------------------------------------------
    // Function to insert a block into the cache
    // -------------------------------------------------------------------------

    void InsertBlock(uint32 cpuID, addr_t ctag, bool dirty, addr_t vcla,
        addr_t pcla, MemoryRequest *request) {

      uint32 index = Index(ctag);

      // check if some block needs to be evicted
      if (_free[index] == 0) {
        
        bool flag = true;
        uint32 victim;

        // choose the cpu for which current > target
        for (victim = 0; victim < _numCPUs; victim ++) {
          if (_target[victim] < _current[index][victim]) {
            flag = false;
            break;
          }
        }

        // if everything is fine, then victim = cpuID
        if (flag) {
          victim = cpuID;
        }

        // evict a line from the victim
        EvictBlock(_tags[victim][index][_current[index][victim] - 1], request);
        _tags[victim][index][_current[index][victim] - 1].valid = false;
        _current[index][victim] --;
        _occupancy[victim] --;
      }

      else {
        _free[index] --;
      }

      // insert the block into the required cpu
      for (int32 i = _associativity - 1; i > 0; i --) {
        _tags[cpuID][index][i] = _tags[cpuID][index][i - 1];
      }

      _tags[cpuID][index][0].valid = true;
      _tags[cpuID][index][0].dirty = dirty;
      _tags[cpuID][index][0].ctag = ctag;
      _tags[cpuID][index][0].vcla = vcla;
      _tags[cpuID][index][0].pcla = pcla;
      _current[index][cpuID] ++;
      _occupancy[cpuID] ++;
    }


    // -------------------------------------------------------------------------
    // Function to evict a block
    // -------------------------------------------------------------------------

    void EvictBlock(TagEntry entry, MemoryRequest *request) {
      if (!entry.valid)
        return;

      INCREMENT(evictions);
      if (entry.dirty) {
        INCREMENT(dirtyevictions);
        MemoryRequest *writeback = new MemoryRequest(
            MemoryRequest::COMPONENT, request -> cpuID, this, 
            MemoryRequest::WRITEBACK, request -> cmpID, 
            entry.vcla, entry.pcla, _blockSize, 
            request -> currentCycle);
        writeback -> icount = request -> icount;
        writeback -> ip = request -> ip;
        SendToNextComponent(writeback);
      }
    }


    // -------------------------------------------------------------------------
    // Function to repartition cache
    // -------------------------------------------------------------------------

    void RepartitionCache() {

      ComputeUtility();

      uint32 avail = _associativity - _numCPUs;
      uint32 maxCPU;
      pair <uint32, uint32> maxMU;
      pair <uint32, uint32> MU;
      vector <uint32> allocated;
      allocated.resize(_numCPUs, 1);
      
      while (avail > 0) {
        maxCPU = 0;
        maxMU = MaxMarginalUtility(0, allocated[0], avail);
        for (uint32 cpu = 1; cpu < _numCPUs; cpu ++) {
          MU = MaxMarginalUtility(cpu, allocated[cpu], avail);
          if (MU.first > maxMU.first) {
            maxCPU = cpu;
            maxMU = MU;
          }
        }
        if (maxMU.first == 0) {
          break;
        }
        allocated[maxCPU] += maxMU.second;
        avail -= maxMU.second;
      }

      if (avail > 0) {
        uint32 cpu = 0;
        while (avail > 0) {
          allocated[cpu] ++;
          avail --;
          cpu ++;
          if (cpu == _numCPUs)
            cpu = 0;
        }
      }

      for (uint32 i = 0; i < _numCPUs; i ++) {
        _target[i] = allocated[i];
        for (uint32 way = 0; way < _associativity; way ++)
          _hits[i][way] /= 2;
      }

    }


    // -------------------------------------------------------------------------
    // Compute utility for all applications
    // -------------------------------------------------------------------------

    void ComputeUtility() {
     
      for (uint32 cpu = 0; cpu < _numCPUs; cpu ++) { 
          _utility[cpu][0] = _hits[cpu][0];
        for (uint32 way = 1; way < _associativity; way ++) {
          _utility[cpu][way] = _utility[cpu][way - 1] + _hits[cpu][way];
        }
      }
    }


    // -------------------------------------------------------------------------
    // Get the maximum marginal utility for an application
    // -------------------------------------------------------------------------

    pair <uint32, uint32> MaxMarginalUtility(uint32 cpuID, uint32 allocated, 
        uint32 avail) {
      uint32 max = 0;
      uint32 ways = 0;
      uint32 mu;
      for (uint32 i = 1; i <= avail; i ++) {
        mu = MarginalUtility(cpuID, allocated, allocated + i);
        if (mu > max) {
          max = mu;
          ways = i;
        }
      }
      return make_pair(max, ways);
    }
    
    // -------------------------------------------------------------------------
    // Get the marginal utility for an application 
    // -------------------------------------------------------------------------

    uint32 MarginalUtility(uint32 cpuID, uint32 way1, uint32 way2) {
      if (way2 == way1) return 0;
      return (_utility[cpuID][way2] - _utility[cpuID][way1]) / (way2 - way1);
    }
};

#endif // __CMP_UCP_H__
