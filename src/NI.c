#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "NI.h"
#include "config.h"
#include "circuit.h"
#include "list_tuples.h"
#include "combinations.h"
#include "coeffs.h"
#include "verification_rules.h"
#include "dimensions.h"
#include "constructive.h"


struct callback_data {
  int ni_order;
};

static void display_failure(const Circuit* c, Comb* comb, int comb_len, SecretDep* secret_deps,
                            void* data_void) {
  (void) secret_deps;

  struct callback_data* data = (struct callback_data*) data_void;

  printf("Gadget is not %d-NI. Example of leaky tuple of size %d:\n",
         data->ni_order, comb_len);
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

int compute_NI_constr(Circuit* circuit, int t) {

  Trie* incompr = compute_incompr_tuples(circuit,
                                         t+1, // t_in
                                         NULL, // prefix
                                         t, // max_size
                                         true, // include_outputs
                                         -1, // min_outputs
                                         0 // debug
                                         );

  if (trie_size(incompr)) {
    printf("Gadget is not NI. "
           "The following tuples contain %d probes (or less), "
           "and leak %d input shares:\n",
           t, t+1);
    print_all_tuples(incompr);
    printf("\n");
    return 0;
  }

  printf("Gadget is %d-NI.\n\n", t);
  return 1;
}

int compute_NI(Circuit* circuit, int cores, int t) {
  //Var* unused;
  //refine_circuit(circuit, &unused);
  if (! circuit->contains_mults) {
    advanced_dimension_reduction(circuit);
    return compute_NI_constr(circuit, t);
  }

  DimRedData* dim_red_data = remove_elementary_wires(circuit);
  advanced_dimension_reduction(circuit);
  bool has_random = true;
  if (!circuit->has_input_rands) {
    has_random = false;
    remove_randoms(circuit);
  }

  struct callback_data data = { .ni_order = t };

  int has_failure = 0;
  for (int size = 0; size <= t; size++) {
    printf("Checking NI ==> %'lu tuples of size %d to check...\n",
           n_choose_k(size, circuit->deps->length), size);
    has_failure = find_first_failure(circuit,
                                     cores,
                                     -1,    // t_in
                                     NULL,  // prefix
                                     size,  // comb_len
                                     t,     // max_len
                                     dim_red_data,  // dim_red_data
                                     has_random, // has_random
                                     NULL,  // first_comb
                                     true, // include_outputs
                                     0,     // shares_to_ignore
                                     false, // PINI
                                     NULL,  // incompr_tuples
                                     display_failure,
                                     (void*)&data);
    if (has_failure) break;
  }

  if (!has_failure) {
    printf("Gadget is %d-NI.\n\n", t);
  }

  free_dim_red_data(dim_red_data);

  return !has_failure;
}
