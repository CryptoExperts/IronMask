#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <gmp.h>

#include "coeffs.h"
#include "parser.h"
#include "list_tuples.h"
#include "combinations.h"


#define table_coeff_size 65 // above 67, binomial coefficients will overflow 64-bit integers
static uint64_t table_coeff[table_coeff_size][table_coeff_size];
static uint64_t table_pow[table_coeff_size][table_coeff_size];
static bool table_coeff_initialized;

// Using a custom Array structure instead of a intVector (of
// structure.h) because we don't want anything to be malloced here to
// be as fast as possible.
typedef struct _array {
  int length;
  uint64_t* content;
} Array;

void compute_tree2(Array current_uple, uint64_t* coeffs, int nb_occ_tuple) {
  if (nb_occ_tuple == current_uple.length) {
    coeffs[nb_occ_tuple] += 1;
    return;
  }
  if (nb_occ_tuple == current_uple.length+1) {
    coeffs[nb_occ_tuple]   += 2;
    coeffs[nb_occ_tuple+1] += 1;
    return;
  }
  int lst[nb_occ_tuple+1]; // TODO: is this large enough??
  int nmin = 1;
  int nmax = current_uple.content[0];
  for (int i = 1; (unsigned long)i < current_uple.content[0] + 1; i++) {
    lst[i] = table_coeff[current_uple.content[0]][i];
  }
  for (int k = 1; k < current_uple.length; k++) {
    int elem = current_uple.content[k];
    if (elem == 1) {
      for (int j = nmax; j > nmin-1; j--) {
        lst[j+1] = lst[j];
      }
    } else {
      for (int i = 1; i < elem+1; i++) {
        lst[i+nmax] = lst[nmax] * table_coeff[elem][i];
      }
      for (int j = nmax-1; j > nmin-1; j--) {
        lst[1+j] = lst[j] * table_coeff[elem][1];
        for (int i = 2; i < elem+1; i++) {
          lst[i+j] += lst[j] * table_coeff[elem][i];
        }
      }
    }
    nmin = nmin + 1;
    nmax = nmax + elem;
  }
  for (int k = nmin; k < nmax+1; k++) {
    coeffs[k] += lst[k];
  }
}

// Warning: this implement is not correct (yet anyways)
void compute_tree3(Array current_uple, uint64_t* coeffs, int nb_occ_tuple) {
  int tmp = current_uple.length;
  if (nb_occ_tuple == current_uple.length) {
    coeffs[nb_occ_tuple] += 1;
    return;
  }
  if (nb_occ_tuple == current_uple.length+1) {
    coeffs[nb_occ_tuple]   += 2;
    coeffs[nb_occ_tuple+1] += 1;
    return;
  }

  int n = current_uple.length;
  int n_2 = 0, n_3 = 0;
  for (int i = 0; i < n; i++) {
    if (current_uple.content[i] == 2) n_2++;
    else if (current_uple.content[i] == 3) n_3++;
  }

  for (int m = 0; m <= nb_occ_tuple - current_uple.length; m++) {
    int sum = 0;
    for (int i = 0; i <= n_2 && i <= m; i++) {
      for (int j = 0; j <= n_3 && j <= m; j++) {
        for (int k = 0; k <= n_3 && k*2 <= m; k++) {
          if (i + j + k*2 != m) continue;
          if (j+k > n_3) continue;
          sum += table_coeff[n_2][i] * table_coeff[n_3][j] * table_coeff[n_3-j][k] *
            table_pow[2][n_2-i] * table_pow[3][n_3-j-k] * table_pow[3][j];
        }
      }
    }
    coeffs[tmp + m] += sum;
  }
}

void update_coeff_c_single(const Circuit* c, uint64_t* coeff_c, Comb* comb, int comb_len) {
  //assert(table_coeff_initialized); // Disabling because fairly costly

  uint64_t nb_occ_tuple = 0;
  Array uple;
  uple.length = comb_len;
  uple.content = alloca(comb_len * sizeof(*uple.content));
  for (int i = 0; i < comb_len; i++) {
    nb_occ_tuple += c->weights[comb[i]];
    uple.content[comb_len-i-1] = c->weights[comb[i]];
  }

  compute_tree2(uple, coeff_c, (int)nb_occ_tuple);
}

void update_coeff_c(const Circuit* c, uint64_t* coeff_c, ListComb* combs, int comb_len) {
  ListCombElem* curr = combs->head;
  Array uple;
  uple.length = comb_len;
  uple.content = alloca(comb_len * sizeof(*uple.content));
  while (curr) {
    uint64_t nb_occ_tuple = 0;
    for (int i = 0; i < comb_len; i++) {
      nb_occ_tuple += c->weights[curr->comb[i]];
      uple.content[comb_len-i-1] = c->weights[curr->comb[i]];
    }

    compute_tree2(uple, coeff_c, (int)nb_occ_tuple);

    curr = curr->next;
  }
}

// It is a bit sad that this function has to be called manually before
// the first call to update_coeff_c. However:
//  - recomputing table_coeff everytime would be too slow
//  - using a static variable inside update_coeff_c would not be
//    thread-safe
//  - overcoming the previous bullet point with mutexes (or similar)
//    would be too slow
// Still, there is currently an assert in update_coeff_c to make sure
// that table_coeff_initialized is indeed initialized; I guess that's
// better than nothing.
void initialize_table_coeffs() {
  table_coeff_initialized = true;
  for (int n = 0; n < table_coeff_size; n++) {
    for (int k = 0; k < table_coeff_size; k++) {
      table_coeff[n][k] = n_choose_k(k,n);
    }
  }
  for (int n = 0; n < table_coeff_size; n++) {
    for (int k = 0; k < table_coeff_size; k++) {
      table_pow[k][n] = pow(k, n);
    }
  }
}

// Computes n_choose_k using GMP floating point numbers
void n_choose_k_gmp(int k, int n, mpf_t res) {

  mpf_init_set_ui(res, 1);

  for (int i = k; i > 0; i--) {
    mpf_mul_ui(res, res, n);
    mpf_div_ui(res, res, k);
    n--;
    k--;
  }

}

// Computes p such that f(p) = p where f is the function defined by
// |coeffs| as:
//
//      f(x) = x * coeffs[0] + x * coeffs[1]**2 + x * coeffs[2] ** 3 ...
//
// Or, written in another manner:
//
//      f(x) = \sum_{i=0}{len} x * coeffs[i]**i
//
// After |last_precise_coeff|, coefficients are not fully computed,
// but can be non-zero because we compute on variables rather than
// wires.
// if |min_max| == 1, then replace unknown coefficients by n choose k
// if |min_max| == -1, then replace unknown coefficients by 0
double compute_leakage_proba(uint64_t* coeffs, int last_precise_coeff, int len,
                             int min_max, bool square_root) {
  mpf_t coeffs_max[len];

  for (int i = 0; i < last_precise_coeff+1; i++) {
    mpf_init_set_ui(coeffs_max[i], coeffs[i]);
  }

  for (int i = last_precise_coeff+1; i < len; i++) {
    if (min_max == 1) {
      n_choose_k_gmp(i, len, coeffs_max[i]);
    } else {
      mpf_init(coeffs_max[i]);
    }
  }

  // Binary search to find leakage proba p
  double p_inf = 0, p_sup = 1, epsilon = 0.000000000001;
  while ( fabs(p_inf - p_sup) > epsilon ) {
    double p = (p_inf + p_sup) / 2;

    mpf_t fp;
    mpf_init(fp);

    for (int i = 1; i < len; i++) {
      mpf_t tmp;
      mpf_init_set_d(tmp, p);
      mpf_pow_ui(tmp, tmp, i);
      mpf_mul(tmp, tmp, coeffs_max[i]);
      mpf_add(fp, fp, tmp);

      mpf_clear(tmp);
    }

    if (square_root) {
      mpf_sqrt(fp, fp);
    }

    if (mpf_cmp_d(fp, p) == 0) break;
    if (mpf_cmp_d(fp, p) == 1) { // f(p) > p
      p_sup = p;
    } else { // f(p) < p
      p_inf = p;
    }

    mpf_clear(fp);
  }

  for (int i = 0; i < len; i++) {
    mpf_clear(coeffs_max[i]);
  }

  return (p_inf+p_sup)/2;
}
