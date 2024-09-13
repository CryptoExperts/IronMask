#include <stdlib.h>

#include "vectors.h"

#define DEFINE_VECTOR_IMPLEM(_name, _type)                              \
                                                                        \
  _name* _name##_make() {                                               \
    return _name##_make_size(VECTOR_INIT_SIZE_DEFAULT);                 \
  }                                                                     \
                                                                        \
  _name* _name##_make_size(int max_size) {                              \
    _name* vec = malloc(sizeof(*vec));                                  \
    vec->length = 0;                                                    \
    vec->max_size = max_size;                                           \
    vec->content = calloc(max_size, sizeof(*vec->content));             \
    return vec;                                                         \
  }                                                                     \
                                                                        \
  _name* _name##_from_array(_type* old_arr, int length, int max_size) { \
    _name* vec = malloc(sizeof(*vec));                                  \
    vec->length = length;                                               \
    vec->max_size = max_size;                                           \
    vec->content = old_arr;                                             \
    return vec;                                                         \
  }                                                                     \
                                                                        \
  void _name##_push(_name* vec, _type val) {                            \
    if (vec->length >= vec->max_size) {                                 \
      vec->max_size *= VECTOR_GROWTH_FACTOR;                            \
      vec->content = realloc(vec->content, vec->max_size * sizeof(*vec->content)); \
    }                                                                   \
    vec->content[vec->length++] = val;                                  \
  }                                                                     \
                                                                        \
  _type _name##_pop(_name* _vec) {                                      \
    if (_vec->length != 0) {                                            \
      return _vec->content[--_vec->length];                             \
    }                                                                   \
    return 0;                                                  \
  }                                                                     \
                                                                        \
  _type _name##_get(_name* vec, int idx) {                              \
    return vec->content[idx];                                           \
  }                                                                     \
                                                                        \
  int _name##_contains(_name* vec, _type val) {                         \
    for (int i = 0; i < vec->length; i++) {                             \
      if (vec->content[i] == val) return 1;                             \
    }                                                                   \
    return 0;                                                           \
  }                                                                     \
                                                                        \
  void _name##_free(_name* vec) {                                       \
    free(vec->content);                                                 \
    free(vec);                                                          \
  }                                                                     \
                                                                        \
  void _name##_deep_free(_name* vec) {                                  \
    for (int i = 0; i < vec->length; i++) {                             \
      free((void*)(uintptr_t)vec->content[i]);                          \
    }                                                                   \
    free(vec->content);                                                 \
    free(vec);                                                          \
  }                                                                     \
                                                                        \
  void _name##_shallow_free(_name* vec) {                               \
    free(vec);                                                          \
  }                                                                     \

DEFINE_VECTOR_IMPLEM(StringVector, char*);

DEFINE_VECTOR_IMPLEM(IntVector, int);

DEFINE_VECTOR_IMPLEM(VarVector, Var);

DEFINE_VECTOR_IMPLEM(DepArrVector, Dependency*);

DEFINE_VECTOR_IMPLEM(VarVecVector, VarVector*);

DEFINE_VECTOR_IMPLEM(BitDepVector, BitDep*);
