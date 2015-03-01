// -----------------------------------------------------------------------------
// File: CmpARC.h
// Description:
//    Implements the Adaptive Replacement Cache.
// -----------------------------------------------------------------------------

#ifndef __CMP_ARC_H__
#define __CMP_ARC_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <list>
#include <vector>
#include <algorithm>

// definitions
#define FOR_EACH(iter,setindex,lname)                                   \
  for (list <TagEntry>::iterator iter = _sets[setindex].lname.begin();  \
       iter != _sets[setindex].lname.end(); iter ++)

// -----------------------------------------------------------------------------
// Class: CmpARC
// Description:
//
// -----------------------------------------------------------------------------

class CmpARC : public MemoryComponent {

protected:

  // -------------------------------------------------------------------------
  // Parameters
  // -------------------------------------------------------------------------

  uint32 _size;
  uint32 _blockSize;
  uint32 _associativity;
  bool _useRRIP;

  uint32 _tagStoreLatency;
  uint32 _dataStoreLatency;

  // -------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------

  struct TagEntry {
    bool valid;
    bool dirty;
    addr_t tag;
    addr_t vcla;
    addr_t pcla;
    saturating_counter repl;
    uint32 appID;
    TagEntry():repl(7,0) { valid = false; dirty = false; }
  };

  struct ARCList {
    list <TagEntry> t1, t2, b1, b2;
    int32 p;
    ARCList() {
      t1.clear();
      t2.clear();
      b1.clear();
      b2.clear();
      p = 0;
    }
  };


  // tag store
  uint32 _numSets;
  vector <ARCList> _sets;

  // occupancy
  vector <uint32> _occupancy;
  
  

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

  CmpARC() {
    _size = 1024;
    _blockSize = 64;
    _associativity = 16;
    _tagStoreLatency = 6;
    _dataStoreLatency = 16;
    _useRRIP = false;
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
      CMP_PARAMETER_BOOLEAN("use-rrip", _useRRIP)

    CMP_PARAMETER_END
  }


  // -------------------------------------------------------------------------
  // Function to initialize statistics
  // -------------------------------------------------------------------------

  void InitializeStatistics() {
    
    INITIALIZE_COUNTER(accesses, "Total Accesses");
    INITIALIZE_COUNTER(reads, "Read Accesses");
    INITIALIZE_COUNTER(writebacks, "Writeback Accesses");
    INITIALIZE_COUNTER(misses, "Total Misses");
    INITIALIZE_COUNTER(evictions, "Evictions");
    INITIALIZE_COUNTER(dirty_evictions, "Dirty Evictions");
  }


  // -------------------------------------------------------------------------
  // Function called when simulation starts
  // -------------------------------------------------------------------------

  void StartSimulation() {
    
    // tag store
    _numSets = (_size * 1024) / (_blockSize * _associativity);
    _sets.resize(_numSets);
    _occupancy.resize(_numCPUs, 0);

    // create the occupancy log file
    if (_numCPUs > 1) 
      NEW_LOG_FILE("occupancy", "occupancy");
  }


  // -------------------------------------------------------------------------
  // Function called at a heart beat. Argument indicates cycles elapsed after
  // previous heartbeat
  // -------------------------------------------------------------------------

  void HeartBeat(cycles_t hbCount) {

    if (_numCPUs > 1) {
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
      
    INCREMENT(accesses);

    // no writes
    if (request -> type == MemoryRequest::WRITE ||
        request -> type == MemoryRequest::PARTIALWRITE) {
      fprintf(stderr, "LLC cannot handle writes\n");
      exit(0);
    }

    // compute the cache block tag
    addr_t ctag = PADDR(request) / _blockSize;

    // check if its a read or write back
    switch (request -> type) {

      // READ request
    case MemoryRequest::READ:
    case MemoryRequest::READ_FOR_WRITE:
    case MemoryRequest::PREFETCH:

      INCREMENT(reads);

      // cache hit
      if (READ_BLOCK(ctag)) {
        request -> serviced = true;
        request -> AddLatency(_tagStoreLatency + _dataStoreLatency);
      }

      // cache miss
      else {
        INCREMENT(misses);
        request -> AddLatency(_tagStoreLatency);
      }

      return _tagStoreLatency;

      
      // WRITEBACK request
    case MemoryRequest::WRITEBACK:

      INCREMENT(writebacks);

      if (!MARK_DIRTY(ctag))
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

    // if its a writeback generated by this component destroy it
    if (request -> iniType == MemoryRequest::COMPONENT &&
        request -> iniPtr == this) {
      request -> destroy = true;
      return 0;
    }

    // get the cache block tag
    addr_t ctag = PADDR(request) / _blockSize;

    // if the block is already present return
    if (LOOK_UP(ctag))
      return 0;

    INSERT_BLOCK(ctag, false, request);

    return 0;
  }

  
  // -------------------------------------------------------------------------
  // index of a cache block
  // -------------------------------------------------------------------------

  uint32 INDEX(addr_t ctag) {
    return ctag % _numSets;
  }

  
  // -------------------------------------------------------------------------
  // Look up for a block
  // -------------------------------------------------------------------------

  bool LOOK_UP(addr_t ctag) {
    uint32 index = INDEX(ctag);
    
    FOR_EACH(it, index, t1) {
      if ((*it).tag == ctag)
        return true;
    }

    FOR_EACH(it, index, t2) {
      if ((*it).tag == ctag)
        return true;
    }

    return false;
  }

  
  // -------------------------------------------------------------------------
  // mark a block as dirty
  // -------------------------------------------------------------------------

  bool MARK_DIRTY(addr_t ctag) {
    uint32 index = INDEX(ctag);
    
    FOR_EACH(it, index, t1) {
      if ((*it).tag == ctag) {
        (*it).dirty = true;
        return true;
      }
    }

    FOR_EACH(it, index, t2) {
      if ((*it).tag == ctag) {
        (*it).dirty = true;
        return true;
      }
    }

    return false;
  }


  // -------------------------------------------------------------------------
  // function to read a block
  // -------------------------------------------------------------------------

  bool READ_BLOCK(addr_t ctag) {
    uint32 index = INDEX(ctag);
    TagEntry entry;

    // check if its in either of the top lists. if its is move it to top of t2
    FOR_EACH(it, index, t1) {
      if ((*it).tag == ctag) {
        TagEntry entry = (*it);
        entry.repl.set(1);
        _sets[index].t1.erase(it);
        _sets[index].t2.push_back(entry);
        return true;
      }
    }

    FOR_EACH(it, index, t2) {
      if ((*it).tag == ctag) {
        TagEntry entry = (*it);
        entry.repl.increment();
        _sets[index].t2.erase(it);
        _sets[index].t2.push_back(entry);
        return true;
      }
    }

    return false;
  }
  

  // -------------------------------------------------------------------------
  // function to insert a block
  // -------------------------------------------------------------------------

  void INSERT_BLOCK(addr_t ctag, bool dirty, MemoryRequest *request) {

    uint32 index = INDEX(ctag);
    uint32 appID = request -> cpuID;
    TagEntry replaced;
    bool backup_hit = false;

    int32 p = _sets[index].p;
    int32 b1 = _sets[index].b1.size();
    int32 b2 = _sets[index].b2.size();
    int32 t1 = _sets[index].t1.size();
    int32 t2 = _sets[index].t2.size();

    // check if the block is in the b1 list
    FOR_EACH(it, index, b1) {
      if ((*it).tag == ctag) {
        TagEntry entry = (*it);
        
        // adapt p
        if (b1 == 0)
          _sets[index].p = _associativity;
        else 
          _sets[index].p = min((int32)_associativity, p + max(b2/b1, 1));

        // Replace blocks
        replaced = REPLACE(index, false);

        // move tag to top of t2
        entry.repl.set(1);
        _sets[index].b1.erase(it);
        _sets[index].t2.push_back(entry);
        backup_hit = true;
        break;
      }
    }

    if (!backup_hit) {
      // check if the block is in the b2 list
      FOR_EACH(it, index, b2) {
        if ((*it).tag == ctag) {
          TagEntry entry = (*it);
          
          // adapt p
          if (b2 == 0)
            _sets[index].p = 0;
          else
            _sets[index].p = max(0, p - max(b1/b2, 1));
          
          // replace blocks
          replaced = REPLACE(index, true);
          
          // move tag to top of t2
          entry.repl.set(1);
          _sets[index].b2.erase(it);
          _sets[index].t2.push_back(entry);
          backup_hit = true;
          break;
        }
      }
    }

    // if its in neither of the lists
    if (!backup_hit) {

      if (t1 + t2 > _associativity) {
        printf("%u: Something wrong. More blocks in cache\n", index);
        exit(0);
      }

      if (b1 + b2 > _associativity) {
        printf("%u: Something wrong. More blocks in history\n", index);
        exit(0);
      }


      bool o_replace = false;
      bool c_replace = false;
      if (t1 + t2 == _associativity)
        c_replace = true;
      if (t1 + t2 + b1 + b2 == 2 * _associativity)
        o_replace = true;

      // otherwise cache is not full, so no replacement needed
      if (c_replace) {

        if (o_replace) {
          if (t1 + b1 >= _associativity && !_sets[index].b1.empty())
            EVICT_BLOCK(_sets[index].b1);
          else
            EVICT_BLOCK(_sets[index].b2);
        }

        if (t1 == _associativity) {
          replaced = EVICT_BLOCK(_sets[index].t1);
          replaced.repl.set(1);
          _sets[index].b1.push_back(replaced);
        }

        else if (t2 == _associativity) {
          replaced = EVICT_BLOCK(_sets[index].t2);
          replaced.repl.set(1);
          _sets[index].b2.push_back(replaced);
        }

        else {
          replaced = REPLACE(index, false);
        }
      }

      TagEntry entry;
      entry.valid = true;
      entry.tag = ctag;
      entry.dirty = dirty;
      entry.vcla = BLOCK_ADDRESS(VADDR(request), _blockSize);
      entry.pcla = BLOCK_ADDRESS(PADDR(request), _blockSize);
      entry.appID = request -> cpuID;
      entry.repl.set(1);
      
      _sets[index].t1.push_back(entry);
    }
      

    _occupancy[request -> cpuID] ++;

    // if the replaced block is valid, evict it
    if (replaced.valid) {
      _occupancy[replaced.appID] --;
      INCREMENT(evictions);

      if (replaced.dirty) {
        INCREMENT(dirty_evictions);
        MemoryRequest *writeback = new
          MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this,
                        MemoryRequest::WRITEBACK, request -> cmpID, 
                        replaced.vcla, replaced.pcla, _blockSize,
                        request -> currentCycle);
        writeback -> icount = request -> icount;
        writeback -> ip = request -> ip;
        SendToNextComponent(writeback);          
      }
    }
    
  }

  
  // -------------------------------------------------------------------------
  // function to replace a block from the cache
  // -------------------------------------------------------------------------

  TagEntry REPLACE(uint32 index, bool b2present) {

    int32 t1 = _sets[index].t1.size();
    int32 p = _sets[index].p;

    TagEntry replaced;

    if ((t1 > 0) &&
        ((t1 > p) || (t1 == p && b2present) || (_sets[index].t2.empty()))) {
      replaced = EVICT_BLOCK(_sets[index].t1);
      replaced.repl.set(1);
      _sets[index].b1.push_back(replaced);
    }
    else {
      replaced = EVICT_BLOCK(_sets[index].t2);
      replaced.repl.set(1);
      _sets[index].b2.push_back(replaced);
    }

    return replaced;
  }

  // -------------------------------------------------------------------------
  // function to evict a block from a list based on the replacement policy
  // -------------------------------------------------------------------------

  TagEntry EVICT_BLOCK(list <TagEntry> &l) {
    TagEntry replaced;

    if (_useRRIP) {
      bool found = false;
      while (!found) {
        for (list <TagEntry>::iterator it = l.begin(); it != l.end(); it ++) {
          if ((*it).repl == 0) {
            replaced = (*it);
            l.erase(it);
            found = true;
            break;
          }
        }
        if (!found) {
          for (list <TagEntry>::iterator it = l.begin(); it != l.end(); it ++) {
            (*it).repl.decrement();
          }
        }
      }
    }
    else {
      replaced = l.front();
      l.pop_front();
    }

    return replaced;
  }
  
};

#endif // __CMP_ARC_H__
