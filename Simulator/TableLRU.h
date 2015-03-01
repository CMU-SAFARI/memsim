// -----------------------------------------------------------------------------
// File: TableLRU.h
// Description:
//    Extends the table class with lru replacement policy
// -----------------------------------------------------------------------------

#ifndef __TABLE_LRU_H__
#define __TABLE_LRU_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"
#include "Table.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// Class: lru_table_t
// Description:
//    Extends the table class with lru replacement policy.
// -----------------------------------------------------------------------------

KVTemplate class lru_table_t : public TableClass {

  protected:

    // members from the table class
    using TableClass::_size;

    // list of node containers
    struct ListNode {
      ListNode *prev;
      uint32 index;
      ListNode *next;
    };

    // array of nodes
    vector <ListNode *> _nodes;

    ListNode *_head;
    ListNode *_tail;

    // -------------------------------------------------------------------------
    // Macros
    // -------------------------------------------------------------------------

    void _push_back(uint32 index) {
      if (_head == NULL) {
        _head = _tail = _nodes[index];
      }
      else {
        _tail -> next = _nodes[index];
        _nodes[index] -> prev = _tail;
        _tail = _nodes[index];
      }
    }

    uint32 _pop_front() {
      ListNode *node = _head;
      _head = _head -> next;
      if (_head != NULL)
        _head -> prev = NULL;
      node -> prev = NULL;
      node -> next = NULL;
      return node -> index;
    }

    void _remove(uint32 index) {
      ListNode *node = _nodes[index];
      if (node -> prev != NULL) node -> prev -> next = node -> next;
      else _head = _head -> next;
      if (node -> next != NULL) node -> next -> prev = node -> prev;
      else _tail = _tail -> prev;
      node -> next = node -> prev = NULL;
    }


    // -------------------------------------------------------------------------
    // Implementing virtual functions from the base table class
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    // Function to update the replacement policy
    // -------------------------------------------------------------------------

    void UpdateReplacementPolicy(uint32 index, TableOp op, policy_value_t pval) {

      switch(op) {
        
        case TABLE_INSERT:
          _push_back(index);
          break;

        case TABLE_READ:
          _remove(index);
          _push_back(index);
          break;

        case TABLE_UPDATE:
          _remove(index);
          _push_back(index);
          break;

        case TABLE_REPLACE:
          _pop_front();
          _push_back(index);
          break;

        case TABLE_INVALIDATE:
          _remove(index);
          break;
      }
    }


    // -------------------------------------------------------------------------
    // Function to return a replacement index
    // -------------------------------------------------------------------------

    uint32 GetReplacementIndex() {
      assert(_head != NULL);
      return _head -> index;
    }


  public:

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    lru_table_t(uint32 size) : TableClass(size) {
      _head = _tail = NULL;
      _nodes.resize(size);
      for (uint32 i = 0; i < size; i ++) {
        _nodes[i] = new ListNode;
        _nodes[i] -> index = i;
        _nodes[i] -> next = NULL;
        _nodes[i] -> prev = NULL;
      }
    }

    // -------------------------------------------------------------------------
    // Destructor
    // -------------------------------------------------------------------------

    ~lru_table_t() {
      for (uint32 i = 0; i < _size; i ++)
        delete _nodes[i];
    }
};

#endif // __TABLE_LRU_H__
