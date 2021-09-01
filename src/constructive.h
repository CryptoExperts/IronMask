#pragma once

#include "circuit.h"
#include "vectors.h"
#include "trie.h"

void build_dependency_arrays(const Circuit* c,
                             VarVector*** secrets,
                             VarVector*** randoms,
                             bool include_outputs,
                             int verbose);

int get_first_rand(Dependency* dep, int deps_size, int first_rand_idx);
void apply_gauss(int deps_size,
                 Dependency* real_dep,
                 Dependency** gauss_deps,
                 Dependency* gauss_rands,
                 int idx);

Trie* compute_incompr_tuples(const Circuit* c,
                             int t_in,  // The number of shares that must be
                                        // leaked for a tuple to be a failure
                             VarVector* prefix, // Prefix to add to all the tuples
                             int max_size, // The maximal size of the incompressible tuples
                             bool include_outputs, // if true, includes outputs
                             int min_outputs, // Number of outputs required per tuple
                             int verbose);

void compute_RP_coeffs_incompr(const Circuit* c, int coeff_max, int debug);
