#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "combinations.h"

// Iterative approach to compute n choose k to avoid overflows
uint64_t n_choose_k(int k, int n) {
  double res = 1;
  for (int i = k; i > 0; i--) {
    res *= n--;
    res /= k--;
  }
  return (uint64_t)round(res);
}

// Returns the rank of the combination |comb|
uint64_t rank(int n, int k, Comb* comb) {
  uint64_t idx = n_choose_k(k,n);
  for (int m = 0; m < k; m++) {
    idx -= n_choose_k(m+1, n-comb[k-m-1]-1);
  }
  return idx;
}

// Returns the combination whose rank is |idx|
Comb* unrank(int n, int k, uint64_t idx) {
  Comb* comb = malloc(k * sizeof(*comb));
  int comb_insert_idx = 0;
  int n_orig = n;
  int k_orig = k;
  idx = n_choose_k(k,n) - idx;

  n--;

  while (comb_insert_idx != k_orig) {
    uint64_t candidate = n_choose_k(k,n);
    if (candidate <= idx) {
      comb[comb_insert_idx++] = n_orig - n - 1;
      idx -= candidate;
      k--;
    }
    n--;
  }

  return comb;
}


int is_sorted_comb(Comb* comb, int comb_len) {
  Comb prev = comb[0];
  for (int i = 1; i < comb_len; i++) {
    if (comb[i] < prev) return 0;
    prev = comb[i];
  }
  return 1;
}

// TODO: use quick-sort or something like that
// TODO: can do even better than quick-sort actually:
//   Comb tmp[SOME_SIZE] = { 0 };
//   for (int i = 0; i < comb_len; i++)
//       tmp[comb[i]] = 1;
//   int idx = 0;
//   for (int i = 0; i < SOME_SIZE; i++)
//       if (tmp[i])
//           comb[idx++] = i;
// The complexity of this sort is linear. SOME_SIZE should
// probably be max(comb). For small combs, this might be slower
// than a quadratic sort...
// TODO: maybe use bubble sort for small comb_len and the sort I
// mention above for large comb_len?
void sort_comb(Comb* comb, int comb_len) {
  for (int i = 1; i < comb_len; i++) {
    if (comb[i] < comb[i-1]) {
      for (int j = i; j > 0 && comb[j] < comb[j-1]; j--) {
        Comb tmp = comb[j];
        comb[j] = comb[j-1];
        comb[j-1] = tmp;
      }
    }
  }
}

/***********************************************************
                Batching Combinations
************************************************************/

Comb* first_comb(int k, int over_alloc) {
  Comb* comb = malloc((k + over_alloc) * sizeof(*comb));
  for (int i = 0; i < k; i++) {
    comb[i] = i;
  }
  return comb;
}

int incr_comb_in_place(Comb* comb, int k, int max) {
  int i = k-1;
  for ( ; i >= 0; i--) {
    if (comb[i] < i + max - k) {
      int new_val = ++comb[i];
      for ( i++; i < k; i++ ) {
        comb[i] = ++new_val;
      }
      return 1;
    }
  }
  return 0;
}

Comb* incr_comb(Comb* prev, int k, int max, int over_alloc) {
  Comb* next = malloc((k + over_alloc) * sizeof(*next));
  next = memcpy(next, prev, k * sizeof(*next));
  if (incr_comb_in_place(next,k,max)) {
    return next;
  } else {
    free(next);
    return NULL;
  }
}

int incr_comb_n_times(Comb* comb, int k, int max, int n) {
  for (int i = 0; i < n; i++) {
    if (! incr_comb_in_place(comb, k, max)) {
      return 0;
    }
  }
  return 1;
}


// Generates |batch_size| combinations inside |combs|. Returns the
// batch size (could be less than |batch_size| for the last batch).
// If |over_alloc| is not 0, then the that many empty elements are
// left at the end of each combination (which can be filled with
// outputs for instance, in order to verify RPC/RPE). The content of
// the extra indices is undefined.
int gen_combinations_batch(Comb** combs, Comb** prev, int k, int max,
                           int batch_size, int over_alloc) {
  Comb* curr = *prev == NULL ? first_comb(k, over_alloc) :
    incr_comb(*prev, k, max+1, over_alloc);
  int i = 0;
  for (; i < batch_size && curr; i++) {
    combs[i] = curr;
    curr = incr_comb(curr, k, max+1, over_alloc);
  }

  free(*prev);
  if (i > 0) {
    *prev = malloc((k + over_alloc) * sizeof(**prev));
    for (int j = 0; j < k; j++) (*prev)[j] = combs[i-1][j];
  }

  return i;
}


/***********************************************************
                Non-Batching Combinations
************************************************************/

void _gen_combinations_aux(Comb** combs, int combs_idx, int j, int k, int max) {
  if (k == 0 || max+1 < k) return;

  if (max == 0) {
    combs[combs_idx][j] = 0;
    return;
  }

  int count = n_choose_k(k,max+1) - n_choose_k(k,max);

  for (int i = 0; i < count; i++) {
    combs[combs_idx+i][j] = max;
  }
  _gen_combinations_aux(combs, combs_idx, j+1, k-1, max-1);
  _gen_combinations_aux(combs, combs_idx + count, j, k, max-1);
}

/* Generates all combinations of size |k| of integers from 0 to |max|
   (included). More efficient than |gen_combinations_batch|, but
   cannot do batching. */
Comb** gen_combinations(uint64_t* len, int k, int max) {
  // Allocating storage
  int total_combs = n_choose_k(k,max+1);
  *len = total_combs;
  Comb** combs = malloc(total_combs * sizeof(*combs));
  for (int i = 0; i < total_combs; i++)
    combs[i] = malloc(k * sizeof(*combs[i]));
  _gen_combinations_aux(combs,0,0,k,max);
  return combs;
}

/* Like |gen_combinations|, but memory is taken as parameter rather
   than using malloc. */
void gen_combinations_no_alloc(Comb** combs, int k, int max) {
  _gen_combinations_aux(combs,0,0,k,max);
}
