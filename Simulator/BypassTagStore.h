// -----------------------------------------------------------------------------
// File: BypassTagStore.h
// Description:
//    This file describes an application-aware set bypass tag store. The high
//    level policy used is assumed to have two insertion states: 0, 1. The tag
//    store monitors the performance of the two policies on a small number of
//    dueling sets and follows the best policy on the remaining sets.
// -----------------------------------------------------------------------------

#ifndef __BYPASS_TAG_STORE_H__
#define __BYPASS_TAG_STORE_H__

#define DUELING_PRIME 443

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"
#include "GenericTable.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <cstdlib>

// -----------------------------------------------------------------------------
// Class: bypass_tagstore_t
// Description:
//    Defines a bypass tag store
// -----------------------------------------------------------------------------

template <class key_t, class value_t>
class bypass_tagstore_t {

protected:
    
  // -------------------------------------------------------------------------
  // Parameters
  // -------------------------------------------------------------------------

  uint32 _numSets;
  uint32 _numSlotsPerSet;
  string _policy;
  uint32 _numApps;
  uint32 _numSamplingSets;

  // -------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------
    

  //vector <saturating_counter> _psel;
  uint32 _threshold;
  
  generic_table_t <key_t, value_t> *_sets;


public:

  // -------------------------------------------------------------------------
  // Constructor
  // -------------------------------------------------------------------------

  bypass_tagstore_t() {
    _numSets = 0;
    _numApps = 0;
    _numSlotsPerSet = 0;
    _policy = "";
    _numSamplingSets = 0;
  }

  // ------------------------------------------------------------------------
  // Set Type
  // -----------------------------------------------------------------------
  
  struct SetType {
    bool leader;
    policy_value_t policy;
    uint32 appID;
    SetType() { leader = false; }
  };


  vector <SetType> _type;

  // -------------------------------------------------------------------------
  // Function to set the tag store parameters
  // -------------------------------------------------------------------------

  void SetTagStoreParameters(uint32 numApps, uint32 numSets,
                             uint32 numSlotsPerSet, string policy, uint32 numSamplingSets
                             ) {
    _numApps = numApps;
    _numSets = numSets;
    _numSlotsPerSet = numSlotsPerSet;
    _policy = policy;
    _numSamplingSets = numSamplingSets;

    // set the table parameters
    _sets = new generic_table_t <key_t, value_t> [_numSets];
    for (uint32 i = 0; i < _numSets; i ++)
      _sets[i].SetTableParameters(_numSlotsPerSet, _policy);


    // check if enough sets are available for dueling
    if (_numSamplingSets * _numApps > _numSets) {
      fprintf(stderr, "Not enough sampling sets available!\n");
      exit(0);
    }


    _type.resize(_numSets);
    cyclic_pointer current(_numSets, 0);

    // create the dueling sets for all the apps
    for (uint32 id = 0; id < _numApps; id ++) {
      for (uint32 sid = 0; sid < _numSamplingSets; sid ++) {
        if (_type[current].leader) {
          fprintf(stderr, "Something wrong in identifying sampling sets\n");
          exit(0);
        }
        _type[current].leader = true;
        _type[current].appID = id;
	_type[current].policy = POLICY_HIGH;
        current.add(DUELING_PRIME);
      }
    }
  }


  // -------------------------------------------------------------------------
  // Function to compute the index
  // -------------------------------------------------------------------------

  uint32 index(key_t key) {
    return key % _numSets;
  }


  // -------------------------------------------------------------------------
  // Function to return a count of number of entries in the tag store
  // -------------------------------------------------------------------------

  uint32 count() {
    assert(_sets != NULL);
    uint32 ret = 0;
    for (uint32 i = 0; i < _numSets; i ++)
      ret += _sets[i].count();
    return ret;
  }


  // -------------------------------------------------------------------------
  // return the policy for a particular application
  // -------------------------------------------------------------------------
  /*
  uint32 policy(uint32 appID) {
    if (_psel[appID] > _threshold)
      return 0;
    else
      return 1;
  }
  */
  // -------------------------------------------------------------------------
  // Function to look up if a key is present 
  // -------------------------------------------------------------------------

  bool lookup(key_t key) {
    assert(_sets != NULL);
    return _sets[index(key)].lookup(key);
  }


  // -------------------------------------------------------------------------
  // Function to insert a key-value pair into the list
  // -------------------------------------------------------------------------

  virtual TableEntry insert(key_t key, value_t value, 
                            policy_value_t pval = POLICY_HIGH)
  {
    assert(_sets!=NULL);
    uint32 setIndex = index(key);
    return _sets[setIndex].insert(key, value, pval);
  }


  // -------------------------------------------------------------------------
  // Function to read a key
  // -------------------------------------------------------------------------

  virtual TableEntry read(key_t key,policy_value_t pval = POLICY_HIGH) {
    assert(_sets != NULL);
    return _sets[index(key)].read(key, pval);
  }

   
  // -------------------------------------------------------------------------
  // Function to update a key
  // -------------------------------------------------------------------------

  virtual TableEntry update(key_t key, value_t value,
                            policy_value_t pval = POLICY_HIGH) {
    assert(_sets != NULL);
    return _sets[index(key)].update(key, value, pval);
  }


  // -------------------------------------------------------------------------
  // Function to silently update a key
  // -------------------------------------------------------------------------

  virtual TableEntry silentupdate(key_t key, policy_value_t pval = POLICY_HIGH) {
    assert(_sets != NULL);
    return _sets[index(key)].silentupdate(key, pval);
  }


  // -------------------------------------------------------------------------
  // Function to invalidate an entry
  // -------------------------------------------------------------------------

  virtual TableEntry invalidate(key_t key) {
    assert(_sets != NULL);
    return _sets[index(key)].invalidate(key);
  }


  // -------------------------------------------------------------------------
  // Function to get an entry by location
  // -------------------------------------------------------------------------

  TableEntry entry_at_location(uint32 setindex, uint32 slotindex) {
    assert(_sets != NULL);
    return _sets[setindex].entry_at_location(slotindex);
  }


  // -------------------------------------------------------------------------
  // operator [] . Provide simple access to value at some key
  // -------------------------------------------------------------------------

  value_t & operator[] (key_t key) {
    assert(_sets != NULL);
    return (_sets[index(key)])[key];
  }


  // -------------------------------------------------------------------------
  // Simple return the entry correponding to the tag
  // -------------------------------------------------------------------------

  TableEntry get(key_t key) {
    return _sets[index(key)].get(key);
  }


  // -------------------------------------------------------------------------
  // Function to force eviction from a set
  // -------------------------------------------------------------------------

  TableEntry force_evict(uint32 index) {
    return _sets[index].force_evict();
  }
};

#endif // __BYPASS_TAGSTORE_H__
