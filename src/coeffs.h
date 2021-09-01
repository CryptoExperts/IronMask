#pragma once

#include <stdlib.h>
#include <stdint.h>

#include "parser.h"
#include "list_tuples.h"

void update_coeff_c_single(const Circuit* c, uint64_t* coeff_c, Comb* comb, int comb_len);
void update_coeff_c(const Circuit* c, uint64_t* coeff_c, ListComb* combs, int comb_len);

void initialize_table_coeffs();

double compute_leakage_proba(uint64_t* coeffs, int last_precise_coeff, int len,
                             int min_max, bool square_root);
