// -----------------------------------------------------------------------------
// File: TableNRU.h
// Description:
//    Extends the table class with nru replacement policy
// -----------------------------------------------------------------------------

#ifndef __TABLE_NRU_H__
#define __TABLE_NRU_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"
#include "Table.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Class: nru_table_t
// Description:
//    Extends the table class with nru replacement policy.
// -----------------------------------------------------------------------------

KVTemplate class nru_table_t : public TableClass {

  protected:

    // members from the table class
    using TableClass::_size;
    
    // referenced bits
    vector <bool> _referenced;

    // current hand
    uint32 _hand;


    // -------------------------------------------------------------------------
    // Implementing virtual functions from the base table class
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    // Function to update the replacement policy
    // -------------------------------------------------------------------------

    void UpdateReplacementPolicy(uint32 index, TableOp op, policy_value_t pval) {

      switch(op) {
        
        case TABLE_INSERT:
          _referenced[index] = true;
          break;

        case TABLE_READ:
          _referenced[index] = true;
          break;

        case TABLE_UPDATE:
          _referenced[index] = true;
          break;

        case TABLE_REPLACE:
          _referenced[index] = true;
          break;

        case TABLE_INVALIDATE:
          _referenced[index] = false;
          break;
      }
    }


    // -------------------------------------------------------------------------
    // Function to return a replacement index
    // -------------------------------------------------------------------------

    uint32 GetReplacementIndex() {
      while (_referenced[_hand] == true) {
        _referenced[_hand] = false;
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

    nru_table_t(uint32 size) : TableClass(size) {
      _referenced.resize(size, false);
    }
};

#endif // __TABLE_NRU_H__
