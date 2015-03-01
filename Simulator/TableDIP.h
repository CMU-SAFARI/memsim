// -----------------------------------------------------------------------------
// File: TableDIP.h
// Description:
//    Extends the table class with dip replacement policy. Essentially LRU but
//    inserts at LRU or MRU position depending on the value of pval.
// -----------------------------------------------------------------------------

#ifndef __TABLE_DIP_H__
#define __TABLE_DIP_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"
#include "Table.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Class: dip_table_t
// Description:
//    Extends the table class with dip replacement policy.
// -----------------------------------------------------------------------------

KVTemplate class dip_table_t : public TableClass {

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

  // counter for BIP
  cyclic_pointer _bipCounter;

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

  void _push_front(uint32 index) {
    if (_head == NULL) {
      _head = _tail = _nodes[index];
    }
    else {
      _head -> prev = _nodes[index];
      _nodes[index] -> next = _head;
      _head = _nodes[index];
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

    // Invalidate
    if (op == TABLE_INVALIDATE) {
      _remove(index);
      return;
    }

    // Read or Update: remove the node and re-insert based on policy
    if (op == TABLE_READ || op == TABLE_UPDATE) {
      _remove(index);
    }

    // else if REPLACE: remove LRU and re-insert based on policy
    else if (op == TABLE_REPLACE) {
      _pop_front();
    }

    // Insert based on policy
    switch (pval) {
    case POLICY_HIGH: _push_back(index); break;
    case POLICY_LOW: _push_front(index); break;
    case POLICY_BIMODAL:
      if (_bipCounter) _push_front(index);
      else _push_back(index); break;
    }
  }


  // -------------------------------------------------------------------------
  // Function to return a replacement index
  // -------------------------------------------------------------------------

  uint32 GetReplacementIndex() {
    assert(_head != NULL);
    _bipCounter.increment();
    return _head -> index;
  }


public:

  // -------------------------------------------------------------------------
  // Constructor
  // -------------------------------------------------------------------------

  dip_table_t(uint32 size) : TableClass(size), _bipCounter(64) {
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

  ~dip_table_t() {
    for (uint32 i = 0; i < _size; i ++)
      delete _nodes[i];
  }
};

#endif // __TABLE_DIP_H__
