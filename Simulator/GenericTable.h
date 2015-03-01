// -----------------------------------------------------------------------------
// File: GenericTable.h
// Description:
//    Wrapper for table with flexible replacement policy.
// -----------------------------------------------------------------------------

#ifndef __GENERIC_TABLE_H__
#define __GENERIC_TABLE_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------
#include "Types.h"
#include "Table.h"
//#include "DBIEntry.h"


// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <string>


// -----------------------------------------------------------------------------
// Class: GenericTable
// Description:
//    Wrapper for table class
// -----------------------------------------------------------------------------

KVTemplate class generic_table_t {


protected:


public:

  // -------------------------------------------------------------------------
  // pointer to a table
  // -------------------------------------------------------------------------

  TableClass *_table;

  // -------------------------------------------------------------------------
  // Constructor
  // -------------------------------------------------------------------------

  generic_table_t() {
    _table = NULL;
  }


  // -------------------------------------------------------------------------
  // Construct generic table with size and policy
  // -------------------------------------------------------------------------

  generic_table_t(uint32 size, string policy) {
    _table = NULL;
    SetTableParameters(size, policy);
  }

  // -------------------------------------------------------------------------
  // Destructor
  // -------------------------------------------------------------------------

  ~generic_table_t() {
    if (_table != NULL)
      delete _table;
  }


  // -------------------------------------------------------------------------
  // Function to set the table parameters
  // -------------------------------------------------------------------------

  void SetTableParameters(uint32 size, string policy);
    
  // Don't we set the table parameters anywhere ?



  // -------------------------------------------------------------------------
  // Function to return a count of number of entries in the table
  // -------------------------------------------------------------------------

  uint32 count() {
    assert(_table != NULL);
    return _table -> count();
  }


  // -------------------------------------------------------------------------
  // Function to look up if a key is present 
  // -------------------------------------------------------------------------

  bool lookup(key_t key) {
    assert(_table != NULL);
    return _table -> lookup(key);
  }


  // -------------------------------------------------------------------------
  // Function to insert a key-value pair into the list
  // -------------------------------------------------------------------------

  virtual TableEntry insert(key_t key, value_t value, 
                            policy_value_t pval = POLICY_HIGH) {
    assert(_table != NULL);
    return _table -> insert(key, value, pval);
  }


  // -------------------------------------------------------------------------
  // Function to read a key
  // -------------------------------------------------------------------------

  virtual TableEntry read(key_t key, policy_value_t pval = POLICY_HIGH) {
    assert(_table != NULL);
    return _table -> read(key, pval);
  }

   
  // -------------------------------------------------------------------------
  // Function to update a key
  // -------------------------------------------------------------------------

  virtual TableEntry update(key_t key, value_t value, 
                            policy_value_t pval = POLICY_HIGH) {
    assert(_table != NULL);
    return _table -> update(key, value, pval);
  }


  // -------------------------------------------------------------------------
  // Function to silently update a key
  // -------------------------------------------------------------------------

  virtual TableEntry silentupdate(key_t key, policy_value_t pval = POLICY_HIGH) {
    assert(_table != NULL);
    return _table -> silentupdate(key, pval);
  }


  // -------------------------------------------------------------------------
  // Function to invalidate an entry
  // -------------------------------------------------------------------------

  virtual TableEntry invalidate(key_t key) {
    assert(_table != NULL);
    return _table -> invalidate(key);
  }


  // -------------------------------------------------------------------------
  // Function to force eviction
  // -------------------------------------------------------------------------

  TableEntry force_evict() {
    assert(_table != NULL);
    return _table -> force_evict();
  }

  key_t to_be_evicted() {
    assert(_table != NULL);
    return _table -> to_be_evicted();
  }


  // -------------------------------------------------------------------------
  // Function to get an entry by index
  // -------------------------------------------------------------------------

  TableEntry entry_at_index(uint32 index) {
    assert(_table != NULL);
    return _table -> entry_at_index(index);
  }


  // -------------------------------------------------------------------------
  // operator [] . Provide simple access to value at some key
  // -------------------------------------------------------------------------

  value_t & operator[] (key_t key) {
    assert(_table != NULL);
    return (*_table)[key];
  }


  // -------------------------------------------------------------------------
  // Simply return the entry for a given key
  // -------------------------------------------------------------------------

  TableEntry get(key_t key) {
    assert(_table != NULL);
    return _table -> get(key);
  }
};


// All the functions till here were just functions of the table class

// -----------------------------------------------------------------------------
// Macros for including more policies
// -----------------------------------------------------------------------------


#define TABLE_POLICY_BEGIN                      \
  if (false) { }

#define TABLE_POLICY_END                                                \
  else {                                                                \
    fprintf(stderr, "Error: Unknown table policy `%s'\n", policy.c_str()); \
    exit(-1);                                                           \
  }

#define TABLE_POLICY(name,type)                 \
  else if (policy.compare(name) == 0) {         \
    _table = new type <key_t, value_t> (size);  \
  }

#include "PolicyList.h"

#endif // __GENERIC_TABLE_H__
