#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "dimensions.h"
#include "config.h"
#include "circuit.h"
#include "combinations.h"

// -----------------------------------------------------------
//
//  "Advanced" dimension reduction, à la Bordes-Karpman: "less"
//  powerful probes are removed (more or less).

//  See Section 3.3 of https://eprint.iacr.org/2019/1165.pdf for a
//  full description of this optimization. The idea if that if all
//  linear combinations of a set P' can be covered by a smaller set P,
//  with fewer or the same number of elements, then we can use P
//  instead of P'. Concretely, this means that we can remove a probe p
//  in P if all linear combinations of P can be generated at the same
//  or lower cost with elements of P\{p}.
//
//  Note that this optimization does not apply to random-probing
//  properties. Thus, once a probe is removed, there is no need to
//  "remember" that we removed this probe or anything like that.
//
//  We proceed in 2 steps: first, we identify the potential candidate
//  probes. Then, we make sure that the probes in question can be
//  removed by generating all linear combinations with and without
//  each probe to remove.
//
//  We have the condition that no product aibj should appear in
//  multiple outputs. Then, we can proceed output-by-output, which
//  means that we don't have to generate the full powerset of P for
//  the whole circuit, but just for each output.
//
//  More concretely, the "potential candidates" that we consider
//  removing are probes of the form "X + aibj" if a probe "X + aibj + ambn"
//  exists.
//



// TODO: must be able to change size dynamically
#define HASH_MASK 0x1ffffff
#define HASH_SIZE (HASH_MASK+1)

// Computes a hash for integer |x|.
// Hash function for integers from https://stackoverflow.com/a/12996028/4990392
static unsigned int hash_int(unsigned int x) {
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = (x >> 16) ^ x;
  return x;
}

// Compute the hash for |dep|. This hash is based only on the randoms inside |dep|.
static unsigned int hash_dep(Dependency* dep, int first_rand_idx, int non_mult_deps_count) {
  unsigned int hash = 0;
  for (int i = first_rand_idx; i < non_mult_deps_count; i++) {
    if (dep[i]) {
      hash += hash_int(i);
    }
  }
  return hash & HASH_MASK;
}

typedef struct _multnode {
  int length;
  uint64_t* mults;
  struct _multnode* next;
} MultNode;

typedef struct _hashnode {
  uint64_t* rands;
  MultNode* mult_node;
  struct _hashnode* next;
} HashNode;

typedef struct _hashmap {
  HashNode** content;
  int deps_size; // size of the Dependency* in the hash
  int first_rand_idx;
  int non_mult_deps_count;
  int mults_len; // Length of the |mults| array in MultNodes
  int rands_len; // Length of the |rands| array in HashNodes
  int count; // Number of elements in the hash
} HashMap;

// Allocates and initializes an empty hash map.
static HashMap* init_map(Circuit* circuit) {
  int deps_size = circuit->deps->deps_size;
  int first_rand_idx = circuit->deps->first_rand_idx;
  int non_mult_deps_count = deps_size - circuit->deps->mult_deps->length;
  HashMap* map    = malloc(sizeof(*map));
  map->content    = calloc(HASH_SIZE, sizeof(*(map->content)));
  map->deps_size  = deps_size;
  map->first_rand_idx = first_rand_idx;
  map->non_mult_deps_count = non_mult_deps_count;
  map->mults_len = (deps_size - non_mult_deps_count) / 64 + 1;
  map->rands_len = (non_mult_deps_count - circuit->secret_count) / 64 + 1;
  map->count      = 0;
  return map;
}

// Check if |map| contains |dep| at index |hash|. If it does, returns
// its |length| field, and otherwise, return 0.
static int hash_contains_keyed(HashMap* map, int length, int hash,
                               uint64_t* rands, uint64_t* mults,
                               HashNode** ret_node) {
  int rands_len = map->rands_len;
  int mults_len = map->mults_len;
  HashNode* node = map->content[hash];
  while (node) {
    if (memcmp(node->rands, rands, rands_len * sizeof(*rands)) == 0) {
      MultNode* mult_node = node->mult_node;
      while (mult_node) {
        if (mult_node->length <= length) {
          int found = 1;
          for (int i = 0; i < mults_len; i++) {
            if ((mult_node->mults[i] & mults[i]) != mults[i]) {
              found = 0;
              break;
            }
          }
          if (found) return 1;
        }
        mult_node = mult_node->next;
      }
      *ret_node = node;
      return 0;
    }
    node = node->next;
  }
  return 0;
}

// Adds |dep| to |map|.
static void add_to_hash_keyed(HashMap* map, int length, int hash,
                              uint64_t* rands, uint64_t* mults) {
  HashNode* node = NULL;
  if (hash_contains_keyed(map, length, hash, rands, mults, &node)) {
    return;
  } else {
    int rands_len = map->rands_len;
    int mults_len = map->mults_len;
    map->count++;

    MultNode* mult_node = malloc(sizeof(*node));
    mult_node->length = length;
    mult_node->mults = malloc(mults_len * sizeof(*mult_node->mults));
    memcpy(mult_node->mults, mults, mults_len * sizeof(*mult_node->mults));

    if (node) {
      mult_node->next = node->mult_node;
      node->mult_node = mult_node;
    } else {
      node = malloc(sizeof(*node));
      node->rands = malloc(rands_len * sizeof(*node->rands));
      memcpy(node->rands, rands, rands_len * sizeof(*node->rands));
      node->mult_node = mult_node;
      mult_node->next = NULL;
      node->next = map->content[hash];
      map->content[hash] = node;
    }
  }
}

static void build_rands_bitmap(HashMap* map, Dependency* dep, uint64_t* rands) {
  int first_rand_idx = map->first_rand_idx;
  int non_mult_deps_count = map->non_mult_deps_count;
  int rands_count = non_mult_deps_count - first_rand_idx;
  int rands_len = map->rands_len;

  memset(rands, 0, rands_len * sizeof(*rands));
  for (int i = 0; i < rands_len; i++) {
    for (int j = 0; j < 64; j++) {
      if (i*64+j >= rands_count) break;
      if (dep[first_rand_idx+i*64+j]) {
        rands[i] |= 1 << j;
      }
    }
  }
}

static void build_mults_bitmap(HashMap* map, Dependency* dep, uint64_t* mults) {
  int non_mult_deps_count = map->non_mult_deps_count;
  int mult_count = map->deps_size - non_mult_deps_count;
  int mults_len = map->mults_len;

  memset(mults, 0, mults_len * sizeof(*mults));
  for (int i = 0; i < mults_len; i++) {
    for (int j = 0; j < 64; j++) {
      if (i*64+j >= mult_count) break;
      if (dep[non_mult_deps_count+i*64+j]) {
        mults[i] |= 1 << j;
      }
    }
  }
}

// Returns true if |map| contains |dep| with a length less or equal to
// |length|. Note that this function is optimized for the expected
// case: |map| should contain |dep|. For instance, after computing
// |hash|, we could add `if (!map->content[hash]) return 0;`, but it's
// expected that |map| contains |dep| and thus contains a node at
// index |hash|.
static int hash_contains(HashMap* map, Dependency* dep, int length) {
  unsigned int hash = hash_dep(dep, map->first_rand_idx, map->non_mult_deps_count);

  uint64_t rands[map->rands_len];
  build_rands_bitmap(map, dep, rands);

  uint64_t mults[map->mults_len];
  build_mults_bitmap(map, dep, mults);

  HashNode* node;
  if (hash_contains_keyed(map, length, hash, rands, mults, &node)) {
    return 1;
  } else {
    return 0;
  }
}

// Adds |dep| to |map| with length |length| associated. |dep| is not
// added to |map| if |map| already contains |dep| with a length
// smaller or equal. Note that by construction, this function should
// not be called if |map| already contains |dep| with a bigger length
// (which removes the need to check if there is such a |dep| with
// larger length and remove it). Finally, note that no memory is
// allocated unless |dep| is added to |map|.
static void add_to_hash(HashMap* map, Dependency* dep, int length) {
  unsigned int hash = hash_dep(dep, map->first_rand_idx, map->non_mult_deps_count);

  uint64_t rands[map->rands_len];
  build_rands_bitmap(map, dep, rands);

  uint64_t mults[map->mults_len];
  build_mults_bitmap(map, dep, mults);

  add_to_hash_keyed(map, length, hash, rands, mults);
}

// Frees |map| and its content.
static void free_hash(HashMap* map) {
  for (int i = 0; i < HASH_SIZE; i++) {
    HashNode* node = map->content[i];
    while (node) {
      MultNode* mult_node = node->mult_node;
      while (mult_node) {
        free(mult_node->mults);
        MultNode* next_mult = mult_node->next;
        free(mult_node);
        mult_node = next_mult;
      }
      free(node->rands);
      HashNode* next = node->next;
      free(node);
      node = next;
    }
  }
  free(map->content);
  free(map);
}


// Computes the xor of all elements of |comb| and puts the result in
// |dep_target| (which is assumed to contain only 0s initially).
void compute_linear_comb(Circuit* circuit,
                         Comb* comb, int comb_len,
                         Dependency* dep_target) {
  DependencyList* deps = circuit->deps;
  int deps_size = deps->deps_size;

  for (int i = 0; i < comb_len; i++) {
    int dep_idx = comb[i];
    Dependency* dep = deps->deps[dep_idx]->content[0];
    for (int j = 0; j < deps_size; j++) {
      dep_target[j] ^= dep[j];
    }
  }
}

// Returns a VarVector* that contains all elements from |subcircuit|
// except for those that are in |to_remove|.
VarVector* remove_from_subcircuit(VarVector* subcircuit, VarVector* to_remove) {
  VarVector* result = VarVector_make();
  for (int i = 0; i < subcircuit->length; i++) {
    Var elem = subcircuit->content[i];
    if (!VarVector_contains(to_remove, elem)) {
      VarVector_push(result, elem);
    }
  }
  return result;
}

// Returns true if |dep| contains no dependencies at all or if it
// contains a single elementary probe or a product of elementary
// probes.
int is_zero_or_elementary(Circuit* circuit, Dependency* dep) {
  DependencyList* deps = circuit->deps;
  int deps_size = deps->deps_size;
  int non_mult_deps_count = deps->deps_size - deps->mult_deps->length;

  int elem_count = 0;
  for (int i = 0; i < circuit->secret_count; i++) {
    if (dep[i]) elem_count += __builtin_popcount(dep[i]);
  }

  for (int i = circuit->secret_count; i < non_mult_deps_count; i++) {
    if (dep[i]) return 0;
  }

  for (int i = non_mult_deps_count; i < deps_size; i++) {
    if (dep[i]) elem_count++;
  }

  return elem_count <= 1;
}

// Returns 1 if |remove_candidate| can be removed from the
// circuit. This function computes all linear combinations that can be
// generated with elements of |subcircuit| with and without
// |remove_candidate|, as well as the number of elements required to
// compute the result. Then, if all combinations generated with
// |remove_candidate| were also generated without it and using fewer
// or the same number of elements, then |remove_candidate| can be
// removed.
int can_be_removed(Circuit* circuit, VarVector* subcircuit, VarVector* remove_candidates) {

  DependencyList* deps = circuit->deps;
  int deps_size = deps->deps_size;

  VarVector* reduced_subcircuit = remove_from_subcircuit(subcircuit, remove_candidates);

  // Step 1: generate all linear combinations |linear_combs| of the
  // reduced set (|reduced_subcircuit|)
  HashMap* linear_combs = init_map(circuit);
  int last_idx = reduced_subcircuit->length;
  for (int size = 1; size <= reduced_subcircuit->length; size++) {
    Comb* comb = first_comb(size, 0);
    do {
      // Computing the actual value of the comb
      Comb real_comb[size];
      for (int i = 0; i < size; i++) real_comb[i] = reduced_subcircuit->content[comb[i]];

      // Computing the sum of |comb|
      Dependency dep[deps_size];
      memset(dep, 0, deps_size * sizeof(*dep));
      compute_linear_comb(circuit, real_comb, size, dep);

      if (!is_zero_or_elementary(circuit, dep)) {
        add_to_hash(linear_combs, dep, size);
      }

    } while (incr_comb_in_place(comb, size, last_idx));
    free(comb);
  }

  // Step 2: check that all linear combinations of the full
  // |subcircuit| that contain variables of |remove_candidates| are
  // contained in |linear_combs|.
  //
  // To avoid generating all combinations of |subcircuit|, we can
  // generate all combinations of |remove_candidates| and all
  // combinations of |reduced_subcircuit|, and concatenate the two.
  int last_idx_cand = remove_candidates->length;
  for (int size_cand = 1; size_cand <= remove_candidates->length; size_cand++) {
    Comb* comb_cand = first_comb(size_cand, reduced_subcircuit->length);
    do {
      Comb real_comb[subcircuit->length];
      for (int i = 0; i < size_cand; i++) real_comb[i] = remove_candidates->content[i];

      for (int size_other = 0; size_other <= reduced_subcircuit->length; size_other++) {
        int full_size = size_cand + size_other;
        Comb* comb_other = first_comb(size_other, 0);
        do {
          for (int i = 0; i < size_other; i++) {
            real_comb[i+size_cand] = reduced_subcircuit->content[comb_other[i]];
          }

          Dependency dep[deps_size];
          memset(dep, 0, deps_size * sizeof(*dep));
          compute_linear_comb(circuit, real_comb, full_size, dep);

          if (!is_zero_or_elementary(circuit, dep)) {
            if (!hash_contains(linear_combs, dep, full_size)) {
              fprintf(stderr, "The following combination cannot be built "
                      "if the selected probes are removed:\n  [ ");
              for (int i = 0; i < full_size; i++) printf("%d ", real_comb[i]);
              printf("]\nContinuing without removing elementary probes for this output.\n");
              exit(1);
              return 0;
            }
          }

        } while (incr_comb_in_place(comb_other, size_other, last_idx));
        free(comb_other);
      }
    } while(incr_comb_in_place(comb_cand, size_cand, last_idx_cand));
    free(comb_cand);
  }

  VarVector_free(reduced_subcircuit);
  free_hash(linear_combs);

  return 1;
}


// Returns 1 if |larger| contains the same secret shares and randoms
// as |smaller| and has _at least_ one more multiplication, and 0
// otherwise.
int has_one_more_mult(Circuit* circuit, Var larger, Var smaller) {
  DependencyList* deps = circuit->deps;
  Dependency* dep_larger  = deps->deps[larger]->content[0];
  Dependency* dep_smaller = deps->deps[smaller]->content[0];
  int non_mult_deps_count = deps->deps_size - deps->mult_deps->length;
  int first_rand_idx = deps->first_rand_idx;

  for (int i = first_rand_idx; i < non_mult_deps_count; i++) {
    if (dep_larger[i] != dep_smaller[i]) {
      // One of the two deps contains a random that the other doesn't.
      return 0;
    }
  }

  if (circuit->contains_mults) {
    for (int i = 0; i < first_rand_idx; i++) {
      if (dep_larger[i] != dep_smaller[i]) {
        // One of the two deps contains an input share that the other doesn't
        return 0;
      }
    }

    int additional_mults_count = 0;
    for (int i = non_mult_deps_count; i < deps->deps_size; i++) {
      if (dep_smaller[i] && !dep_larger[i]) {
        // |smaller| contains a multiplication that |larger| does not
        // contain. This is the opposite of what we want!
        return 0;
      }
      if (dep_larger[i] && !dep_smaller[i]) {
        additional_mults_count++;
      }
    }

    // Note: should there be something special if
    // |additional_mults_count| is 2 or more? Well, if it's 3 then
    // that's an automatic failure; for instance:
    //     p1 = X, p2 = X ^ a0 ^ a1 ^ a2
    // The tuple (p1,p2) leaks 3 secret shares with 2 elements.
    return additional_mults_count == 1;
  } else {
    // Linear gadget: checking only input shares
    for (int i = 0; i < first_rand_idx; i++) {
      if ((dep_larger[i] & dep_smaller[i]) != dep_smaller[i]) {
        // |smaller| contains a secret share that |larger| doesn't contain.
        return 0;
      }
    }
    return 1;
  }
}

// Returns an array of all probes that are candidates for removal in
// |subcircuit|. Candidates are probe that have the forme "X + aibj"
// if a probe "X" and a probe "X + aibj + anbm" exist (we say
// informally that "X" is _smaller_ and that "X + aibj + anbm" is
// _larger_).
VarVector* get_remove_candidates(Circuit* circuit, VarVector* subcircuit) {
  VarVector* candidates = VarVector_make();
  for (Var idx = 0; idx < subcircuit->length; idx++) {
    Var candidate = subcircuit->content[idx];
    int smaller_exists = 0, larger_exists = 0;
    int smaller_idx = -1, larger_idx = -1;
    for (Var other_idx = 0; other_idx < subcircuit->length; other_idx++) {
      Var to_compare = subcircuit->content[other_idx];
      if (to_compare == candidate) continue;
      if (has_one_more_mult(circuit, candidate, to_compare)) {
        smaller_idx = to_compare;
        smaller_exists = 1;
      } else if (has_one_more_mult(circuit, to_compare, candidate)) {
        larger_idx = to_compare;
        larger_exists = 1;
      }
    }
    if (smaller_exists && larger_exists) {
      VarVector_push(candidates, candidate);
    }
  }
  return candidates;
}

// Returns true if the |sub_dep| is contained inside |out_dep|.
int is_subprobe(Circuit* circuit, Dependency* sub_dep, Dependency* out_dep) {
  for (int i = 0; i < circuit->secret_count; i++) {
    if ((sub_dep[i] & out_dep[i]) != sub_dep[i]) {
      // |sub_dep| contains a secret share that isn't in |out_dep|
      return 0;
    }
  }

  for (int i = circuit->secret_count; i < circuit->deps->deps_size; i++) {
    if (sub_dep[i] && !out_dep[i]) {
      // |sub_dep| contains a random or multiplication that isn't in |out_dep|
      return 0;
    }
  }

  // All secret shares, randoms and multiplications of |sub_dep| are
  // also in |out_dep|.
  return 1;
}

// Extracts the variables used to compute each output of |circuit|.
VarVector** extract_outputs_subcircuit(Circuit* circuit) {
  int first_output_idx = circuit->length;
  VarVector** subcircuits = malloc(circuit->share_count * sizeof(*subcircuits));

  for (int i = 0; i < circuit->share_count; i++) {
    subcircuits[i] = VarVector_make();
    Dependency* out_dep = circuit->deps->deps[first_output_idx + i]->content[0];

    for (int j = 0; j < circuit->deps->length; j++) {
      Dependency* dep = circuit->deps->deps[j]->content[0];
      if (is_subprobe(circuit, dep, out_dep)) {
        VarVector_push(subcircuits[i], j);
      }
    }
  }

  return subcircuits;
}

void advanced_dimension_reduction(Circuit* circuit) {

  if (circuit->output_count == 2) {
  // TODO: handle multiple outputs
    return;
  }

  if (!circuit->contains_mults) {
    // For now, we only handle multiplication gadgets. This is a
    // logical first choice: they contain much more variables than
    // linear gadgets. Also, for linear gadgets, our constructive
    // algorithm is quite fast and does not really require this
    // optimization.
    // TODO: handle linear gadgets as well.
    return;
  }

  if (circuit->has_input_rands) {
    // We remove wires of the form "R + aibj" when a wire "R + aibj + anbm"
    // exists, but this only works if inputs are not refreshed before being
    // multiplied.
    return;
  }

  printf("Starting advanced dimension reduction...\n");

  time_t start, end;
  time(&start);

  // Step 1: extract the sub-circuit used for each output. They should
  // be disjoint except for the inputs and randoms.
  VarVector** subcircuits = extract_outputs_subcircuit(circuit);

  // Step 2:
  //  for each sub-circuit c':
  //    while c' has a candidate p for removal:
  //      compute subsets of c' with and without p
  //      if without p produces the same subsets, remove p
  //      update all subcircuit (decrement numbers under p) and |circuit|
  VarVector* to_remove = VarVector_make();
  for (int i = 0; i < circuit->share_count; i++) {
    // TODO: Like Bordes-Karpman, we try to remove all candidates at
    // once. It's obviously more efficient, but it would be
    // interesting to see if any probes can be missed this way. On ISW
    // (primary target of Border-Karpman), I guess that doing all
    // probes at once is enough.
    VarVector* remove_candidates = get_remove_candidates(circuit, subcircuits[i]);
    if (remove_candidates->length &&
        can_be_removed(circuit, subcircuits[i], remove_candidates)) {
      for (int j = 0; j < remove_candidates->length; j++) {
        VarVector_push(to_remove, remove_candidates->content[j]);
      }
    }
    VarVector_free(remove_candidates);
  }

  for (int i = 0; i < circuit->share_count; i++) {
    VarVector_free(subcircuits[i]);
  }
  free(subcircuits);

  // Removing all variables from |to_remove| from |circuit|.
  DependencyList* deps = circuit->deps;

  DependencyList* new_deps = malloc(sizeof(*new_deps));
  new_deps->deps_size      = deps->deps_size;
  new_deps->first_rand_idx = deps->first_rand_idx;
  new_deps->mult_deps      = deps->mult_deps;
  new_deps->length         = 0;
  new_deps->deps           = malloc(deps->length * sizeof(*new_deps->deps));
  new_deps->deps_exprs     = malloc(deps->length * sizeof(*new_deps->deps_exprs));
  new_deps->names          = malloc(deps->length * sizeof(*new_deps->names));
  new_deps->contained_secrets = malloc(deps->length * sizeof(*new_deps->contained_secrets));
  new_deps->bit_deps       = malloc(deps->length * sizeof(*new_deps->bit_deps));

  for (int i = 0; i < deps->length; i++) {
    if (!VarVector_contains(to_remove, i)) {
      int insert_idx = new_deps->length;
      new_deps->deps[insert_idx]  = deps->deps[i];
      new_deps->deps_exprs[insert_idx] = deps->deps_exprs[i];
      new_deps->names[insert_idx] = deps->names[i];
      new_deps->contained_secrets[insert_idx] = deps->contained_secrets[i];
      new_deps->bit_deps[insert_idx] = deps->bit_deps[i];
      new_deps->length++;
    }
  }
  VarVector_free(to_remove);

  circuit->deps = new_deps;
  circuit->length = circuit->length - deps->length + new_deps->length;

  time(&end);
  uint64_t diff_time = (uint64_t)difftime(end, start);

  printf("Advanced dimension reduction completed in %lu min %lu sec.\n",
         diff_time / 60, diff_time % 60);
  printf("old circuit: %d vars -- new circuit: %d vars.\n\n",
         deps->length, new_deps->length);
}



// -----------------------------------------------------------
//
//  Basic dimension reduction: only elementary deterministic probes
//  (and products of non-refreshed such probes) are removed.
//
//


// Returns 1 if something was added into |elementary_wires| and 0
// otherwise.
int add_to_elem_array_nomult(Circuit* circuit, Var dep_idx,
                             VarVector** elementary_wires) {
  Dependency* dep = circuit->deps->deps[dep_idx]->content[0];
  for (int i = 0; i < circuit->secret_count; i++) {
    if (dep[i]) {
      int share_idx = __builtin_ffs(dep[i]) - 1;
      int input_idx = i * circuit->share_count + share_idx;
      VarVector_push(elementary_wires[input_idx], dep_idx);
      return 1;
    }
  }
  return 0;
}

void add_to_elem_array(Circuit* circuit, Var dep_idx,
                       VarVector** elementary_wires) {
  DependencyList* deps    = circuit->deps;
  Dependency* dep         = deps->deps[dep_idx]->content[0];
  int deps_size           = deps->deps_size;
  int non_mult_deps_count = deps_size - deps->mult_deps->length;

  if (add_to_elem_array_nomult(circuit, dep_idx, elementary_wires)) return;

  assert(!circuit->has_input_rands);

  for (int i = non_mult_deps_count; i < deps_size; i++) {
    if (dep[i]) {
      MultDependency* mult_dep = deps->mult_deps->deps[i-non_mult_deps_count];
      add_to_elem_array_nomult(circuit, mult_dep->left_idx, elementary_wires);
      add_to_elem_array_nomult(circuit, mult_dep->right_idx, elementary_wires);
    }
  }
}

bool is_elementary(Circuit* circuit, Dependency* dep) {
  DependencyList* deps    = circuit->deps;
  int deps_size           = deps->deps_size;
  int first_rand_idx      = deps->first_rand_idx;
  int has_input_rands     = circuit->has_input_rands;
  int non_mult_deps_count = deps_size - deps->mult_deps->length;

  int last_idx = has_input_rands ? deps_size : non_mult_deps_count;
  for (int i = first_rand_idx; i < last_idx; i++) {
    if (dep[i]) return 0;
  }

  int mult_count = 0;
  for (int i = non_mult_deps_count; i < deps_size; i++) {
    if (dep[i]) mult_count++;
  }
  if (mult_count > 1) return 0;

  int input_count = 0;
  for (int i = 0; i < first_rand_idx; i++) {
    if (dep[i]) input_count += __builtin_popcount(dep[i]);
  }

  assert(mult_count == 0 || input_count == 0);
  return mult_count + input_count == 1;
}

// Removes elementary deterministic wires from |circuit|. This
// includes input shares, and product of non-refreshed input shares. A
// DimRedData structure is returned; this structure can then be used
// to expand almost-failures with elementary deterministic probes to
// obtain actual failures.
DimRedData* remove_elementary_wires(Circuit* circuit) {
  // TODO: print error if glitches are enabled
  DimRedData* data_ret = malloc(sizeof(*data_ret));
  data_ret->length = circuit->secret_count * circuit->share_count;
  data_ret->elementary_wires = malloc(circuit->secret_count * circuit->share_count *
                                      sizeof(*data_ret->elementary_wires));
  for (int i = 0; i < circuit->secret_count * circuit->share_count; i++) {
    data_ret->elementary_wires[i] = VarVector_make();
  }

  data_ret->removed_wires = VarVector_make();

  data_ret->new_to_old_mapping = malloc(circuit->deps->length *
                                        sizeof(*data_ret->new_to_old_mapping));
  data_ret->old_circuit = shallow_copy_circuit(circuit);

  DependencyList* deps = circuit->deps;

  DependencyList* new_deps = malloc(sizeof(*new_deps));
  new_deps->deps_size      = deps->deps_size;
  new_deps->first_rand_idx = deps->first_rand_idx;
  new_deps->mult_deps      = deps->mult_deps;
  new_deps->length         = 0;
  new_deps->deps           = malloc(deps->length * sizeof(*new_deps->deps));
  new_deps->deps_exprs     = malloc(deps->length * sizeof(*new_deps->deps_exprs));
  new_deps->names          = malloc(deps->length * sizeof(*new_deps->names));
  new_deps->contained_secrets = malloc(deps->length * sizeof(*new_deps->contained_secrets));
  new_deps->bit_deps       = malloc(deps->length * sizeof(*new_deps->bit_deps));


  for (int i = 0; i < deps->length; i++) {
    Dependency* dep = deps->deps[i]->content[0];
    if (is_elementary(circuit, dep)) {
      add_to_elem_array(circuit, i, data_ret->elementary_wires);
      VarVector_push(data_ret->removed_wires, i);
      continue;
    }

    data_ret->new_to_old_mapping[new_deps->length] = i;
    new_deps->names[new_deps->length]      = deps->names[i];
    new_deps->deps[new_deps->length]       = deps->deps[i];
    new_deps->deps_exprs[new_deps->length] = deps->deps_exprs[i];
    new_deps->contained_secrets[new_deps->length] = deps->contained_secrets[i];
    new_deps->bit_deps[new_deps->length]   = deps->bit_deps[i];
    new_deps->length++;
  }

  circuit->deps = new_deps;
  circuit->length = circuit->length - deps->length + new_deps->length;

  printf("Dimension reduction: old circuit: %d vars -- new circuit: %d vars.\n\n",
         deps->length, new_deps->length);

  return data_ret;
}


// -----------------------------------------------------------
//
//  Basic dimension reduction 2: only elementary random probes are
//  removed. This optimization is used only in the probing model for
//  now, which partly explain why no DimRedData or similar is
//  returned: since we don't want to recompute all failures but just
//  know if one exists, we don't need to bother with structures to
//  rebuild the full failures.
//
//

// Remove random wires from |circuit|. Note that
// remove_elementary_wires should have been called first.
void remove_randoms(Circuit* circuit) {
  DependencyList* deps = circuit->deps;

  DependencyList* new_deps = malloc(sizeof(*new_deps));
  new_deps->deps_size      = deps->deps_size;
  new_deps->first_rand_idx = deps->first_rand_idx;
  new_deps->mult_deps      = deps->mult_deps;
  new_deps->length         = 0;
  new_deps->deps           = malloc(deps->length * sizeof(*new_deps->deps));
  new_deps->deps_exprs     = malloc(deps->length * sizeof(*new_deps->deps_exprs));
  new_deps->names          = malloc(deps->length * sizeof(*new_deps->names));
  new_deps->contained_secrets = malloc(deps->length * sizeof(*new_deps->contained_secrets));
  new_deps->bit_deps       = malloc(deps->length * sizeof(*new_deps->bit_deps));

  int non_mult_deps_count = deps->deps_size - deps->mult_deps->length;

  for (int i = 0; i < deps->length; i++) {
    if (i < non_mult_deps_count - circuit->secret_count) {
      // Checking that it's actually a random, just to be sure.
      int rand_count = 0;
      for (int j = circuit->secret_count; j < non_mult_deps_count; j++) {
        rand_count += deps->deps[i]->content[0][j];
      }
      assert(rand_count == 1);
      for (int j = non_mult_deps_count; j < deps->deps_size; j++) {
        assert(!deps->deps[i]->content[0][j]);
      }
      continue;
    }
    new_deps->names[new_deps->length]      = deps->names[i];
    new_deps->deps[new_deps->length]       = deps->deps[i];
    new_deps->deps_exprs[new_deps->length] = deps->deps_exprs[i];
    new_deps->contained_secrets[new_deps->length] = deps->contained_secrets[i];
    new_deps->bit_deps[new_deps->length]   = deps->bit_deps[i];
    new_deps->length++;
  }

  circuit->deps = new_deps;
  circuit->length = circuit->length - deps->length + new_deps->length;

  printf("Randoms removed. old circuit: %d vars -- new circuit: %d vars.\n\n",
         deps->length, new_deps->length);
}


void free_dim_red_data(DimRedData* dim_red_data) {
  for (int i = 0; i < dim_red_data->length; i++) {
    VarVector_free(dim_red_data->elementary_wires[i]);
  }
  free(dim_red_data->elementary_wires);
  free(dim_red_data->new_to_old_mapping);
  VarVector_free(dim_red_data->removed_wires);
  free(dim_red_data);
}
