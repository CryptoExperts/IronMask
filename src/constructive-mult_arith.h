#pragma once

#include "circuit.h"
#include "trie.h"

Trie* compute_incompr_tuples_mult_arith(const Circuit* c, int coeff_max,
                                  bool include_outputs, int required_outputs,
                                  bool RPC, int t_in, int cores, 
                                  bool one_failure, int verbose);
                                  
Trie** compute_incompr_tuples_mult_RPE_arith(const Circuit* c, int coeff_max, 
                                       bool include_outputs, 
                                       int required_outputs, int t_in, 
                                       int cores, int verbose); 
