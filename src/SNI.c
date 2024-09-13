#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <inttypes.h>

#include "NI.h"
#include "SNI.h"
#include "config.h"
#include "circuit.h"
#include "list_tuples.h"
#include "combinations.h"
#include "coeffs.h"
#include "verification_rules.h"
#include "constructive.h"
#include "dimensions.h"


struct callback_data {
  int sni_order;
};

static void display_failure(const Circuit* c, Comb* comb, int comb_len, SecretDep* secret_deps,
                            void* data_void) {
  (void) secret_deps;

  struct callback_data* data = (struct callback_data*) data_void;

  printf("\n\nGadget is not %d-SNI. Example of leaky tuple of size %d:\n",
         data->sni_order, comb_len);
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


void compute_SNI_with_incompr(Circuit* circuit, int t) {
  if (t >= circuit->share_count) {
    fprintf(stderr, "Gadget with %d shares cannot be %d-SNI.\n", circuit->share_count, t);
    return;
  }
  Trie* incompr_NI = compute_incompr_tuples(circuit,
                                            t+1, // t_in
                                            NULL, // prefix
                                            t, // max_size
                                            true, // include_outputs
                                            0, // min_outputs
                                            0 // verbose
                                            );
  if (trie_size(incompr_NI)) {
    fprintf(stderr, "Gadget is not NI, and thus not SNI either. The following tuples are leaking: ");
    print_all_tuples(incompr_NI);
    return;
  }
  fprintf(stderr, "Gadget is %d-NI. Checking SNI now...\n\n", t);

  for (int out_size = 1; out_size <= t; out_size++) {
    fprintf(stderr, "\rChecking with %d output wires...", out_size);
    fflush(stderr);
    int share_count_for_failure = t - out_size + 1;
    Trie* incompr = compute_incompr_tuples(circuit,
                                           share_count_for_failure, // t_in
                                           NULL, // prefix
                                           t, // max_size
                                           true, // include_outputs
                                           out_size, // min_outputs
                                           0 // debug
                                           );
    if (trie_size(incompr)) {
      fprintf(stderr, "\rGadget is not SNI. "
              "The following tuples are leaking %d input shares "
              "with %d output probes:\n",
              share_count_for_failure, out_size);
      print_all_tuples(incompr);
      return;
    }
    free_trie(incompr);
  }

  printf("\rGadget is %d-SNI.                                         \n\n", t);

}


void compute_SNI(Circuit* circuit, int cores, int t) {
  DimRedData* dim_red_data = remove_elementary_wires(circuit, true);
  advanced_dimension_reduction(circuit);

  /* if (! circuit->contains_mults) { */
  /*   compute_SNI_with_incompr(circuit, t); */
  /*   return; */
  /* } */

  bool has_random = true;
  if (!circuit->has_input_rands) {
     has_random = false;
     remove_randoms(circuit);
  }

  struct callback_data data = { .sni_order = t };

  // Checking for out_size = 0 (basically like checking NI but excluding outputs)
  fprintf(stderr, "Checking SNI: out_size = 0 ==> %" PRIu64 " tuples...\n",
          n_choose_k(t, circuit->length));
  for (int comb_len = 0; comb_len <= t; comb_len++) {
    if (find_first_failure(circuit,
                           cores,
                           t, // t_in
                           NULL,       // prefix
                           comb_len,   // comb_len
                           t,     // max_len
                           dim_red_data, // dim_red_data
                           has_random, // has_random
                           NULL,  // first_comb
                           false, // include_outputs
                           0,     // shares_to_ignore
                           false, // PINI
                           NULL,  // incompr_tuples
                           display_failure,
                           (void*)&data)) {
      goto end_fail;
    }
  }

  for (int out_size = 1; out_size <= t; out_size++) {
    uint64_t out_comb_len;
    Comb** out_comb_arr = gen_combinations(&out_comb_len, out_size,
                                           circuit->output_count * circuit->share_count - 1);
    VarVector verif_prefix = { .length = out_size, .max_size = out_size, .content = NULL };
    int share_count_for_failure = t - out_size;

    fprintf(stderr, "Checking SNI: out_size = %d ==> %" PRIu64 " tuples...\n", out_size,
           out_comb_len * n_choose_k(t-out_size, circuit->length));

    for (unsigned int j = 0; j < out_comb_len; j++) {
      verif_prefix.content = out_comb_arr[j];
      for (int k = 0; k < out_size; k++) verif_prefix.content[k] += circuit->length;

      for (int comb_len = out_size; comb_len <= t; comb_len++) {
        int has_failure = find_first_failure(circuit,
                                             cores,
                                             share_count_for_failure, // t_in
                                             &verif_prefix,  // prefix
                                             comb_len,     // comb_len
                                             t,     // max_len
                                             dim_red_data, // dim_red_data
                                             has_random, // has_random
                                             NULL,  // first_comb
                                             false, // include_outputs
                                             0,     // shares_to_ignore
                                             false, // PINI
                                             NULL,  // incompr_tuples
                                             display_failure,
                                             (void*)&data);

        if (has_failure) {
          goto end_fail;
        }
      }
    }

    // Freeing combinations
    for (unsigned int i = 0; i < out_comb_len; i++) free(out_comb_arr[i]);
    free(out_comb_arr);
  }
  printf("\n\nGadget is %d-SNI.\n\n", t);

 end_fail:;

  free_dim_red_data(dim_red_data);
}
