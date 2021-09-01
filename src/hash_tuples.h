#pragma once

// This file offers a HashMap datastructure. This HashMap is
// implemented as an array of linked lists.
//
// In practice, we are using a few hashmaps in IronMask, but each of
// them has it specificities, meaning that no two hashes are
// similar. This hashmap defined in this file is not used anywhere
// anymore, but other hashmaps are inspired by it.
//
// TODO: make this hashmap implem somewhat generic and base all hash
// maps of IronMask on it.

#include <stdlib.h>
#include <stdint.h>

#include "combinations.h"

typedef struct _hashnode {
  uint8_t length;
  uint8_t sorted; // If true, then this element is sorted, otherwire it's not
  int generation;
  Comb* comb;
  struct _hashnode* next;
} HashNode;

typedef struct _hashmap {
  HashNode** content;
} HashMap;

HashMap init_hash();
void add_to_hash(HashMap map, Comb* comb, int comb_len, int generation);
void add_to_hash_no_copy(HashMap map, Comb* comb, int comb_len, int generation);
int hash_contains(HashMap map, Comb* comb, int comb_len);
void merge_hashes(HashMap dst, HashMap src);
int hash_size(HashMap map);
void free_hash();
