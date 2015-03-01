// -----------------------------------------------------------------------------
// File: TableFIFO.h
// Description:
//    Extends the table class with fifo replacement policy
// -----------------------------------------------------------------------------

#ifndef __TABLE_FIFO_H__
#define __TABLE_FIFO_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"
#include "Table.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Class: fifo_table_t
// Description:
//    Extends the table class with fifo replacement policy.
// -----------------------------------------------------------------------------

KVTemplate class fifo_table_t : public TableClass {

  protected:

    // members from the table class
    using TableClass::_size;

    // fifo queue
    list <uint32> _queue;

    // -------------------------------------------------------------------------
    // Implementing virtual functions from the base table class
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    // Function to update the replacement policy
    // -------------------------------------------------------------------------

    void UpdateReplacementPolicy(uint32 index, TableOp op, policy_value_t pval) {

      switch(op) {
        
        case TABLE_INSERT:
          _queue.push_back(index);
          break;

        case TABLE_READ:
          break;

        case TABLE_UPDATE:
          break;

        case TABLE_REPLACE:
          _queue.pop_front();
          _queue.push_back(index);
          break;

        case TABLE_INVALIDATE:
          break;
      }
    }


    // -------------------------------------------------------------------------
    // Function to return a replacement index
    // -------------------------------------------------------------------------

    uint32 GetReplacementIndex() {
      assert(!_queue.empty());
      return _queue.front();
    }


  public:

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    fifo_table_t(uint32 size) : TableClass(size) {
      _queue.clear();
    }
};

#endif // __TABLE_FIFO_H__
