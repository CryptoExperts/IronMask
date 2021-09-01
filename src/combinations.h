#pragma once

#include <stdlib.h>
#include <stdint.h>

#include "circuit.h"

typedef Var Comb;

uint64_t n_choose_k(int k, int n);
uint64_t rank(int n, int k, Comb* comb);
Comb* unrank(int n, int k, uint64_t idx);

Comb* first_comb(int k, int over_alloc);
int incr_comb_in_place(Comb* comb, int k, int max);

int gen_combinations_batch(Comb** combs, Comb** prev, int k, int max,
                           int batch_size, int over_alloc);

/* // Generates all combinations of size |k| of integers from 0 to |max| (included) */
Comb** gen_combinations(uint64_t* len, int k, int max);

void gen_combinations_no_alloc(Comb** combs, int k, int max);

int is_sorted_comb(Comb* comb, int comb_len);
void sort_comb(Comb* comb, int comb_len);
