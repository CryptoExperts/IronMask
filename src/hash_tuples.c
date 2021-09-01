#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "hash_tuples.h"

// TODO: it would be nice to be able to resize the hash...
#define HASH_MASK 0x1ffffff // 500M -> times 8 (sizeof a pointer) = 4GB table
#define HASH_SIZE (HASH_MASK+1)


HashMap init_hash() {
  HashMap map;
  map.content = calloc(HASH_SIZE, sizeof(*map.content));
  return map;
}

// I don't remember where I borrowed this hash function from... I'm
// thinking maybe StackOverflow, but I'm not sure. Anyways, if you
// (the reader) plan on reusing this hash function, don't cite me as
// the source for it, but have a look around the web, and you should
// easily find the actual author.
unsigned int hash_int(unsigned int x) {
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = (x >> 16) ^ x;
  return x;
}

unsigned int hash_comb(Comb* comb, int comb_len) {
  unsigned int hash = 0;
  for (int i = 0; i < comb_len; i++) {
    hash += hash_int(comb[i]);
  }
  return hash & HASH_MASK;
}


// Returns the generation of |comb| in |map| if it exists at index |key|; 0 otherwise.
int _hash_contains(HashMap map, Comb* comb, int comb_len, unsigned int key) {
  HashNode* curr = map.content[key];
  Comb* sorted_comb;
  int is_sorted = 0;
  while (curr != NULL) {
    if (curr->length != comb_len) {
      goto next;
    }
    if (!is_sorted) {
      if (is_sorted_comb(comb, comb_len)) {
        sorted_comb = comb;
      } else {
        sorted_comb = alloca(comb_len * sizeof(*sorted_comb));
        memcpy(sorted_comb, comb, comb_len * sizeof(*sorted_comb));
        sort_comb(sorted_comb, comb_len);
      }
      is_sorted = 1;
    }
    if (!curr->sorted) {
      if (!is_sorted_comb(curr->comb, curr->length)) {
        sort_comb(curr->comb, curr->length);
      }
      curr->sorted = 1;
    }
    for (int i = 0; i < comb_len; i++) {
      if (curr->comb[i] != sorted_comb[i]) {
        goto next;
      }
    }
    return curr->generation;

  next:
    curr = curr->next;
  }
  return 0;
}

// Returns 1 if |map| contains |comb|, 0 otherwise
int hash_contains(HashMap map, Comb* comb, int comb_len) {
  unsigned int key = hash_comb(comb, comb_len);
  return _hash_contains(map, comb, comb_len, key);
}

void add_to_hash(HashMap map, Comb* comb, int comb_len, int generation) {
  unsigned int key = hash_comb(comb, comb_len);
  if (_hash_contains(map, comb, comb_len, key)) return;
  Comb* comb_copy = malloc(comb_len * sizeof(*comb_copy));
  memcpy(comb_copy, comb, comb_len * sizeof(*comb_copy));
  HashNode* node = malloc(sizeof(*node));
  node->length = comb_len;
  node->sorted = 0;
  node->comb   = comb_copy;
  node->generation = generation;
  node->next = map.content[key];
  map.content[key] = node;
}

// Adds |comb| to |map|, without copying it
void add_to_hash_no_copy(HashMap map, Comb* comb, int comb_len, int generation) {
  unsigned int key = hash_comb(comb, comb_len);
  HashNode* node = malloc(sizeof(*node));
  node->length = comb_len;
  node->sorted = 0;
  node->comb   = comb;
  node->generation = generation;
  node->next = map.content[key];
  map.content[key] = node;
}

// Adds elements from |src| into |dst|, while freeing |src| (and its content).
// Note: this is actually pretty expensive because hashes are very
// large... There is a tradeoff to be found between number of
// collisions and hash size...
void merge_hashes(HashMap dst, HashMap src) {
  for (int i = 0; i < HASH_SIZE; i++) {
    HashNode* node = src.content[i];
    while (node) {
      if (! _hash_contains(dst, node->comb, node->length, i)) {
        HashNode* next = node->next;
        node->next = dst.content[i];
        dst.content[i] = node;
        node = next;
      } else {
        HashNode* next = node->next;
        free(node->comb);
        free(node);
        node = next;
      }
    }
    src.content[i] = NULL;
  }
}


int hash_size(HashMap map) {
  int total = 0;
  for (int i = 0; i < HASH_SIZE; i++) {
    HashNode* curr = map.content[i];
    while (curr) {
      total++;
      curr = curr->next;
    }
  }
  return total;
}

void free_hash(HashMap map) {
  for (int i = 0; i < HASH_SIZE; i++) {
    HashNode* node = map.content[i];
    while (node) {
      free(node->comb);
      HashNode* next = node->next;
      free(node);
      node = next;
    }
  }
  free(map.content);
}
