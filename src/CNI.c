#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <inttypes.h>

#include "CNI.h"
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
  int faults;
};

static int generate_names(ParsedFile * pf, char *** names_ptr){

  int length = pf->randoms->next_val + pf->eqs->size;
  // we don't need to consider faults on output shares
  length = length - pf->nb_duplications * pf->shares * pf->out->next_val;

  char ** outputs = malloc(pf->nb_duplications * pf->shares * pf->out->next_val * sizeof(*outputs));
  int idx = 0;
  StrMapElem * o = pf->out->head;
  while(o){
    for(int i=0; i< pf->shares; i++){
      for(int j=0; j< pf->nb_duplications; j++){
        outputs[idx] = malloc(10 * sizeof(*outputs[idx]));
        sprintf(outputs[idx], "%s%d_%d", o->key, i, j);
        idx++;
      }
    }
    o = o->next;
  }
  

  *names_ptr = malloc(length * sizeof(*names_ptr));
  char ** names = *names_ptr;

  idx = 0;
  // StrMapElem * elem = pf->in->head;
  // while(elem){
  //   for(int i=0; i<pf->shares; i++){
  //     for(int j=0; j<pf->nb_duplications; j++){
  //       names[idx] = malloc(10 * sizeof(*names[idx]));
  //       sprintf(names[idx], "%s%d_%d", elem->key, i, j);
  //       idx++;
  //     }
  //   }
  //   elem = elem->next;
  // }

  StrMapElem * elem = pf->randoms->head;
  while(elem){
    names[idx] = strdup(elem->key);
    idx++;
    elem = elem->next;
  }

  EqListElem * elem_eq = pf->eqs->head;
  while(elem_eq){
    bool out = false;
    for(int i=0; i< pf->nb_duplications * pf->shares * pf->out->next_val; i++){
      if(strcmp(elem_eq->dst, outputs[i]) == 0){
        out = true;
        break;
      }
    }
    if(!out){
      names[idx] = strdup(elem_eq->dst);
      idx++;
    }
    elem_eq = elem_eq->next;
  }

  for(int i=0; i<pf->nb_duplications * pf->shares * pf->out->next_val; i++){
    free(outputs[i]);
  }
  free(outputs);

  return length;
}

static void display_failure(const Circuit* c, Comb* comb, int comb_len, SecretDep* secret_deps,
                            void* data_void) {
  (void) secret_deps;

  struct callback_data* data = (struct callback_data*) data_void;

  printf("Gadget is not (%d,%d)-CNI. Example of leaky tuple of size %d:\n",
         data->ni_order, data->faults, comb_len);
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

int compute_CNI(ParsedFile * pf, int cores, int t, int k, bool set) {

  // int length = 1;
  // char ** names = malloc(sizeof(*names));
  // names[0] = strdup("temp18");
  // has_failure = compute_CNI_with_faults_list(pf, cores, t, k, names, length);
  // return 0;
  //print_circuit(circuit);
  
  char ** names;
  int length = generate_names(pf, &names);

  Faults * fv = malloc(sizeof(*fv));
  fv->length = k;

  int has_failure = 0;

  bool has_random = true;
  struct callback_data data = { .ni_order = t, .faults = k };

  for(int i=1; i<=k; i++){

    fv->length = i;

    Comb * comb = first_comb(i, 0);
    do{

      printf("################ Cheking CNI with faults on ");
      for(int j=0; j<i; j++){
        printf("%s, ", names[comb[j]]);
      }
      printf("...\n");

      FaultedVar ** v = malloc(i * sizeof(*v));

      for(int j=0; j<i; j++){
        v[j] = malloc(sizeof(*v[j]));
        v[j]->set = set;
        v[j]->name = names[comb[j]];
        v[j]->fault_on_input = false;
      }

      fv->vars = v;

      Circuit * c = gen_circuit(pf, pf->glitch, pf->transition, fv);
      //print_circuit(c);
      DimRedData* dim_red_data = remove_elementary_wires(c, false);
      
      int has_failure = 0;
      for (int size = 0; size <= t; size++) {
        printf("Checking CNI ==> %" PRIu64 " tuples of size %d to check...\n",
              n_choose_k(size, c->deps->length), size);
        has_failure = find_first_failure(c,
                                        cores,
                                        -1,    // t_in
                                        NULL,  // prefix
                                        size,  // comb_len
                                        t,     // max_len
                                        dim_red_data,  // dim_red_data
                                        has_random, // has_random
                                        NULL,  // first_comb
                                        false, // include_outputs
                                        0,     // shares_to_ignore
                                        false, // PINI
                                        NULL,  // incompr_tuples
                                        display_failure,
                                        (void*)&data);
        if (has_failure) break;
      }

      if (has_failure) {
        print_circuit(c);
        printf("Gadget is not (%d,%d)-CNI with faults on ", data.ni_order, data.faults);
        for(int j=0; j<i; j++){
          printf("%s, ", names[comb[j]]);
        }
        printf("\n");
      }

      for(int j=0; j<i; j++){
        free(v[j]);
      }
      free(v);

      free_dim_red_data(dim_red_data);
      free_circuit(c);
      
      if(has_failure){
        printf("------\n");
        printf("################\n\n");
        return has_failure;
      }

      printf("################\n\n");


    }while(incr_comb_in_place(comb, i, length));

    free(comb);
  }

  for(int i=0; i<length; i++){
    free(names[i]);
  }

  free(names);

  free(fv);

  if(!has_failure){
    printf("Gadget is (%d,%d)-CNI\n", data.ni_order, data.faults);
  }
  

  return !has_failure;
}
