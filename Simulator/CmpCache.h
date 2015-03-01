// -----------------------------------------------------------------------------
// File: CmpCache.h
// Description:
//    Defines a simple cache component. The cache has a single bank and a single
//    read-write port. Policy is specified using one of the table policies.
// -----------------------------------------------------------------------------

#ifndef __CMP_CACHE_H__
#define __CMP_CACHE_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------


// All components inherit from MemoryComponent
#include "MemoryComponent.h"
#include "GenericTagStore.h"
#include "Types.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Class: CmpCache
// Description:
//    Defines a simple cache component.
// -----------------------------------------------------------------------------

class CmpCache : public MemoryComponent {

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

  bool _virtualTag;					// whether or not tag is virtual
  bool _serialLookup;				// whether or not to perform serial lookup (leads to addtnl tagstore latency)

  bool _evictionLog;					// whether eviciton data is to be stored
  bool _exclusive;

  bool _forwardFake;
  bool _demotePH;
  
  // -------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------

  struct CacheTagValue {
    bool dirty;
    bool prefetched;
    addr_t vcla;
    addr_t pcla;
    uint32 reuse;
    CacheTagValue() { dirty = false; reuse = 0; prefetched = false; }
  };

  // tag store
  uint32 _numSets;
  generic_tagstore_t <addr_t, CacheTagValue> _tags;

  // eviction log
  struct EvictionData {
    uint32 count;					// what does count keep track of ?
    uint32 dirty;
    list <uint32> reuse;
    EvictionData() { count = 0; dirty = 0; reuse.clear(); }
  };

// each address has an associated eviction data

  map <addr_t, EvictionData> _evictionData;
  map <uint32, uint64> _reuse;


  // -------------------------------------------------------------------------
  // Declare counters
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

  CmpCache() {
    _size = 32768;
    _blockSize = 64;
    _associativity = 2;
    _policy = "lru";
    _tagStoreLatency = 1;
    _dataStoreLatency = 2;
    _virtualTag = true;
    _serialLookup = false;
    _evictionLog = false;
    _exclusive = false;
    _demotePH = false;
    _forwardFake = false;
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
      CMP_PARAMETER_BOOLEAN("virtual-tag", _virtualTag)
      CMP_PARAMETER_BOOLEAN("serial-lookup", _serialLookup)
      CMP_PARAMETER_BOOLEAN("eviction-log", _evictionLog)
      CMP_PARAMETER_BOOLEAN("exclusive", _exclusive)
      CMP_PARAMETER_BOOLEAN("forward-fake", _forwardFake)
      CMP_PARAMETER_BOOLEAN("demote-ph", _demotePH)

    CMP_PARAMETER_END
  }


// these are the parameters for the cache component

  // -------------------------------------------------------------------------
  // Function to initialize statistics
  // -------------------------------------------------------------------------

  void InitializeStatistics() {
    INITIALIZE_COUNTER(accesses, "Total Accesses");
    INITIALIZE_COUNTER(reads, "Read Accesses");
    INITIALIZE_COUNTER(writes, "Write Accesses");
    INITIALIZE_COUNTER(partialwrites, "Partial Write Accesses");
    INITIALIZE_COUNTER(writebacks, "Writeback Accesses");
    INITIALIZE_COUNTER(misses, "Total Misses");
    INITIALIZE_COUNTER(readmisses, "Read Misses");
    INITIALIZE_COUNTER(writemisses, "Write Misses");
    INITIALIZE_COUNTER(evictions, "Evictions");
    INITIALIZE_COUNTER(dirtyevictions, "Dirty Evictions");
  }


  // -------------------------------------------------------------------------
  // Function called when simulation starts
  // -------------------------------------------------------------------------

  void StartSimulation() {
    // compute the number of sets and initialize the tag store
    _numSets = _size / (_blockSize * _associativity);
    _tags.SetTagStoreParameters(_numSets, _associativity, _policy);
  }

    
  // -------------------------------------------------------------------------
  // End simulation
  // -------------------------------------------------------------------------


  void EndSimulation() {
    DUMP_STATISTICS;
    if (_evictionLog) {
      string filename = _simulationFolderName + "/" + _name + ".eviction";
      FILE *file;
      file = fopen(filename.c_str(), "w");
      assert(file != NULL);
      map <addr_t, EvictionData>::iterator it;
// iterate through eviction data for all address blocks
      list <uint32>::iterator rit;
// iterate through reuse data for all eviction data
      for (it = _evictionData.begin(); it != _evictionData.end(); it ++) {
        fprintf(file, "%llu ", it -> first);
        fprintf(file, "%u %u", it -> second.count, it -> second.dirty);
        for (rit = it -> second.reuse.begin(); rit != it -> second.reuse.end();
             rit ++)
          fprintf(file, " %u", *rit);
        fprintf(file, "\n");
      }
      fclose(file);
      filename = _simulationFolderName + "/" + _name + ".reuse";
      file = fopen(filename.c_str(), "w");
      assert(file != NULL);
      map <uint32, uint64>::iterator reuseit;
      for (reuseit = _reuse.begin(); reuseit != _reuse.end(); reuseit ++) {
        fprintf(file, "%u %llu", reuseit -> first, reuseit -> second);
        fprintf(file, "\n");
      }
      fclose(file);
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

    // compute the cache block tag -> ctag
    addr_t ctag = (_virtualTag ? (request -> virtualAddress) : 
                   (request -> physicalAddress)) / _blockSize;
      
    table_t <addr_t, CacheTagValue>::entry tagentry;
    cycles_t latency;

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
    case MemoryRequest::READ:
    case MemoryRequest::READ_FOR_WRITE:
    case MemoryRequest::PREFETCH:

      INCREMENT(reads);

      // if the block is present, return the data to the requestor
      //     latency = serial-lookup?tag + data
      // if the block is not present, then request should be sent to next
      //     latency = tag
      // cache stalls for the tag

      tagentry = _tags.read(ctag);
      if (tagentry.valid) {
        latency = (_serialLookup ? _tagStoreLatency : 0) + 
          _dataStoreLatency;
        _tags[ctag].reuse ++;

        if (_tags[ctag].prefetched) {
          _tags[ctag].prefetched = false;
          if (_demotePH)
            _tags.read(ctag, POLICY_LOW);
          if (_forwardFake) {
            MemoryRequest *fake = new MemoryRequest(*request);
            fake -> type = MemoryRequest::FAKE_READ;
            SendToNextComponent(fake);
          }
        }
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

      INCREMENT(writes);

      // the processor does not stall in this case.
      // if the block is present, mark it as dirty
      // if the block is not present, send it to the next component
      // (hopefully MSHR).
      // cache stalls for tag
          
      tagentry = _tags.silentupdate(ctag);
      if (tagentry.valid) {
        _tags[ctag].dirty = true;
        request -> serviced = true;
      }
      else {
        INCREMENT(misses);
        INCREMENT(writemisses);
      }
      return _tagStoreLatency;
	// in write, add latency for only the tagstore (stalling for tag), but no latency for datawrite, as it is hidden 
        // latency          



      // PARTIAL WRITE request
    case MemoryRequest::PARTIALWRITE:

      INCREMENT(partialwrites);

      // if the block is present, return it to the requestor
      //    latency = serial-tag?tag + data
      // if the block is not present, send a miss request
      //    latency = tag
      // cache stalls for the tag
          
      tagentry = _tags.read(ctag); // TODO: Check this line
      if (tagentry.valid) {
        _tags[ctag].dirty = true; // CHANGE
        latency = (_serialLookup ? _tagStoreLatency : 0) + 
          _dataStoreLatency;
        request -> serviced = true;
      }
      else {
        INCREMENT(misses);
        INCREMENT(writemisses);
        latency = _tagStoreLatency;
      }

      request -> AddLatency(latency);
      return _tagStoreLatency;
          

      // WRITEBACK request
    case MemoryRequest::WRITEBACK:

      INCREMENT(writebacks);

      // if the block is present, mark it as dirty
      // if the block is not present, evict a block and insert this into the cache
      // cache stalls for the tag

      if (_tags.lookup(ctag)) {
        _tags[ctag].dirty = true;
      }
      else {
        tagentry = _tags.insert(ctag, CacheTagValue());
        // this will return the evicted entry
        _tags[ctag].dirty = true;
        _tags[ctag].vcla = ((request -> virtualAddress)/_blockSize)*_blockSize;
        _tags[ctag].pcla = ((request -> physicalAddress)/_blockSize)*_blockSize;
        EvictBlock(tagentry, request);
      }

      request -> serviced = true;
      return _tagStoreLatency;
    }
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

    // get the cache block tag
    addr_t ctag = (_virtualTag ? request -> virtualAddress : 
                   request -> physicalAddress) / _blockSize;

    // else check if the block is already present in the cache
    if (_tags.lookup(ctag))
      return 0;

    table_t <addr_t, CacheTagValue>::entry tagentry;

    // else insert the block into the cache
    tagentry = _tags.insert(ctag, CacheTagValue());
    _tags[ctag].vcla = ((request -> virtualAddress) / _blockSize) * _blockSize;
    _tags[ctag].pcla = ((request -> physicalAddress) / _blockSize) * _blockSize;
    if (request -> type == MemoryRequest::WRITE || 
        request -> type == MemoryRequest::PARTIALWRITE ||
        request -> dirtyReply)
      _tags[ctag].dirty = true;

    if (request -> type == MemoryRequest::PREFETCH)
      _tags[ctag].prefetched = true;

    // Need to clean this up
    request -> dirtyReply = false;

    EvictBlock(tagentry, request);
    return 0;
  }


  // -------------------------------------------------------------------------
  // Function to evict a tag entry
  // This function will collect stats related to the eviction and then
  // if the tagentry to be evicted is dirty, generate a writeback request
  // and sets its dirtyReply to True
  // else if the tagentry to be evicted is exclusive, then generate a
  // writeback request and set its dirtyReply to False
  // -------------------------------------------------------------------------

  void EvictBlock(table_t <addr_t, CacheTagValue>::entry tagentry,
                  MemoryRequest *request) {
    if (tagentry.valid) {
      if (_evictionLog) {
        _evictionData[tagentry.key].count ++;
        _evictionData[tagentry.key].reuse.push_back(tagentry.value.reuse);
        _reuse[tagentry.value.reuse] ++;
      }
      INCREMENT(evictions);
      if (tagentry.value.dirty) {
        if (_evictionLog) {
          _evictionData[tagentry.key].dirty ++;
        }
        INCREMENT(dirtyevictions);
        MemoryRequest *writeback =
          new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this, 
                            MemoryRequest::WRITEBACK, request -> cmpID, 
                            tagentry.value.vcla, tagentry.value.pcla, _blockSize, 
                            request -> currentCycle);
        writeback -> dirtyReply = true;
        writeback -> icount = request -> icount;
        writeback -> ip = request -> ip;
        SendToNextComponent(writeback);
      }
      else if (_exclusive) {
        MemoryRequest *writeback =
          new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this, 
                            MemoryRequest::WRITEBACK, request -> cmpID, 
                            tagentry.value.vcla, tagentry.value.pcla, _blockSize, 
                            request -> currentCycle);
        writeback -> dirtyReply = false;
        writeback -> icount = request -> icount;
        writeback -> ip = request -> ip;
        SendToNextComponent(writeback);
      }
    }
  }
};

#endif // __CMP_CACHE_H__
