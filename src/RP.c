#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#include "RP.h"
#include "config.h"
#include "circuit.h"
#include "list_tuples.h"
#include "combinations.h"
#include "coeffs.h"
#include "verification_rules.h"
#include "dimensions.h"


struct callback_data {
  uint64_t* coeffs;
  bool dimension_reduction;
  Circuit* init_circuit;
  int* new_to_old_mapping;
};


static void update_coeffs(const Circuit* c, Comb* comb, int comb_len, SecretDep* secret_deps,
                          void* data_void) {
  struct callback_data* data = (struct callback_data*) data_void;
  (void) secret_deps;
  uint64_t* coeffs = data->coeffs;

  /* printf("[ "); */
  /* for (int i = 0; i < comb_len; i++) printf("%d ", comb[i]); */
  /* printf("]\n"); */

  update_coeff_c_single(c, coeffs, comb, comb_len);
}


void compute_RP_coeffs(Circuit* circuit, int cores, int coeff_max, int opt_incompr) {
  // Initializing coefficients
  uint64_t coeffs[circuit->total_wires+1];
  for (int i = 0; i <= circuit->total_wires; i++) {
    coeffs[i] = 0;
  }

  DimRedData* dim_red_data = remove_elementary_wires(circuit);
  int coeff_max_main_loop = coeff_max == -1 ? circuit->length :
    coeff_max > circuit->length ? circuit->length : coeff_max;

  if (coeff_max == -1) {
    coeff_max = dim_red_data->old_circuit->length;
  }

  Trie* incompr_tuples = opt_incompr ? make_trie(circuit->length) : NULL;

  struct callback_data data = {
    .coeffs = coeffs,
  };


  // Computing coefficients
  printf("f(p) = [ "); fflush(stdout);
  for (int size = 0; size <= coeff_max_main_loop; size++) {

    find_all_failures(circuit,
                      cores,
                      -1,    // t_in
                      NULL,  // prefix
                      size,  // comb_len
                      coeff_max,  // max_len
                      dim_red_data,
                      true, // has_random
                      NULL,  // first_comb
                      false,  // include_outputs
                      0,     // shares_to_ignore
                      false, // PINI
                      incompr_tuples,
                      update_coeffs,
                      (void*)&data);

    // A failure of size 0 is not possible. However, we still want to
    // iterate in the loop with |size| = 0 to generate the tuples with
    // only elementary shares (which, because of the dimension
    // reduction, are never generated otherwise).
    if (size > 0) {
      printf("%llu, ", coeffs[size]); fflush(stdout);
    }
  }

  for (int i = coeff_max_main_loop+1; i < dim_red_data->old_circuit->total_wires-1; i++) {
    printf("%llu, ", coeffs[i]);
  }
  printf("%llu ]\n", coeffs[circuit->total_wires]);

  double p_min = compute_leakage_proba(coeffs, coeff_max,
                                       circuit->total_wires+1,
                                       1, // minimax
                                       false); // square root
  double p_max = compute_leakage_proba(coeffs, coeff_max,
                                       circuit->total_wires+1,
                                       -1, // minimax
                                       false); // square root

  printf("\n");
  printf("pmax = %.10f -- log2(pmax) = %.10f\n", p_max, log2(p_max));
  printf("pmin = %.10f -- log2(pmin) = %.10f\n", p_min, log2(p_min));
  printf("\n");
}
