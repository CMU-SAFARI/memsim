// -----------------------------------------------------------------------------
// File: Table.h
// Description:
//    Defines an abstract table class. A table is a bounded key-value store. It
//    needs to be extended with a replacement policy
// -----------------------------------------------------------------------------

// *** index is used to point to location in the vector table_t, and key is a map to vector

#ifndef __TABLE_H__
#define __TABLE_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <vector>
#include <list>
#include <map>
#include <cassert>


// -----------------------------------------------------------------------------
// Some definitions
// -----------------------------------------------------------------------------

#define KVTemplate template <class key_t, class value_t>
#define TableClass table_t <key_t, value_t>
#define TableEntry typename TableClass::entry
#define TableOp typename TableClass::operation
#define TABLE_INSERT TableClass::T_INSERT
#define TABLE_READ TableClass::T_READ
#define TABLE_UPDATE TableClass::T_UPDATE
#define TABLE_REPLACE TableClass::T_REPLACE
#define TABLE_INVALIDATE TableClass::T_INVALIDATE

enum policy_value_t {
  POLICY_HIGH = 0,
  POLICY_BIMODAL = 1,
  POLICY_LOW = 2  
};



// -----------------------------------------------------------------------------
// Class: Table
// Description:
//    Defines an abstract table class
// -----------------------------------------------------------------------------

KVTemplate class table_t {

public:

  // -------------------------------------------------------------------------
  // Entry structure. 
  // -------------------------------------------------------------------------

  struct entry {
    bool valid;
    uint32 index;
    key_t key;
    value_t value;

    // default constructor
    entry() {
      valid = false;
    }

    // invalid entry constructor from index
    entry(uint32 eindex) {
      valid = false;
      index = eindex;
    }

    // construct entry from index, key and value
    entry(uint32 eindex, key_t ekey, value_t evalue) {
      valid = true;
      index = eindex;
      key = ekey;
      value = evalue;
    }
  };


public:

  // -------------------------------------------------------------------------
  // Set of table operations
  // -------------------------------------------------------------------------

  enum operation { T_INSERT, T_REPLACE, T_READ, T_UPDATE, T_INVALIDATE };


  // -------------------------------------------------------------------------
  // vector of table entries
  // -------------------------------------------------------------------------

  vector <entry> _table;


  // -------------------------------------------------------------------------
  // Size of the table
  // -------------------------------------------------------------------------

  uint32 _size;


  // -------------------------------------------------------------------------
  // map of Key index to speed up search
  // -------------------------------------------------------------------------

  map <key_t, uint32> _keyIndex;


  // -------------------------------------------------------------------------
  // List of free indices
  // -------------------------------------------------------------------------

  list <uint32> _freeList;


  // -------------------------------------------------------------------------
  // Flag indicating if key is same as index
  // -------------------------------------------------------------------------

  bool _indexIsKey;


  // -------------------------------------------------------------------------
  // Function to get a free entry
  // -------------------------------------------------------------------------

  uint32 GetFreeEntry() {
    if (!_freeList.empty()) {
      uint32 index = _freeList.front();
      _freeList.pop_front();
      return index;
    }
    return _size;
  }


  // ----------------------------------------------------------------------------
  // Function to search for a key, basically returns the index if entry is valid
  // ----------------------------------------------------------------------------

  uint32 SearchForKey(key_t key) {
    if (_indexIsKey) {
      assert(key < _size && key >= 0);
      if (_table[key].valid)
        return key;
    }
    else {
      if (_keyIndex.find(key) != _keyIndex.end())	// end is returned if key is not found
        return _keyIndex[key];				// return the value (index) in the map _keyIndex
    }
    return _size;
  }


  // -----------------------------------------------------------------------------
  // Function to insert an entry
  // -------------------------------------------------------------------------
    
  void InsertEntry(entry e) {
    if (!_indexIsKey)
      _keyIndex.insert(make_pair(e.key, e.index));	// make_pair is a part of std namespace
    else 
      assert(e.key < _size && e.key >= 0);
    _table[e.index] = e;
  }


  // -------------------------------------------------------------------------
  // Replace an entry
  // -------------------------------------------------------------------------

	// replace e1 with e2
  void ReplaceEntry(entry e1, entry e2) {
    if (!_indexIsKey) {
      _keyIndex.erase(e1.key);
      _keyIndex.insert(make_pair(e2.key, e1.index));
    }
    e2.index = e1.index;
    _table[e2.index] = e2;
  }


  // -------------------------------------------------------------------------
  // Invalidate an entry
  // -------------------------------------------------------------------------

  void InvalidateEntry(entry e) {
    if (!_indexIsKey)
      _keyIndex.erase(e.key);
    _table[e.index].valid = false;
    _freeList.push_back(e.index);
  }


  // -------------------------------------------------------------------------
  // Function to update the replacement policy. Need to be implemented by
  // deriving classes.
  // -------------------------------------------------------------------------

  virtual void UpdateReplacementPolicy(uint32 index, operation op, 
                                       policy_value_t pval) = 0;


  // -------------------------------------------------------------------------
  // Function to get replacement index
  // -------------------------------------------------------------------------

  virtual uint32 GetReplacementIndex() = 0;


public:


  // -------------------------------------------------------------------------
  // Table constructor
  // -------------------------------------------------------------------------

  table_t(uint32 size) {
    // initialize members
    _size = size;
    _table.resize(size);
    _keyIndex.clear();
    _indexIsKey = false;
    // add the indices to the free list
    for (uint32 i = 0; i < _size; i ++)
      _freeList.push_back(i);
  }


  // -------------------------------------------------------------------------
  // Function to return a count of number of entries in the table
  // -------------------------------------------------------------------------

  uint32 count() {
    return _size - _freeList.size();
  }


  // -------------------------------------------------------------------------
  // Function to look up if a key is present 
  // -------------------------------------------------------------------------

  bool lookup(key_t key) {
    if (SearchForKey(key) != _size)
      return true;
    return false;
  }


  // -------------------------------------------------------------------------
  // Function to insert a key-value pair into the list
  // -------------------------------------------------------------------------


/* this checks if key is already present -> return the corresponding entry
   if not, then insert the entry directly if key is index -> return corr. entry
           insert entry after getting index from freelist -> return corr. entry
   if key is not present and table is full, then replace an entry with entry to be inserted -> return evicted entry
*/
  virtual entry insert(key_t key, value_t value,
                       policy_value_t pval = POLICY_HIGH) {
    uint32 index;

    // check if the key is already present
    if ((index = SearchForKey(key)) != _size)
      return _table[index];						// valid will be true

    // if index is key
    if (_indexIsKey) {
      index = key;
      UpdateReplacementPolicy(index, T_INSERT, pval);			// why do we need it here?
      InsertEntry(entry(index, key, value));
      return entry(index);						// valid will be false
    }

    // check if there is a free slot
    if ((index = GetFreeEntry()) != _size) {
      // update with replacement policy
      UpdateReplacementPolicy(index, T_INSERT, pval);
      // insert the entry and return
      InsertEntry(entry(index, key, value));
      return entry(index);						// valid will be false
    }

    // get a replacement index

    index = GetReplacementIndex();					// implemented by derived class
    UpdateReplacementPolicy(index, T_REPLACE, pval);
    entry evicted = _table[index];
    ReplaceEntry(evicted, entry(index, key, value));
    return evicted;							// valid will be true
  }


  // -------------------------------------------------------------------------
  // Function to read a key
  // -------------------------------------------------------------------------

  virtual entry read(key_t key, policy_value_t pval = POLICY_HIGH) {
    uint32 index;
    // check if the element is present
    if ((index = SearchForKey(key)) == _size)
      return entry();
    // update the replacement policy and return
    UpdateReplacementPolicy(index, T_READ, pval);
    return _table[index];
  }

   
  // -------------------------------------------------------------------------
  // Function to update a key
  // -------------------------------------------------------------------------

  virtual entry update(key_t key, value_t value,
                       policy_value_t pval = POLICY_HIGH) {
    uint32 index;
    // check if the key is present
    if ((index = SearchForKey(key)) == _size)
      return entry();
    // update replacement policy and return
    _table[index].value = value;
    UpdateReplacementPolicy(index, T_UPDATE, pval);
    return _table[index];
  }


  // -------------------------------------------------------------------------
  // Function to silently update a key
  // -------------------------------------------------------------------------

  virtual entry silentupdate(key_t key, policy_value_t pval = POLICY_HIGH) {
    uint32 index;
    // check if the key is present
    if ((index = SearchForKey(key)) == _size)
      return entry();
    // update replacement policy and return
    UpdateReplacementPolicy(index, T_UPDATE, pval);
    return _table[index];
  }


  // -------------------------------------------------------------------------
  // Function to invalidate an entry
  // -------------------------------------------------------------------------

  virtual entry invalidate(key_t key) {
    uint32 index;
    // check if the key is present
    if ((index = SearchForKey(key)) == _size)
      return entry();
    // update replacement policy
    UpdateReplacementPolicy(index, T_INVALIDATE, POLICY_HIGH);
    entry evicted = _table[index];
    InvalidateEntry(evicted);
    return evicted;
  }


  // -------------------------------------------------------------------------
  // Function to get an entry by index
  // -------------------------------------------------------------------------

  entry entry_at_index(uint32 index) {
    assert(index < _size);
    return _table[index];
  }


  // -------------------------------------------------------------------------
  // Function to force replacement
  // -------------------------------------------------------------------------

  entry force_evict() {
    uint32 index = GetReplacementIndex();
    entry evicted = _table[index];
    UpdateReplacementPolicy(index, T_INVALIDATE, POLICY_HIGH);
    InvalidateEntry(evicted);
    return evicted;
  }

  key_t to_be_evicted() {
    uint32 index = GetReplacementIndex();
    return _table[index].key;
  }

  // -------------------------------------------------------------------------
  // operator [] . Provide simple access to value at some key
  // -------------------------------------------------------------------------

  value_t & operator[] (key_t key) {
    uint32 index = SearchForKey(key);
    assert(index != _size);
    return _table[index].value;
  }


  // -------------------------------------------------------------------------
  // Return the entry for a given key
  // -------------------------------------------------------------------------
    
  entry get(key_t key) {
    uint32 index = SearchForKey(key);
    if (index != _size)
      return _table[index];
    else
      return entry();
  }

};


#endif // __TABLE_H__
