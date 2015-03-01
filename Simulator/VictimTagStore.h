// -----------------------------------------------------------------------------
// File: VictimTags.h
// Description:
//    Data structure to keep track of recently evicted blocks in a cache
// -----------------------------------------------------------------------------

#ifndef __VICTIM_TAG_STORE_H__
#define __VICTIM_TAG_STORE_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"
#include "BloomFilter.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <set>

// -----------------------------------------------------------------------------
// Class: VictimTagStore
// Description:
//    This class defines a victim tag store. It keep tracks of the recently
//    evicted lines from the cache.
// -----------------------------------------------------------------------------

class victim_tag_store_t {

  protected:

    // -------------------------------------------------------------------------
    // Parameters
    // -------------------------------------------------------------------------

    uint32 _numBlocks;
    bool _useBloomFilter;
    bool _ideal;
    bool _noClear;
    bool _decoupleClear;
    bool _segmented;

    // -------------------------------------------------------------------------
    // Private members
    // -------------------------------------------------------------------------

    set <addr_t> _index;
    set <addr_t> _remove;
    bloom_filter_t _bf;
    uint32 _numCurrentBlocks;
    uint32 _numHits;
    list <addr_t> _fifo;

    set <addr_t> _sindex[2];
    int _cindex;

  public:

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    victim_tag_store_t() {
      _numBlocks = 0;
      _numHits = 0;
      _index.clear();
      _sindex[0].clear();
      _sindex[1].clear();
      _cindex = 0;
      _fifo.clear();
      _useBloomFilter = false;
      _noClear = false;
      _decoupleClear = false;
      _segmented = false;
    }


    // -------------------------------------------------------------------------
    // Constructor with parameters
    // -------------------------------------------------------------------------

  victim_tag_store_t(uint32 numBlocks, bool useBloomFilter=false, bool ideal=false,
                     bool noClear = false, bool decoupleClear = false,
                     bool segmented = false, uint32 alpha = 8) {
    initialize(numBlocks, useBloomFilter, ideal, noClear, decoupleClear, segmented, alpha);
  }


    // -------------------------------------------------------------------------
    // Initialize
    // -------------------------------------------------------------------------

  void initialize(uint32 numBlocks, bool useBloomFilter=false, bool ideal=false,
                  bool noClear=false, bool decoupleClear = false,
                  bool segmented = false, uint32 alpha = 8) {

      _numBlocks = numBlocks;
      _index.clear();
      _sindex[0].clear();
      _sindex[1].clear();
      _cindex = 0;
      _remove.clear();
      _numCurrentBlocks = 0;
      _useBloomFilter = useBloomFilter;
      _fifo.clear();
      _ideal = ideal;
      _noClear = noClear;
      _decoupleClear = decoupleClear;
      _segmented = segmented;
      _numHits = 0;
      
      _bf.initialize(numBlocks, alpha);
    }


    // -------------------------------------------------------------------------
    // Insert a block into the victim tag store
    // -------------------------------------------------------------------------

    void insert(addr_t tag) {

      if (_numBlocks == 0) return;
      if (_index.find(tag) != _index.end()) return;

      if (_segmented) {
        if (_numCurrentBlocks == (_numBlocks/2)) {
          _cindex = 1 - _cindex;
          _sindex[_cindex].clear();
          _numCurrentBlocks = 0;
        }
        _sindex[_cindex].insert(tag);
        _numCurrentBlocks ++;
        return;
      }
      
      if ((!_decoupleClear) && (_numCurrentBlocks == _numBlocks)) {
        if (_noClear) {
          addr_t last = _fifo.front();
          while (_remove.find(last) != _remove.end()) {
            _remove.erase(last);
            _fifo.pop_front();
            last = _fifo.front();
          }
          _fifo.pop_front();
          _index.erase(last);
          _numCurrentBlocks --;
        }
        else {
          _bf.clear();
          _fifo.clear();
          _index.clear();
          _numCurrentBlocks = 0;
        }
      }

      else if (_numCurrentBlocks == 2 * _numBlocks) {
        _bf.clear();
        _fifo.clear();
        _index.clear();
        _numCurrentBlocks = 0;
      }

      if (_useBloomFilter) _bf.insert(tag);

      _index.insert(tag);
      _fifo.push_back(tag);
      _numCurrentBlocks ++;
    }


    // -------------------------------------------------------------------------
    // Check if a block is present in the victim tag store or not
    // -------------------------------------------------------------------------

    bool test(addr_t tag) {
      bool result;
      if (_numBlocks == 0)
        return false;

      if (_segmented) {
        if ((_sindex[0].find(tag) != _sindex[0].end()) ||
            (_sindex[1].find(tag) != _sindex[1].end())) {
          return true;
        }
        return false;
      }
      
      if (_index.find(tag) != _index.end()) {
        if (_ideal) {
          _index.erase(tag);
          _remove.insert(tag);
          _numCurrentBlocks --;
        }

        if (_useBloomFilter)
          result = _bf.test(tag, true);
        else 
          result = true;
        
        _numHits ++;
        if (_decoupleClear && (100 * _numHits == 75 * _numBlocks))  {
          _bf.clear();
          _fifo.clear();
          _index.clear();
          _numCurrentBlocks = 0;
          _numHits = 0;
        }

        return result;
      }

      if (_useBloomFilter)
        return _bf.test(tag, false);
      else 
        return false;
    }

    // -------------------------------------------------------------------------
    // false positives
    // -------------------------------------------------------------------------

    uint64 false_positives() {
      return _bf.false_positives();
    }

    double false_positive_rate() {
      return _bf.false_positive_rate();
    }
    
};

typedef victim_tag_store_t evicted_address_filter_t;

#endif // __VICTIM_TAG_STORE_H__
