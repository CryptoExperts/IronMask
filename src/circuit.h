#ifndef CIRCUIT_H
#define CIRCUIT_H

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
#define BITCORRECTION_OUTPUTS_MAX_LEN 7
#define BITDUPLICATE_SECRETS_MAX_LEN 20 // Enough for gadgets with 2 inputs of 10 shares,
                                        // this field is only useful in case of faults and
                                        // correction gadgets, so we can't verify gadgets this
                                        // big anyway

// The BitDep structure is a more compact representation of
// dependencies: instead of using a array of Dependency, where most
// elements (randoms and multiplications) can only be 0 or 1, BitDep
// uses Dependency only for secrets and bitvectors for randoms and
// multiplications.
typedef struct _bitDep {
  Dependency secrets[2];
  Dependency duplicate_secrets[BITDUPLICATE_SECRETS_MAX_LEN];
  uint64_t randoms[RANDOMS_MAX_LEN];
  uint64_t mults[BITMULT_MAX_LEN];
  uint64_t correction_outputs[BITCORRECTION_OUTPUTS_MAX_LEN];
  Dependency out;
  Dependency constant;
} BitDep;


typedef struct _multDep {
  /* |left_ptr| and |right_ptr| provide pointers
      into the main dependencies. */
  char * name;
  char * name_left;
  char * name_right;
  Dependency* left_ptr;
  Dependency* right_ptr;
  int idx_same_dependencies;
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

typedef struct _correctionOutputDependency {
  struct _DepArrVector_vector** correction_outputs_deps;
  struct _BitDepVector_vector** correction_outputs_deps_bits;
  BitDep ** total_deps;
  char ** correction_outputs_names;
  int length;
} CorrectionOutputs;

typedef struct _dependencyList {
  struct _DepArrVector_vector** deps;
  Dependency** deps_exprs;
  char** names;  // Variables associated to elements of |deps|
                 // (only used for debuging purposes)
  int deps_size; // Length of each element inside |deps|
  int length; // Length of |deps|
  int first_rand_idx; // Index of first random variable in each
                      // element of |deps|
  int first_mult_idx; // Index of first mult variable in each
                      // element of |deps|
  int first_correction_idx; // Index of first correction output variable in each
                            // element of |deps|
  Dependency** contained_secrets; // Array of size |length| containing the
                                  // secret shares contained in each DepArrVector
                                  // of |deps|.
  struct _BitDepVector_vector** bit_deps; // bitvector-based representation of |deps|
  MultDependencyList* mult_deps;
  CorrectionOutputs * correction_outputs;
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
  int nb_duplications;
  int* weights;        // Array of size |circuit_length|
  int contains_mults;  // 1 if the circuit contains multiplications, 0 otherwise
  int total_wires;     // Total number of wires
  bool faults_on_inputs;

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

typedef struct _faulted_var{
  char * name;
  bool set;
  bool fault_on_input;
  int share;
  int duplicate;
}FaultedVar;

typedef struct _faults_struct{
  FaultedVar ** vars;
  int length;
}Faults;


BitDep * init_bit_dep();
void set_bit_dep_zero(BitDep* bit_dep);
void compute_total_wires(Circuit* c);
void compute_rands_usage(Circuit* c);
void compute_contained_secrets(Circuit* c, int ** temporary_mult_idx);
void compute_bit_deps(Circuit* circuit, int ** temporary_mult_idx);
void compute_total_correction_bit_deps(Circuit * circuit);
void print_circuit(const Circuit* c);

void print_circuit_after_dim_red(const Circuit* c, const Circuit* c_old);
void free_circuit(Circuit* c);
Circuit* shallow_copy_circuit(Circuit* c);

#endif