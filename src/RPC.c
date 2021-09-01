#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#include "RPC.h"
#include "config.h"
#include "circuit.h"
#include "list_tuples.h"
#include "combinations.h"
#include "coeffs.h"
#include "verification_rules.h"

struct callback_data {
  int t;
  uint64_t* coeffs;
};


static void update_coeffs(const Circuit* c, Comb* comb, int comb_len, SecretDep* secret_deps,
                          void* data_void) {
  (void) secret_deps;
  struct callback_data* data = (struct callback_data*) data_void;
  int t = data->t;

  update_coeff_c_single(c, data->coeffs, &comb[t], comb_len-t);
}

void compute_RPC_coeffs(Circuit* circuit, int cores, int coeff_max,
                        int opt_incompr, int t, int t_output) {
  // Initializing coefficients
  uint64_t coeffs[circuit->total_wires+1];
  for (int i = 0; i <= circuit->total_wires; i++) {
    coeffs[i] = 0;
  }

  if (coeff_max == -1) {
    coeff_max = circuit->length;
  }

  Trie* incompr_tuples = opt_incompr ? make_trie(circuit->length) : NULL;

  // Generating combinations of |t| elements corresponding to the outputs
  uint64_t out_comb_len;
  Comb** out_comb_arr = gen_combinations(&out_comb_len, t_output,
                                         circuit->output_count * circuit->share_count - 1);
  for (unsigned int i = 0; i < out_comb_len; i++) {
    for (int k = 0; k < t_output; k++) {
      out_comb_arr[i][k] += circuit->length;
    }
  }

  uint64_t** coeffs_out_comb;
  coeffs_out_comb = malloc(out_comb_len * sizeof(*coeffs_out_comb));
  for (unsigned i = 0; i < out_comb_len; i++) {
    coeffs_out_comb[i] = calloc(circuit->total_wires + 1, sizeof(*coeffs_out_comb[i]));
  }

  VarVector verif_prefix = { .length = t_output, .max_size = t_output, .content = NULL };

  struct callback_data data = { .t = t_output, .coeffs = NULL };


  // Computing coefficients
  printf("f(p) = [ "); fflush(stdout);
  for (int size = 1; size <= coeff_max; size++) {

    for (unsigned int i = 0; i < out_comb_len; i++) {
      verif_prefix.content = out_comb_arr[i];
      data.coeffs = coeffs_out_comb[i];

      find_all_failures(circuit,
                        cores,
                        t, // t_in
                        &verif_prefix,  // prefix
                        size+verif_prefix.length, // comb_len
                        size+verif_prefix.length, // max_len
                        NULL,  // dim_red_data
                        true,  // has_random
                        NULL,  // first_comb
                        false, // include_outputs
                        0,     // shares_to_ignore
                        false, // PINI
                        incompr_tuples, // incompr_tuples
                        update_coeffs,
                        (void*)&data);

#define max(a,b) ((a) > (b) ? (a) : (b))
      coeffs[size] = max(coeffs[size], coeffs_out_comb[i][size]);
    }

    printf("%lu, ", coeffs[size]);
    fflush(stdout);
  }

  // Printing the remaining coefficients
  for (int i = coeff_max+1; i <= circuit->total_wires; i++) {
    for (unsigned j = 0; j < out_comb_len; j++) {
      coeffs[i] = max(coeffs[i], coeffs_out_comb[j][i]);
    }
    printf("%lu%s ", coeffs[i], i == circuit->total_wires ? "" : ",");
  }
  printf("]\n");


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


  // Freeing stuffs
  for (unsigned i = 0; i < out_comb_len; i++) {
    free(out_comb_arr[i]);
    free(coeffs_out_comb[i]);
  }
  free(out_comb_arr);
  free(coeffs_out_comb);
  if (incompr_tuples) free_trie(incompr_tuples);
}
