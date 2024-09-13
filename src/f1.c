#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "circuit.h"
#include "vectors.h"

BitDep * init_bit_dep(){
  BitDep * bit_dep = malloc(sizeof(*bit_dep));
  bit_dep->constant = 0;
  bit_dep->out = 0;
  bit_dep->secrets[0] = bit_dep->secrets[1] = 0;

  memset(bit_dep->randoms, 0, RANDOMS_MAX_LEN*sizeof(*bit_dep->randoms));
  memset(bit_dep->mults, 0, BITMULT_MAX_LEN*sizeof(*bit_dep->mults));
  memset(bit_dep->correction_outputs, 0, BITCORRECTION_OUTPUTS_MAX_LEN*sizeof(*bit_dep->correction_outputs));

  return bit_dep;

}

void set_bit_dep_zero(BitDep* bit_dep){
  bit_dep->constant = 0;
  bit_dep->out = 0;
  bit_dep->secrets[0] = bit_dep->secrets[1] = 0;

  memset(bit_dep->randoms, 0, RANDOMS_MAX_LEN*sizeof(*bit_dep->randoms));
  memset(bit_dep->mults, 0, BITMULT_MAX_LEN*sizeof(*bit_dep->mults));
  memset(bit_dep->correction_outputs, 0, BITCORRECTION_OUTPUTS_MAX_LEN*sizeof(*bit_dep->correction_outputs));

}

// Count the total number of wires based on the array |c->weights|,
// and updates |c->total_wires| with this values.
void compute_total_wires(Circuit* c) {
  int total = 0;
  for (int i = 0; i < c->length; i++) {
    total += c->weights[i];
    //printf("%s %d\n", c->deps->names[i] ,c->weights[i]);
  }
  c->total_wires = total;
  //exit(EXIT_FAILURE);
}


// Computes |c->i1_rands|, |c->i2_rands|, |c->out_rands|, ie, which
// randoms are used to refresh the 1st input, the 2nd input, and the
// output. Note that this is only computed if |c| contains some
// multiplications.
void compute_rands_usage(Circuit* c) {
  if (!c->contains_mults) {
    c->i1_rands  = NULL;
    c->i2_rands  = NULL;
    c->out_rands = NULL;
    c->has_input_rands = false;
    return;
  }
  DependencyList* deps = c->deps;
  int first_rand_idx = deps->first_rand_idx;
  int nb_rands = c->random_count;
  int mult_count = c->deps->mult_deps->length;

  int non_mult_deps_count = c->secret_count + c->random_count;
  bool* i1_rands  = calloc(non_mult_deps_count, sizeof(*i1_rands));
  bool* i2_rands  = calloc(non_mult_deps_count, sizeof(*i2_rands));
  bool* out_rands = calloc(non_mult_deps_count, sizeof(*out_rands));
  bool has_input_rands = false;

  // Slightly weird to put input 1 and 2 here since they are not
  // randoms, but it simplifies the code later..
  i1_rands[0] = 1;
  i2_rands[1] = 1;

  // We iterate over each probe (dependency), and, if they contain a
  // random, we check what else it contains:
  //
  //  - if it contains an input share, then this random is associated
  //    with the corresponding input.
  //
  //  - if it contains a multiplication, then this random is added to
  //    out_rands.
  for (int i = 0; i < deps->length; i++) {

    Dependency* dep = deps->deps_exprs[i];

    if (dep[0]) {
      for (int j = first_rand_idx; j < first_rand_idx+nb_rands; j++) {
        i1_rands[j] |= dep[j];
        if (dep[j]) has_input_rands = true;
      }
    }
    if ((c->secret_count>1) &&  (dep[1])) {
      for (int j = first_rand_idx; j < first_rand_idx+nb_rands; j++) {
        i2_rands[j] |= dep[j];
        if (dep[j]) has_input_rands = true;
      }
    }
    for (int k = non_mult_deps_count; k < non_mult_deps_count + mult_count; k++) {
      // If dep[k] != 0, then either the current variable is a mult variable and it only has
      // a value =1 on the corresponding mult_index (no effect on the out_rands) in dep, or the
      // current variable is a linear combination of multiplication variables and randoms
      // and hence out_rands is updated (e.g. an output share).
      if (dep[k]) {
        for (int j = first_rand_idx; j < first_rand_idx+nb_rands; j++) {
          out_rands[j] |= dep[j];
        }
        break;
      }
    }
  }



  // At this point, it's possible that some randoms are neither in
  // i1_rands/i2_rands nor out_rands. For instance, consider the
  // following gadget:
  //
  //      ta = a0 ^ r0
  //      tb = b0 ^ r1
  //      mt = ta * tb
  //      ...
  //      ha = r0 ^ r4
  //      hb = r1 ^ r5
  //      mh = ha * hb
  //      ...
  //      _ = mt + mh
  //
  // In this case, r4 and r5 are not directly added with an input or a
  // multiplication. However, they are added to randoms (r0 and r1)
  // that themselves are added to some input shares (namely, a0 and b0).
  //
  // To account for such cases, we now iterate over all dependencies,
  // and check that all randoms are either in i1_rands, i2_rands or
  // out_rands. If we find some lone randoms, we infer their "group"
  // from other randoms of the dependency.
  //
  // (this could have been done in the previous step, but it seems
  // more maintainable to do it separately)
  bool br = false;
  for (int i = 0; i < deps->length; i++) {
    Dependency* dep = deps->deps_exprs[i];
    for (int j = first_rand_idx; j < first_rand_idx+nb_rands; j++) {
      if (dep[j] && !i1_rands[j] && !i2_rands[j] && !out_rands[j]) {
        // The random i does not have a group yet -> searching for
        // other randoms in the dependency (one of them should have a
        // group).
        for (int k = first_rand_idx; k < first_rand_idx+nb_rands; k++) {
          br = false;
          if (dep[k] && i1_rands[k]) {
            i1_rands[j] |= dep[j];
            br = true;
          } 
          if (dep[k] && i2_rands[k]) {
            i2_rands[j] |= dep[j];
            br = true;
          } 
          if (dep[k] && out_rands[k]) {
            out_rands[j] |= dep[j];
            br = true;
          }
          if(br) break;
        }
      }
    }
  }


  for(int i=first_rand_idx; i<non_mult_deps_count; i++){
    int cpt = 0;
    if(i1_rands[i]) cpt++;
    if(i2_rands[i]) cpt++;
    if(out_rands[i]) cpt++;
    if(cpt > 1){
      fprintf(stderr, "compute_rands_usage(): Unsupported format for random '%d' in a multiplication gadget.\n", i- c->secret_count);
      //exit(EXIT_FAILURE);
    }
    // if(cpt == 0){
    //   fprintf(stderr, "compute_rands_usage(): random '%d' not found in gadget.\n", i- c->secret_count);
    //   exit(EXIT_FAILURE);
    // }
  }

  c->i1_rands        = i1_rands;
  c->i2_rands        = i2_rands;
  c->out_rands       = out_rands;
  c->has_input_rands = has_input_rands;
}


void _update_contained_secrets(Dependency** contained_secrets, int idx, DependencyList* deps,
                               int secret_count, int non_mult_deps_count,
                               Dependency* dep, int ** temporary_mult_idx) {
  
  bool inps = false;
  // Direct dependencies on secrets
  for (int i = 0; i < secret_count; i++) {
    contained_secrets[idx][i] |= dep[i];
    if(dep[i]){
      inps = true;
    }
  }

  // Following multiplications
  bool mult = false;
  for (int i = 0; i < deps->mult_deps->length; i++) {
    if (dep[non_mult_deps_count+i]) {

      if(mult){
        fprintf(stderr, "_update_contained_secrets(): Unsupported format for variable '%s' in a multiplication gadget.\n", deps->names[idx]);
        exit(EXIT_FAILURE);
      }

      MultDependency* mult_dep = deps->mult_deps->deps[i];

      if(!(mult_dep->contained_secrets)){
        mult = true;

        if(inps){
          fprintf(stderr, "_update_contained_secrets(): Unsupported format for variable '%s' in a multiplication gadget.\n", deps->names[idx]);
          exit(EXIT_FAILURE);
        }

        mult_dep->contained_secrets = calloc(secret_count, sizeof(*mult_dep->contained_secrets));

        Dependency * contained_secrets_left = contained_secrets[temporary_mult_idx[i][0]];
        Dependency * contained_secrets_right = contained_secrets[temporary_mult_idx[i][1]];

        if((!contained_secrets_left) || (!contained_secrets_right)){
          fprintf(stderr, "_update_contained_secrets(): Unsupported format for variable '%s' in a multiplication gadget.\n", deps->names[idx]);
          exit(EXIT_FAILURE);
        }

        for (int k = 0; k < secret_count; k++) {
          mult_dep->contained_secrets[k] |= contained_secrets_left[k] | contained_secrets_right[k];
        }
      }

      for (int k = 0; k < secret_count; k++) {
        contained_secrets[idx][k] |= mult_dep->contained_secrets[k];
      }
    }
  }

  CorrectionOutputs * correction_outputs = deps->correction_outputs;
  int start = deps->first_correction_idx;
  for (int i = start; i < start + correction_outputs->length; i++) {
    if(dep[i]){
      DepArrVector* deps_sub = correction_outputs->correction_outputs_deps[i-start];
      printf("var = %s, len = %d\n", correction_outputs->correction_outputs_names[i-start], deps_sub->length);
      for(int k=0; k< deps_sub->length; k++){
        _update_contained_secrets(contained_secrets, idx, deps, secret_count,
                                non_mult_deps_count, deps_sub->content[k], temporary_mult_idx);
      }
    }
  }
}


// Computes |c->deps->contained_secrets|, ie, which secret shares are
// in each variable.
void compute_contained_secrets(Circuit* c, int ** temporary_mult_idx) {
  int non_mult_deps_count = c->secret_count + c->random_count;

  Dependency** contained_secrets = malloc(c->deps->length * sizeof(*contained_secrets));
  for (int i = 0; i < c->deps->length; i++) {
    // Allocating arrays of size 2 instead of size |c->secret_count|
    // so that we can always access the 2nd element without undefined
    // behavior, which removes the need for some "if (secret_count ==
    // 2)" at a few places.
    assert(c->secret_count <= 2); // Just in case, although for now this can't be false
    contained_secrets[i] = calloc(2, sizeof(**contained_secrets));
    DepArrVector* dep_arr = c->deps->deps[i];
    for (int dep_idx = 0; dep_idx < dep_arr->length; dep_idx++) {
      _update_contained_secrets(contained_secrets, i, c->deps, c->secret_count,
                                non_mult_deps_count, dep_arr->content[dep_idx], temporary_mult_idx);
    }
  }

  MultDependencyList * mult_deps = c->deps->mult_deps;
  for (int i = 0; i < mult_deps->length; i++) {

      MultDependency* mult_dep = mult_deps->deps[i];

      if(!(mult_dep->contained_secrets)){
          mult_dep->contained_secrets = calloc(2, sizeof(**contained_secrets));

          for(int i=0; i< c->deps->length; i++){
            if(strcmp(c->deps->names[i], mult_dep->name) == 0){
              mult_dep->contained_secrets[0] = contained_secrets[i][0];
              mult_dep->contained_secrets[1] = contained_secrets[i][1];
              break;
            }
          }

      }
  }

  c->deps->contained_secrets = contained_secrets;
}


// Creates the BitDeps corresponding to the DependencyList in |circuit|.
void compute_bit_deps(Circuit* circuit, int ** temporary_mult_idx) {
  DependencyList* deps = circuit->deps;
  BitDepVector** bit_deps = malloc(deps->length * sizeof(*bit_deps));

  int secret_count = circuit->secret_count;
  int random_count = circuit->random_count + secret_count;
  int mult_count   = deps->mult_deps->length;
  int non_mult_deps_count = random_count;

  int corr_first_idx = deps->first_correction_idx;
  int corr_outputs_count = deps->correction_outputs->length;

  int bit_rand_len = 1 + random_count / 64;
  int bit_mult_len = (mult_count == 0) ? 0 :  1 + mult_count / 64;
  int bit_correction_outputs_len = (corr_outputs_count == 0) ? 0 : 1 + corr_outputs_count / 64;

  for (int i = 0; i < deps->length; i++) {
    DepArrVector* dep = deps->deps[i];
    bit_deps[i] = BitDepVector_make();
    for (int j = 0; j < dep->length; j++) {
      BitDep* bit_dep = init_bit_dep();
      bit_dep->secrets[0] = dep->content[j][0];
      if (secret_count == 2) bit_dep->secrets[1] = dep->content[j][1];

      // TODO: right now, the randoms will be offset by
      // |secret_count|, as they are everywhere. I don't know if this
      // is really desirable though.
      for (int k = 0; k < bit_rand_len; k++) {
        for (int l = 0; l < 64; l++) {
          if (k*64+l < secret_count) continue;
          if (k*64+l >= random_count) continue;
          if (dep->content[j][k*64+l]) {
            bit_dep->randoms[k] |= 1ULL << l;
          }
        }
      }

      for (int k = 0; k < bit_mult_len; k++) {
        for (int l = 0; l < 64; l++) {
          if (k*64+l >= mult_count) break;
          if (dep->content[j][non_mult_deps_count+k*64+l]) {
            bit_dep->mults[k] |= 1ULL << l;
          }
        }
      }

      for (int k = 0; k < bit_correction_outputs_len; k++) {
        for (int l = 0; l < 64; l++) {
          if (k*64+l >= corr_outputs_count) break;
          if (dep->content[j][corr_first_idx+k*64+l]) {
            bit_dep->correction_outputs[k] |= 1ULL << l;
          }
        }
      }

      bit_dep->constant = dep->content[j][deps->deps_size-1];

      bit_dep->out = 0;

      BitDepVector_push(bit_deps[i], bit_dep);
    }
  }

  if(corr_outputs_count != 0){

    DepArrVector ** correction_outputs_deps = deps->correction_outputs->correction_outputs_deps;
    BitDepVector ** correction_outputs_deps_bits = malloc(corr_outputs_count  * sizeof(*correction_outputs_deps_bits));
    for (int i = 0; i < corr_outputs_count; i++) {
      DepArrVector* dep = correction_outputs_deps[i];
      correction_outputs_deps_bits[i] = BitDepVector_make();
      for (int j = 0; j < dep->length; j++) {
        BitDep* bit_dep = init_bit_dep();
        bit_dep->secrets[0] = dep->content[j][0];
        if (secret_count == 2) bit_dep->secrets[1] = dep->content[j][1];

        for (int k = 0; k < bit_rand_len; k++) {
          for (int l = 0; l < 64; l++) {
            if (k*64+l < secret_count) continue;
            if (k*64+l >= random_count) continue;
            if (dep->content[j][k*64+l]) {
              bit_dep->randoms[k] |= 1ULL << l;
            }
          }
        }

        for (int k = 0; k < bit_mult_len; k++) {
          for (int l = 0; l < 64; l++) {
            if (k*64+l >= mult_count) break;
            if (dep->content[j][non_mult_deps_count+k*64+l]) {
              bit_dep->mults[k] |= 1ULL << l;
            }
          }
        }

        for (int k = 0; k < bit_correction_outputs_len; k++) {
          for (int l = 0; l < 64; l++) {
            if (k*64+l >= corr_outputs_count) break;
            if (dep->content[j][corr_first_idx+k*64+l]) {
              bit_dep->correction_outputs[k] |= 1ULL << l;
            }
          }
        }

        bit_dep->constant = dep->content[j][deps->deps_size-1];
        bit_dep->out = 0;

        BitDepVector_push(correction_outputs_deps_bits[i], bit_dep);
      }
    }

    circuit->deps->correction_outputs->correction_outputs_deps_bits = correction_outputs_deps_bits;
  }

  /* printf("Bit dependencies:\n"); */
  /* for (int i = 0; i < deps->length; i++) { */
  /*   BitDepVector* arr = bit_deps[i]; */
  /*   printf(" %3d: { ", i); */
  /*   for (int j = 0; j < arr->length; j++) { */
  /*     printf("  [ %02x %02x | ", */
  /*            arr->content[j]->secrets[0], arr->content[j]->secrets[1]); */
  /*     for (int k = 0; k < bit_rand_len; k++) printf("%016lx ", arr->content[j]->randoms[k]); */
  /*     printf("| "); */
  /*     for (int k = 0; k < bit_mult_len; k++) printf("%016lx ", arr->content[j]->mults[k]); */
  /*     printf("]"); */
  /*     if (j != arr->length-1) printf("\n"); */
  /*   } */
  /*   printf(" }\n"); */
  /* } */

  // Setting the bits_left and bits_right fields of mult_deps.
  for (int i = 0; i < deps->mult_deps->length; i++) {
    MultDependency* mult_dep = deps->mult_deps->deps[i];
    int * tmp_mult_ops_idx = temporary_mult_idx[i];

    mult_dep->bits_left  = bit_deps[tmp_mult_ops_idx[0]];
    mult_dep->bits_right = bit_deps[tmp_mult_ops_idx[1]];
  }

  circuit->deps->bit_deps = bit_deps;
  memset(circuit->bit_out_rands, 0, RANDOMS_MAX_LEN * sizeof(*circuit->bit_out_rands));
  memset(circuit->bit_i1_rands,  0, RANDOMS_MAX_LEN * sizeof(*circuit->bit_i1_rands));
  memset(circuit->bit_i2_rands,  0, RANDOMS_MAX_LEN * sizeof(*circuit->bit_i2_rands));
  if (circuit->has_input_rands) {
    // Note that we start the following loop at |i| = 0 so that the
    // first and second inputs are taken into account: the function
    // compute_rands_usage from circuit.c sets i1_rands[0] and
    // i2_rands[1] to 1. Alternatively, we could have initialized
    // |bit_i1_rands| to 1 and |bit_i2_rands| to 2.
    for (int i = 0; i < bit_rand_len; i++) {
      for (int j = 0; j < 64; j++) {
        int idx = i*64+j;
        if (idx >= random_count) continue;
        if (circuit->out_rands[idx]) {
          circuit->bit_out_rands[i] |= 1ULL << j;
        } else if (circuit->i1_rands[idx]) {
          circuit->bit_i1_rands[i] |= 1ULL << j;
        } else if (circuit->i2_rands[idx]) {
          circuit->bit_i2_rands[i] |= 1ULL << j;
        }
        // else {
        //   printf("Random %d (%s) is not in out_rands/i1_rands/i2_rands...\n",
        //         idx, circuit->deps->names[idx]);
        //   assert(false);
        // }

      }
    }
  }
}


void compute_total_correction_bit_deps(Circuit * circuit){
  int corr_outputs_count = circuit->deps->correction_outputs->length;
  if(corr_outputs_count == 0){
    return;
  }

  int bit_rand_len = 1 + (circuit->random_count + circuit->secret_count) / 64;
  int bit_mult_len = (circuit->deps->mult_deps->length == 0) ? 0 :  1 + circuit->deps->mult_deps->length / 64;
  int bit_correction_outputs_len = (corr_outputs_count == 0) ? 0 : 1 + corr_outputs_count / 64;
  BitDepVector ** correction_outputs_deps_bits = circuit->deps->correction_outputs->correction_outputs_deps_bits;
  BitDep ** total_deps = malloc(corr_outputs_count * sizeof(*total_deps));

  for (int i = 0; i < corr_outputs_count; i++) {
    BitDepVector* dep = correction_outputs_deps_bits[i];
    total_deps[i] = init_bit_dep();

    for (int j = 0; j < dep->length; j++) {
      BitDep* bit_dep = dep->content[j];

      total_deps[i]->secrets[0] |= bit_dep->secrets[0];
      if (circuit->secret_count == 2) total_deps[i]->secrets[1] |= bit_dep->secrets[1];
      for (int k = 0; k < bit_rand_len; k++) {
        total_deps[i]->randoms[k] |= bit_dep->randoms[k];
      }
      for (int k = 0; k < bit_mult_len; k++) {
        total_deps[i]->mults[k] |= bit_dep->mults[k];
      }
      total_deps[i]->constant |= bit_dep->constant;

      for (int l = 0; l < bit_correction_outputs_len; l++) {
        uint64_t corr_out_elem = bit_dep->correction_outputs[l];

        while (corr_out_elem != 0) {
          int corr_output_idx_in_elem = __builtin_ia32_lzcnt_u64(corr_out_elem);
          corr_out_elem &= ~(1ULL << (63-corr_output_idx_in_elem));
          int corr_out_idx = l * 64 + (63-corr_output_idx_in_elem);

          BitDep * bit_dep_inter = total_deps[corr_out_idx];

          total_deps[i]->secrets[0] |= bit_dep_inter->secrets[0];
          if (circuit->secret_count == 2) total_deps[i]->secrets[1] |= bit_dep_inter->secrets[1];
          for (int k = 0; k < bit_rand_len; k++) {
            total_deps[i]->randoms[k] |= bit_dep_inter->randoms[k];
          }
          for (int k = 0; k < bit_mult_len; k++) {
            total_deps[i]->mults[k] |= bit_dep_inter->mults[k];
          }
          total_deps[i]->constant |= bit_dep_inter->constant;
        }
      }
    }
  }
  circuit->deps->correction_outputs->total_deps = total_deps;
}



void print_circuit(const Circuit* c) {
  DependencyList* deps = c->deps;
  MultDependencyList* mult_deps = deps->mult_deps;

  printf("Circuit with %d variables\n", c->length);
  printf("total_wires = %d\n", c->total_wires);
  printf("secret_count = %d\n", c->secret_count);
  printf("output_count = %d\n", c->output_count);
  printf("random_count = %d\n", c->random_count);
  printf("share_count = %d\n", c->share_count);
  printf("all_shares_mask = %d\n", c->all_shares_mask);
  printf("contains_mults = %d\n", c->contains_mults);
  printf("has_input_rands = %d\n", c->has_input_rands);
  printf("mult_count = %d\n", c->deps->mult_deps->length);
  

  printf("Dependencies:\n");
  for (int i = 0; i < deps->length; i++) {
    printf("%3d: {", i);
    for (int j = 0; j < deps->deps[i]->length; j++) {
      printf(j == 0 ? " [ " : "       [ ");
      for (int k = 0; k < c->secret_count; k++) {
        printf("%d ", deps->deps[i]->content[j][k]);
      }
      printf(", ");
      for (int k = c->deps->first_rand_idx; k < c->deps->first_rand_idx+c->random_count; k++) {
        printf("%d ", deps->deps[i]->content[j][k]);
      }
      printf(", ");
      for (int k = c->deps->first_mult_idx; k < c->deps->first_mult_idx+c->deps->mult_deps->length; k++) {
        printf("%d ", deps->deps[i]->content[j][k]);
      }
      if(c->deps->correction_outputs->length)
        printf(", ");
      for (int k = c->deps->first_correction_idx; k < c->deps->first_correction_idx+c->deps->correction_outputs->length; k++) {
        printf("%d ", deps->deps[i]->content[j][k]);
      }
      printf(", %d", deps->deps[i]->content[j][c->deps->deps_size-1]);
      printf(j == deps->deps[i]->length-1 ? "] " : "]\n");
    }

      for (int k = 0; k < c->secret_count; k++) {
        printf("%d ", deps->contained_secrets[i][k]);
      }

    printf(" }  [%s] %d %d\n", deps->names[i], c->weights[i], c->deps->deps[i]->length);
  }

  if (c->contains_mults) {
    printf("\nMultiplications:\n");
    for (int i = 0; i < mult_deps->length; i++) {
      printf("%d - %s: %s * %s, [%d %d], same as %d\n", i,
             mult_deps->deps[i]->name,
             mult_deps->deps[i]->name_left, mult_deps->deps[i]->name_right,
             mult_deps->deps[i]->contained_secrets[0], mult_deps->deps[i]->contained_secrets[1],
             mult_deps->deps[i]->idx_same_dependencies);
    }

    int non_mult_deps_count = c->secret_count + c->random_count;
    int refresh_i1 = 0, refresh_i2 = 0, refresh_out = 0;
    for (int i = deps->first_rand_idx; i < non_mult_deps_count; i++) {
      refresh_i1 += c->i1_rands[i];
      refresh_i2 += c->i2_rands[i];
      refresh_out += c->out_rands[i];
    }

    printf("\nRandoms:\n");
    for(int i=deps->first_rand_idx; i<non_mult_deps_count; i++){
      printf("%d, %d, %d\n", c->i1_rands[i], c->i2_rands[i], c->out_rands[i]);
    }

    printf("\nRefreshes: %d on input 1, %d on input 2, %d on output.\n\n",
           refresh_i1, refresh_i2, refresh_out);
  }

  if(deps->correction_outputs->length){
    printf("Correction outputs:\n");
    for(int i=0; i<deps->correction_outputs->length; i++){
      printf("%d: %s, %d\n", i, deps->correction_outputs->correction_outputs_names[i], deps->correction_outputs->correction_outputs_deps[i]->length);
      
      for(int j=0; j<deps->correction_outputs->correction_outputs_deps[i]->length; j++){

        printf(j == 0 ? " [ " : "    [ ");
        for (int k = 0; k < c->secret_count; k++) {
          printf("%d ", deps->correction_outputs->correction_outputs_deps[i]->content[j][k]);
        }
        printf(", ");
        for (int k = c->deps->first_rand_idx; k < c->deps->first_rand_idx+c->random_count; k++) {
          printf("%d ", deps->correction_outputs->correction_outputs_deps[i]->content[j][k]);
        }
        printf(", ");
        for (int k = c->deps->first_mult_idx; k < c->deps->first_mult_idx+c->deps->mult_deps->length; k++) {
          printf("%d ", deps->correction_outputs->correction_outputs_deps[i]->content[j][k]);
        }
        printf(", ");
        for (int k = c->deps->first_correction_idx; k < c->deps->first_correction_idx+c->deps->correction_outputs->length; k++) {
          printf("%d ", deps->correction_outputs->correction_outputs_deps[i]->content[j][k]);
        }
        printf(", %d", deps->correction_outputs->correction_outputs_deps[i]->content[j][c->deps->deps_size-1]);
        printf(j == deps->correction_outputs->correction_outputs_deps[i]->length-1 ? "] " : "]\n");
      }
    
      printf("\n");
    }
    printf("\n");
  }

}

void free_circuit(Circuit* c) {
  for (int i = 0; i < c->deps->mult_deps->length; i++) {
    //if(c->deps->mult_deps->deps[i]->idx_same_as == -1){
    free(c->deps->mult_deps->deps[i]->contained_secrets);
    //}
    free(c->deps->mult_deps->deps[i]);
  }
  free(c->deps->mult_deps->deps);
  free(c->deps->mult_deps);
  for (int i = 0; i < c->deps->length; i++) {
    DepArrVector_free(c->deps->deps[i]);
    free(c->deps->names[i]);
    free(c->deps->contained_secrets[i]);
    BitDepVector_deep_free(c->deps->bit_deps[i]);
  }
  free(c->deps->deps);
  free(c->deps->deps_exprs);
  free(c->deps->names);
  free(c->deps->contained_secrets);
  free(c->deps->bit_deps);
  free(c->deps);
  free(c->weights);
  free(c);
}


Circuit* shallow_copy_circuit(Circuit* c) {
  Circuit* new_circuit = malloc(sizeof(*new_circuit));

  new_circuit->deps              = c->deps;
  new_circuit->length            = c->length;
  new_circuit->secret_count      = c->secret_count;
  new_circuit->output_count      = c->output_count;
  new_circuit->share_count       = c->share_count;
  new_circuit->random_count      = c->random_count;
  new_circuit->all_shares_mask   = c->all_shares_mask;
  new_circuit->weights           = c->weights;
  new_circuit->contains_mults    = c->contains_mults;
  new_circuit->total_wires       = c->total_wires;
  new_circuit->i1_rands          = c->i1_rands;
  new_circuit->i2_rands          = c->i2_rands;
  new_circuit->out_rands         = c->out_rands;
  new_circuit->has_input_rands   = c->has_input_rands;
  new_circuit->transition        = c->transition;
  new_circuit->glitch            = c->glitch;
  memcpy(new_circuit->bit_out_rands, c->bit_out_rands,
         RANDOMS_MAX_LEN * sizeof(*c->bit_out_rands));
  memcpy(new_circuit->bit_i1_rands, c->bit_i1_rands,
         RANDOMS_MAX_LEN * sizeof(*c->bit_i1_rands));
  memcpy(new_circuit->bit_i2_rands, c->bit_i2_rands,
         RANDOMS_MAX_LEN * sizeof(*c->bit_i2_rands));

  return new_circuit;
}