#pragma once

#include <stdlib.h>
#include <stdint.h>

#include "vectors.h"
#include "list_tuples.h"
#include "parser.h"
#include "trie.h"
#include "dimensions.h"

#define hamming_weight(x) __builtin_popcount(x)
#define make_t_in_from_mask(mask) (hamming_weight(mask) - 1)

// This structure stores randoms used for the Gauss elimination.
typedef struct _gauss_rand {
  bool is_set;
  int idx;
  uint64_t mask;
} GaussRand;

/* void factorize_inner_mults(const Circuit* c, Dependency** factorized_deps, MultDependency* mult); */
/* void factorize_mults(const Circuit* c, Dependency** local_deps, */
/*                      Dependency** deps1, Dependency** deps2, */
/*                      int* deps1_length, int* deps2_length, */
/*                      int local_deps_len); */


int is_failure(const Circuit* c, int t_in, int comb_len, Comb* tuple,
               bool has_random, SecretDep* secret_deps, Trie* incompr_tuples);

// Finds all failures of size |comb_len|, and calls |failure_callback|
// for each of them.
int find_all_failures(const Circuit* c,             // The circuit
                      int cores,                    // How many threads to use
                      int t_in,                     // The number of shares that must be
                                                    // leaked for a tuple to be a failure
                      VarVector* prefix,             // Prefix to add to all the tuples
                      int comb_len,                 // The length of the tuples
                      int max_len,                  // Maximum length allowed
                      const DimRedData* dim_red_data, // Data to generate the actual tuples
                                                      // after the dimension reduction
                      bool has_random, // Should be false if randoms have been removed
                      Comb* first_tuple,            // The first tuple
                      bool include_outputs,         // If true, include outputs in the tuples
                      Dependency shares_to_ignore,  // Shares that do not count in failures
                                                    // (used only for PINI)
                      bool PINI,                    // If true, we are checking PINI
                      Trie* incompr_tuples,         // The trie of incompressible tuples
                                                    // (set to NULL to disable this optim)
                      void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void* data),
                      //     ^^^^^^^^^^^^^^^^
                      // The function to call when a failure is found
                      void* data // additional data to pass to |failure_callback|
                      );

// Finds the first failure of size |comb_len|, and calls
// |failure_callback| with this failure.
int find_first_failure(const Circuit* c,             // The circuit
                       int cores,                    // How many threads to use
                       int t_in,                     // The number of shares that must be
                                                     // leaked for a tuple to be a failure
                       VarVector* prefix,             // Prefix to add to all the tuples
                       int comb_len,                 // The length of the tuples
                       int max_len,                  // Maximum length allowed
                       const DimRedData* dim_red_data, // Data to generate the actual tuples
                                                       // after the dimension reduction
                       bool has_random, // Should be false if randoms have been removed
                       Comb* first_tuple,            // The first tuple
                       bool include_outputs,         // If true, include outputs in the tuples
                       Dependency shares_to_ignore,  // Shares that do not count in failures
                                                     // (used only for PINI)
                       bool PINI,                    // If true, we are checking PINI
                       Trie* incompr_tuples,         // The trie of incompressible tuples
                                                     // (set to NULL to disable this optim)
                       void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void* data),
                       //     ^^^^^^^^^^^^^^^^
                       // The function to call when a failure is found
                       void* data // additional data to pass to |failure_callback|
                       );


// This is the actual primitive. You probably don't want to call it
// yourself, but rather call find_all_failures or
// find_first_failure. Still, if you know what you are doing, go
// ahead :-)
int _verify_tuples(const Circuit* circuit, // The circuit
                   int t_in, // The number of shares that must be
                             // leaked for a tuple to be a failure
                   VarVector* prefix, // Prefix to add to all the tuples
                   int comb_len, // The length of the tuples (includes prefix->length)
                   int max_len, // Maximum length allowed
                   const DimRedData* dim_red_data, // Data to generate the actual tuples
                                                   // after the dimension reduction
                   bool has_random, // Should be false if randoms have been removed
                   Comb* first_tuple, // The first tuple
                   uint64_t tuple_count, // How many tuples to consider (-1 to consider all)
                   bool include_outputs, // If true, include outputs in the tuples
                   Dependency shares_to_ignore, // Shares that do not count in failures
                                                // (used only for PINI)
                   bool PINI, // If true, we are checking PINI
                   bool stop_at_first_failure, // If true, stops after the first failure
                   bool only_one_tuple, // If true, stops after checking a single tuple
                   SecretDep* secret_deps_out, // The secret deps to set as output
                   Trie* incompr_tuples, // The trie of incompressible tuples
                                         // (set to NULL to disable this optim)
                   void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void*),
                   //    ^^^^^^^^^^^^^^^^
                   // The function to call when a failure is found
                   void* data // additional data to pass to |failure_callback|
                   );


int check_output_uniformity(const Circuit * circuit, BitDep** output_deps, GaussRand * gauss_rands);

int find_first_failure_freeSNI_IOS(const Circuit* c,             // The circuit
                       int cores,                    // How many threads to use
                       int comb_len,                 // The length of the tuples
                       int comb_free_space,
                       const DimRedData* dim_red_data, // Data to generate the actual tuples
                                                       // after the dimension reduction
                       bool has_random,  // Should be false if randoms have been removed
                       BitDep** output_deps,
                       GaussRand * output_gauss_rands,
                       void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void* data),
                       //     ^^^^^^^^^^^^^^^^
                       // The function to call when a failure is found
                       void* data, // additional data to pass to |failure_callback|
                       bool freesni,
                       bool ios);

int _verify_tuples_freeSNI_IOS(const Circuit* circuit, // The circuit
                   int comb_len, // The length of the tuples
                   int comb_free_space,
                   const DimRedData* dim_red_data, // Data to generate the actual tuples
                                                   // after the dimension reduction
                   bool has_random,  // Should be false if randoms have been removed
                   bool stop_at_first_failure, // If true, stops after the first failure
                   BitDep** output_deps,
                   GaussRand * output_gauss_rands,
                   void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void*),
                   //    ^^^^^^^^^^^^^^^^
                   // The function to call when a failure is found
                   void* data, // additional data to pass to |failure_callback|
                   bool freesni,
                   bool ios
                  );