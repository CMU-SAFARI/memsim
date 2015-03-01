// -----------------------------------------------------------------------------
// File: BloomFilter.h
// Description:
//    This file defines a bloom filter class that approximates a set.
// -----------------------------------------------------------------------------

#ifndef __BLOOM_FILTER_H__
#define __BLOOM_FILTER_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <bitset>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include "Types.h"

#include <iostream>

using namespace std;

#define RAND_SEED 29346
#define MAX_BITS 262144


// -----------------------------------------------------------------------------
// Class: bloom_filter_t
// Description:
//    This class implements a bloom filter.
// -----------------------------------------------------------------------------

class bloom_filter_t {

protected:
  
  // ---------------------------------------------------------------------------
  // Parameters
  // ---------------------------------------------------------------------------

  uint32 _expectedMaxCount;
  uint32 _alpha;
  uint32 _numHashFunctions;

  // ---------------------------------------------------------------------------
  // Private structures
  // ---------------------------------------------------------------------------

  bitset <MAX_BITS> _filter;
  vector <uint64> _hashOdds;
  uint32 _logSize;
  uint32 _maxBitPosition;

  
  // ---------------------------------------------------------------------------
  // temporary structure used on every insert and check
  // ---------------------------------------------------------------------------
  
  vector <uint64> _indices;

  
  // ---------------------------------------------------------------------------
  // Stats
  // ---------------------------------------------------------------------------

  uint32 _numElements;
  uint64 _falsePositives;
  uint64 _tests;

  
public:

  // ---------------------------------------------------------------------------
  // Constructor
  // ---------------------------------------------------------------------------

  bloom_filter_t() {
    _filter.reset();
    _indices.clear();
    _hashOdds.clear();
  }

  
  // ---------------------------------------------------------------------------
  // Parameterized constructor
  // ---------------------------------------------------------------------------

  bloom_filter_t(uint32 expectedMaxCount, uint32 alpha,
                 uint32 numHashFunctions = 0) {
    initialize(expectedMaxCount, alpha, numHashFunctions);
  }

  
  // ---------------------------------------------------------------------------
  // Initialize
  // ---------------------------------------------------------------------------

  void initialize(uint32 expectedMaxCount, uint32 alpha,
                  uint32 numHashFunctions = 0) {

    // copy members
    _expectedMaxCount = expectedMaxCount;
    _alpha = alpha;

    if (numHashFunctions) 
      _numHashFunctions = numHashFunctions;
    else
      _numHashFunctions = ceil(log(2) * alpha);

    _logSize = log2(_expectedMaxCount * _alpha); 

    _numElements = 0;
    _falsePositives = 0;
    _tests = 0;
    _maxBitPosition = 34;

    // initialize the hash functions
    _indices.resize(_numHashFunctions);
    _hashOdds.resize(_numHashFunctions);

    compute_hash_functions();
  }

  
  // ---------------------------------------------------------------------------
  // insert an element
  // ---------------------------------------------------------------------------

  void insert(uint64 element) {

    // compute the indices
    compute_indices(element);
    
    // set all the bits corresponding to the indices
    for (uint32 i = 0; i < _numHashFunctions; i ++) {
      _filter.set(_indices[i]);
    }

    _numElements ++;
  }

  
  // ---------------------------------------------------------------------------
  // test an element
  // ---------------------------------------------------------------------------

  bool test(uint64 element, bool exists = false) {

    _tests ++;
    
    // compute the indices
    compute_indices(element);

    // check if any of the bits are not set
    for (uint32 i = 0; i < _numHashFunctions; i ++) {
      if (!_filter.test(_indices[i]))
        return false;
    }

    // check if its a false positive
    if (!exists)
      _falsePositives ++;

    return true;
  }

  
  // ---------------------------------------------------------------------------
  // clear out the filter
  // ---------------------------------------------------------------------------

  void clear() {
    _filter.reset();
    _numElements = 0;
  }

  
  // ---------------------------------------------------------------------------
  // return number of false positives
  // ---------------------------------------------------------------------------

  uint64 false_positives() {
    return _falsePositives;
  }

  double false_positive_rate() {
    return (double)(_falsePositives) * 100.0 / _tests;
  }

  // ---------------------------------------------------------------------------
  // nubmer of set bits
  // ---------------------------------------------------------------------------

  uint32 count() {
    return _filter.count();
  }


  virtual void compute_hash_functions() {
    srand(RAND_SEED);
      
    for (uint32 i = 0; i < _numHashFunctions; i ++) {
      uint32 random = rand();
      uint64 odd = 2 * random + 1;
      _hashOdds[i] = odd;
    }

    srand(time(NULL));
  }

  
  // ---------------------------------------------------------------------------
  // function to compute the indices of a given element
  // ---------------------------------------------------------------------------

  virtual void compute_indices(uint64 element) {
    for (uint32 i = 0; i < _numHashFunctions; i ++) {
      _indices[i] = element * _hashOdds[i];
      _indices[i] &= ((1llu << _maxBitPosition) - 1);
      _indices[i] >>= (_maxBitPosition - _logSize);
    }
  }
  

};



class h3_bloom_filter_t : public bloom_filter_t {

protected:

  struct h3 {
    vector <uint64> columns;
  };
  
  vector <h3> _hashes;

  
public:

  h3_bloom_filter_t() {
  }
  
  h3_bloom_filter_t(uint32 expectedMaxCount, uint32 alpha,
                    uint32 numHashFunctions = 0) :
    bloom_filter_t(expectedMaxCount, alpha, numHashFunctions)  {
  }
  
  void compute_hash_functions() {
    
    _hashes.resize(_numHashFunctions);
    
    srand(RAND_SEED);

    for (uint32 i = 0; i < _numHashFunctions; i ++) {
      _hashes[i].columns.resize(_logSize);
      for (uint32 j = 0; j < _logSize; j ++) {
        uint32 rand1 = rand();
        uint32 rand2 = rand();
        uint64 rand64 = rand1 * rand2;
        _hashes[i].columns[j] = (rand64 & ((1llu << _maxBitPosition) - 1));
      }
    }

    srand(time(NULL));
  }

  void compute_indices(uint64 element) {
    for (uint32 i = 0; i < _numHashFunctions; i ++) {
      _indices[i] = 0;
      for (uint32 j = 0; j < _logSize; j ++) {
        uint64 out = (element & _hashes[i].columns[j]);
        uint32 bit = 0;
        for (uint32 k = 0; k < _maxBitPosition; k ++)
          bit = bit ^ ((out >> k) & 1);
        _indices[i] = (_indices[i] << 1) | bit;
      }
    }
  }
  
};

#endif // __BLOOM_FILTER_H__
