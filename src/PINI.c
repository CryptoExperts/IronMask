#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "NI.h"
#include "PINI.h"
#include "config.h"
#include "circuit.h"
#include "list_tuples.h"
#include "combinations.h"
#include "coeffs.h"
#include "verification_rules.h"
#include "dimensions.h"
#include "constructive.h"



struct callback_data {
  int pini_order;
};

static void display_failure(const Circuit* c, Comb* comb, int comb_len, SecretDep* secret_deps,
                            void* data_void) {
  (void) secret_deps;

  struct callback_data* data = (struct callback_data*) data_void;

  printf("Gadget is not %d-PINI. Example of leaky tuple of size %d:\n",
         data->pini_order, comb_len);
  printf("  [ ");
  for (int i = 0; i < comb_len; i++) {
    printf("%d ", comb[i]);
  }
  printf("]  (with ids: [ ");
  for (int i = 0; i < comb_len; i++) {
    printf("%s ",c->deps->names[comb[i]]);
  }
  printf("])\n\n");
}



void compute_PINI(Circuit* circuit, int cores, int t) {
  if (t >= circuit->share_count) {
    printf("Gadget with %d shares cannot be %d-PINI.\n\n", circuit->share_count, t);
    return;
  }

  struct callback_data data;
  data.pini_order = t;


  if (find_first_failure(circuit,
                         cores,
                         t,     // t_in
                         NULL,  // prefix
                         t,     // comb_len
                         t,     // max_len
                         NULL,  // dim_red_data
                         true,  // has_random
                         NULL,  // first_comb
                         false, // include_outputs
                         0,     // shares_to_ignore
                         true,  // PINI
                         NULL,  // incompr_tuples
                         display_failure,
                         (void*)&data)) {
    goto end_fail;
  }
  goto end_success;

  for (int out_size = 1; out_size < t; out_size++) {
    uint64_t out_comb_len;
    Comb** out_comb_arr = gen_combinations(&out_comb_len, out_size,
                                           circuit->output_count * circuit->share_count - 1);
    VarVector verif_prefix = { .length = out_size, .max_size = out_size, .content = NULL };
    int share_count_for_failure = t - out_size;

    for (unsigned int j = 0; j < out_comb_len; j++) {
      verif_prefix.content = out_comb_arr[j];
      for (int k = 0; k < out_size; k++) verif_prefix.content[k] += circuit->length;
      Dependency shares_to_ignore = 0;
      for (int k = 0; k < out_size; k++) {
        shares_to_ignore |= 1 << out_comb_arr[j][k];
      }


      int has_failure = find_first_failure(circuit,
                                           cores,
                                           share_count_for_failure, // t_in
                                           &verif_prefix,  // prefix
                                           t,     // comb_len
                                           t,     // max_len
                                           NULL,  // dim_red_data
                                           true,  // has_random
                                           NULL,  // first_comb
                                           false, // include_outputs
                                           shares_to_ignore,
                                           true,  // PINI
                                           NULL,  // incompr_tuples
                                           display_failure,
                                           (void*)&data);

      if (has_failure) {
        goto end_fail;
      }
    }

    // Freeing combinations
    for (unsigned int i = 0; i < out_comb_len; i++) free(out_comb_arr[i]);
    free(out_comb_arr);
  }
 end_success:
  printf("Gadget is %d-PINI.\n\n", t);

 end_fail:;
}
