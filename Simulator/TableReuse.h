// -----------------------------------------------------------------------------
// File: TableReuse.h
// Description:
//    Extends the table class with reuse replacement policy
// -----------------------------------------------------------------------------

#ifndef __TABLE_REUSE_H__
#define __TABLE_REUSE_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"
#include "Table.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Class: reuse_table_t
// Description:
//    Extends the table class with reuse replacement policy.
// -----------------------------------------------------------------------------

KVTemplate class reuse_table_t : public TableClass {

  protected:

    // members from the table class
    using TableClass::_size;

    // reuse queue
    vector <uint32> _reuse;

    // hand
    uint32 _hand;

    // max reuse value
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
          _reuse[index] = 0;
          _hand = (index + 1);
          if (_hand == _size) _hand = 0;
          break;

        case TABLE_READ:
          if (_reuse[index] != _max)
            _reuse[index] ++;
          break;

        case TABLE_UPDATE:
          if (_reuse[index] != _max)
            _reuse[index] ++;
          break;

        case TABLE_REPLACE:
          _reuse[index] = 0;
          _hand = index + 1;
          if (_hand == _size) _hand = 0;
          break;

        case TABLE_INVALIDATE:
          _reuse[index] = 0;
          break;
      }
    }


    // -------------------------------------------------------------------------
    // Function to return a replacement index
    // -------------------------------------------------------------------------

    uint32 GetReplacementIndex() {
      while (_reuse[_hand] != 0) {
        _reuse[_hand] --;
        _hand ++;
        if (_hand == _size)
          _hand = 0;
      }
      return _hand;
    }


  public:

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    reuse_table_t(uint32 size) : TableClass(size) {
      _reuse.resize(size, 0);
      _hand = 0;
      _max = 3;
    }
};

#endif // __TABLE_REUSE_H__
