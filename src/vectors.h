#pragma once

// This file provides Vector datatypes. It uses a generic macro-based
// implementation, which means that it's pretty easy to define new
// Vector structures with the same API: just call the macro
// DEFINE_VECTOR_TYPE in this file, and the macro DEFINE_VECTOR_IMPLEM
// in vectors.c.

#include "circuit.h"

#define VECTOR_GROWTH_FACTOR 2
#define VECTOR_INIT_SIZE_DEFAULT 10

#ifdef __GNUC__
#define MAYBE_UNUSED_VARIABLE __attribute__((unused))
#else
#define MAYBE_UNUSED_VARIABLE
#endif


#define DEFINE_VECTOR_TYPE(_name, _type)                            \
  typedef struct _ ## _name ##_vector {                             \
    int max_size;                                                   \
    int length;                                                     \
    _type* content;                                                 \
  } _name;                                                          \
                                                                    \
  static _name MAYBE_UNUSED_VARIABLE empty_ ## _name = { .max_size = 0, .length = 0, .content = NULL }; \
                                                                    \
  _name* _name##_make();                                            \
  _name* _name##_make_size(int max_size);                           \
  _name* _name##_from_array(_type* arr, int length, int max_size);  \
  void _name##_push(_name* vec, _type val);                         \
  _type _name##_pop(_name* vec);                                    \
  _type _name##_get(_name* vec, int idx);                           \
  /* contains uses ==; do not use for strings */                    \
  int _name##_contains(_name* vec, _type val);                      \
  void _name##_free(_name* vec);                                    \
  void _name##_deep_free(_name* vec);                               \
  void _name##_shallow_free(_name* vec);                            \


DEFINE_VECTOR_TYPE(IntVector, int);

DEFINE_VECTOR_TYPE(VarVector, Var);

DEFINE_VECTOR_TYPE(DepArrVector, Dependency*);

DEFINE_VECTOR_TYPE(VarVecVector, VarVector*);

DEFINE_VECTOR_TYPE(BitDepVector, BitDep*);


typedef VarVector Tuple;
#define Tuple_make() VarVector_make()
#define Tuple_make_size(_size) VarVector_make_size(_size)
#define Tuple_from_array(_arr, _length, _max_size) VarVector_from_array(_arr, _lenth, _max_size)
#define Tuple_push(_arr, _val) VarVector_push(_arr, _val)
#define Tuple_pop(_arr) VarVector_pop(_arr)
#define Tuple_get(_arr, _idx) VarVector_get(_arr, _idx)
#define Tuple_contains(_arr, _val) VarVector_contains(_arr, _val)
#define Tuple_free(_arr) VarVector_free(_arr)
#define Tuple_shallow_free(_arr) VarVector_shallow_free(_arr)
