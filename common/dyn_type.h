#ifndef _RADIUM_COMMON_DYN_TYPE_H
#define _RADIUM_COMMON_DYN_TYPE_H

#include <wchar.h>

#include "ratio_type.h"

struct _hash_t;
typedef struct _hash_t hash_t;

struct _dynvec_t;
typedef struct _dynvec_t dynvec_t;


enum DynType{
  UNINITIALIZED_TYPE = 0, // Must be 0
  STRING_TYPE,
  INT_TYPE,
  FLOAT_TYPE,
  HASH_TYPE,
  ARRAY_TYPE,
  RATIO_TYPE,
  BOOL_TYPE // must be placed last
};

#define NUM_DYNTYPE_TYPES (1+BOOL_TYPE)

typedef struct{
  enum DynType type;
  union{
    const wchar_t *string;
    int64_t int_number;
    double float_number;
    hash_t *hash;
    dynvec_t *array;
    Ratio *ratio;
    bool bool_number;
  };
} dyn_t;

struct _dynvec_t{
  int num_elements;
  int num_elements_allocated;
  dyn_t *elements;
};

extern const dyn_t g_empty_dynvec;
extern const dyn_t g_uninitialized_dyn;

#endif
