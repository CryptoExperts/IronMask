#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <gmp.h>

#include "parser.h"
#include "list_tuples.h"

void update_coeff_c_single(const Circuit* c, uint64_t* coeff_c, Comb* comb, int comb_len);
void update_coeff_c(const Circuit* c, uint64_t* coeff_c, ListComb* combs, int comb_len);

void initialize_table_coeffs();

double compute_leakage_proba(uint64_t* coeffs, int last_precise_coeff, int len,
                             int min_max, bool square_root);

void get_failure_proba(uint64_t* coeffs, int len, double p, int coeff_max);


void compute_combined_intermediate_leakage_proba(uint64_t* coeffs, int k, int total, int coeffs_size, double p, double f, mpf_t res, int c_max);

void compute_combined_intermediate_mu(int k, int total, double f, mpf_t res);
void compute_combined_mu_max(int k, int total, double f, mpf_t res);

void compute_combined_final_proba(mpf_t epsilon, mpf_t mu);
