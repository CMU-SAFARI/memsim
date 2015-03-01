
// -----------------------------------------------------------------------------
// File: TableMinW.h
// Description:
//    Extends the table class with minw replacement policy
// -----------------------------------------------------------------------------

#ifndef __TABLE_MINW_H__
#define __TABLE_MINW_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"
#include "Table.h"
//#include "DBIEntry.h"
// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <vector>

// -----------------------------------------------------------------------------
// Class: maxw_table_t
// Description:
//    Extends the table class with maxw replacement policy.
// -----------------------------------------------------------------------------

//class maxw_table_t : public TableClass <addr_t, DBIEntry> {
KVTemplate class minw_table_t : public TableClass {
protected:

  // members from the table class
  
  // -------------------------------------------------------------------------
  // Implementing virtual functions from the base table class
  // -------------------------------------------------------------------------

  // -------------------------------------------------------------------------
  // Function to update the replacement policy
  // -------------------------------------------------------------------------

  void UpdateReplacementPolicy(uint32 index, TableOp op, policy_value_t pval) {

    // Invalidate: do nothing
    /*
    if (op == TABLE_INVALIDATE) return;

    // if read or update, promotion policy based on pval
    else if (op == TABLE_READ || op == TABLE_UPDATE) {
      switch (pval) {
      case POLICY_HIGH: _rrpv[index].increment(); break;
      case POLICY_LOW: _rrpv[index].set(0);
      case POLICY_BIMODAL:
        if (_brripCounter) _rrpv[index].set(0);
        else _rrpv[index].increment();
        break;
      }
    }

    // if insert or replace, promotion policy based on pval
    else if (op == TABLE_INSERT || op == TABLE_REPLACE) {
      switch (pval) {
      case POLICY_HIGH: _rrpv[index].set(1); break;
      case POLICY_LOW: _rrpv[index].set(0);
      case POLICY_BIMODAL:
        if (_brripCounter) _rrpv[index].set(0);
        else _rrpv[index].set(1);
        break;
      }
    }
    */
  }


  // -------------------------------------------------------------------------
  // Function to return a replacement index
  // -------------------------------------------------------------------------

  uint32 GetReplacementIndex() {
  /*
    _brripCounter.increment();
    while (1) {
      for (uint32 i = 0; i < _size; i ++) {
        if (_rrpv[i] == 0)
          return i;
      }

      for (uint32 i = 0; i < _size; i ++) {
        _rrpv[i].decrement();
      }
    }
   */
/*
  int max = 0;
  uint32 maxindex = 0;
  vector<TableClass::entry>::iterator it;
  for (it = _table.begin(); it != _table.end(); ++it){
  if((*it).value.dirtyBits.count() > max) {maxindex = it -> key; max = (*it).value.dirtyBits.count();}
  }

  for(uint32 i = 0; i < _table.size(); i++){
  if(_table[i].value.dirtyBits.count() > max) {maxindex = _table[i].key; max = _table[i].value.dirtyBits.count();}
  }  
  return maxindex;*/
  return 0;
  }


public:

  // -------------------------------------------------------------------------
  // Constructor
  // -------------------------------------------------------------------------

  minw_table_t(uint32 size) : TableClass(size) {
   
  }
};

#endif // __TABLE_MINW_H__
