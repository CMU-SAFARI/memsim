// -----------------------------------------------------------------------------
// File: TableDRRIP.h
// Description:
//    Extends the table class with drrip replacement policy
// -----------------------------------------------------------------------------

#ifndef __TABLE_DRRIP_H__
#define __TABLE_DRRIP_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"
#include "Table.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <vector>

// -----------------------------------------------------------------------------
// Class: drrip_table_t
// Description:
//    Extends the table class with drrip replacement policy.
// -----------------------------------------------------------------------------

KVTemplate class drrip_table_t : public TableClass {

protected:

  // members from the table class
  using TableClass::_size;

  // list of rrpvs
  vector <saturating_counter> _rrpv;

  // max value
  uint32 _max;

  // BRRIP counter
  cyclic_pointer _brripCounter;

  // -------------------------------------------------------------------------
  // Implementing virtual functions from the base table class
  // -------------------------------------------------------------------------

  // -------------------------------------------------------------------------
  // Function to update the replacement policy
  // -------------------------------------------------------------------------

  void UpdateReplacementPolicy(uint32 index, TableOp op, policy_value_t pval) {

    // Invalidate: do nothing
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
  }


  // -------------------------------------------------------------------------
  // Function to return a replacement index
  // -------------------------------------------------------------------------

  uint32 GetReplacementIndex() {
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
  }


public:

  // -------------------------------------------------------------------------
  // Constructor
  // -------------------------------------------------------------------------

  drrip_table_t(uint32 size) : TableClass(size), _brripCounter(67) {
    _max = 7;
    _rrpv.resize(size, saturating_counter(_max));
  }
};

#endif // __TABLE_DRRIP_H__
