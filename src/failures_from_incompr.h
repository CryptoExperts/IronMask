#pragma once

#include "circuit.h"
#include "trie.h"

void determine_HASH_MASK(int c_len, int coeff_max);

void compute_failures_from_incompressibles(const Circuit* c, Trie* incompr,
                                           int coeff_max, int verbose);
                                           
void compute_failures_from_incompressibles_RPC(const Circuit* c, Trie* incompr,
                                               Trie *incompr2, int coeff_max, 
                                               int verbose, uint64_t *coeffs,
                                               bool RPE_and);
                                               
void compute_failures_from_incompressibles_RPE2_parallel(const Circuit* c, 
                                                         Trie **incompr,
                                                         Trie **incompr2, 
                                                         int coeff_max, 
                                                         int verbose, 
                                                         uint64_t *coeffs,
                                                         uint64_t *coeffs2, 
                                                         uint64_t *coeffs_and,
                                                         int cores);
                                                
void compute_failures_from_incompressibles_RPE2_single(const Circuit* c, 
                                                       Trie **incompr,
                                                       int len_incompr,
                                                       int coeff_max, 
                                                       int verbose, 
                                                       uint64_t *coeffs);
