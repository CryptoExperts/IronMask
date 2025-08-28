#ifndef CONSTRUCTIVE_ARITH_H
#define CONSTRUCTIVE_ARITH_H

#pragma once

#include "circuit.h"
#include "vectors.h"
#include "trie.h"

void build_dependency_arrays_arith(const Circuit* c,
                                   VarVector*** secrets,
                                   VarVector*** randoms,
                                   bool include_outputs,
                                   int verbose);

void add_tuple_to_trie_arith(Trie* incompr_tuples, Tuple* curr_tuple,
                             const Circuit* c, int secret_idx, 
                             int revealed_secret);


int get_first_rand_arith(Dependency* dep, int deps_size, int first_rand_idx);

void apply_gauss_arith(int deps_size,
                       Dependency* real_dep,
                       Dependency** gauss_deps,
                       Dependency* gauss_rands,
                       int idx,
                       int characteristic);

Trie* compute_incompr_tuples_arith(const Circuit* c,
                                   int t_in,  // The number of shares that must be
                                        // leaked for a tuple to be a failure
                                   VarVector* prefix, // Prefix to add to all the tuples
                                   int max_size, // The maximal size of the incompressible tuples
                                   bool include_outputs, // if true, includes outputs
                                   int min_outputs, // Number of outputs required per tuple
                                   bool RPC,
                                   int cores,
                                   bool one_failure,
                                   int verbose);

void compute_RP_coeffs_incompr_arith(const Circuit* c, int coeff_max, int cores, 
                                     int debug);

void compute_RPC_coeffs_incompr_arith(const Circuit* c, int coeff_max, 
                                      bool include_output, int required_output, 
                                      int cores, int debug);

void compute_RPE_coeffs_incompr_arith(const Circuit* c, int coeff_max,
                                      bool include_output , int required_output,
                                      int cores, int verbose);
                                      
#endif
