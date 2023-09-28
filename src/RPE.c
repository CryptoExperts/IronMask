#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include "RPE.h"
#include "config.h"
#include "circuit.h"
#include "list_tuples.h"
#include "combinations.h"
#include "coeffs.h"
#include "verification_rules.h"

#define COEFFS_COUNT    4
#define I1_or_I2        0
#define I1              1
#define I2              2
#define I1_and_I2       3

#define I1_STR "I1"
#define I2_STR "I2"
#define I1_or_I2_STR "I1_or_I2"
#define I1_and_I2_STR "I1_and_I2"

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))
static double min_arr(double* arr, int len) {
  double min_val = arr[0];
  for (int i = 1; i < len; i++) min_val = min(min_val, arr[i]);
  return min_val;
}


/*************************************************

   HashTable implementation, used for RPE2


  TODO: make this implementation more generic so that it can be used
  by failures_from_incompr as well.

**************************************************/



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

typedef struct _hashnode {
  Comb* comb;
  int comb_len;
  int count;
  struct _hashnode* next;
} HashNode;

typedef struct _hashmap {
  HashNode** content;
  int count; // The number of elements that are in this hashmap
  int collisions; // The number of collisions
} HashMap;

// Allocates and initializes an empty hash map.
static HashMap* init_hash() {
  HashMap* map    = malloc(sizeof(*map));
  map->content    = calloc(HASH_SIZE, sizeof(*(map->content)));
  map->count      = 0;
  map->collisions = 0;
  return map;
}


static HashNode* hash_contains_keyed(HashMap* map, Comb* comb, int comb_len, int hash) {
  HashNode* node = map->content[hash];
  while (node) {
    if (comb_len == node->comb_len &&
        memcmp(node->comb, comb, comb_len * sizeof(*comb)) == 0) {
      return node;
    }
    node = node->next;
  }
  return NULL;
}

// Adds |comb| to |map|.
static Comb* add_to_hash_keyed(HashMap* map, Comb* comb, int comb_len, int hash,
                               int realloc_comb) {
  HashNode* node = hash_contains_keyed(map, comb, comb_len, hash);
  if (node) {
    node->count++;
    return node->comb;
  } else {
    node = malloc(sizeof(*node));
    Comb* comb_copy = comb;
    if (realloc_comb) {
      comb_copy = malloc(comb_len * sizeof(*comb_copy));
      memcpy(comb_copy, comb, comb_len * sizeof(*comb_copy));
    }
    node->comb = comb_copy;
    node->comb_len = comb_len;
    node->count = 1;
    node->next = map->content[hash];
    map->content[hash] = node;
    return comb_copy;
  }
}

// Adds |comb| to |map|. If |realloc_comb| is true, then another comb
// is malloced before adding it to the hash. Otherwise, |comb| is
// directly added in the hash. This is useful because for RPE2, combs
// can be put in at most 4 hashes: I1_or_I2, I1, I2 and
// I1_and_I2. It's thus less time-consuming and less memory-consuming
// to only malloc the comb once for I1_or_I2 and reuse it as is in the
// other tables.
static Comb* add_to_hash(HashMap* map, Comb* comb, int comb_len, int realloc_comb) {
  unsigned int hash = hash_comb(comb, comb_len);
  return add_to_hash_keyed(map, comb, comb_len, hash, realloc_comb);
}

// Removes from |map| all element whose count is 1. If |free_comb| is
// true, then each comb's ->comb member is freed, otherwise they
// aren't.
static void remove_count_1(HashMap* map, int free_comb) {
  for (int i = 0; i < HASH_SIZE; i++) {
    HashNode* prev = NULL;
    HashNode* node = map->content[i];
    while (node) {
      HashNode* next = node->next;
      if (node->count == 1) {
        if (free_comb) free(node->comb);
        if (prev) {
          prev->next = next;
        } else {
          map->content[i] = next;
        }
        free(node);
      } else {
        prev = node;
      }
      node = next;
    }
  }
}

// Removes from |map| all element whose count is |target|. If
// |free_comb| is true, then each comb's ->comb member is freed,
// otherwise they aren't.
static void remove_count_diff(HashMap* map, int target, int free_comb) {
  for (int i = 0; i < HASH_SIZE; i++) {
    HashNode* prev = NULL;
    HashNode* node = map->content[i];
    while (node) {
      HashNode* next = node->next;
      if (node->count != target) {
        if (free_comb) free(node->comb);
        if (prev) {
          prev->next = next;
        } else {
          map->content[i] = next;
        }
        free(node);
      } else {
        prev = node;
      }
      node = next;
    }
  }
}

// Removes from |map| all elements whose length is |n|. If |free_comb|
// is true, then each comb's ->comb member is freed, otherwise they
// aren't.
static void remove_len_n(HashMap* map, int n, int free_comb) {
  for (int i = 0; i < HASH_SIZE; i++) {
    HashNode* prev = NULL;
    HashNode* node = map->content[i];
    while (node) {
      HashNode* next = node->next;
      if (node->comb_len == n) {
        if (free_comb) free(node->comb);
        if (prev) {
          prev->next = next;
        } else {
          map->content[i] = next;
        }
        free(node);
      } else {
        prev = node;
      }
      node = next;
    }
  }
}

// Empties |map| without freeing |map| itself or its ->content
// member. If |free_comb| is true, then each comb's ->comb member is
// freed, otherwise they aren't.
static void empty_hash(HashMap* map, int free_comb) {
  for (int i = 0; i < HASH_SIZE; i++) {
    HashNode* node = map->content[i];
    while (node) {
      HashNode* next = node->next;
      if (free_comb) free(node->comb);
      free(node);
      node = next;
    }
    map->content[i] = NULL;
  }
}


/*************************************************

   Functions to compute RPE1 coefficients

**************************************************/

struct callback_data_RPE1 {
  int t;
  uint64_t** coeff_c;
};

static void update_coeffs_RPE(const Circuit* c, Comb* comb, int comb_len,
                              SecretDep* secret_deps,
                              void* data_void) {
  struct callback_data_RPE1* data = (struct callback_data_RPE1*) data_void;
  int t = data->t;
  uint64_t** coeff_c = data->coeff_c;

  int secret_count = c->secret_count;
  assert(secret_count <= 2);
  // I1_or_I2 can be updated without checking |secret_deps|, since at
  // least one has to be true, or |comb| would not be a failure and
  // this function would not be called.
  update_coeff_c_single(c, coeff_c[I1_or_I2], &comb[t], comb_len-t);

  if (secret_count > 1) {

    if (secret_deps[0]) {
      update_coeff_c_single(c, coeff_c[I1], &comb[t], comb_len-t);
    }

    if (secret_deps[1]) {
      update_coeff_c_single(c, coeff_c[I2], &comb[t], comb_len-t);
    }

    if (secret_deps[0] && secret_deps[1]) {
      update_coeff_c_single(c, coeff_c[I1_and_I2], &comb[t], comb_len-t);
    }
  }
}

// RPE1:
//
//
// High-level pseudo-code:
//
//     for each combination |C_o| of |t_output| output shares:
//       for each tuple |P| of size |t_output|+|k|:
//         if |P| is a failure, increment coefficients related to |C_o|
//     for each size 1 to coeff_max:
//       get the max of the coefficients of that size
//
// Note that in practice we distinguish "failure" into:
//  - failure for the first input
//  - failure for the second input
//  - failure for both inputs
//  - (not used in official definition, mostly for debuging): failure for either input.
//
uint64_t** compute_RPE1(Circuit* circuit, DimRedData* dim_red_data,
                        int cores, int coeff_max, int t, int t_output) {
  int secret_count = circuit->secret_count;
  int coeffs_count = secret_count == 1 ? 1 : COEFFS_COUNT;

  uint64_t** coeffs = malloc(coeffs_count * sizeof(*coeffs));
  for (int i = 0; i < coeffs_count; i++) {
    coeffs[i] = calloc(circuit->total_wires+1, sizeof(*coeffs[i]));
  }

  int coeff_max_main_loop = coeff_max == -1 ? circuit->length :
    coeff_max > circuit->length ? circuit->length : coeff_max;

  if (coeff_max == -1) {
    coeff_max = dim_red_data->old_circuit->length;
  }

  // Generating combinations of |t| elements corresponding to the outputs
  uint64_t out_comb_len;
  Comb** out_comb_arr;
  if (circuit->output_count == 1) {
    out_comb_arr = gen_combinations(&out_comb_len, t_output,
                                    circuit->share_count - 1);
    for (unsigned int i = 0; i < out_comb_len; i++) {
      for (int k = 0; k < t_output; k++) {
        out_comb_arr[i][k] += circuit->length;
      }
    }
  } else { // copy gadget with 2 outputs
    // TODO: this assumes that the 2 outputs are one after then others
    // in the input file. We should make sure in the parser that this
    // is the case.
    uint64_t out_comb_len_1, out_comb_len_2;
    Comb **out_comb_arr_1, **out_comb_arr_2;
    out_comb_arr_1 = gen_combinations(&out_comb_len_1, t_output,
                                      circuit->share_count - 1);
    out_comb_arr_2 = gen_combinations(&out_comb_len_2, t_output,
                                      circuit->share_count - 1);
    assert(out_comb_len_1 == out_comb_len_2);
    for (unsigned int i = 0; i < out_comb_len_1; i++) {
      for (int k = 0; k < t_output; k++) {
        out_comb_arr_1[i][k] += circuit->length;
        out_comb_arr_2[i][k] += circuit->length + circuit->share_count;
      }
    }
    // Generating tuples from the carthesian product of
    // |out_comb_arr_1| and |out_comb_arr_2|
    out_comb_len = out_comb_len_1 * out_comb_len_2;
    out_comb_arr = malloc(out_comb_len * sizeof(*out_comb_arr));
    int out_comb_arr_idx = 0;
    for (unsigned i = 0; i < out_comb_len_1; i++) {
      for (unsigned j = 0; j < out_comb_len_2; j++) {
        out_comb_arr[out_comb_arr_idx] = malloc(t_output*2 * sizeof(**out_comb_arr));
        memcpy(&out_comb_arr[out_comb_arr_idx][0], out_comb_arr_1[i],
               t_output * sizeof(**out_comb_arr_1));
        memcpy(&out_comb_arr[out_comb_arr_idx][t_output], out_comb_arr_2[j],
               t_output * sizeof(**out_comb_arr_2));
        out_comb_arr_idx++;
      }
    }
    t_output *= 2;
  }

  uint64_t*** coeffs_out_comb;
  coeffs_out_comb = malloc(out_comb_len * sizeof(*coeffs_out_comb));
  for (unsigned i = 0; i < out_comb_len; i++) {
    coeffs_out_comb[i] = malloc(coeffs_count * sizeof(*coeffs_out_comb[i]));
    for (int j = 0; j < coeffs_count; j++) {
      coeffs_out_comb[i][j] = calloc(circuit->total_wires + 1, sizeof(*coeffs_out_comb[i][j]));
    }
  }

  struct callback_data_RPE1 data = { .t = t_output, .coeff_c = NULL };
  VarVector verif_prefix = { .length = t_output, .max_size = t_output, .content = NULL };

  for (int size = 0; size <= coeff_max_main_loop; size++) {

    for (unsigned int i = 0; i < out_comb_len; i++) {
      verif_prefix.content = out_comb_arr[i];
      data.coeff_c = coeffs_out_comb[i];

      find_all_failures(circuit,
                        cores,
                        t, // t_in
                        &verif_prefix,  // prefix
                        size+verif_prefix.length, // comb_len
                        coeff_max+verif_prefix.length, // max_len
                        dim_red_data,  // dim_red_data
                        true,  // has_random
                        NULL,  // first_comb
                        false, // include_outputs
                        0,     // shares_to_ignore
                        false, // PINI
                        NULL, // incompr_tuples
                        update_coeffs_RPE,
                        (void*)&data);
    }

    for (int i = 0; i < coeffs_count; i++) {
      for (unsigned j = 0; j < out_comb_len; j++) {
#define max(a,b) ((a) > (b) ? (a) : (b))
        coeffs[i][size] = max(coeffs[i][size], coeffs_out_comb[j][i][size]);
      }
    }
  }

  for (int size = coeff_max_main_loop+1; size <= circuit->total_wires; size++) {
    for (int i = 0; i < coeffs_count; i++) {
      for (unsigned j = 0; j < out_comb_len; j++) {
        coeffs[i][size] = max(coeffs[i][size], coeffs_out_comb[j][i][size]);
      }
    }
  }

  printf("REP1- I1_or_I2: [ ");
  for (int i = 0; i < circuit->total_wires; i++)
    printf("%llu, ", coeffs[I1_or_I2][i]);
  printf("]\n");

  if (coeffs_count > 1) {
    printf("REP1- I1: [ ");
    for (int i = 0; i < circuit->total_wires; i++)
      printf("%llu, ", coeffs[I1][i]);
    printf("]\n");
    printf("REP1- I2: [ ");
    for (int i = 0; i < circuit->total_wires; i++)
      printf("%llu, ", coeffs[I2][i]);
    printf("]\n");
    printf("REP1- I1_and_I2: [ ");
    for (int i = 0; i < circuit->total_wires; i++)
      printf("%llu, ", coeffs[I1_and_I2][i]);
    printf("]\n");
  }
  printf("\n");

  for (unsigned i = 0; i < out_comb_len; i++) {
    free(out_comb_arr[i]);
  }
  free(out_comb_arr);
  for (unsigned i = 0; i < out_comb_len; i++) {
    for (int j = 0; j < coeffs_count; j++) {
      free(coeffs_out_comb[i][j]);
    }
    free(coeffs_out_comb[i]);
  }
  free(coeffs_out_comb);

  return coeffs;
}


/*************************************************

   Functions to compute RPE2 coefficients

**************************************************/

// The problem: the trie is meant to store either incompressible
// tuples or tuples of the same size. In particular, it does not
// handle tuples that are subtuples of other tuples in the trie, since
// the condition used to determine if the end of a tuple is reached is
// "does it have children?".


struct callback_data_RPE2 {
  int base_size;
  HashMap** failures;
  int count;
  bool low_memory;
  // The variables below are used only if |low_memory| is true.
  Circuit* circuit;
  DimRedData* dim_red_data;
  int t_in;
  uint64_t out_comb_len;
  Comb** out_comb_arr;
  uint64_t** coeffs;
};

void save_failure_to_map(const Circuit* c, Comb* comb, int comb_len,
                         SecretDep* secret_deps,
                         void* data_void) {
  struct callback_data_RPE2* data = (struct callback_data_RPE2*) data_void;
  int base_size = data->base_size;
  HashMap** failures = data->failures;
  int count = data->count;
  int secret_count = c->secret_count;

  // Note: no need to insert curr->secret_deps in the tries, since
  // each trie corresponds to a given secret_deps. For instance,
  // when inserting in failures[I1_or_I2], we know that one of the 2
  // is a failure (and we don't care which one).

  if (count == 0) {
    Comb* comb_malloced = add_to_hash(failures[I1_or_I2], &comb[base_size],
                                      comb_len-base_size, 1);
    if (secret_count > 1) {
      if (secret_deps[0]) {
        add_to_hash(failures[I1], comb_malloced, comb_len-base_size, 0);
      }
      if (secret_deps[1]) {
        add_to_hash(failures[I2], comb_malloced, comb_len-base_size, 0);
      }
      if (secret_deps[0] && secret_deps[1]) {
        add_to_hash(failures[I1_and_I2], comb_malloced, comb_len-base_size, 0);
      }
    }
  } else {
    int hash = hash_comb(&comb[base_size], comb_len-base_size);
    if (hash_contains_keyed(failures[I1_or_I2], &comb[base_size], comb_len-base_size, hash)) {
      Comb* comb_malloced = add_to_hash_keyed(failures[I1_or_I2], &comb[base_size],
                                              comb_len-base_size, hash, 1);
      if (secret_count > 1) {
        if (secret_deps[0]) {
          add_to_hash_keyed(failures[I1], comb_malloced, comb_len-base_size, hash, 0);
        }
        if (secret_deps[1]) {
          add_to_hash_keyed(failures[I2], comb_malloced, comb_len-base_size, hash, 0);
        }
        if (secret_deps[0] && secret_deps[1]) {
          add_to_hash_keyed(failures[I1_and_I2], comb_malloced, comb_len-base_size, hash, 0);
        }
      }
    } else {
      // Not already in the hash, no need to add it; nothing to do
    }
  }
}

void update_coeffs_from_maps(Circuit* c, uint64_t** coeff_c, HashMap** maps,
                              int comb_len, int coeffs_count) {
  for (int i = 0; i < coeffs_count; i++) {
    HashMap* map = maps[i];
    for (int j = 0; j < HASH_SIZE; j++) {
      HashNode* node = map->content[j];
      while (node) {
        if (node->comb_len == comb_len) {
          update_coeff_c_single(c, coeff_c[i], node->comb, comb_len);
        }
        node = node->next;
      }
    }
  }
}


void check_failure_and_update_coeffs(const Circuit* c, Comb* comb, int comb_len,
                                     SecretDep* secret_deps,
                                     void* data_void) {
  struct callback_data_RPE2* data = (struct callback_data_RPE2*) data_void;
  int base_size = data->base_size;
  int secret_count = c->secret_count;

  uint64_t** coeffs = data->coeffs;
  uint64_t out_comb_len = data->out_comb_len;
  Comb** out_comb_arr = data->out_comb_arr;
  SecretDep secret_deps_other[2];


  Comb new_comb[comb_len];
  memcpy(&new_comb[base_size], &comb[base_size], (comb_len-base_size) * sizeof(*new_comb));
  for (unsigned i = 1; i < out_comb_len && (secret_deps[0] || secret_deps[1]); i++) {
    memcpy(new_comb, out_comb_arr[i], base_size * sizeof(*new_comb));
    memset(secret_deps_other, 0, 2 * sizeof(*secret_deps_other));
    if (!is_failure(data->dim_red_data->old_circuit,
                    data->t_in, // t_in
                    comb_len, // comb_len
                    new_comb, // comb
                    true, // has_random
                    secret_deps_other, //secret_deps
                    NULL // incompr_tuples
                    )) {
      return;
    }
    secret_deps[0] &= secret_deps_other[0];
    secret_deps[1] &= secret_deps_other[1];
  }
  if (! (secret_deps[0] || secret_deps[1])) {
    return;
  }
  update_coeff_c_single(c, coeffs[I1_or_I2], &comb[base_size], comb_len-base_size);
  if (secret_count > 1) {
    if (secret_deps[0]) {
      update_coeff_c_single(c, coeffs[I1], &comb[base_size], comb_len-base_size);
    }
    if (secret_deps[1]) {
      update_coeff_c_single(c, coeffs[I2], &comb[base_size], comb_len-base_size);
    }
    if (secret_deps[0] && secret_deps[1]) {
      update_coeff_c_single(c, coeffs[I1_and_I2], &comb[base_size], comb_len-base_size);
    }
  }
}



// RPE2:
//
//
// High-level pseudo-code:
//
//     for each combination of outputs |C_o| of size |circuit->share_count - 1|:
//       store all failures that contain |C_o|
//     coefficients = number a failures that are failures for all |C_o|
//
// Note that once again, we distinguish failures on one input, one the
// other one and on both.
//
// Because we have to store all failures when checking RPE, memory
// consumption quickly becomes an issue. This, we use batching to only
// check batches of |BATCH_SIZE| tuples (|BATCH_SIZE| is defined in
// config.h, at the time of writting, it's 1 million). At most, we
// thus have to keep only 1 million tuples in the hashes, which should
// not be too much.
//
uint64_t** compute_RPE2(Circuit* circuit, DimRedData* dim_red_data,
                        int cores, int coeff_max, int t, int low_memory) {
  (void) cores; // Due to the batching, multithreading cannot be used.
  int secret_count = circuit->secret_count;
  int coeffs_count = secret_count == 1 ? 1 : COEFFS_COUNT;
  int t_output = circuit->share_count - 1;

  uint64_t** coeffs = malloc(coeffs_count * sizeof(*coeffs));
  for (int i = 0; i < coeffs_count; i++) {
    coeffs[i] = calloc(circuit->total_wires+1, sizeof(*coeffs[i]));
  }


  int coeff_max_main_loop = coeff_max == -1 ? circuit->length :
    coeff_max > circuit->length ? circuit->length : coeff_max;

  if (coeff_max == -1) {
    coeff_max = dim_red_data->old_circuit->length;
  }

   // Generating combinations of |t| elements corresponding to the outputs
  uint64_t out_comb_len;
  Comb** out_comb_arr;
  if (circuit->output_count == 1) {
    out_comb_arr = gen_combinations(&out_comb_len, t_output,
                                    circuit->share_count - 1);
    for (unsigned int i = 0; i < out_comb_len; i++) {
      for (int k = 0; k < t_output; k++) {
        out_comb_arr[i][k] += circuit->length;
      }
    }
  } else { // copy gadget with 2 outputs
    // TODO: this assumes that the 2 outputs are one after then others
    // in the input file. We should make sure in the parser that this
    // is the case.
    uint64_t out_comb_len_1, out_comb_len_2;
    Comb **out_comb_arr_1, **out_comb_arr_2;
    out_comb_arr_1 = gen_combinations(&out_comb_len_1, t_output,
                                      circuit->share_count - 1);
    out_comb_arr_2 = gen_combinations(&out_comb_len_2, t_output,
                                      circuit->share_count - 1);
    assert(out_comb_len_1 == out_comb_len_2);
    for (unsigned int i = 0; i < out_comb_len_1; i++) {
      for (int k = 0; k < t_output; k++) {
        out_comb_arr_1[i][k] += circuit->length;
        out_comb_arr_2[i][k] += circuit->length + circuit->share_count;
      }
    }
    // Generating tuples from the carthesian product of
    // |out_comb_arr_1| and |out_comb_arr_2|
    out_comb_len = out_comb_len_1 * out_comb_len_2;
    out_comb_arr = malloc(out_comb_len * sizeof(*out_comb_arr));
    int out_comb_arr_idx = 0;
    for (unsigned i = 0; i < out_comb_len_1; i++) {
      for (unsigned j = 0; j < out_comb_len_2; j++) {
        out_comb_arr[out_comb_arr_idx] = malloc(t_output*2 * sizeof(**out_comb_arr));
        memcpy(&out_comb_arr[out_comb_arr_idx][0], out_comb_arr_1[i],
               t_output * sizeof(**out_comb_arr_1));
        memcpy(&out_comb_arr[out_comb_arr_idx][t_output], out_comb_arr_2[j],
               t_output * sizeof(**out_comb_arr_2));
        out_comb_arr_idx++;
      }
    }
    t_output *= 2;
  }

  if (low_memory) {
    // Updating all combinations of |out_comb_arr| (except the 1st
    // one) with indices of wires before dimension reduction.
    for (unsigned i = 1; i < out_comb_len; i++) {
      for (int j = 0; j < t_output; j++) {
        out_comb_arr[i][j] = dim_red_data->new_to_old_mapping[out_comb_arr[i][j]];
      }
    }
  }

  HashMap* all_failures[coeffs_count];
  for (int i = 0; i < coeffs_count; i++) {
    all_failures[i] = init_hash();
  }

  struct callback_data_RPE2 data = {
    .base_size = t_output,
    .failures = all_failures,
    .low_memory = low_memory,
    .t_in = t,
    .circuit = circuit,
    .dim_red_data = dim_red_data,
    .out_comb_len = out_comb_len,
    .out_comb_arr = out_comb_arr,
    .coeffs = coeffs
  };
  VarVector verif_prefix = { .length = t_output, .max_size = t_output, .content = NULL };

  for (int size = 0; size <= coeff_max_main_loop; size++) {

    if (low_memory) {
      // If |low_memory| is true, then tuples are exhaustively
      // considered for the first output combination only
      // (|out_comb_arr[0]|), and check_failure_and_update_coeffs will
      // check if the failures are failures for all other elements of
      // |out_comb_arr| before updating the coefficients.
      verif_prefix.content = out_comb_arr[0];
      data.count = 0;

      find_all_failures(circuit,
                        cores,
                        t, // t_in
                        &verif_prefix, // prefix
                        size+verif_prefix.length, // comb_len
                        coeff_max+verif_prefix.length, //max_len
                        dim_red_data, // dim_red_data
                        true,         // has_random
                        NULL,         // first_tuple
                        NULL,         // include_outputs
                        0,            // shares_to_ignore
                        false,        // PINI
                        NULL,         // incompr_tuples
                        check_failure_and_update_coeffs,
                        (void*)&data);
    } else {

      uint64_t total_combs = n_choose_k(size, circuit->length);

      for (uint64_t current_comb_idx = 0; current_comb_idx < total_combs;
           current_comb_idx += BATCH_SIZE) {
        printf("  + current_comb_idx = %llu / %llu\n", current_comb_idx, total_combs);
        Comb* current_comb = unrank(circuit->length, size, current_comb_idx);

        for (unsigned int i = 0; i < out_comb_len; i++) {
          printf("    - i = %d / %llu\n", i, out_comb_len);
          verif_prefix.content = out_comb_arr[i];
          data.count = i;

          _verify_tuples(circuit,
                         t, // t_in
                         &verif_prefix, // prefix
                         size+verif_prefix.length, // comb_len
                         coeff_max+verif_prefix.length, //max_len
                         dim_red_data, // dim_red_data
                         true,         // has_random
                         current_comb, // first_tuple
                         BATCH_SIZE,   // tuple_count
                         NULL,         // include_outputs
                         0,            // shares_to_ignore
                         false,        // PINI
                         0,            // stop_at_first_failure
                         0,            // only_one_tuple
                         NULL,         // secret_deps_out
                         NULL,         // incompr_tuples
                         save_failure_to_map,
                         (void*)&data);
        }

        for (int i = 0; i < coeffs_count; i++) {
          remove_count_diff(all_failures[i], out_comb_len, i == 0);
        }
        for (int i = size; i <= size+verif_prefix.length; i++) {
          update_coeffs_from_maps(circuit, coeffs, all_failures, i, coeffs_count);
        }
        for (int i = 0; i < coeffs_count; i++) {
          empty_hash(all_failures[i], i == 0);
        }

        free(current_comb);
      }
    }
  }

  for (int i = 0; i < coeffs_count; i++) {
    empty_hash(all_failures[i], i == 0);
    free(all_failures[i]->content);
    free(all_failures[i]);
  }

  printf("REP2- I1_or_I2: [ ");
  for (int i = 0; i < circuit->total_wires; i++)
    printf("%llu, ", coeffs[I1_or_I2][i]);
  printf("]\n");

  if (coeffs_count > 1) {
    printf("REP2- I1: [ ");
    for (int i = 0; i < circuit->total_wires; i++)
      printf("%llu, ", coeffs[I1][i]);
    printf("]\n");
    printf("REP2- I2: [ ");
    for (int i = 0; i < circuit->total_wires; i++)
      printf("%llu, ", coeffs[I2][i]);
    printf("]\n");
    printf("REP2- I1_and_I2: [ ");
    for (int i = 0; i < circuit->total_wires; i++)
      printf("%llu, ", coeffs[I1_and_I2][i]);
    printf("]\n");
  }
  printf("\n");

  for (unsigned i = 0; i < out_comb_len; i++) {
    free(out_comb_arr[i]);
  }
  free(out_comb_arr);

  return coeffs;
}



/*************************************************

   Functions to compute copy coefficients

**************************************************/


// RPE copy:
//
// If |first_output| == 0, then the 2nd output is considered first,
// otherwise the first one is.
uint64_t** compute_RPE_copy(Circuit* circuit, DimRedData* dim_red_data,
                            int cores, int coeff_max, int t, int first_output) {
  assert(circuit->secret_count == 1);
  int coeffs_count = 1;
  int t_output = circuit->share_count - 1;

  uint64_t** coeffs = malloc(coeffs_count * sizeof(*coeffs));
  for (int i = 0; i < coeffs_count; i++) {
    coeffs[i] = calloc(circuit->total_wires+1, sizeof(*coeffs[i]));
  }


  int coeff_max_main_loop = coeff_max == -1 ? circuit->length :
    coeff_max > circuit->length ? circuit->length : coeff_max;

  if (coeff_max == -1) {
    coeff_max = dim_red_data->old_circuit->length;
  }

   // Generating combinations of |t| elements corresponding to the outputs
  uint64_t out_comb_len_1, out_comb_len_2;
  Comb **out_comb_arr_1, **out_comb_arr_2;
  out_comb_arr_1 = gen_combinations(&out_comb_len_1, t,
                                    circuit->share_count - 1);
  out_comb_arr_2 = gen_combinations(&out_comb_len_2, t_output,
                                    circuit->share_count - 1);
  for (unsigned int i = 0; i < out_comb_len_1; i++) {
    for (int k = 0; k < t_output; k++) {
      out_comb_arr_1[i][k] += circuit->length + (first_output ? 0 : circuit->share_count);
    }
  }
  for (unsigned int i = 0; i < out_comb_len_2; i++) {
    for (int k = 0; k < t_output; k++) {
      out_comb_arr_2[i][k] += circuit->length + (first_output ? circuit->share_count : 0);
    }
  }

  HashMap* all_failures[1] = { init_hash() };

  struct callback_data_RPE2 data = {
    .base_size = t + t_output,
    .failures = all_failures,
    .low_memory = false
    // No need to set the other elements since low_memory is false
  };
  VarVector verif_prefix = { .length = t_output+t, .max_size = t_output+t,
    .content = malloc((t+t_output) * sizeof(*verif_prefix.content)) };

  uint64_t* local_coeffs = alloca((circuit->total_wires + 1) * sizeof(*local_coeffs));
  for (unsigned int i = 0; i < out_comb_len_1; i++) {
    memcpy(verif_prefix.content, out_comb_arr_1[i], t * sizeof(**out_comb_arr_1));
    memset(local_coeffs, 0, (circuit->total_wires + 1) * sizeof(*local_coeffs));

    for (int size = 0; size <= coeff_max_main_loop; size++) {

      for (unsigned int j = 0; j < out_comb_len_2; j++) {
        memcpy(&verif_prefix.content[t], out_comb_arr_2[j], t_output * sizeof(**out_comb_arr_2));

        find_all_failures(circuit,
                          cores,
                          t, // t_in
                          &verif_prefix,  // prefix
                          size+verif_prefix.length, // comb_len
                          coeff_max+verif_prefix.length, // max_len
                          dim_red_data,  // dim_red_data
                          true,  // has_random
                          NULL,  // first_comb
                          false, // include_outputs
                          0,     // shares_to_ignore
                          false, // PINI
                          NULL, // incompr_tuples
                          save_failure_to_map,
                          (void*)&data);

        if (j == 1) {
          remove_count_1(all_failures[0], 1);
        }
      }

      remove_count_diff(all_failures[0], out_comb_len_2, 1);
      update_coeffs_from_maps(circuit, &local_coeffs, all_failures, size, coeffs_count);

      // Removing failures of size |size| to keep the memory and the
      // collisions as low as possible.
      remove_len_n(all_failures[0], size, 1);
    }

    for (int c = 0; c < circuit->total_wires+1; c++) {
      coeffs[0][c] = max(coeffs[0][c], local_coeffs[c]);
    }

    empty_hash(all_failures[0], 1);
  }

  printf("REP%d%d- I1_or_I2: [ ", first_output ? 1 : 2, first_output ? 2 : 1);
  for (int i = 0; i < circuit->total_wires; i++)
    printf("%llu, ", coeffs[0][i]);
  printf("]\n\n");

  for (unsigned i = 0; i < out_comb_len_1; i++) {
    free(out_comb_arr_1[i]);
  }
  free(out_comb_arr_1);
  for (unsigned i = 0; i < out_comb_len_2; i++) {
    free(out_comb_arr_2[i]);
  }
  free(out_comb_arr_2);

  return coeffs;
}






void compute_RPE_coeffs(Circuit* circuit, int cores, int coeff_max, int t, int t_output) {

  DimRedData* dim_red_data = remove_elementary_wires(circuit);

  uint64_t** coeffs_RPE1 = compute_RPE1(circuit, dim_red_data, cores, coeff_max, t, t_output);
  uint64_t** coeffs_RPE2 = compute_RPE2(circuit, dim_red_data, cores, coeff_max, t, true);

  uint64_t **coeffs_RPE12 = NULL, **coeffs_RPE21 = NULL;
  if (circuit->output_count == 2) {
    coeffs_RPE12 = compute_RPE_copy(circuit, dim_red_data, cores, coeff_max, t, 1);
    coeffs_RPE21 = compute_RPE_copy(circuit, dim_red_data, cores, coeff_max, t, 0);
  }

  // Compute amplification order
  int d1 = 0, d2 = 0, d12 = 0;
  double c_d1 = 0, c_d2 = 0, c_d12 = 0;
  for (int i = 0; i < circuit->total_wires+1; i++) {
    if (d1 && d2 && d12) break;
    if (d1 && circuit->secret_count == 1) break;

    if (circuit->secret_count == 1) {
      if (coeffs_RPE1[I1_or_I2][i] || coeffs_RPE2[I1_or_I2][i]) {
        d1 = i;
        c_d1 = max(coeffs_RPE1[I1_or_I2][i], coeffs_RPE2[I1_or_I2][i]);
        break;
      }
      if (circuit->output_count == 2) {
        if (coeffs_RPE1[I1_or_I2][i] || coeffs_RPE2[I1_or_I2][i] ||
            coeffs_RPE12[I1_or_I2][i] || coeffs_RPE21[I1_or_I2][i]) {
          d1 = i;
          break;
        }
      }
    } else { // secret_count == 2 (add or mult)
      if (!d1 && (coeffs_RPE1[I1][i] || coeffs_RPE2[I1][i])) {
        d1 = i;
        c_d1 = max(coeffs_RPE1[I1][i], coeffs_RPE2[I1][i]);
      }
      if (circuit->secret_count == 2) {
        if (!d2 && (coeffs_RPE1[I2][i] || coeffs_RPE2[I2][i])) {
          d2 = i;
          c_d2 = max(coeffs_RPE1[I2][i], coeffs_RPE2[I2][i]);
        }
        if (!d12 && (coeffs_RPE1[I1_and_I2][i] || coeffs_RPE2[I1_and_I2][i])) {
          d12 = i;
          c_d12 = sqrt(max(coeffs_RPE1[I1_and_I2][i], coeffs_RPE2[I1_and_I2][i]));
        }
      }
    }
  }
  int d = 0;
  double cd = 0;
  int div_by_2 = 0;
  if (circuit->secret_count == 1 && circuit->output_count == 2) { // copy
    d = d1;
    cd = 0;
  } else if (circuit->secret_count == 1 && circuit->output_count == 1) { // refresh
    d = d1;
    cd = c_d1;
  } else if (circuit->secret_count == 2 && circuit->output_count == 1) { // add/mult
    if (d1 < d2) {
      d = d1;
      cd = c_d1;
    } else if (d2 > d1) {
      d = d2;
      cd = c_d2;
    } else {
      d = d1;
      cd = max(c_d1, c_d2);
    }
    if ((double)d > ((double)d12)/2) {
      if ((d12/2) * 2 == d12) {
        d = d12/2;
      } else {
        d = d12;
        div_by_2 = 1;
      }
      cd = c_d12;
    } else if ((double)d == ((double)d12)/2) {
      cd = max(cd, c_d12);
    }
  }

  printf("Amplification order d = %d%s\n", d, div_by_2 ? "/2" : "");
  if (circuit->output_count == 1) {
    printf("Coeff c%d%s = %f\n", d, div_by_2 ? "/2" : "", cd);
  }
  printf("\n");

  if (coeff_max == -1) {
    coeff_max = dim_red_data->old_circuit->length;
  }

  // Computing leakage probability from coefficients
  double p[2];
  for (int i = 0; i < 2; i++) {
    // i == 0 --> replace last coeffs by 0
    // i == 1 --> replace last coeffs by (n choose k)
    int min_max = i == 0 ? -1 : 1;
    if (circuit->secret_count == 2) {
      double p_arr[6] = {
        compute_leakage_proba(coeffs_RPE1[I1], coeff_max,
                              circuit->total_wires+1, min_max, false),
        compute_leakage_proba(coeffs_RPE1[I2], coeff_max,
                              circuit->total_wires+1, min_max, false),
        compute_leakage_proba(coeffs_RPE1[I1_and_I2], coeff_max,
                              circuit->total_wires+1, min_max, true),
        compute_leakage_proba(coeffs_RPE2[I1], coeff_max,
                              circuit->total_wires+1, min_max, false),
        compute_leakage_proba(coeffs_RPE2[I2], coeff_max,
                              circuit->total_wires+1, min_max, false),
        compute_leakage_proba(coeffs_RPE2[I1_and_I2], coeff_max,
                              circuit->total_wires+1, min_max, true) };
      p[i] = min_arr(p_arr, 6);
    } else if (circuit->output_count == 2) {
      double p_arr[4] = {
        compute_leakage_proba(coeffs_RPE1[I1_or_I2], coeff_max,
                              circuit->total_wires+1, min_max, false),
        compute_leakage_proba(coeffs_RPE2[I1_or_I2], coeff_max,
                              circuit->total_wires+1, min_max, false),
        compute_leakage_proba(coeffs_RPE12[I1_or_I2], coeff_max,
                              circuit->total_wires+1, min_max, false),
        compute_leakage_proba(coeffs_RPE21[I1_or_I2], coeff_max,
                              circuit->total_wires+1, min_max, false) };
      p[i] = min_arr(p_arr, 4);
    } else {
      double p_arr[2] = {
        compute_leakage_proba(coeffs_RPE1[I1_or_I2], coeff_max,
                              circuit->total_wires+1, min_max, false),
        compute_leakage_proba(coeffs_RPE2[I1_or_I2], coeff_max,
                              circuit->total_wires+1, min_max, false) };
      p[i] = min_arr(p_arr, 2);
    }
  }
  printf("pmax = %.10f -- log2(pmax) = %.10f\n", p[0], log2(p[0]));
  printf("pmin = %.10f -- log2(pmin) = %.10f\n", p[1], log2(p[1]));
  printf("\n");

  free(coeffs_RPE1[I1_or_I2]);
  free(coeffs_RPE2[I1_or_I2]);

  if (circuit->secret_count == 2) {
    free(coeffs_RPE1[I1]);
    free(coeffs_RPE1[I2]);
    free(coeffs_RPE1[I1_and_I2]);
    free(coeffs_RPE2[I1]);
    free(coeffs_RPE2[I2]);
    free(coeffs_RPE2[I1_and_I2]);
  }

  free(coeffs_RPE1);
  free(coeffs_RPE2);

  if (circuit->output_count == 2) {
    free(coeffs_RPE12[0]);
    free(coeffs_RPE21[0]);
    free(coeffs_RPE12);
    free(coeffs_RPE21);
  }

  free_dim_red_data(dim_red_data);
}


// Test of compute_leakage_proba_max.
// Should find (more or less):
//   Tolerated leakage probability: 0.04286507265888017670
//   log2(0.04286507265888017670) = -4.54405360091824039870

/* void compute_RPE_coeffs(Circuit* circuit, int cores, int coeff_max, int t, int t_output) { */
/*   uint64_t tab[37] = { 0.0, 0.0, 3.0, 118.0, 2457.0, 34998.0, 1947792, 8347680, 30260340, 94143280, 254186856, 600805296, 1251677700, 2310789600, 3796297200, 5567902560, 7307872110, 8597496600, 9075135300, 8597496600, 7307872110, 5567902560, 3796297200, 2310789600, 1251677700, 600805296, 254186856, 94143280, 30260340, 8347680, 1947792, 376992, 58905, 7140, 630, 36, 1}; */

/*   compute_leakage_proba_max(tab, 37, 37); */
/* } */
