// -----------------------------------------------------------------------------
// File: TableGeneration.h
// Description:
//    Extends the table class with generation replacement policy
// -----------------------------------------------------------------------------

#ifndef __TABLE_GENERATION_H__
#define __TABLE_GENERATION_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"
#include "Table.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Class: generation_table_t
// Description:
//    Extends the table class with generation replacement policy.
// -----------------------------------------------------------------------------

KVTemplate class generation_table_t : public TableClass {

  protected:

    // members from the table class
    using TableClass::_size;
    using TableClass::_table;

    // vector of generations and references
    struct Generation {
      saturating_counter generation;
      bool referenced;
      Generation(uint32 max):generation(max) {
      }
    };

    vector <Generation> _nodes;

    // current hand
    cyclic_pointer _hand;

    // maximum generation
    uint32 _maxGeneration;


    // -------------------------------------------------------------------------
    // Implementing virtual functions from the base table class
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    // Function to update the replacement policy
    // -------------------------------------------------------------------------

    void UpdateReplacementPolicy(uint32 index, TableOp op, policy_value_t pval) {

      switch(op) {
        
        case TABLE_INSERT:
          _nodes[index].generation.set(pval);
          _nodes[index].referenced = false;
          break;

        case TABLE_READ:
          _nodes[index].referenced = true;
          break;

        case TABLE_UPDATE:
          _nodes[index].referenced = true;
          break;

        case TABLE_REPLACE:
          _nodes[index].generation.set(pval);
          _nodes[index].referenced = false;
          break;

        case TABLE_INVALIDATE:
          _nodes[index].generation.set(0);
          _nodes[index].referenced = false;
          break;
      }
    }


    // -------------------------------------------------------------------------
    // Function to return a replacement index
    // -------------------------------------------------------------------------

    uint32 GetReplacementIndex() {
      while (!(_nodes[_hand].generation == 0 && 
            _nodes[_hand].referenced == false &&
            _table[_hand].valid == true)) {

        if (_nodes[_hand].referenced) {
          _nodes[_hand].referenced = false;
          _nodes[_hand].generation.increment();
        }
        else {
          _nodes[_hand].generation.decrement();
        }
        _hand.increment();
      }
      return _hand;
    }


  public:

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    generation_table_t(uint32 size) : TableClass(size), _hand(size) {
      _maxGeneration = 3;
      _nodes.resize(size, Generation(_maxGeneration));
    }
};

#endif // __TABLE_GENERATION_H__
