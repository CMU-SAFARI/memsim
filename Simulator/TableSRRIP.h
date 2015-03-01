// -----------------------------------------------------------------------------
// File: TableSRRIP.h
// Description:
//    Extends the table class with srrip replacement policy
// -----------------------------------------------------------------------------

#ifndef __TABLE_SRRIP_H__
#define __TABLE_SRRIP_H__

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
// Class: srrip_table_t
// Description:
//    Extends the table class with srrip replacement policy.
// -----------------------------------------------------------------------------

KVTemplate class srrip_table_t : public TableClass {

  protected:

    // members from the table class
    using TableClass::_size;

    // list of rrpvs
    vector <saturating_counter> _rrpv;

    // max value
    uint32 _max;

    // -------------------------------------------------------------------------
    // Implementing virtual functions from the base table class
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    // Function to update the replacement policy
    // -------------------------------------------------------------------------

    void UpdateReplacementPolicy(uint32 index, TableOp op, policy_value_t pval) {

      switch(op) {
        
        case TABLE_INSERT:
          _rrpv[index].set(1);
          break;

        case TABLE_READ:
          _rrpv[index].increment();
          break;

        case TABLE_UPDATE:
          _rrpv[index].increment();
          break;

        case TABLE_REPLACE:
          _rrpv[index].set(1);
          break;

        case TABLE_INVALIDATE:
          _rrpv[index].set(0);
          break;
      }
    }


    // -------------------------------------------------------------------------
    // Function to return a replacement index
    // -------------------------------------------------------------------------

    uint32 GetReplacementIndex() {
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

    srrip_table_t(uint32 size) : TableClass(size) {
      _max = 7;
      _rrpv.resize(size, saturating_counter(_max));
    }
};

#endif // __TABLE_SRRIP_H__
