#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"

typedef VAR_TYPE Var;

typedef DEPENDENCY_TYPE Dependency;

#define RANDOMS_MAX_LEN 5 // Enough to store 64*5 = 320 randoms
#define BITMULT_MAX_LEN 7 // Enough to store 64*7 = 448 = 21*21 -->
                          // multiplication gadgets up to order 21

// The BitDep structure is a more compact representation of
// dependencies: instead of using a array of Dependency, where most
// elements (randoms and multiplications) can only be 0 or 1, BitDep
// uses Dependency only for secrets and bitvectors for randoms and
// multiplications.
typedef struct _bitDep {
  Dependency secrets[2];
  uint64_t randoms[RANDOMS_MAX_LEN];
  uint64_t mults[BITMULT_MAX_LEN];
  Dependency out;
} BitDep;


typedef struct _multDep {
  /* |left_idx| and |right_idx| provide indices in the main
      dependencies, while |left_ptr| and |right_ptr| provide pointers
      into the main dependencies. The latter are faster (one less
      indirection required), but the former are useful in some cases
      (copying the dependencies for instance). */
  /* |left_idx| and |right_idx| are deprecated. Use |left_ptr| and
      |right_ptr| instead */
  int left_idx;
  int right_idx;
  Dependency* left_ptr;
  Dependency* right_ptr;
  struct _BitDepVector_vector* bits_left;
  struct _BitDepVector_vector* bits_right;
  Dependency* contained_secrets; // Array of size secret_count containing
                                 // the secret deps contained in
                                 // |left_ptr| and |right_ptr|
} MultDependency;

typedef struct _multDependencyList {
  MultDependency** deps;
  //int deps_size;
  int length;
} MultDependencyList;

typedef struct _dependencyList {
  struct _DepArrVector_vector** deps;
  Dependency** deps_exprs;
  char** names;  // Variables associated to elements of |deps|
                 // (only used for debuging purposes)
  int deps_size; // Length of each element inside |deps|
  int length; // Length of |deps|
  int first_rand_idx; // Index of first random variable in each
                      // element of |deps|
  Dependency** contained_secrets; // Array of size |length| containing the
                                  // secret shares contained in each DepArrVector
                                  // of |deps|.
  struct _BitDepVector_vector** bit_deps; // bitvector-based representation of |deps|
  MultDependencyList* mult_deps;
} DependencyList;


typedef struct _circuit {
  DependencyList* deps;
  int length;          // Length of circuit: inputs * shares + secret + deps->length
                       // Notably, circuit_length does not include outputs
  int secret_count;    // Number of secret inputs
  int output_count;    // Number of outputs
  int share_count;     // Number of shares
  int random_count;    // Number of random variables
  int all_shares_mask; // = (1 << share_count) - 1
  int* weights;        // Array of size |circuit_length|
  int contains_mults;  // 1 if the circuit contains multiplications, 0 otherwise
  int total_wires;     // Total number of wires

  // The following 3 members are arrays of size deps->deps_size, where
  // a cell at 1 (resp. 0) indicates that the random at this index is
  // used (resp. not used) to refresh the input/output corresponding
  // to the member (ie, i1, i2 or out)
  bool* i1_rands;       // Randoms used to refresh the 1st input
  bool* i2_rands;       // Randoms used to refresh the 2nd input
  bool* out_rands;      // Randoms used to refresh the output
  bool has_input_rands; // True if the circuit is a multiplication and
                        // refreshes its inputs; false otherwise

  bool transition;      // True if transitions need to be taken into account in the verification
  bool glitch;          // True if glitches need to be taken into account in the verification

  // Bitvector-based dependencies, for better performance in verification_rules
  uint64_t bit_out_rands[RANDOMS_MAX_LEN];
  uint64_t bit_i1_rands[RANDOMS_MAX_LEN];
  uint64_t bit_i2_rands[RANDOMS_MAX_LEN];
} Circuit;


void compute_total_wires(Circuit* c);
void compute_rands_usage(Circuit* c);
void compute_contained_secrets(Circuit* c);
void compute_bit_deps(Circuit* circuit);
void print_circuit(const Circuit* c);
void free_circuit(Circuit* c);
Circuit* shallow_copy_circuit(Circuit* c);
