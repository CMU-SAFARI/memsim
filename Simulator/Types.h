// -----------------------------------------------------------------------------
// File: Types.h
// Description:
//    Defines some basic types used by other files
// -----------------------------------------------------------------------------

#ifndef __TYPES_H__
#define __TYPES_H__

#include <cassert>

using namespace std;

// -----------------------------------------------------------------------------
// Tell simics that you have all these types defined
// -----------------------------------------------------------------------------

#define HAVE_INT8
#define HAVE_INT16
#define HAVE_INT32
#define HAVE_INT64
#define HAVE_UINT8
#define HAVE_UINT16
#define HAVE_UINT32
#define HAVE_UINT64

// -----------------------------------------------------------------------------
// Basic signed integer types
// -----------------------------------------------------------------------------

typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

// -----------------------------------------------------------------------------
// Basic unsigned integer types
// -----------------------------------------------------------------------------

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

// -----------------------------------------------------------------------------
// Address and cycles
// -----------------------------------------------------------------------------

typedef uint64 addr_t;

#define BLOCK_ADDRESS(addr,size) (((addr)/(size))*(size))

#ifndef SIMICS_SIMULATOR
typedef uint64 cycles_t;
#else
#include <simics/core/types.h>
#endif


// -----------------------------------------------------------------------------
// Saturating counter
// -----------------------------------------------------------------------------

class saturating_counter {

  private:
    uint32 _value;
    uint32 _max;

  public:

    saturating_counter(uint32 max = 0, uint32 initial = 0) {
      _max = max;
      _value = initial;
    }

    void set_max(uint32 max) {
      _max = max;
    }
    
    void set(uint32 value) {
      _value = value;
    }

    void increment() {
      if (_value < _max) _value ++;
    }

    void decrement() {
      if (_value > 0) _value --;
    }

    operator uint32 () const {
      return _value;
    }
};


// -----------------------------------------------------------------------------
// Cyclic pointer
// -----------------------------------------------------------------------------

class cyclic_pointer {

  private:
    uint32 _hand;
    uint32 _size;

  public:

    cyclic_pointer(uint32 size, uint32 initial = 0) {
      _size = size;
      _hand = initial;
    }

    void set(uint32 value) {
      _hand = value;
    }

    void increment() {
      _hand ++; if (_hand == _size) _hand = 0;
    }

    void decrement() {
      if (_hand == 0) _hand = _size - 1;
      else _hand --;
    }

    void add(uint32 value) {
      _hand = (_hand + value) % _size;
    }

    operator uint32 () const {
      return _hand;
    }
};

#endif // __TYPES_H__
