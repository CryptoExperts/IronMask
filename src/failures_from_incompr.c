#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <inttypes.h>

#include "constructive-mult.h"
#include "circuit.h"
#include "combinations.h"
#include "list_tuples.h"
#include "verification_rules.h"
#include "trie.h"
#include "coeffs.h"

// For debug purposes only: the number of failures that are generated
// multiple times.
int regenerated = 0;




/* **************************************************************** */
/*                    Hash table implementation                     */
/* **************************************************************** */



// Small note regarding the size of the hash table. In the current
// implementation, the whole hash table has to be iterated through to
// compute each coefficients. This operation is a bit expensive for
// small coefficients (iterating millions of elements to find out that
// only 10 are non-null...). However, when the table starts to contain
// more elements, it's important to avoid collisions as much as
// possible, and thus to have a large table.
//
// Also, note that it would be a bit expansive to dynamically change
// the size of the hash map, since it would require recomputing the
// hash for each tuple it contains...
#define HASH_MASK 0x1ffffff // 500M -> times 8 (sizeof a pointer) = 4GB table
#define HASH_SIZE (HASH_MASK+1)

// Computes a hash for integer |x|.
// Hash function for integers from https://stackoverflow.com/a/12996028/4990392
static unsigned int hash_int(unsigned int x) {
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = (x >> 16) ^ x;
  return x;
}

// Computes a hash for |comb| by summing the hashes of each of its elements.
static unsigned int hash_comb(Comb* comb, int comb_len) {
  unsigned int hash = 0;
  for (int i = 0; i < comb_len; i++) {
    hash += hash_int(comb[i]);
  }
  return hash & HASH_MASK;
}

// |hash| is the hash of a Comb*, and |x| an element that we would
// like to add to that Comb*. This function returns the hash of the
// new Comb* (without having to recompute it entirely)
static unsigned int update_hash(unsigned int hash, int x) {
  return (hash + hash_int(x)) & HASH_MASK;
}

typedef struct _hashnode {
  Comb* comb;
  struct _hashnode* next;
} HashNode;

typedef struct _hashmap {
  HashNode** content;
  unsigned int comb_len; // The size of the tuples inside this hash
  int count; // The number of elements that were added to this hashmap
} HashMap;

// Allocates and initializes an empty hash map capable of holding
// elements of size |comb_len|.
static HashMap* init_hash(int comb_len) {
  HashMap* map = malloc(sizeof(*map));
  map->content  = calloc(HASH_SIZE, sizeof(*(map->content)));
  map->comb_len = comb_len;
  map->count    = 0;
  return map;
}

// Frees all elements contained in |map|, but does not free |map| itself.
static void empty_hash(HashMap* map, int verbose) {
  int used_buckets = 0;
  int collisions = 0;
  for (int i = 0; i < HASH_SIZE; i++) {
    HashNode* node = map->content[i];
    if (node) used_buckets++;
    while (node) {
      HashNode* next = node->next;
      free(node->comb);
      free(node);
      if (next) collisions++;
      node = next;
    }
    map->content[i] = NULL;
  }
  map->count = 0;

  if (verbose > 5) {
    printf("Comb_len=%d ---> map used at %d%%  ---  collisions:%d%%.\n", map->comb_len,
           (int)(((double)used_buckets / (HASH_SIZE)) * 100),
           (int)(((double)collisions / used_buckets) * 100));
  }
}

// Frees the content of |map|, as well as |map| itself.
static void free_hash(HashMap* map, int verbose) {
  empty_hash(map, verbose);
  free(map->content);
  free(map);
}

// Adds |comb| to |map| at index |hash|. Do not check whether it's
// already in it or not.
// Assumes that |comb| is already sorted.
static void add_to_hash_with_key(HashMap* map, Comb* comb, unsigned int hash) {
  HashNode* old = map->content[hash];
  HashNode* new = malloc(sizeof(*new));
  new->comb = comb;
  new->next = old;
  map->content[hash] = new;
  map->count++;
}

// Adds |comb| to |map| without checking wether it is already in it or not.
// Assumes that |comb| is already sorted.
/* static void add_to_hash(HashMap* map, Comb* comb) { */
/*   unsigned int hash = hash_comb(comb, map->comb_len); */
/*   add_to_hash_with_key(map, comb, hash); */
/* } */

// Adds |comb| to |map| only if it is not already in it.
// Assumes that |comb| is already sorted.
static void add_to_hash_checked(HashMap* map, Comb* comb) {
  unsigned int hash = hash_comb(comb, map->comb_len);
  HashNode* node = map->content[hash];
  while (node) {
    if (memcmp(node->comb, comb, map->comb_len * sizeof(*comb)) == 0) {
      return;
    }
    node = node->next;
  }
  add_to_hash_with_key(map, comb, hash);
}

// Adds in |map| the incompressible tuples of size |size| contained in
// |incompr|.
static void add_incompr_to_map(HashMap* map, Trie* incompr, int size) {
  ListComb* incompr_list = list_from_trie(incompr, size);
  ListCombElem* curr = incompr_list->head;
  while (curr) {
    sort_comb(curr->comb, size);
    // Note: add_to_hash could probably have been used instead of the
    // checked version, since incompressible tuples can not be built
    // from smaller tuples, which means that |map| cannot already
    // contain |curr->comb|. (I've used _checked just in case, in
    // order to avoid any potential bug...)
    add_to_hash_checked(map, curr->comb);

    ListCombElem* next = curr->next;
    // Not freeing curr->comb since it now is in the hashmap
    free(curr);
    curr = next;
  }
}



/* **************************************************************** */
/*                         Failures generation                      */
/* **************************************************************** */



// Update the coefficients |coeffs| with the tuples contained in |map|.
void update_coeffs_with_hash(const Circuit* c, uint64_t* coeffs, HashMap* map) {
  int comb_len = map->comb_len;
  for (int i = 0; i < HASH_SIZE; i++) {
    HashNode* node = map->content[i];
    while (node) {
      update_coeff_c_single(c, coeffs, node->comb, comb_len);
      node = node->next;
    }
  }
}

// Checks if the tuple (|comb|, |x|) is in |dst| at index |hash|. If
// not, then this tuple is added to |dst|. |comb_len| is the length of
// |comb|.
//
// The code is somewhat not straightfoward because it does not build
// the tuple (|comb|, |x|) to check whether its in |dst| or not (in
// order to avoid mallocing too much).
void check_comb_and_add(HashMap* dst, unsigned int hash,
                        Comb* comb, int x, int comb_len) {
  // Part 1: check if the tuple (|comb|, |x|) is in |dst|
  HashNode* node = dst->content[hash];
  while (node) {
    Comb* node_comb = node->comb;
    bool is_same = true;
    for (int i = 0, j = 0; i < comb_len+1; i++) {
      if (node_comb[i] != x) {
        if (node_comb[i] != comb[j]) {
          is_same = false;
          break;
        } else {
          j++;
        }
      }
    }

    if (is_same) {
      regenerated++;
      return;
    } else {
      node = node->next;
    }
  }

  // Part 2: the tuple is not already in the hash -> build it now.
  Comb* new_comb = malloc((comb_len+1) * sizeof(*new_comb));
  int i = 0;
  while (i < comb_len && comb[i] < x) {
    new_comb[i] = comb[i];
    i++;
  }
  new_comb[i++] = x;
  while (i < comb_len+1) {
    new_comb[i] = comb[i-1];
    i++;
  }

  // Part 3: add the tuple to the hash.
  HashNode* new_node = malloc(sizeof(*new_node));
  new_node->comb     = new_comb;
  new_node->next     = dst->content[hash];
  dst->content[hash]  = new_node;
  dst->count++;
}

// This function considers all super-tuples of |comb| with 1 more
// element that |comb|. For each of those tuples, it calls
// check_comb_and_add, which will add them to |dst| if they are not
// already in it.
//
// The non-optimized pseudo-code of this function is:
//
//    for i = 0 to var_count-1:
//        if comb does not contain i:
//            if dst does not contain the tuple (comb, i):
//                add (comb, i) to dst
//
// We take advantage of the fact that |comb| is sorted (by
// construction) to optimize this pseudo-code:
//
//  - we can thus avoid the step "if comb does not contain i",
//    which would otherwise have a linear cost in |comb_len|
//
//  - we do not have to generate the tuple (|comb|, |i|) to check
//    if it is in |dst|.
//
void expand_tuple(HashMap* dst, unsigned int hash, Comb* comb, int comb_len, int var_count) {
  int first = comb[0];
  int last  = comb[comb_len-1];
  // Adding elements at the start
  for (int i = 0; i < first; i++) {
    int new_hash = update_hash(hash, i);
    check_comb_and_add(dst, new_hash, comb, i, comb_len);
  }
  // Adding elements in the middle
  for (int j = 0; j < comb_len-1; j++) {
    for (int i = comb[j]+1; i < comb[j+1]; i++) {
      int new_hash = update_hash(hash, i);
      check_comb_and_add(dst, new_hash, comb, i, comb_len);
    }
  }
  // Adding elements at the end
  for (int i = last+1; i < var_count; i++) {
    int new_hash = update_hash(hash, i);
    check_comb_and_add(dst, new_hash, comb, i, comb_len);
  }
}

// For each tuple in |curr|, this function will generate all
// super-tuples with 1 more elements and add them in |next| if they
// are not already in it.
void expand_tuples(HashMap* curr, HashMap* next, int var_count) {
  int comb_len = curr->comb_len;

  for (int i = 0; i < HASH_SIZE; i++) {
    HashNode* node = curr->content[i];
    while (node) {
      expand_tuple(next, i, node->comb, comb_len, var_count);
      node = node->next;
    }
  }
}

// Pseudo-code:
//
//  procedure gen_failures(_incompr_):   # _incompr_ is the trie of incompressible failures
//  |
//  |   _curr_ = {}
//  |   for i = 1 to max size of tuple in |incompr|:
//  |   |   _next_ = {}
//  |   |   for failure _f_ in _curr_:
//  |   |   |   for each number _n_ in _1_.._numberOfVariables_:
//  |   |   |   |   _t_ = (_f_, _n_)
//  |   |   |   |   if _t_ \notin _next_:
//  |   |   |   |   |   add _t_ to _next_
//  |   |   add all elements of size _i_ of _incompr_ in _next_
//  |   |   Count elements in _next_    # That's the i-th coeff
//  |   |   _curr_ = _next_
//
void compute_failures_from_incompressibles(const Circuit* c, Trie* incompr,
                                           int coeff_max, int verbose) {
  int var_count = c->length;
  int concise = verbose < 5;

  uint64_t coeffs[c->total_wires+1];
  for (int i = 0; i <= c->total_wires; i++) {
    coeffs[i] = 0;
  }
  if (coeff_max == -1) coeff_max = c->total_wires+1;


  if (concise) {
    printf("[ ");
    fflush(stdout);
  }

  HashMap* curr = init_hash(1);
  HashMap* next = init_hash(0);
  for (int i = 0; i < coeff_max; i++) {
    next->comb_len = i+1;
    expand_tuples(curr, next, var_count);

    add_incompr_to_map(next, incompr, i+1);
    update_coeffs_with_hash(c, coeffs, next);
    if (concise) {
      printf("%"PRIu64", ", coeffs[i+1]);
      fflush(stdout);
    } else {
      printf("c%d = %"PRIu64"\n", i+1, coeffs[i+1]);

      printf("Regenerated: %d%% (%d / %d)\n",
             (int)((double)regenerated/next->count*100),
             regenerated, next->count);
      regenerated = 0;
    }

    empty_hash(curr, verbose);
    HashMap* tmp = curr;
    curr = next;
    next = tmp;
  }

  if (concise) {
    for (int i = coeff_max+1; i < c->total_wires-1; i++) {
      printf("%"PRIu64", ", coeffs[i]);
    }
    printf("%"PRIu64" ]\n", coeffs[c->total_wires]);
  } else {
    for (int i = coeff_max+1; i < c->total_wires; i++) {
      printf("c%d = %"PRIu64"\n", i, coeffs[i]);
    }
  }

  free_hash(curr, verbose);
  free_hash(next, verbose);



  double p_min = compute_leakage_proba(coeffs, coeff_max,
                                       c->total_wires+1,
                                       1, // minimax
                                       false); // square root
  double p_max = compute_leakage_proba(coeffs, coeff_max,
                                       c->total_wires+1,
                                       -1, // minimax
                                       false); // square root

  printf("\n");
  printf("pmax = %.10f -- log2(pmax) = %.10f\n", p_max, log2(p_max));
  printf("pmin = %.10f -- log2(pmin) = %.10f\n", p_min, log2(p_min));
  printf("\n");
}
