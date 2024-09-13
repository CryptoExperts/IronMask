#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <inttypes.h>

#include "freeSNI.h"
#include "config.h"
#include "circuit.h"
#include "list_tuples.h"
#include "combinations.h"
#include "coeffs.h"
#include "verification_rules.h"
#include "dimensions.h"
#include "constructive.h"
#include "trie.h"
#include "vectors.h"

struct callback_data {
  int ni_order;
};

static void display_failure(const Circuit* c, Comb* comb, int comb_len, SecretDep* secret_deps,
                            void* data_void) {
  (void) secret_deps;

  struct callback_data* data = (struct callback_data*) data_void;

  printf("Gadget is not free-%d-SNI. Example of leaky tuple of size %d:\n",
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


int compute_freeSNI(Circuit* circuit, int cores, int t) {
  //Var* unused;
  //refine_circuit(circuit, &unused);

  if(circuit->secret_count > 2){
    printf("GADGET WITH MORE THAN 2 INPUTS NOT SUPPORTED YET !\n\n");
    return 0;
  }

  DimRedData* dim_red_data = remove_elementary_wires(circuit, true);
  //advanced_dimension_reduction(circuit);
  struct callback_data data = { .ni_order = t };

  bool has_random = true;
  // if (!circuit->has_input_rands) {
  //    has_random = false;
  //    remove_randoms(circuit);
  // }


  GaussRand output_gauss_rands[circuit->share_count];
  BitDep * output_deps[circuit->share_count];
  for(int i=0; i<(circuit->share_count); i++){
    output_deps[i] = alloca(sizeof(**output_deps));
  }
  int has_failure = check_output_uniformity(circuit, output_deps, output_gauss_rands);
  if(has_failure){
    printf("Gadget is not freeSNI. Output Sharing is not independent and uniform !\n");
    free_dim_red_data(dim_red_data);
    return has_failure;
  }
  
  for (int size = 1; size <= t; size++) {
    printf("############################\n");
    printf("Checking freeSNI ==> %"PRIu64" tuples of size %d to check...\n",
           n_choose_k(size, circuit->deps->length), size);
    has_failure = find_first_failure_freeSNI_IOS(circuit,
                                     cores,
                                     size,  // comb_len
                                     t-size, 
                                     dim_red_data,  // dim_red_data
                                     has_random,
                                     output_deps,
                                     output_gauss_rands,
                                     display_failure,
                                     (void*)&data,
                                     true,
                                     false);
    if (has_failure) break;
  }

  if (!has_failure) {
    printf("Gadget is free-%d-SNI.\n\n", t);
  }

  free_dim_red_data(dim_red_data);

  return !has_failure;
}
