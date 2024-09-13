#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <inttypes.h>

#include "verification_rules.h"
#include "list_tuples.h"
#include "combinations.h"
#include "trie.h"
#include "vectors.h"

/**********************************************************************
              Very high level description

  This module exhaustively iterates all tuples to find failures. See
  the comments before the function verify_tuples for more details.

************************************************************************/

#define min(_a,_b) ((_a) <= (_b) ? (_a) : (_b))
#define max(_a,_b) ((_a) >= (_b) ? (_a) : (_b))


#if 0
// V1 -- not functional
//
// The idea here was to expand each tuple with the necessary secret
// shares to make it a failure (if needed). Then, the tuple would need
// to be padded with other variables that have been removed. The issue
// is that we'd need to add variables with secret shares for both
// inputs (separately), and then pad them (separately), which would
// generate multiple times the same tuples. This is easily fixed with
// a Trie to avoid duplicate, but this puts more stress on the memory,
// which I expect to not be good for performance.
//
// A better idea might be:
//   - For the 1st input, we add the necessary secret shares, then
//     do the padding; nothing special.
//   - For the 2nd input, we add the necessary secret shares, then
//     do the padding, except that if the tuples are also failures for
//     the 1st input, then we skip them.
//
void _expand_tuple_rec(const Circuit* c, int t_in, Comb* curr_comb, int comb_len,
                       Comb* elementary_comb, int elementary_len,
                       VarVector** elementaries,
                       int input_num, Dependency secret_deps[2],
                       void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void*),
                       void* data) {
  // TODO: the same tuple can be generated multiple times: once per
  // input. For instance, the tuple (a0*b0, a1*b1) is a failure for
  // both inputs (if t_in == 1), and will be generated once for "a"
  // and once for "b".
  if (elementary_len == 0) {
    SecretDep leaky_inputs[2];
    leaky_inputs[0] = hamming_weight(secret_deps[0]) > t_in;
    if (c->secret_count == 2) leaky_inputs[1] = hamming_weight(secret_deps[1]) > t_in;
    failure_callback(c, curr_comb, comb_len, leaky_inputs, data);
    return;
  }
  int elem_wire_index = *elementary_comb;
  secret_deps[input_num] |= (1 << elem_wire_index);
  VarVector* elem_wire_arr = elementaries[elem_wire_index];
  for (int i = 0; i < elem_wire_arr->length; i++) {
    curr_comb[comb_len] = elem_wire_arr->content[i];
    _expand_tuple_rec(c, t_in, curr_comb, comb_len+1,
                      &elementary_comb[1], elementary_len-1,
                      elementaries, input_num, secret_deps,
                      failure_callback, data);
  }
}

void expand_tuple_to_failure(const Circuit* c, int t_in,
                             Comb* curr_comb, int comb_len,
                             SecretDep leaky_inputs[2], Dependency secret_deps[2],
                             int max_len, VarVector** elementaries,
                             int* new_to_old_mapping,
                             VarVector* removed_wires,
                             void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void*),
                             void* data) {

  Comb curr_comb_fixed[max_len];
  for (int i = 0; i < comb_len; i++) {
    curr_comb_fixed[i] = new_to_old_mapping[curr_comb[i]];
  }

  /* printf("Failure basis: [ "); */
  /* for (int i = 0; i < comb_len; i++) printf("%d ", curr_comb_fixed[i]); */
  /* printf("]\n"); */

  if (comb_len == max_len) {
    failure_callback(c, curr_comb, comb_len, leaky_inputs, data);
    return;
  }


  for (int input_num = 0; input_num < c->secret_count; input_num++) {
    // Computing which secret shares are missing
    Dependency in_tuple = secret_deps[input_num];
    Comb needed_shares[c->share_count];
    int needed_shares_length = 0;
    int offset = input_num * c->share_count;
    for (int j = 0; j < c->share_count; j++) {
      if (!(in_tuple & (1 << j))) {
        needed_shares[needed_shares_length++] = j + offset;
      }
    }

    // Computing the tuples of elementary wires that are needed to
    // make |curr_comb| a failure.
    Dependency secret_deps_bkp[2] = { secret_deps[0], secret_deps[1] };
    int min_shares_needed_count = t_in + 1 - hamming_weight(in_tuple);
    int max_shares_needed_count = max_len - comb_len;
    for (int elementary_len = min_shares_needed_count;
         elementary_len <= max_shares_needed_count; elementary_len++) {
      Comb elementary_comb[max_shares_needed_count];
      for (int j = 0; j < elementary_len; j++) elementary_comb[j] = j;
      do {

        _expand_tuple_rec(c, t_in, curr_comb_fixed, comb_len,
                          elementary_comb, elementary_len,
                          elementaries,
                          input_num, secret_deps,
                          failure_callback, data);

      } while (incr_comb_in_place(elementary_comb, elementary_len, needed_shares_length));
    }

    secret_deps[0] = secret_deps_bkp[0];
    secret_deps[1] = secret_deps_bkp[1];
  }
}
#else
// V2 -- functional
//
// Instead of adding the specific shares that are missing for
// |curr_comb| to be a failure, we try to add all possible
// combinations of removed variables. We then look if the shares we
// added were enough to make the tuple a failure: if yes, then we have
// a failure, and if not, then we don't.
//
// Fairly simple, but because we generate some tuples that cannot
// possibly be failures, this is not optimal.

int expand_tuple_to_failure(const Circuit* c,
                            int t_in, int shares_to_ignore,
                            Comb* curr_comb, int comb_len,
                            SecretDep leaky_inputs[2], Dependency secret_deps[2],
                            int max_len,  const DimRedData* dim_red_data,
                            void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void*),
                            void* data) {

  int* new_to_old_mapping = dim_red_data->new_to_old_mapping;
  VarVector* removed_wires = dim_red_data->removed_wires;
  Circuit* old_circuit = dim_red_data->old_circuit;

  Comb curr_comb_fixed[max_len];
  for (int i = 0; i < comb_len; i++) {
    curr_comb_fixed[i] = new_to_old_mapping[curr_comb[i]];
  }

  if (comb_len == max_len) {
    failure_callback(old_circuit, curr_comb_fixed, comb_len,
                     leaky_inputs, data);
    return 1;
  }

  // failure_callback(old_circuit, curr_comb_fixed, comb_len,
  //                    leaky_inputs, data);
  // return 1;

  /* printf("Failure basis: [ "); */
  /* for (int i = 0; i < comb_len; i++) printf("%d ", curr_comb_fixed[i]); */
  /* printf("]\n"); */

  const DependencyList* deps = old_circuit->deps;
  int failure_count = 0;

  //printf("1\n");
  int max_elementary = removed_wires->length;
  //printf("2\n");
  int elem_add_max_len = min(max_len - comb_len,max_elementary);
  int elem_add_min_len = 1 + t_in - hamming_weight(secret_deps[0]);
  if (c->secret_count == 2) {
    elem_add_min_len = min(elem_add_min_len, 1 + t_in - hamming_weight(secret_deps[1]));
  }
  elem_add_min_len = max(elem_add_min_len, 0);

  for (int elem_comb_len = elem_add_min_len;
       elem_comb_len <= elem_add_max_len; elem_comb_len++) 
  {
    Comb elementary_comb[elem_comb_len];
    for (int i = 0; i < elem_comb_len; i++) elementary_comb[i] = i;

    do {
      /* printf("Elem comb: [ "); */
      /* for (int i = 0; i < elem_comb_len; i++) printf("%d ", elementary_comb[i]); */
      /* printf("]\n"); */

      Dependency secret_deps_elem[2] = { 0 };
      for (int i = 0; i < elem_comb_len; i++) {
        Var dep_real_idx = removed_wires->content[elementary_comb[i]];
        curr_comb_fixed[comb_len+i] = dep_real_idx;
        Dependency* dep_secrets = deps->contained_secrets[dep_real_idx];
        secret_deps_elem[0] |= dep_secrets[0];
        secret_deps_elem[1] |= dep_secrets[1];
      }

      SecretDep leaky_inputs_loc[2] = { leaky_inputs[0], leaky_inputs[1] };

      int is_failure = 0;
      int secret_deps_0 = (secret_deps[0] | secret_deps_elem[0]) & ~shares_to_ignore;
      if (hamming_weight(secret_deps_0) > t_in) {
        is_failure = 1;
        leaky_inputs_loc[0] = 1;
      }
      if (c->secret_count > 1) {
        int secret_deps_1 = (secret_deps[1] | secret_deps_elem[1]) & ~shares_to_ignore;
        if (hamming_weight(secret_deps_1) > t_in) {
          is_failure = 1;
          leaky_inputs_loc[1] = 1;
        }
      }

      /* printf("Tuple: [ "); */
      /* for (int i = 0; i < comb_len; i++) printf("%d ", curr_comb_fixed[i]); */
      /* printf("| "); */
      /* for (int i = comb_len; i < comb_len+elem_comb_len; i++) printf("%d ", curr_comb_fixed[i]); */
      /* printf("]\n"); */
      /* printf("  secret_deps = { %d, %d }\n", secret_deps[0], secret_deps[1]); */
      /* printf("  secret_deps_elem = { %d, %d }\n", secret_deps_elem[0], secret_deps_elem[1]); */
      /* printf("  secret_deps finals = { %d, %d }\n", secret_deps_0, secret_deps_1); */
      /* printf("  leaky_inputs_loc = { %d, %d }\n", leaky_inputs_loc[0], leaky_inputs_loc[1]); */

      if (is_failure) {
        failure_count++;
        failure_callback(old_circuit, curr_comb_fixed, comb_len+elem_comb_len,
                         leaky_inputs_loc, data);
      }

    } while(incr_comb_in_place(elementary_comb, elem_comb_len, max_elementary));
  }

  return failure_count;
}
#endif

// Returns the maximum number of secret shares of the same input in
// |comb|, ignoring |shares_to_ignore| (used for PINI).
int get_number_of_shares(const Circuit* c, const Comb* comb, int comb_len,
                         Dependency shares_to_ignore, bool PINI) {
  DependencyList* deps = c->deps;
  int secret_count = c->secret_count;
  Dependency secret_dep_1 = 0, secret_dep_2 = 0;
  for (int i = 0; i < comb_len; i++) {
    int idx = comb[i];
    secret_dep_1 |= deps->contained_secrets[idx][0];
    secret_dep_2 |= deps->contained_secrets[idx][1];
  }
  if (PINI) {
    return hamming_weight((secret_dep_1 | secret_dep_2) & ~shares_to_ignore);
  }
  int count_1 = hamming_weight(secret_dep_1 & ~shares_to_ignore);
  int count_2 = secret_count == 2 ? hamming_weight(secret_dep_2 & ~shares_to_ignore) : 0;
  return count_1 > count_2 ? count_1 : count_2;
}

#define Is_leaky(_v, _t_in, _comb_free_space) (hamming_weight(_v)+_comb_free_space > (_t_in))

// Updates |secret_deps_ret| with the secret shares contained in the
// tuple |local_deps| (whose length is |local_deps_len|). Returns 1 if
// this tuples is a failure with regard to |t_in| and 0 otherwise.
int set_contained_shares(char* leaky_inputs, Dependency* secret_deps,
                         BitDep** local_deps,
                         GaussRand* gauss_rands,
                         int local_deps_len, int secret_count, int t_in,
                         int comb_free_space,
                         Dependency shares_to_ignore, bool PINI) {
  for (int i = 0; i < local_deps_len; i++) {
    if (gauss_rands[i].is_set) continue;
    secret_deps[0] |= local_deps[i]->secrets[0];
    secret_deps[1] |= local_deps[i]->secrets[1];
  }
  int ret = 0;
  if (PINI) {
    secret_deps[0] |= secret_deps[1];
  }
  // TODO: unroll the following loop manually?
  for (int k = 0; k < secret_count; k++) {
    secret_deps[k] &= ~shares_to_ignore;
    if (Is_leaky(secret_deps[k], t_in, comb_free_space)) {
      ret = 1;
    }
    if (Is_leaky(secret_deps[k], t_in, 0)) {
      leaky_inputs[k] = 1;
    }
  }
  return ret;
}


// |gauss_deps|: the dependencies after gauss elimination up to index
//               |idx| (excluded). Note that even if an element is
//               fully masked by a random, we keep its other
//               dependencies in order to be able to use them to
//               perform subsequent eliminations.
//
// |gauss_rands|: the randoms used for the gauss elimination. Contains
//                -1 when an element does not contain any random.
//
// |real_dep|: the dependency to add to |gauss_deps|, at index |idx|.
//
// |deps_size|: the size of the dependencies in |gauss_deps| and of |real_dep|.
//
// This function adds |real_dep| to |gauss_deps|, and performs a Gauss
// elimination on this element: all previous elements of |gauss_deps|
// have already been eliminated, and we xor them as needed with |real_dep|.
static void gauss_step(BitDep* real_dep,
                       BitDep** gauss_deps,
                       GaussRand* gauss_rands,
                       int bit_rand_len,
                       int bit_mult_len,
                       int bit_correction_outputs_len,
                       int idx) {
  BitDep* dep_target = gauss_deps[idx];
  if (dep_target != real_dep) {
    memcpy(dep_target, real_dep, sizeof(*dep_target));
  }
  for (int i = 0; i < idx; i++) {
    if (!gauss_rands[i].is_set) continue;
    int r_idx  = gauss_rands[i].idx;
    uint64_t r_mask = gauss_rands[i].mask;
    if (dep_target->randoms[r_idx] & r_mask) {
      dep_target->secrets[0] ^= gauss_deps[i]->secrets[0];
      dep_target->secrets[1] ^= gauss_deps[i]->secrets[1];
      for (int j = 0; j < bit_rand_len; j++) {
        dep_target->randoms[j] ^= gauss_deps[i]->randoms[j];
      }
      for (int j = 0; j < bit_mult_len; j++) {
        dep_target->mults[j] ^= gauss_deps[i]->mults[j];
      }
      for (int j = 0; j < bit_correction_outputs_len; j++) {
        dep_target->correction_outputs[j] ^= gauss_deps[i]->correction_outputs[j];
      }
      dep_target->constant ^= gauss_deps[i]->constant;

      dep_target->out ^= gauss_deps[i]->out;
    }
  }
}


// Sets |gauss_rands[idx]| to contain the first random that appears in
// |deps[idx]|.
static void set_gauss_rand(BitDep** deps, GaussRand* gauss_rands,
                           int idx, int bit_rand_len,
                           CorrectionOutputs * correction_outputs, int bit_correction_outputs_len) {
  BitDep* dep = deps[idx];
  for (int i = 0; i < bit_rand_len; i++) {
    if (dep->randoms[i]) {
      if(correction_outputs->length == 0){
        gauss_rands[idx].is_set = 1;
        gauss_rands[idx].idx    = i;
        gauss_rands[idx].mask   = 1ULL << (63-__builtin_ia32_lzcnt_u64(dep->randoms[i]));
        return;
      }
      else{
        bool corr = false;
        for(int j=0; j<bit_correction_outputs_len; j++){
          uint64_t corr_out_elem = dep->correction_outputs[j];
          while (corr_out_elem != 0) {
            corr = true;
            int corr_output_idx_in_elem = __builtin_ia32_lzcnt_u64(corr_out_elem);
            corr_out_elem &= ~(1ULL << (63-corr_output_idx_in_elem));
            int corr_out_idx = j * 64 + (63-corr_output_idx_in_elem);

            uint64_t rdep = correction_outputs->total_deps[corr_out_idx]->randoms[i];

            if(((rdep & dep->randoms[i]) ^ dep->randoms[i]) != 0){
              gauss_rands[idx].is_set = 1;
              gauss_rands[idx].idx    = i;
              gauss_rands[idx].mask   = 1ULL << (63-__builtin_ia32_lzcnt_u64((rdep & dep->randoms[i]) ^ dep->randoms[i]));
              return;
            }
          }
        }
        if(!corr){
          gauss_rands[idx].is_set = 1;
          gauss_rands[idx].idx    = i;
          gauss_rands[idx].mask   = 1ULL << (63-__builtin_ia32_lzcnt_u64(dep->randoms[i]));
          return;
        }
      }
    }
  }
  gauss_rands[idx].mask = 0;
  gauss_rands[idx].is_set = 0;
}

// Checks if the tuple |local_deps| is a failure when adding randoms
// (that were removed from the circuit by the function
// |remove_randoms| of dimensions.c).
static int is_failure_with_randoms(const Circuit* circuit,
                            BitDep** local_deps, GaussRand* gauss_rands, BitDep** local_deps_copy, GaussRand* gauss_rands_copy,
                            int local_deps_len, int t_in,
                            int comb_free_space, Dependency shares_to_ignore, bool PINI) {
  if (comb_free_space == 0) return 0;
  /* printf("Trying to expand with randoms\n"); */
  /* printf("Tuple: {\n"); */
  /* for (int i = 0; i < local_deps_len; i++) { */
  /*   printf("  [ %d %d | %016lx | ", local_deps[i]->secrets[0], local_deps[i]->secrets[1], */
  /*          local_deps[i]->randoms); */
  /*   for (int j = 0; j < 1+circuit->deps->mult_deps->length/64; j++) { */
  /*     printf("%016lx ", local_deps[i]->mults[j]); */
  /*   } */
  /*   printf("]\n"); */
  /* } */
  /* printf("}\n"); */

  DependencyList* deps = circuit->deps;
  int secret_count = circuit->secret_count;
  int random_count = circuit->random_count;
  int mult_count = deps->mult_deps->length;
  int non_mult_deps_count = circuit->deps->first_mult_idx;
  int bit_rand_len = 1 + random_count / 64;
  int corr_outputs_count = deps->correction_outputs->length;

  int bit_mult_len = (mult_count == 0) ? 0 :  1 + mult_count / 64;
  int bit_correction_outputs_len = (corr_outputs_count == 0) ? 0 : 1 + corr_outputs_count / 64;

  // Collecting all randoms of |local_deps| in the binary array |randoms|.
  uint64_t randoms[bit_rand_len];
  memset(randoms, 0, bit_rand_len * sizeof(*randoms));
  for (int i = 0; i < local_deps_len; i++) {
    for (int j = 0; j < bit_rand_len; j++) {
      randoms[j] |= local_deps[i]->randoms[j];
    }
  }

  // if(pr){
  //   for (int j = 0; j < bit_rand_len; j++) {
  //     printf("%llu\n",randoms[j]);
  //     printf("rand_count = %d\n", circuit->random_count);
  //   }
  // }

  // Building the array |randoms_arr| that contains the index of the
  // randoms in |local_deps|.
  Var randoms_arr[non_mult_deps_count];
  int randoms_arr_len = 0;
  for (int i = 0; i < bit_rand_len; i++) {
    uint64_t rand_elem = randoms[i];
    while (rand_elem != 0) {
      int rand_idx_in_elem = 63-__builtin_ia32_lzcnt_u64(rand_elem);
      rand_elem &= ~(1ULL << rand_idx_in_elem);
      randoms_arr[randoms_arr_len++] = rand_idx_in_elem + i*64;
    }
  }

  // if(pr){
  //   printf("Free Space = %d\n", comb_free_space);
  //   printf("Elems : ");
  //   for(int i=0; i<randoms_arr_len; i++){
  //     printf("%hu ", randoms_arr[i]);
  //   }
  //   printf("\n");
  // }

  int copy_size = 0;

  // Generating all combinations of randoms of |randoms_arr| and
  // checking if they make the tuple become a failure.
  Comb randoms_comb[randoms_arr_len];
  //int size = min(comb_free_space, randoms_arr_len);
  for (int size = 1; size <= comb_free_space && size <= randoms_arr_len; size++) {
    for (int i = 0; i < size; i++) randoms_comb[i] = i;
    do {

      Comb real_comb[size];
      for (int i = 0; i < size; i++) real_comb[i] = randoms_arr[randoms_comb[i]];
      uint64_t selected_randoms[bit_rand_len];
      memset(selected_randoms, 0, bit_rand_len * sizeof(*selected_randoms));

      // if(pr){
      //   printf("randoms_comb comb = ");
      //   for (int i = 0; i < comb_free_space; i++){ printf("%u ", randoms_comb[i]); }
      //   printf("\n");
      // }

      for (int i = 0; i < size; i++) {
        // if(pr) printf("real comb = %d ", real_comb[i]);
        int idx    = real_comb[i] / 64;
        // if(pr) printf(", idx = %d ", idx);
        // int offset = 63-__builtin_ia32_lzcnt_u64(real_comb[i]);
        // if(pr) printf(", offset = %d\n", offset);
        selected_randoms[idx] |= 1ULL << real_comb[i];
      }

      // if(pr){
      //   //selected_randoms[0] = 1 << 6;
      //   for (int j = 0; j < bit_rand_len; j++) {
      //     printf("selected = %llu\n",selected_randoms[j]);
      //   }
        
      //   //printf("selected new = %lu\n",selected_randoms[0] & randoms[0]);
      // }

      Dependency secret_deps[secret_count];
      secret_deps[0] = secret_deps[1] = 0;
      copy_size = 0;

      // Computing which secret shares are leaked
      for (int i = 0; i < local_deps_len; i++) {
        if (!gauss_rands[i].is_set) {
          secret_deps[0] |= local_deps[i]->secrets[0];
          if (secret_count == 2) secret_deps[1] |= local_deps[i]->secrets[1];
        } else {

          memcpy(local_deps_copy[copy_size], local_deps[i], sizeof(*local_deps[i]));
          for (int j = 0; j < bit_rand_len; j++) {
            local_deps_copy[copy_size]->randoms[j] = (local_deps[i]->randoms[j] & selected_randoms[j]) ^ local_deps[i]->randoms[j];
          }
          copy_size ++;
        }
      }

      for (int i = 0; i < copy_size; i++) {
          gauss_step(local_deps_copy[i], local_deps_copy, gauss_rands_copy,
                    bit_rand_len, bit_mult_len, bit_correction_outputs_len, i);
          set_gauss_rand(local_deps_copy, gauss_rands_copy, i, bit_rand_len, circuit->deps->correction_outputs, bit_correction_outputs_len);
      }

      for (int i = 0; i < copy_size; i++) {
        if (!gauss_rands_copy[i].is_set) {
          secret_deps[0] |= local_deps_copy[i]->secrets[0];
          if (secret_count == 2) secret_deps[1] |= local_deps_copy[i]->secrets[1];
        }
      }

      if (PINI) {
        secret_deps[0] |= secret_deps[1];
      }

      // Checking if enough secret shares are leaking for the tuple to
      // be a failure.
      for (int k = 0; k < secret_count; k++) {
        secret_deps[k] &= ~shares_to_ignore;
        if (Is_leaky(secret_deps[k], t_in, comb_free_space-size)) {
          return 1;
        }
      }

    } while (incr_comb_in_place(randoms_comb, size, randoms_arr_len));
  }

  /* printf("Didn't manage to get a failure...\n"); */

  return 0;
}


// Updates the factorization table |factorized_deps| according to the
// dependencies of |mult|.
//
// |factorized_deps| is a 2D-array where each sub-array corresponds to
// an element "e" (a share of an input or a random refreshing an
// input), and the content of the sub-array corresponds to the
// shares/randoms (from the other input) that this element "e" is
// multiplied with.
//
// For instance, if we have multiplication "(a0 ^ r0) & (b0 ^ r1)",
// then this function will do:
//    factorized_deps[a0] ^= (b0 ^ r1)
//    factorized_deps[r0] ^= (b0 ^ r1)
//    factorized_deps[b0] ^= (a0 ^ r0)
//    factorized_deps[r1] ^= (a0 ^ r0)
//

static void factorize_expr(BitDep** factorized_deps, int idx,
                           Dependency* dep, int secret_count, 
                           int first_rand_idx,
                           int non_mult_deps_count, int corr_output_length,
                           int corr_output_first_idx, int deps_size){

  for (int j = 0; j < secret_count; j++) {
    if (dep[j]) {
      factorized_deps[idx]->secrets[j] ^= dep[j];
    }
  }
  for (int j = first_rand_idx; j < non_mult_deps_count; j++) {
    if (dep[j]) {
      factorized_deps[idx]->randoms[(j-first_rand_idx)/64] ^= 1ULL << ((j-first_rand_idx)%64);
    }
  }
  for(int k=0; k<corr_output_length; k++){
    if (dep[k+corr_output_first_idx]) {
      factorized_deps[idx]->correction_outputs[k/64] ^= 1ULL << (k%64);
    }
  }
  if(dep[deps_size-1]){
    factorized_deps[idx]->constant = 1;
  }
}


void factorize_inner_mults(const Circuit* c, BitDep** factorized_deps,
                           MultDependency* mult) {
  int secret_count        = c->secret_count;
  int share_count         = c->share_count;
  int inputs_real_count   = secret_count * share_count;
  int non_mult_deps_count = c->deps->first_mult_idx;
  int first_rand_idx = c->deps->first_rand_idx;
  Dependency* left  = mult->left_ptr;
  Dependency* right = mult->right_ptr;

  /*for(int i=0; i<c->deps->deps_size; i++){
    printf("%" PRId32", %"PRId32"\n", left[i], right[i]);
  }*/

  int corr_output_first_idx = c->deps->first_correction_idx;
  int corr_output_length = c->deps->correction_outputs->length;

  // Inputs
  for (int i = 0; i < 2; i++) {
    if ((left[i]!=0) && (right[i]!=0)){
      fprintf(stderr, "factorize_inner_mults(): Unsupported format for variable '%s' in a multiplication gadget. Operands contain input shares from the same input %d\n", mult->name, i);
      exit(EXIT_FAILURE);
    }
    for (int j = 0; j < share_count; j++) {
      if (left[i] & (1ULL << j)) {
        factorize_expr(factorized_deps, i*share_count+j, right,
                       secret_count, first_rand_idx,
                       non_mult_deps_count, corr_output_length,
                       corr_output_first_idx, c->deps->deps_size);

      } else if (right[i] & (1ULL << j)) {

        factorize_expr(factorized_deps, i*share_count+j, left,
                       secret_count, first_rand_idx,
                       non_mult_deps_count, corr_output_length,
                       corr_output_first_idx, c->deps->deps_size);
      }
    }
  }

  int rand_offset = inputs_real_count;
  // Randoms
  for (int i = first_rand_idx; i < non_mult_deps_count; i++) {
    if ((left[i]) && (right[i])){
      fprintf(stderr, "factorize_inner_mults(): Unsupported format for variable '%s' in a multiplication gadget. Operands contain same random '%d'\n", mult->name, i-c->secret_count);
      exit(EXIT_FAILURE);
    }
    if (left[i]) {

      factorize_expr(factorized_deps, i-first_rand_idx+rand_offset, right,
                     secret_count, first_rand_idx,
                     non_mult_deps_count, corr_output_length,
                     corr_output_first_idx, c->deps->deps_size);

    } else if (right[i]) {

      factorize_expr(factorized_deps, i-first_rand_idx+rand_offset, left,
                     secret_count, first_rand_idx,
                     non_mult_deps_count, corr_output_length,
                     corr_output_first_idx, c->deps->deps_size);
    }
  }

  int corr_output_offset = inputs_real_count + c->random_count;
  //correction outputs terms
  for (int i = 0; i < corr_output_length; i++) {
    if ((left[i+corr_output_first_idx]) && (right[i+corr_output_first_idx])){
      fprintf(stderr, "factorize_inner_mults(): Unsupported format for variable '%s' in a multiplication gadget. Operands contain same correction output of index '%d'\n", mult->name, i);
      exit(EXIT_FAILURE);
    }
    if (left[i+corr_output_first_idx]) {

      factorize_expr(factorized_deps, i+corr_output_offset, right,
                     secret_count, first_rand_idx,
                     non_mult_deps_count, corr_output_length,
                     corr_output_first_idx, c->deps->deps_size);

    } else if (right[i+corr_output_first_idx]) {

      factorize_expr(factorized_deps, i+corr_output_offset, left,
                  secret_count, first_rand_idx,
                  non_mult_deps_count, corr_output_length,
                  corr_output_first_idx, c->deps->deps_size);
    }
  }


  //constant term
  int const_offset = inputs_real_count + c->random_count + c->deps->correction_outputs->length;
  if (left[c->deps->deps_size-1]) {
    factorize_expr(factorized_deps, const_offset, right,
                     secret_count, first_rand_idx,
                     non_mult_deps_count, corr_output_length,
                     corr_output_first_idx, c->deps->deps_size);
  } 
  const_offset++;
  if (right[c->deps->deps_size-1]) {
    factorize_expr(factorized_deps, const_offset, left,
                     secret_count, first_rand_idx,
                     non_mult_deps_count, corr_output_length,
                     corr_output_first_idx, c->deps->deps_size);
  }

}

// Factorizes the dependencies in |comb|. For each element of |comb|,
// all dependencies are unfolded (by distributing the multiplication
// inside the additions).
void factorize_mults(const Circuit* c, BitDep** local_deps,
                     BitDep** deps_fact,
                     int* deps_length_fact,
                     int local_deps_len) {
  DependencyList* deps    = c->deps;
  int rand_count          = c->random_count;
  int mult_count          = deps->mult_deps->length;
  int bit_rand_len = 1 + rand_count / 64;
  int bit_mult_len = (mult_count == 0) ? 0 :  1 + mult_count / 64;

  int corr_outputs_count = deps->correction_outputs->length;
  int bit_correction_outputs_len = (corr_outputs_count == 0) ? 0 : 1 + corr_outputs_count / 64;

  const uint64_t* bit_i1_rands  = c->bit_i1_rands;
  const uint64_t* bit_i2_rands  = c->bit_i2_rands;
  const uint64_t* bit_out_rands = c->bit_out_rands;

  int inputs_real_count = c->secret_count * c->share_count;
  int factorized_deps_length = inputs_real_count + c->random_count + deps->correction_outputs->length + 2; // + 2 for the constant terms on both sides

  BitDep* factorized_deps[factorized_deps_length];
  for (int i = 0; i < factorized_deps_length; i++) {
    factorized_deps[i] = alloca(sizeof(**factorized_deps));
  }

  for (int i = 0; i < local_deps_len; i++) {
    BitDep* dep = local_deps[i];

    int has_i1_rand = 0; //dep->secrets[0];
    int has_i2_rand = 0; //dep->secrets[1];
    int has_out_rand = 0;
    for (int j = 0; j < bit_rand_len; j++) {
      // printf("%"PRId64", %"PRId64", %"PRId64"\n",bit_i1_rands[j], bit_i2_rands[j], bit_out_rands[j] );
      if (dep->randoms[j] & bit_i1_rands[j]) {
        has_i1_rand = 1;
        break;
      } else if (dep->randoms[j] & bit_i2_rands[j]) {
        has_i2_rand = 1;
        break;
      } else if (dep->randoms[j] & bit_out_rands[j]) {
        has_out_rand = 1;
        break;
      }
    }
    if (has_i1_rand || has_i2_rand) {
      deps_fact[*deps_length_fact]->secrets[0] = dep->secrets[0];
      deps_fact[*deps_length_fact]->secrets[1] = dep->secrets[1];
      memcpy(deps_fact[*deps_length_fact]->randoms, dep->randoms,
             bit_rand_len * sizeof(*dep->randoms));
      memcpy(deps_fact[*deps_length_fact]->correction_outputs, dep->correction_outputs,
             bit_correction_outputs_len * sizeof(*dep->correction_outputs));
      // No need to copy multiplications
      (*deps_length_fact)++;

      // By construction, if an element contains a random related to
      // either input, it cannot contain a multiplication, and thus
      // there is nothing to factorize. We can thus skip to next
      // element of the tuple.
      for(int k=0; k< bit_mult_len; k++){
        if(dep->mults[k] != 0){
          fprintf(stderr, "factorize_mults(): Unsupported variables in gadget. A variable should not contain input randoms and multiplications. Exiting...\n");
          exit(EXIT_FAILURE);
        }
      }
      continue;
      
    } else if (has_out_rand) {
      continue;
    }

    for (int j = 0; j < bit_rand_len; j++) assert(!dep->randoms[j]);

    // Calling factorize_inner_mults for each multiplication will
    // effectively factorize them. For instance, consider:
    //    (a0 ^ r0) & (b0 ^ r2) ^ (a0 ^ r1) & (b1 ^ r3)
    // After the first factorize_inner_mults, |factorized_deps| is as follows:
    //    a0: b0 ^ r2
    //    r0: b0 ^ r2
    //    b0: a0 ^ r0
    //    r2: a0 ^ r0
    // And after the second:
    //    a0: b0 ^ r2 ^ b1 ^ r3   --- here we see the factorization in action
    //    r0: b0 ^ r2
    //    r1: b1 ^ r3
    //    b0: a0 ^ r0
    //    r2: a0 ^ r0
    //    b1: a0 ^ r1
    //    r3: a0 ^ r1

    // Resetting |factorized_deps| (note that the ->mults field is
    // never used in |factorized_deps|)
    for (int j = 0; j < factorized_deps_length; j++) {
      factorized_deps[j]->secrets[0] = 0;
      factorized_deps[j]->secrets[1] = 0;
      memset(factorized_deps[j]->randoms, 0,
             bit_rand_len * sizeof(*factorized_deps[j]->randoms));
      memset(factorized_deps[j]->correction_outputs, 0,
             bit_correction_outputs_len * sizeof(*factorized_deps[j]->correction_outputs));
    }

    for (int j = 0; j < bit_mult_len; j++) {
      uint64_t mult_elem = dep->mults[j];
      while (mult_elem != 0) {
        int mult_idx_in_elem = __builtin_ia32_lzcnt_u64(mult_elem);
        mult_elem &= ~(1ULL << (63-mult_idx_in_elem));
        int mult_idx = j * 64 + (63-mult_idx_in_elem);
        MultDependency* mult = deps->mult_deps->deps[mult_idx];
        //printf("mult = %s\n", mult->name);
        factorize_inner_mults(c, factorized_deps, mult);
      }
    }

    // We now copy the factorized exrepssions in |deps_fact|
    for (int i = 0; i < factorized_deps_length; i++) {

      int rand_set = 0;
      for (int j = 0; j < bit_rand_len && !rand_set; j++) {
        rand_set |= factorized_deps[i]->randoms[j];
      }

      int corr_output_set = 0;
      for (int j = 0; j < bit_correction_outputs_len && !corr_output_set; j++) {
        corr_output_set |= factorized_deps[i]->correction_outputs[j];
      }

      if (!(factorized_deps[i]->secrets[0] ||
            factorized_deps[i]->secrets[1] ||
            rand_set || corr_output_set)) {
        // This element is empty; continuing
        continue;
      }

      deps_fact[*deps_length_fact]->secrets[0] = factorized_deps[i]->secrets[0];
      deps_fact[*deps_length_fact]->secrets[1] = factorized_deps[i]->secrets[1];

      memcpy(deps_fact[*deps_length_fact]->randoms, factorized_deps[i]->randoms,
              bit_rand_len * sizeof(*factorized_deps[i]->randoms));

      memcpy(deps_fact[*deps_length_fact]->correction_outputs, factorized_deps[i]->correction_outputs,
              bit_correction_outputs_len * sizeof(*factorized_deps[i]->correction_outputs));
      (*deps_length_fact)++;
    }
  }

}

static void replace_correction_outputs_in_dep(BitDep** local_deps, int idx, GaussRand* gauss_rands,
                                                   int * local_deps_len, int bit_correction_outputs_len,
                                                   int bit_rand_len, int bit_mult_len,
                                                   CorrectionOutputs * correction_outputs){

  if(gauss_rands[idx].is_set) return;

  BitDep * bit_dep = local_deps[idx];

  for(int i=0; i< bit_correction_outputs_len; i++){
    //printf("Entering %d\n", idx);
    uint64_t corr_out_elem = bit_dep->correction_outputs[i];

    while (corr_out_elem != 0) {
      int corr_output_idx_in_elem = __builtin_ia32_lzcnt_u64(corr_out_elem);
      corr_out_elem &= ~(1ULL << (63-corr_output_idx_in_elem));
      int corr_out_idx = i * 64 + (63-corr_output_idx_in_elem);

      BitDepVector * bit_dep_arr = correction_outputs->correction_outputs_deps_bits[corr_out_idx];

      //printf("i = %d of length %d\n", corr_out_idx, bit_dep_arr->length);

      for(int dep_idx=0; dep_idx< bit_dep_arr->length; dep_idx++){

        gauss_step(bit_dep_arr->content[dep_idx], local_deps, gauss_rands,
                bit_rand_len, bit_mult_len, bit_correction_outputs_len, *local_deps_len);
        set_gauss_rand(local_deps, gauss_rands, *local_deps_len, bit_rand_len, correction_outputs, bit_correction_outputs_len);

        (*local_deps_len)++;

        replace_correction_outputs_in_dep(local_deps, *local_deps_len - 1, gauss_rands,
                                               local_deps_len, bit_correction_outputs_len,
                                               bit_rand_len, bit_mult_len,
                                               correction_outputs);
      }
    }
  }
}

// Increments |comb| (in place) whose length is |k|. Returns the first
// index that was modified. The maximum value that |comb| can contain
// is |max|.
//
// As an optimization, there is a special case for incrementing the
// last and one before last places (it improves performance by about
// 50%). For this optimization to be correct, |comb| should be
// over-allocated with two element set to |max| (or more) right before
// its start (so that comb[k-2] and comb[k-3] are not out of bounds).
int incr_comb_in_place_get_index(Comb* comb, int k, int max) {
  // TODO: investigate wether this optimization is worth it
  /* if (comb[k-1] < max) { */
  /*   comb[k-1]++; */
  /*   return k-1; */
  /* } */
  /* if (comb[k-2] < max-1) { */
  /*   comb[k-2]++; */
  /*   comb[k-1] = comb[k-2]+1; */
  /*   return k-2; */
  /* } */
  /* if (comb[k-3] < max-1) { */
  /*   comb[k-3]++; */
  /*   comb[k-2] = comb[k-3]+1; */
  /*   comb[k-1] = comb[k-3]+2; */
  /*   return k-3; */
  /* } */

  int i = k-1;
  for ( ; i >= 0; i--) {
    if (comb[i] < i + max - k) {
      int ret_val = i;
      int new_val = ++comb[i];
      for ( ++i; i < k; i++ ) {
        comb[i] = ++new_val;
      }
      return ret_val;
    }
  }
  return -1;
}

// Generates the tuple/comb right after |curr_comb|. The parameter
// |sub_comb_len| is the length of |curr_comb| without the length of
// |prefix|. Put otherwise, the full length of |curr_comb| is
// |prefix->length + sub_comb_len|.
int next_comb(Comb* curr_comb, int sub_comb_len, int last_var, VarVector* prefix) {
  if (!prefix) return incr_comb_in_place_get_index(curr_comb, sub_comb_len, last_var);
  return incr_comb_in_place_get_index(&curr_comb[prefix->length], sub_comb_len, last_var);
}

// Generates the first tuple/comb.
Comb* init_comb(Comb* first_comb, int sub_comb_len, VarVector* prefix, int max_comb_len) {
  // TODO: remove this malloc and take additional parameter
  Comb* comb = malloc((max_comb_len+2) * sizeof(*comb));

  // Over-allocating before |comb|. See explanations on incr_comb_in_place_get_index
  comb[0] = -1; comb++;
  comb[1] = -1; comb++;

  // Setting prefix
  for (int i = 0; i < prefix->length; i++) comb[i] = prefix->content[i];

  // Setting actual comb
  if (first_comb) {
    for (int i = 0; i < sub_comb_len; i++) comb[prefix->length+i] = first_comb[i];
  } else {
    for (int i = 0; i < sub_comb_len; i++) comb[prefix->length+i] = i;
  }

  return comb;
}

// verify_tuples is our generic verification function. Depending on
// its parameters, it can:
//
//   - Check if |first_tuple| is a failure or not (if |only_one_tuple|
//     is true)
//
//   - Find the first failure of size |comb_len| (if
//     |stop_at_first_failure| is true)
//
//   - Find all failures of size |comb_len|
//
// In the 2nd and 3rd case, the function |failure_callback| is called
// on each failure found.
//
// |failure_callback| should expect the following arguments: the
// circuit, the failure tuple (as a Comb*), its length, a SecretDep*
// revealing which inputs are leaked, and any additional data that are
// passed to verify_tuples in the |data| parameter.
//
// If |first_tuple| is not NULL, then instead of generating all tuples
// of size |comb_len|, all tuples after |first_tuple| are generated
// (in ascending order).
//
// If |prefix| is not NULL, then its content is prepended to the
// generated tuples.
//
//
// The verification is done using those few steps:
//
//  - If a tuple contains less than |t_in| secret shares, then it
//    can't be a failure.
//
//  - A gauss elimination is then performed. In the case of a linear
//    gadget, it's easy to then check if |t_in| secret shares are
//    leaked. In the case of a multiplication gadget, one or two
//    additional steps are performed:
//
//    * Inputs 1 and 2 (and their randoms) are extracted from the
//      tuple post gauss elimination. A gauss elimination is applied
//      on both of the resulting tuples, after which it's easy to
//      check if |t_in| shares are leaked.
//
//    * If the previous step reveals that not all shares are leaked,
//      dependencies on each inputs are factorized, after which a new
//      gauss elimination is done on each.
//
int _verify_tuples(const Circuit* circuit, // The circuit
                   int t_in, // The number of shares that must be
                             // leaked for a tuple to be a failure
                   VarVector* prefix, // Prefix to add to all the tuples
                   int comb_len, // The length of the tuples (includes prefix->length)
                   int max_len, // Maximum length allowed
                   const DimRedData* dim_red_data, // Data to generate the actual tuples
                                                   // after the dimension reduction
                   bool has_random, // Should be false if randoms have been removed
                   Comb* first_tuple, // The first tuple
                   uint64_t tuple_count, // How many tuples to consider (-1 to consider all)
                   bool include_outputs, // If true, include outputs in the tuples
                   Dependency shares_to_ignore, // Shares that do not count in failures
                                                // (used only for PINI)
                   bool PINI, // If true, we are checking PINI
                   bool stop_at_first_failure, // If true, stops after the first failure
                   bool only_one_tuple, // If true, stops after checking a single tuple
                   SecretDep* secret_deps_out, // The secret deps to set as output
                   Trie* incompr_tuples, // The trie of incompressible tuples
                                         // (set to NULL to disable this optim)
                   void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void*),
                   //    ^^^^^^^^^^^^^^^^
                   // The function to call when a failure is found
                   void* data // additional data to pass to |failure_callback|
                   ) {

  DependencyList* deps    = circuit->deps;
  int secret_count        = circuit->secret_count;
  int random_count        = circuit->random_count;
  int mult_count          = deps->mult_deps->length;
  int contains_mults      = circuit->contains_mults;
  int corr_outputs_count = deps->correction_outputs->length;

  int bit_rand_len = 1 + (random_count / 64);
  int bit_mult_len = (mult_count == 0) ? 0 :  1 + mult_count / 64;
  int bit_correction_outputs_len = (corr_outputs_count == 0) ? 0 : 1 + corr_outputs_count / 64;


  prefix = prefix ? prefix : &empty_VarVector;
  t_in = t_in > 0 ? t_in : hamming_weight(circuit->all_shares_mask) - 1;

  int last_var = include_outputs ? deps->length : circuit->length;
  int sub_comb_len = comb_len - prefix->length;


  // Retrieving bitvector-based structures
  BitDepVector** bit_deps  = deps->bit_deps;
  //const uint64_t* bit_out_rands = circuit->bit_out_rands;
  //const uint64_t* bit_i1_rands  = circuit->bit_i1_rands;
  //const uint64_t* bit_i2_rands  = circuit->bit_i2_rands;

  // Since elementary probes have been removed, a tuple can be a failure
  // without containing more than |t_in| secret shares: it suffices
  // that it contains |t_in-comb_free_space| secret shares, since the
  // missing secret shares can be taken from elementary probes.
  int comb_free_space = max_len - comb_len;
  /* printf("max_len = %d -- comb_len = %d ==> comb_free_space = %d\n", */
  /*        max_len, comb_len, comb_free_space); */

  // Local dependencies
  int local_deps_max_size = deps->length * 10; // sounds reasonable?
  BitDep* local_deps[local_deps_max_size];
  BitDep* local_deps_copy[local_deps_max_size];
  for (int i = 0; i < local_deps_max_size; i++) {
    local_deps[i] = alloca(sizeof(**local_deps));
    set_bit_dep_zero(local_deps[i]);

    local_deps_copy[i] = alloca(sizeof(**local_deps_copy));
    set_bit_dep_zero(local_deps_copy[i]);
  }
  GaussRand gauss_rands[local_deps_max_size];
  GaussRand gauss_rands_copy[local_deps_max_size];
  
  // Used when factorizing multiplications
  BitDep* deps_fact[local_deps_max_size];
  for (int i = 0; i < local_deps_max_size; i++) {
    deps_fact[i] = alloca(sizeof(**deps_fact));
    set_bit_dep_zero(deps_fact[i]);
  }
  int deps_length_fact = 0;
  GaussRand deps_rands_fact[local_deps_max_size];


  SecretDep leaky_inputs[2] = { 0 }; // We could use |secret_count| instead of 2. However,
                                     // using 2 by default makes the code a bit simpler
                                     // (no need to add "if (secret_count == 2)" everywhere)
  Dependency secret_deps[2] = { 0 };
  int failure_count = 0;
  uint64_t tuples_checked = 0;

  if (comb_len == 0) {
    if (failure_callback && comb_free_space) {
      //printf("here %d\n", max_len);
      Comb curr_comb[max_len];
      return expand_tuple_to_failure(circuit, t_in, shares_to_ignore,
                                     curr_comb, comb_len, leaky_inputs, secret_deps,
                                     max_len, dim_red_data, failure_callback, data);
    }
    return 0;
  }

  // Stuff to perform the Gaussian elimination on the fly.
  // That quite a bit of "stuffs" because 3 Gaussian eliminations are
  // performed on the fly (in the case of multiplication gadgets): one
  // on the output randoms, one on the input randoms before
  // factorization, and one on the input randoms after
  // factorization. The principle in the same in all 3 cases though,
  // which explain why there are 3 very similar blocks of declarations
  // bellow.
  int first_invalid_local_deps_index = 0; // Last index of the tuple for
                                          // which local_deps is still valid
  int new_first_invalid_local_deps_index = 0;
  int tuple_to_local_deps_map[comb_len];
  tuple_to_local_deps_map[0] = 0;
  int local_deps_len = 0;

  int first_invalid_mult_index_fact = 0;
  int local_deps_to_mult_map_fact[local_deps_max_size];
  local_deps_to_mult_map_fact[0] = 0;

  Comb* curr_comb = init_comb(first_tuple, sub_comb_len, prefix, max_len);
  do {
    tuples_checked++;
    first_invalid_local_deps_index = min(new_first_invalid_local_deps_index,
                                         first_invalid_local_deps_index);

    /* printf("Tuple: [ "); */
    /* for (int i = 0; i < comb_len; i++) printf("%d ", curr_comb[i]); */
    /* printf("]  -- first_invalid_local_deps_index = %d\n", first_invalid_local_deps_index); */

    int number_of_shares = get_number_of_shares(circuit, curr_comb, comb_len,
                                                shares_to_ignore, PINI);
    /* printf("number_of_shares+comb_len = %d + %d = %d <= %d = t_in\n", */
    /*        number_of_shares, comb_free_space, number_of_shares + comb_free_space, t_in); */
    if (number_of_shares+comb_free_space <= t_in) {
      //printf(" --> That's not enough shares\n");
      goto process_success;
    }
    /* printf(" --> Enough shares\n"); */

    if (incompr_tuples) {
      // TODO: this will not work if |prefix| is used, since in that
      // case, |curr_comb| will not be sorted. Anyways, we don't use
      // this optimization in practice, so let's not bother about that
      // just right now.
      SecretDep* trie_secret_deps = trie_contains_subset(incompr_tuples, curr_comb, comb_len);
      if (trie_secret_deps) {
        leaky_inputs[0] = trie_secret_deps[0];
        if (secret_count == 2) leaky_inputs[1] = trie_secret_deps[1];
        goto process_success;
      }
    }

    // Reseting |leaky_inputs| (this is not done earlier to avoid
    // resetting it when it's not needed)
    leaky_inputs[0] = leaky_inputs[1] = 0;
    secret_deps[0] = secret_deps[1] = 0;

    local_deps_len = tuple_to_local_deps_map[first_invalid_local_deps_index];

    // 1- Updating |local_deps| while applying a simple Gaussian elimination
    for (int i = first_invalid_local_deps_index; i < comb_len; i++) {
      tuple_to_local_deps_map[i] = local_deps_len;
      BitDepVector* bit_dep_arr = bit_deps[curr_comb[i]];
      for (int dep_idx = 0; dep_idx < bit_dep_arr->length; dep_idx++) {
        gauss_step(bit_dep_arr->content[dep_idx], local_deps, gauss_rands,
                   bit_rand_len, bit_mult_len, bit_correction_outputs_len, local_deps_len);
        set_gauss_rand(local_deps, gauss_rands, local_deps_len, bit_rand_len, deps->correction_outputs, bit_correction_outputs_len);
        local_deps_len++;
        replace_correction_outputs_in_dep(local_deps, local_deps_len - 1, gauss_rands, &local_deps_len, 
                                               bit_correction_outputs_len, bit_rand_len,
                                               bit_mult_len, deps->correction_outputs);
      }
    }
    first_invalid_mult_index_fact = min(first_invalid_mult_index_fact,
                                        first_invalid_local_deps_index);
    first_invalid_local_deps_index = comb_len;

    if (! contains_mults) {
      if (set_contained_shares(leaky_inputs, secret_deps, local_deps, gauss_rands,
                               local_deps_len, secret_count, t_in,
                               comb_free_space, shares_to_ignore, PINI)) {
        goto process_failure;
      } else if (!has_random &&
                 is_failure_with_randoms(circuit, local_deps, gauss_rands, local_deps_copy, gauss_rands_copy,
                                         local_deps_len, t_in,
                                         comb_free_space, shares_to_ignore, PINI)) {
        goto process_failure;
      } else {
        goto process_success;
      }
    } else {
      secret_deps[0] = secret_deps[1] = 0;
      leaky_inputs[0] = leaky_inputs[1] = 0;

      if (!circuit->has_input_rands) {
        // Special case for multiplications without input randoms: no
        // factorization is required, nor any Gaussian elimination on
        // input randoms.
        Dependency secret_share_0 = 0, secret_share_1 = 0;
        uint64_t all_mults[BITMULT_MAX_LEN] = { 0 };
        for (int i = 0; i < local_deps_len; i++) {
          if (! gauss_rands[i].is_set) {
            secret_share_0 |= local_deps[i]->secrets[0];
            secret_share_1 |= local_deps[i]->secrets[1];
            for (int j = 0; j < bit_mult_len; j++) {
              all_mults[j] |= local_deps[i]->mults[j];
            }
          }
        }
        for (int i = 0; i < bit_mult_len; i++) {
          uint64_t mult_elem = all_mults[i];
          while (mult_elem != 0) {
            int mult_idx_in_elem = __builtin_ia32_lzcnt_u64(mult_elem);
            mult_elem &= ~(1ULL << (63-mult_idx_in_elem));
            int mult_idx = i * 64 + (63-mult_idx_in_elem);
            //printf("--- %d\n\n", mult_idx);
            Dependency* this_secret_shares = deps->mult_deps->deps[mult_idx]->contained_secrets; //dim_red_data->old_circuit->deps->mult_deps->deps[mult_idx]->contained_secrets;
            secret_share_0 |= this_secret_shares[0];
            secret_share_1 |= this_secret_shares[1];
          }
        }
        if (PINI) {
          secret_share_0 |= secret_share_1;
        }
        secret_deps[0] = secret_share_0;
        secret_deps[1] = secret_share_1;
        leaky_inputs[0] = Is_leaky(secret_share_0, t_in, 0);
        leaky_inputs[1] = Is_leaky(secret_share_1, t_in, 0);
        if (Is_leaky(secret_share_0, t_in, comb_free_space) ||
            Is_leaky(secret_share_1, t_in, comb_free_space)) {
          goto process_failure;
        } else if (!has_random &&
                   is_failure_with_randoms(circuit, local_deps, gauss_rands, local_deps_copy, gauss_rands_copy,
                                           local_deps_len, t_in,
                                           comb_free_space, shares_to_ignore, PINI)) {
          goto process_failure;
        } else {
          goto process_success;
        }
      }

      // Factorizing tuple

      // for (int h = 0; h < local_deps_len; h++) {
      //   printf("  [ %"PRId32" %"PRId32" | ",
      //             local_deps[h]->secrets[0], local_deps[h]->secrets[1]);
      //   for (int k = 0; k < bit_rand_len; k++){
      //     printf("%"PRId64" ", local_deps[h]->randoms[k]);
      //   }
      //   printf(" | "); 
      //   if(circuit->contains_mults){
      //     for (int k = 0; k < bit_mult_len; k++){
      //       printf("%"PRId64" ", local_deps[h]->mults[k]);
      //     }
      //   } 
      //   printf(" | "); 
      //   for (int k = 0; k < bit_correction_outputs_len; k++){
      //     printf("%" PRId64 " ", local_deps[h]->correction_outputs[k]);
      //   }
      //   printf("] -- gauss_rand = %"PRId64"\n", gauss_rands[h].mask);
      
      // }
      // // printf("\n");

      // printf("FACTORISING %s\n\n", circuit->deps->names[curr_comb[0]]);

      int first_invalid_mult_index_in_local_deps_fact =
        tuple_to_local_deps_map[first_invalid_mult_index_fact];
      first_invalid_mult_index_fact = comb_len;

      deps_length_fact =
        local_deps_to_mult_map_fact[first_invalid_mult_index_in_local_deps_fact];
      
      int up_to_date_deps_length_fact = deps_length_fact;

      // factorize
      for (int i = first_invalid_mult_index_in_local_deps_fact; i < local_deps_len; i++) {
        local_deps_to_mult_map_fact[i] = deps_length_fact;

        //printf("length before = %d\n", deps_length_fact);

        // TODO: it's more efficient to call factorize_mults only once...
        factorize_mults(circuit, &local_deps[i], deps_fact,
                        &deps_length_fact, 1);

        // printf("BEFORE:\n");
        // for (int h = 0; h < deps_length_fact; h++) {
        //   printf("  [ %"PRId32" %"PRId32" | ",
        //             deps_fact[h]->secrets[0], deps_fact[h]->secrets[1]);
        //   for (int k = 0; k < bit_rand_len; k++){
        //     printf("%"PRId64" ", deps_fact[h]->randoms[k]);
        //   }
        //   printf(" | "); 
        //   if(circuit->contains_mults){
        //     for (int k = 0; k < bit_mult_len; k++){
        //       printf("%"PRId64" ", deps_fact[h]->mults[k]);
        //     }
        //   } 
        //   printf(" | "); 
        //   for (int k = 0; k < bit_correction_outputs_len; k++){
        //     printf("%" PRId64 " ", deps_fact[h]->correction_outputs[k]);
        //   }
        //   printf("] -- gauss_rand = %"PRId64"\n", deps_rands_fact[h].mask);
        
        // }
        // printf("\n");

        //printf("done factorizing, length=%d / %d\n",up_to_date_deps_length_fact, deps_length_fact);

        // Apply Gauss on both tuples
        for (int l = up_to_date_deps_length_fact; l < deps_length_fact; l++) {
          gauss_step(deps_fact[l], deps_fact, deps_rands_fact, bit_rand_len, bit_mult_len, bit_correction_outputs_len, l);
          set_gauss_rand(deps_fact, deps_rands_fact, i, bit_rand_len, deps->correction_outputs, bit_correction_outputs_len);

          //printf("%d\n",l);
          replace_correction_outputs_in_dep(deps_fact, l, deps_rands_fact, &deps_length_fact, 
                                                bit_correction_outputs_len, bit_rand_len,
                                                bit_mult_len, deps->correction_outputs);

          //printf("next\n");
        }

        // printf("AFTER:\n");
        // for (int h = 0; h < deps_length_fact; h++) {
        //   printf("  [ %"PRId32" %"PRId32" | ",
        //             deps_fact[h]->secrets[0], deps_fact[h]->secrets[1]);
        //   for (int k = 0; k < bit_rand_len; k++){
        //     printf("%"PRId64" ", deps_fact[h]->randoms[k]);
        //   }
        //   printf(" | "); 
        //   if(circuit->contains_mults){
        //     for (int k = 0; k < bit_mult_len; k++){
        //       printf("%"PRId64" ", deps_fact[h]->mults[k]);
        //     }
        //   } 
        //   printf(" | "); 
        //   for (int k = 0; k < bit_correction_outputs_len; k++){
        //     printf("%" PRId64 " ", deps_fact[h]->correction_outputs[k]);
        //   }
        //   printf("] -- gauss_rand = %"PRId64"\n", deps_rands_fact[h].mask);
        
        // }
        // printf("\n");

        up_to_date_deps_length_fact = deps_length_fact;

      }

      /*for (int i = first_invalid_mult_index_in_local_deps_fact; i < local_deps_len; i++) {
        local_deps_to_mult_map_fact[i] = deps_length_fact;

        // TODO: it's more efficient to call factorize_mults only once...
        factorize_mults(circuit, &local_deps[i], deps_fact,
                        &deps_length_fact, 1);

        for (int j = up_to_date_deps_length_fact; j < deps_length_fact; j++) {

          gauss_step(deps_fact[j], deps_fact, deps_rands_fact, bit_rand_len, bit_mult_len, bit_correction_outputs_len, j);
          set_gauss_rand(deps_fact, deps_rands_fact, j, bit_rand_len);

          replace_correction_outputs_in_last_dep(deps_fact, j, deps_rands_fact, &deps_length_fact, 
                                               bit_correction_outputs_len, bit_rand_len,
                                               bit_mult_len, deps->correction_outputs);

          printf("FINISHED\n");
        }

        up_to_date_deps_length_fact = deps_length_fact;
      }*/

      secret_deps[0] = secret_deps[1] = 0;
      leaky_inputs[0] = leaky_inputs[1] = 0;

      int is_failure = set_contained_shares(leaky_inputs, secret_deps,
                                        deps_fact, deps_rands_fact,
                                        deps_length_fact, secret_count, t_in,
                                        comb_free_space, shares_to_ignore, PINI);

      if (!is_failure) goto process_success;
    }

    process_failure:
    // The tuple is a failure
    // printf("COMB_LEN = %d\n", comb_len);
    // printf("COMB_FREE_SPACE = %d\n", comb_free_space);
    if (failure_callback) {
      if (!has_random) {
        printf("A failure was found. Some randoms might be missing from the tuple you get.\n");
        failure_callback(circuit, curr_comb, comb_len, leaky_inputs, data);
      } else {
        if (dim_red_data) {
          expand_tuple_to_failure(circuit, t_in, shares_to_ignore,
                                  curr_comb, comb_len, leaky_inputs, secret_deps,
                                  max_len, dim_red_data, failure_callback, data);
        } else {
          failure_callback(circuit, curr_comb, comb_len, leaky_inputs, data);
        }
        // failure_callback(circuit, curr_comb, comb_len, leaky_inputs, data);
      }
    }
    if (incompr_tuples) {
      insert_in_trie(incompr_tuples, curr_comb, comb_len, leaky_inputs);
    }
    failure_count++;
    if (stop_at_first_failure) {
      break;
    }

    if (only_one_tuple) {
      secret_deps_out[0] = leaky_inputs[0];
      secret_deps_out[1] = leaky_inputs[1];
    }

  process_success:;
    if (only_one_tuple) break;

  } while (((new_first_invalid_local_deps_index =
             next_comb(curr_comb, sub_comb_len, last_var, prefix)) >= 0) &&
           (tuple_count == -1ULL || --tuple_count != 0));

  // Remember that |curr_comb| is over-allocated with 2 elements at
  // the begining that are never used. Thus, the actual malloc'd
  // pointer is at index |curr_comb-2|.
  free(curr_comb-2);

  return failure_count;
}


struct verify_tuples_args {
  const Circuit* circuit; // The circuit
  int t_in; // The number of shares that must be
            // leaked for a tuple to be a failure
  VarVector* prefix; // Prefix to add to all the tuples
  int comb_len; // The length of the tuples (includes prefix->length)
  int max_len; // Maximum length allowed
  const DimRedData* dim_red_data; // Data to generate the actual tuples
                                  // after the dimension reduction
  bool has_random; // Should be false if randoms have been removed
  Comb* first_tuple; // The first tuple
  uint64_t tuple_count; // How many tuples to consider (-1 to consider all)
  bool include_outputs; // If true, include outputs in the tuples
  Dependency shares_to_ignore; // Shares that do not count in failures
                               // (used only for PINI)
  bool PINI; // If true, we are checking PINI
  bool stop_at_first_failure; // If true, stops after the first failure
  Trie* incompr_tuples; // The trie of incompressible tuples
                        // (set to NULL to disable this optim)
  void (*failure_callback)(const Circuit*,Comb*, int, SecretDep*, void*);
  //     ^^^^^^^^^^^^^^^^
  // The function to call when a failure is found
  void* data; // additional data to pass to |failure_callback|
};

void* _verify_tuples_thread_start(void* void_args) {
  struct verify_tuples_args* args = (struct verify_tuples_args*) void_args;

  _verify_tuples(args->circuit,
                 args->t_in,
                 args->prefix,
                 args->comb_len,
                 args->max_len,
                 args->dim_red_data,
                 args->has_random,
                 args->first_tuple,
                 args->tuple_count,
                 args->include_outputs,
                 args->shares_to_ignore,
                 args->PINI,
                 args->stop_at_first_failure,
                 false, // only_one_tuple
                 NULL, // secret_deps
                 args->incompr_tuples,
                 args->failure_callback,
                 args->data
                 );
  free(args);

  // Note: The return value here doesn't matter, since
  // thread_failure_callback increments a counter of failures.
  return 0;
}

struct thread_callback_data {
  void* data; // The original data
  void (*failure_callback)(const Circuit*,Comb*,
                           int, SecretDep*, void*); // The original callback function
  Trie* seen_tuples; // Failures that have already been seen
  pthread_mutex_t* mutex; // To avoid concurrence issues in |failure_callback|
  int* failure_count; // Total number of failures
  VarVector* prefix; // Prefix, to skip this part of the tuples in |seen_tuples|
};

void thread_failure_callback(const Circuit* circuit, Comb* comb, int comb_len,
                             SecretDep* secret_deps, void* data) {
  struct thread_callback_data* thread_data = (struct thread_callback_data*) data;

  Comb* comb_no_prefix = comb;
  int comb_len_no_prefix = comb_len;
  if (thread_data->prefix && thread_data->prefix->length) {
    comb_no_prefix = &comb_no_prefix[thread_data->prefix->length];
    comb_len_no_prefix -= thread_data->prefix->length;
  }

  pthread_mutex_lock(thread_data->mutex);

  if (trie_contains(thread_data->seen_tuples, comb_no_prefix, comb_len_no_prefix)) {
    pthread_mutex_unlock(thread_data->mutex);
    return;
  }

  (*(thread_data->failure_count))++;
  insert_in_trie(thread_data->seen_tuples, comb_no_prefix, comb_len_no_prefix, NULL);
  thread_data->failure_callback(circuit, comb, comb_len, secret_deps, thread_data->data);

  pthread_mutex_unlock(thread_data->mutex);
}

// A wrapper for _verify_tuples that will automatically parallelize the computation.
int _verify_tuples_parallel(const Circuit* circuit, // The circuit
                            int cores, // How many threads to use
                            int t_in, // The number of shares that must be
                                      // leaked for a tuple to be a failure
                            VarVector* prefix, // Prefix to add to all the tuples
                            int comb_len, // The length of the tuples (includes prefix->length)
                            int max_len, // Maximum length allowed
                            const DimRedData* dim_red_data, // Data to generate the actual tuples
                                                            // after the dimension reduction
                            bool has_random, // Should be false if randoms have been removed
                            Comb* first_tuple, // The first tuple
                            uint64_t tuple_count, // How many tuples to consider (-1 to consider all)
                            bool include_outputs, // If true, include outputs in the tuples
                            Dependency shares_to_ignore,  // Shares that do not count in failures
                                                          // (used only for PINI)
                            bool PINI, // If true, we are checking PINI
                            bool stop_at_first_failure, // If true, stops after the first failure
                            bool only_one_tuple, // If true, stops after checking a single tuple
                            Trie* incompr_tuples, // The trie of incompressible tuples
                            // (set to NULL to disable this optim)
                            void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void*),
                            //     ^^^^^^^^^^^^^^^^
                            // The function to call when a failure is found
                            void* data // additional data to pass to |failure_callback|
                            ) {
  if (cores == 1 || first_tuple != NULL) {
    return _verify_tuples(circuit, t_in, prefix, comb_len, max_len,
                          dim_red_data, has_random, first_tuple, tuple_count,
                          include_outputs, shares_to_ignore, PINI,
                          stop_at_first_failure, only_one_tuple,
                          NULL, incompr_tuples, failure_callback, data);
  }

  int max_in_prefix = 0;
  if (prefix) {
    for (int i = 0; i < prefix->length; i++) {
      max_in_prefix = max(max_in_prefix, prefix->content[i]);
    }
  }

  if (cores == -1) cores = CORES_TO_USE_FOR_MULTITHREADING;
  int real_comb_len = comb_len - (prefix ? prefix->length : 0);
  int max_vars_in_tuples = include_outputs ? circuit->deps->length : circuit->length;
  max_vars_in_tuples = max(max_in_prefix+1, max_vars_in_tuples);
  int trie_size;
  if (dim_red_data) {
    trie_size = include_outputs ? dim_red_data->old_circuit->deps->length :
      dim_red_data->old_circuit->length;
    trie_size = max(max_vars_in_tuples, trie_size);
  } else {
    trie_size = max_vars_in_tuples;
  }
  uint64_t total_tuples = n_choose_k(real_comb_len, max_vars_in_tuples);
  uint64_t tuples_per_core = total_tuples / cores;

  // Initializing threads data
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  Trie* seen_tuples = make_trie(trie_size);
  int failure_count = 0;

  struct thread_callback_data thread_data = {
    .data = data,
    .failure_callback = failure_callback,
    .seen_tuples = seen_tuples,
    .mutex = &mutex,
    .failure_count = &failure_count,
    .prefix = prefix
  };

  pthread_t threads[cores];

  for (int i = 0; i < cores; i++) {
    first_tuple = unrank(max_vars_in_tuples, real_comb_len, tuples_per_core * i);
    struct verify_tuples_args* args = malloc(sizeof(*args));
    args->circuit = circuit;
    args->t_in = t_in;
    args->prefix = prefix;
    args->comb_len = comb_len;
    args->max_len = max_len;
    args->dim_red_data = dim_red_data;
    args->has_random = has_random;
    args->first_tuple = first_tuple;
    args->tuple_count = tuples_per_core + 1; // +1 needed to not forget any tuples
    args->include_outputs = include_outputs;
    args->shares_to_ignore = shares_to_ignore;
    args->PINI = PINI;
    args->stop_at_first_failure = stop_at_first_failure;
    args->incompr_tuples = incompr_tuples;
    args->failure_callback = thread_failure_callback;
    args->data = (void*)&thread_data;

    pthread_create(&threads[i], NULL, _verify_tuples_thread_start, (void*) args);
  }

  for (int i = 0; i < cores; i++) {
    void* unused;
    pthread_join(threads[i], &unused);
  }

  return failure_count;
}

int is_failure(const Circuit* circuit, // The circuit
               int t_in, // The number of shares that must be
                         // leaked for a tuple to be a failure
               int comb_len, // The length of the tuples (includes prefix->length)
               Comb* tuple, // The tuple to check
               bool has_random, // Should be false if randoms have been removed
               SecretDep* secret_deps, // The secret deps to set as output
               Trie* incompr_tuples // The trie of incompressible tuples
                                    // (set to NULL to disable this optim)
               ) {
  return _verify_tuples(circuit, t_in,
                        NULL, // prefix
                        comb_len,
                        comb_len, // max_len
                        NULL, // dim_red_data
                        has_random,
                        tuple,
                        -1, // tuple_count
                        false, // include_outputs
                        0, // shares_to_ignore
                        false, // PINI
                        false, // stop_at_first_failure
                        true, // only_one_tuple
                        secret_deps,
                        incompr_tuples,
                        NULL, // failure_callback
                        NULL // data
                        );
}

// Finds all failures of size |comb_len|, and calls |failure_callback|
// for each of them.
int find_all_failures(const Circuit* circuit, // The circuit
                      int cores, // How many threads to use
                      int t_in, // The number of shares that must be
                                // leaked for a tuple to be a failure
                      VarVector* prefix, // Prefix to add to all the tuples
                      int comb_len, // The length of the tuples (includes prefix->length)
                      int max_len, // Maximum length allowed
                      const DimRedData* dim_red_data, // Data to generate the actual tuples
                                                      // after the dimension reduction
                      bool has_random, // Should be false if randoms have been removed
                      Comb* first_tuple, // The first tuple
                      bool include_outputs, // If true, include outputs in the tuples
                      Dependency shares_to_ignore,  // Shares that do not count in failures
                                                    // (used only for PINI)
                      bool PINI, // If true, we are checking PINI
                      Trie* incompr_tuples, // The trie of incompressible tuples
                                            // (set to NULL to disable this optim)
                      void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void*),
                      //     ^^^^^^^^^^^^^^^^
                      // The function to call when a failure is found
                      void* data // additional data to pass to |failure_callback|
                      ) {
  return _verify_tuples_parallel(circuit, cores, t_in, prefix, comb_len,
                                 max_len, dim_red_data, has_random, first_tuple,
                                 -1, // tuple_count
                                 include_outputs, shares_to_ignore, PINI,
                                 false, // stop at first failure
                                 false, // only_one_tuple
                                 incompr_tuples, failure_callback, data);
}

// Finds the first failure of size |comb_len|, and calls
// |failure_callback| with this failure.
int find_first_failure(const Circuit* circuit, // The circuit
                       int cores, // How many threads to use
                       int t_in, // The number of shares that must be
                                 // leaked for a tuple to be a failure
                       VarVector* prefix, // Prefix to add to all the tuples
                       int comb_len, // The length of the tuples (includes prefix->length)
                       int max_len, // Maximum length allowed
                       const DimRedData* dim_red_data, // Data to generate the actual tuples
                                                       // after the dimension reduction
                       bool has_random, // Should be false if randoms have been removed
                       Comb* first_tuple, // The first tuple
                       bool include_outputs, // If true, include outputs in the tuples
                       Dependency shares_to_ignore,  // Shares that do not count in failures
                                                     // (used only for PINI)
                       bool PINI, // If true, we are checking PINI
                       Trie* incompr_tuples, // The trie of incompressible tuples
                       // (set to NULL to disable this optim)
                       void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void*),
                       //     ^^^^^^^^^^^^^^^^
                       // The function to call when a failure is found
                       void* data // additional data to pass to |failure_callback|
                       ) {
  return _verify_tuples_parallel(circuit, cores, t_in, prefix, comb_len,
                                 max_len, dim_red_data, has_random, first_tuple,
                                 -1, // tuple_count
                                 include_outputs, shares_to_ignore, PINI,
                                 true, // stop at first failure
                                 false, // only_one_tuple
                                 incompr_tuples, failure_callback, data);
}


int find_first_failure_freeSNI_IOS(const Circuit* c,             // The circuit
                       int cores,             // How many threads to use
                       int comb_len,                 // The length of the tuples
                       int comb_free_space,
                       const DimRedData* dim_red_data, // Data to generate the actual tuples
                                                       // after the dimension reduction
                       bool has_random,  // Should be false if randoms have been removed
                       BitDep** output_deps,
                       GaussRand * output_gauss_rands,
                       void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void* data),
                       //     ^^^^^^^^^^^^^^^^
                       // The function to call when a failure is found
                       void* data, // additional data to pass to |failure_callback|
                       bool freesni,
                       bool ios) {

  if (cores == 1){
    return _verify_tuples_freeSNI_IOS(c, 
                              comb_len,
                              comb_free_space,
                              dim_red_data,
                              has_random,
                              true, // stop at first failure
                              output_deps,
                              output_gauss_rands,
                              failure_callback, 
                              data,
                              freesni,
                              ios);
  }
  else{
    printf("MULTI CORES VERIFICATION NOT SUPPORTED YET FOR FREESNI / IOS\n");
    return 1;
  }
  return 1;
}


static void compute_input_secrets(const Circuit* circuit, BitDep* dep, Dependency secrets[2], int mult_count, bool xor){
  if(! circuit->contains_mults){
    secrets[0] = dep->secrets[0];
    secrets[1] = dep->secrets[1];

    if(xor){
      secrets[0] = secrets[0] ^ ((1 << circuit->share_count)-1);

      if(circuit->secret_count > 1)
        secrets[1] = secrets[1] ^ ((1 << circuit->share_count)-1);
    }
  }
  else{
    int bit_mult_len = (mult_count / 64) + (mult_count % 64 != 0);
    secrets[0] = 0;
    secrets[1] = 0;
    for (int i = 0; i < bit_mult_len-1; i++) {
      uint64_t mult_elem = dep->mults[i];
      if(xor){
        mult_elem = mult_elem ^ 0xffffffffffffffff;
      }
      while (mult_elem != 0) {
        int mult_idx_in_elem = __builtin_ia32_lzcnt_u64(mult_elem);
        mult_elem &= ~(1ULL << (63-mult_idx_in_elem));
        int mult_idx = i * 64 + (63-mult_idx_in_elem);
        Dependency* this_secret_shares = circuit->deps->mult_deps->deps[mult_idx]->contained_secrets;
        // printf("contained secrets = %lu %lu\n", this_secret_shares[0], this_secret_shares[1]);
        secrets[0] |= this_secret_shares[0];
        secrets[1] |= this_secret_shares[1];
      }
    }
    int i = bit_mult_len-1;
    uint64_t mult_elem = dep->mults[i];
    if(xor){
      if(mult_count % 64 == 0){
        mult_elem = mult_elem ^ 0xffffffffffffffff;
      }
      else{
         mult_elem = mult_elem ^ ((1ULL << (mult_count % 64)) - 1);
      }
      // if(mult_count <= 64){
      //   mult_elem = mult_elem ^ ((1ULL << mult_count) - 1);
      // }
      // else{
      //   mult_elem = mult_elem ^ (1 << (mult_count % 64));
      // }
    }
    while (mult_elem != 0) {
      int mult_idx_in_elem = __builtin_ia32_lzcnt_u64(mult_elem);
      mult_elem &= ~(1ULL << (63-mult_idx_in_elem));
      int mult_idx = i * 64 + (63-mult_idx_in_elem);
      Dependency* this_secret_shares = circuit->deps->mult_deps->deps[mult_idx]->contained_secrets;
      // printf("contained secrets = %lu %lu\n", this_secret_shares[0], this_secret_shares[1]);
      secrets[0] |= this_secret_shares[0];
      secrets[1] |= this_secret_shares[1];
    }

  }
}


int check_output_uniformity(const Circuit * circuit, BitDep** output_deps, GaussRand * gauss_rands){
  DependencyList* deps    = circuit->deps;
  int secret_count        = circuit->secret_count;
  int random_count        = circuit->random_count;
  int bit_rand_len = 1 + (random_count / 64);
  int mult_count          = deps->mult_deps->length;
  int bit_mult_len = (mult_count == 0) ? 0 :  1 + mult_count / 64;

  // Retrieving bitvector-based structures
  BitDepVector** bit_deps  = deps->bit_deps;

  for (int i = 0; i < circuit->share_count; i++) {
    output_deps[i]->out = 1ULL << i;
    BitDepVector* arr = bit_deps[deps->length - circuit->share_count + i];

    // we don't iterate on arr to consider glitches and transitions yet
    int j = 0;
    output_deps[i]->secrets[0] = arr->content[j]->secrets[0];
    output_deps[i]->secrets[1] = arr->content[j]->secrets[1];
    for (int k = 0; k < bit_rand_len; k++){
      output_deps[i]->randoms[k] = arr->content[j]->randoms[k];
    } 
    if(circuit->contains_mults){
      for (int k = 0; k < bit_mult_len; k++){
        output_deps[i]->mults[k] = arr->content[j]->mults[k];
      }
    } 

  }

  printf("Output Bit dependencies BEFORE reduction:\n");
  for (int i = 0; i < circuit->share_count; i++) {
    printf(" %3d: { ", i);
    printf("  [ %u %u | ",
              output_deps[i]->secrets[0], output_deps[i]->secrets[1]);
    for (int k = 0; k < bit_rand_len; k++){
      printf("%" PRIu64 " ", output_deps[i]->randoms[k]);
    }
    printf(" | "); 
    if(circuit->contains_mults){
      for (int k = 0; k < bit_mult_len; k++){
        printf("%" PRIu64 "", output_deps[i]->mults[k]);
      }
    } 
    printf("| %u ", output_deps[i]->out); 
    printf("]");
    printf(" -- gauss_rand = %" PRIu64 "", gauss_rands[i].mask);
    printf(" }\n");
  }
  printf("\n\n");

  int bit_correction_outputs_len = (deps->correction_outputs->length == 0) ? 0 : 1 + deps->correction_outputs->length / 64;

  for(int i=0; i< circuit->share_count; i++){
    gauss_step(output_deps[i], output_deps, gauss_rands, bit_rand_len, bit_mult_len, 0, i);
    set_gauss_rand(output_deps, gauss_rands, i, bit_rand_len, deps->correction_outputs, bit_correction_outputs_len);
  }

  printf("Output Bit dependencies AFTER reduction:\n");
  for (int i = 0; i < circuit->share_count; i++) {
    printf(" %3d: { ", i);
    printf("  [ %u %u | ",
              output_deps[i]->secrets[0], output_deps[i]->secrets[1]);
    for (int k = 0; k < bit_rand_len; k++){
      printf("%" PRIu64 " ", output_deps[i]->randoms[k]);
    }
    printf(" | "); 
    if(circuit->contains_mults){
      for (int k = 0; k < bit_mult_len; k++){
        printf("%" PRIu64 " ", output_deps[i]->mults[k]);
      }
    } 
    printf("| %u ", output_deps[i]->out); 
    printf("]");
    printf(" -- gauss_rand = %" PRIu64, gauss_rands[i].mask);
    printf(" }\n");
  }
  printf("\n\n");

  Dependency secrets[circuit->share_count][2];
  uint64_t t = (1 << circuit->share_count)-1;
  for (int i = 0; i < circuit->share_count-1; i++) {
    compute_input_secrets(circuit, output_deps[i], secrets[i], mult_count, false);
    bool all_zero = true;
    for (int k = 0; k < bit_rand_len; k++){
      if(output_deps[i]->randoms[k] != 0){
        all_zero = false;
      }
      if(all_zero){
        return 1;
      }
    }
    if(output_deps[i]->out == t){
      return 1;
    }
  }

  compute_input_secrets(circuit, output_deps[circuit->share_count-1], secrets[circuit->share_count-1], mult_count, false);
  if(secrets[circuit->share_count-1][0] != t){
    return 1;
  }
  if((secret_count == 2) && (secrets[circuit->share_count-1][1] != t)){
    return 1;
  }
  for (int k = 0; k < bit_rand_len; k++){
    if(output_deps[circuit->share_count-1]->randoms[k] != 0){
      return 1;
    }
  }
  if(output_deps[circuit->share_count-1]->out != t){
    return 1;
  }

  return 0;

}


static int fail_to_choose_set_I_freeSNI_IOS(const Circuit* circuit,
                      int comb_len,
                      BitDep** local_deps,
                      int local_deps_len,
                      int local_deps_len_tmp,
                      GaussRand* gauss_rands,
                      Dependency ** secrets,
                      Dependency ** secrets_xor,
                      Dependency* choices,
                      Dependency* final_inputs,
                      Dependency* final_output,
                      bool freesni,
                      bool ios,
                      int * total_iterations,
                      int * total_tuples){

  int mult_count          = circuit->deps->mult_deps->length;
  uint64_t t = (1 << circuit->share_count)-1;

  ////////////////////////////////////////////////////////////////////
  // Next, try to construct the set O of output shares independent
  // and uniform conditioned on the probes simulated by I,J , and 
  // c_{|I \inter J}
  ////////////////////////////////////////////////////////////////////
  int index_choices = 0;
  for(int i = local_deps_len_tmp; i< local_deps_len; i++){
    if(gauss_rands[i].is_set) continue;

    if(local_deps[i]->out == 0){
      compute_input_secrets(circuit, local_deps[i], secrets[i], mult_count, false);
      final_inputs[0] = final_inputs[0] | secrets[i][0];
      final_inputs[1] = final_inputs[1] | secrets[i][1];
      //continue;
    }
    else{
      // free SNI
      if(freesni){
        compute_input_secrets(circuit, local_deps[i], secrets[i], mult_count, false);
        compute_input_secrets(circuit, local_deps[i], secrets_xor[i], mult_count, true);
        if((hamming_weight(secrets[i][0] | local_deps[i]->out) > comb_len) ||
            (hamming_weight(secrets[i][1] | local_deps[i]->out) > comb_len)){
      
          final_inputs[0] = final_inputs[0] | (secrets_xor[i][0]) | (local_deps[i]->out ^ t);
          final_inputs[1] = final_inputs[1] | (secrets_xor[i][1]) | (local_deps[i]->out ^ t);
        }
        else if((hamming_weight((secrets_xor[i][0]) | (local_deps[i]->out ^ t)) <= comb_len) &&
                (hamming_weight((secrets_xor[i][1]) | (local_deps[i]->out ^ t)) <= comb_len)){
          choices[index_choices] = i;
          index_choices++;
        } 
        else{
          final_inputs[0] = final_inputs[0] | secrets[i][0] | local_deps[i]->out;
          final_inputs[1] = final_inputs[1] | secrets[i][1] | local_deps[i]->out;
        }

      }

      // IOS
      else if(ios){
        compute_input_secrets(circuit, local_deps[i], secrets[i], mult_count, false);
        compute_input_secrets(circuit, local_deps[i], secrets_xor[i], mult_count, true);
        
        if((hamming_weight(secrets[i][0]) > comb_len) || (hamming_weight(local_deps[i]->out) > comb_len) ||
            (hamming_weight(secrets[i][1]) > comb_len)){
      
          final_inputs[0] = final_inputs[0] | (secrets_xor[i][0]);
          final_inputs[1] = final_inputs[1] | (secrets_xor[i][1]);
          final_output[0] = final_output[0] | (local_deps[i]->out ^ t);

        }
        else if((hamming_weight(secrets_xor[i][0]) <= comb_len) && (hamming_weight(local_deps[i]->out ^ t) <= comb_len) &&
                (hamming_weight(secrets_xor[i][1]) <= comb_len)){
          choices[index_choices] = i;
          index_choices++;
        } 
        else{
          final_inputs[0] = final_inputs[0] | secrets[i][0];
          final_inputs[1] = final_inputs[1] | secrets[i][1];
          final_output[0] = final_output[0] | local_deps[i]->out;
        }
      }
      
    } 
  }

  ////////////////////////////////////////////////////////////////////
  // Try to complete sets of input shares that will satisfy
  // the property
  ////////////////////////////////////////////////////////////////////
  if(index_choices > 0){
    //printf("After: final_inputs = %lu, %lu\n", final_inputs[0], final_inputs[1]);
    (*total_tuples)++;
    bool fail = true;
    Dependency final_inputs_tmp[2] = {final_inputs[0], final_inputs[1]};
    Dependency final_output_tmp = final_output[0];

    for(uint64_t i = 0; i < (1ULL << index_choices); i++){
      (*total_iterations)++;
      final_inputs_tmp[0] = final_inputs[0];
      final_inputs_tmp[1] = final_inputs[1];
      final_output_tmp = final_output[0];

      for(int j = 0; j< index_choices; j++){
        if(freesni){
          if(((1ULL << j) & i)){
            final_inputs_tmp[0] = final_inputs_tmp[0] | (secrets_xor[choices[j]][0]) | (local_deps[choices[j]]->out ^t);
            final_inputs_tmp[1] = final_inputs_tmp[1] | (secrets_xor[choices[j]][1]) | (local_deps[choices[j]]->out ^t);
          }
          else{
            final_inputs_tmp[0] = final_inputs_tmp[0] | secrets[choices[j]][0] | local_deps[choices[j]]->out;
            final_inputs_tmp[1] = final_inputs_tmp[1] | secrets[choices[j]][1] | local_deps[choices[j]]->out;
          }
        }
        else if(ios){
          if(((1ULL << j) & i)){
            final_inputs_tmp[0] = final_inputs_tmp[0] | (secrets_xor[choices[j]][0]);
            final_inputs_tmp[1] = final_inputs_tmp[1] | (secrets_xor[choices[j]][1]);
            final_output_tmp = final_output_tmp | (local_deps[choices[j]]->out ^t);
          }
          else{
            final_inputs_tmp[0] = final_inputs_tmp[0] | secrets[choices[j]][0];
            final_inputs_tmp[1] = final_inputs_tmp[1] | secrets[choices[j]][1];
            final_output_tmp = final_output_tmp | local_deps[choices[j]]->out;
          }
        }
      }
      if((hamming_weight(final_inputs_tmp[0]) <= comb_len) && 
          (hamming_weight(final_inputs_tmp[1]) <= comb_len) &&
          (hamming_weight(final_output_tmp) <= comb_len)){
        fail = false;
        break;
      }
    }
    if(fail){
      final_inputs[0] = final_inputs_tmp[0];
      final_inputs[1] = final_inputs_tmp[1];
      return 1;
    }
    else{
      return 0;
    }
  }
  else{
    if((hamming_weight(final_inputs[0]) > comb_len) ||
        (hamming_weight(final_inputs[1]) > comb_len) ||
        (hamming_weight(final_output[0]) > comb_len)){
      return 1;
    }
    else{
      return 0;
    }
  } 

}


// Checks if the tuple |local_deps| is a failure when adding randoms
// (that were removed from the circuit by the function
// |remove_randoms| of dimensions.c).
static int is_failure_with_randoms_freeSNI_IOS(const Circuit* circuit,
                      int comb_len,
                      int comb_free_space,
                      BitDep** local_deps,
                      int local_deps_len,
                      BitDep** local_deps_copy, 
                      GaussRand* gauss_rands_copy,
                      Dependency ** secrets,
                      Dependency ** secrets_xor,
                      Dependency* choices,
                      Dependency* final_inputs,
                      Dependency* final_output,
                      bool freesni,
                      bool ios,
                      int * total_iterations,
                      int * total_tuples) {
  if (comb_free_space == 0) return 0;

  DependencyList* deps = circuit->deps;
  int random_count = circuit->random_count;
  int mult_count = deps->mult_deps->length;
  int non_mult_deps_count = circuit->deps->first_mult_idx;
  int bit_rand_len = 1 + random_count / 64;
  int bit_mult_len = (mult_count == 0) ? 0 :  1 + mult_count / 64;

  int bit_correction_outputs_len = (deps->correction_outputs->length == 0) ? 0 : 1 + deps->correction_outputs->length / 64;

  // Collecting all randoms of |local_deps| in the binary array |randoms|.
  uint64_t randoms[bit_rand_len];
  memset(randoms, 0, bit_rand_len * sizeof(*randoms));
  for (int i = 0; i < local_deps_len; i++) {
    for (int j = 0; j < bit_rand_len; j++) {
      randoms[j] |= local_deps[i]->randoms[j];
    }
  }

  // Building the array |randoms_arr| that contains the index of the
  // randoms in |local_deps|.
  Var randoms_arr[non_mult_deps_count];
  int randoms_arr_len = 0;
  for (int i = 0; i < bit_rand_len; i++) {
    uint64_t rand_elem = randoms[i];
    while (rand_elem != 0) {
      int rand_idx_in_elem = 63-__builtin_ia32_lzcnt_u64(rand_elem);
      rand_elem &= ~(1ULL << rand_idx_in_elem);
      randoms_arr[randoms_arr_len++] = rand_idx_in_elem + i*64;
    }
  }

  int copy_size = 0;

  // Generating all combinations of randoms of |randoms_arr| and
  // checking if they make the tuple become a failure.
  Comb randoms_comb[randoms_arr_len];
  for (int size = 1; size <= comb_free_space && size <= randoms_arr_len; size++) {
    for (int i = 0; i < size; i++) randoms_comb[i] = i;
    do {

      Comb real_comb[size];
      for (int i = 0; i < size; i++) real_comb[i] = randoms_arr[randoms_comb[i]];
      uint64_t selected_randoms[bit_rand_len];
      memset(selected_randoms, 0, bit_rand_len * sizeof(*selected_randoms));


      for (int i = 0; i < size; i++) {
        int idx    = real_comb[i] / 64;
        selected_randoms[idx] |= 1ULL << real_comb[i];
      }

      copy_size = 0;

      // Computing which secret shares are leaked
      for (int i = 0; i < local_deps_len; i++) {
        memcpy(local_deps_copy[copy_size], local_deps[i], sizeof(*local_deps[i]));
        for (int j = 0; j < bit_rand_len; j++) {
          local_deps_copy[copy_size]->randoms[j] = (local_deps[i]->randoms[j] & selected_randoms[j]) ^ local_deps[i]->randoms[j];
        }
        copy_size ++;
      }

      for (int i = 0; i < copy_size; i++) {
          gauss_step(local_deps_copy[i], local_deps_copy, gauss_rands_copy,
                    bit_rand_len, bit_mult_len, 0, i);
          set_gauss_rand(local_deps_copy, gauss_rands_copy, i, bit_rand_len, deps->correction_outputs, bit_correction_outputs_len);
      }

      final_inputs[0] = 0; final_inputs[1] = 0;
      final_output[0] = 0;

      if(fail_to_choose_set_I_freeSNI_IOS(
        circuit, comb_len+size, local_deps_copy, local_deps_len,
        0, gauss_rands_copy, secrets, secrets_xor, choices, 
        final_inputs, final_output, freesni, ios, total_iterations, total_tuples)){

          return 1;
      }

    } while (incr_comb_in_place(randoms_comb, size, randoms_arr_len));
  }

  return 0;
}




int _verify_tuples_freeSNI_IOS(const Circuit* circuit, // The circuit
                   int comb_len, // The length of the tuples (includes prefix->length)
                   int comb_free_space,
                   const DimRedData* dim_red_data, // Data to generate the actual tuples
                                                   // after the dimension reduction
                   bool has_random,  // Should be false if randoms have been removed
                   bool stop_at_first_failure, // If true, stops after the first failure
                   BitDep** output_deps,
                   GaussRand * output_gauss_rands,
                   void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void*),
                   //    ^^^^^^^^^^^^^^^^
                   // The function to call when a failure is found
                   void* data, // additional data to pass to |failure_callback|
                   bool freesni,
                   bool ios
                   ) {

  DependencyList* deps    = circuit->deps;
  // Retrieving bitvector-based structures
  BitDepVector** bit_deps  = deps->bit_deps;

  int secret_count        = circuit->secret_count;
  int random_count        = circuit->random_count;
  int mult_count          = deps->mult_deps->length;
  int bit_mult_len = (mult_count == 0) ? 0 :  1 + mult_count / 64;
  int bit_rand_len = 1 + (random_count / 64);
  int last_var = circuit->length;

  int bit_correction_outputs_len = (deps->correction_outputs->length == 0) ? 0 : 1 + deps->correction_outputs->length / 64;

  //VarVector* prefix = &empty_VarVector;


  // Local dependencies
  int local_deps_max_size = deps->length * 10; // sounds reasonable?

  // to test independence of output shares
  GaussRand gauss_rands[local_deps_max_size];  
  GaussRand gauss_rands_copy[local_deps_max_size];  
  BitDep* local_deps[local_deps_max_size];
  BitDep* local_deps_copy[local_deps_max_size];

  // to determine the sets of input shares to simulate the t_1 + t_2 probes
  GaussRand gauss_rands_without_outs[local_deps_max_size];

  BitDep* local_deps_without_outs[local_deps_max_size];

  Dependency choices[local_deps_max_size];
  Dependency* secrets[local_deps_max_size];
  Dependency* secrets_xor[local_deps_max_size];
  choices[0] = -1;


  for(int i=0; i<local_deps_max_size; i++){
    local_deps[i] = alloca(sizeof(**local_deps));
    local_deps_copy[i] = alloca(sizeof(**local_deps_copy));

    local_deps_without_outs[i] = alloca(sizeof(**local_deps_without_outs));
    secrets[i] = alloca(sizeof(**secrets));
    secrets_xor[i] = alloca(sizeof(**secrets_xor));

    secrets[i][0] = 0;
    secrets[i][1] = 0;
    secrets_xor[i][0] = 0;
    secrets_xor[i][1] = 0;
  }

  int failure_count = 0;
  uint64_t tuples_checked = 0;

  int local_deps_without_outs_len = 0;
  int local_deps_without_len_tmp  = 0;

  ////////////////////////////////////////////////////////////////////
  // Copy the already reduced n-1 output shares passed to the function
  // to avoid doing row reduction on them at each iteration
  // this is used with |local_deps|
  ////////////////////////////////////////////////////////////////////
  int local_deps_len = 0;
  int local_deps_len_tmp;

  for(int i=0; i< (circuit->share_count)-1; i++){
    memcpy(local_deps[i], output_deps[i], sizeof(*output_deps[i]));
    memcpy(gauss_rands+i, output_gauss_rands+i, sizeof(gauss_rands[i]));

    local_deps_len++;
  }
  local_deps_len_tmp = local_deps_len;

  // Start iterating on the sets of probes
  Comb* curr_comb = init_comb(NULL, comb_len, &empty_VarVector, comb_len);
  
  int total_iterations = 0;
  int total_tuples = 0;
  Dependency final_inputs[2];
  Dependency final_output[1] = {0}; // only for IOS
  int first_invalid_local_deps_index = 0;

  do {
    tuples_checked++;
    
    local_deps_len = local_deps_len_tmp + first_invalid_local_deps_index;
    local_deps_without_outs_len = local_deps_without_len_tmp + first_invalid_local_deps_index;

    ////////////////////////////////////////////////////////////////////
    // Updating |local_deps| and |local_deps_without_outs|
    // with INTERNAL DEPS
    ////////////////////////////////////////////////////////////////////
    for (int i = first_invalid_local_deps_index; i < comb_len; i++) {
      BitDepVector* bit_dep_arr = bit_deps[curr_comb[i]];

      for (int dep_idx = 0; dep_idx < bit_dep_arr->length; dep_idx++) {

        gauss_step(bit_dep_arr->content[dep_idx], local_deps, gauss_rands,
                   bit_rand_len, bit_mult_len, 0, local_deps_len);
        set_gauss_rand(local_deps, gauss_rands, local_deps_len, bit_rand_len, deps->correction_outputs, bit_correction_outputs_len);

        //without outs
        gauss_step(bit_dep_arr->content[dep_idx], local_deps_without_outs, gauss_rands_without_outs,
                   bit_rand_len, bit_mult_len, 0, local_deps_without_outs_len);
        set_gauss_rand(local_deps_without_outs, gauss_rands_without_outs, local_deps_without_outs_len, bit_rand_len, deps->correction_outputs, bit_correction_outputs_len);


        local_deps_len++;
        local_deps_without_outs_len++;

      }

    }

    ////////////////////////////////////////////////////////////////////
    // determining the sets of input shares to simulate the
    // probes -> SNI like verification
    ////////////////////////////////////////////////////////////////////
    final_inputs[0] = 0; final_inputs[1] = 0; final_output[0] = 0;
    for(int i = 0; i < local_deps_without_outs_len; i++){
      if(gauss_rands_without_outs[i].is_set) continue;

      compute_input_secrets(circuit, local_deps_without_outs[i], secrets[i], mult_count, false);
      final_inputs[0] = final_inputs[0] | secrets[i][0];
      final_inputs[1] = final_inputs[1] | secrets[i][1];
    }

    if((hamming_weight(final_inputs[0]) > comb_len) ||
       (((secret_count==2) && (hamming_weight(final_inputs[1]) > comb_len)))){
        
        printf("SNI-like Failure\n");
        goto process_failure;
    }

    // bool pr = false;
    // if(curr_comb[0] == 8 && curr_comb[1] == 14){
    //   pr = true;
    //   printf("After Gauss: {\n");
    //   for (int i = 0; i < local_deps_len; i++) {
    //     printf("  [ %lu %lu | ",
    //               local_deps[i]->secrets[0], local_deps[i]->secrets[1]);
    //     for (int k = 0; k < bit_rand_len; k++){
    //       printf("%llu ", local_deps[i]->randoms[k]);
    //     }
    //     printf(" | "); 
    //     if(circuit->contains_mults){
    //       for (int k = 0; k < bit_mult_len; k++){
    //         printf("%llu ", local_deps[i]->mults[k]);
    //       }
    //     } 
    //     printf("| %u ", local_deps[i]->out); 
    //     printf("] -- gauss_rand = %llu\n", gauss_rands[i].mask);
      
    //   }
    //   printf("\n");
    // }


    if(fail_to_choose_set_I_freeSNI_IOS(
          circuit, comb_len, local_deps, local_deps_len, local_deps_len_tmp, gauss_rands, secrets, secrets_xor, choices, final_inputs,
          final_output, freesni, ios, &total_iterations, &total_tuples
        )
    ){
      goto process_failure;
    }
    else if(!has_random &&
            is_failure_with_randoms_freeSNI_IOS(circuit, comb_len, comb_free_space, local_deps,
                                                local_deps_len, local_deps_copy, gauss_rands_copy,
                                                secrets, secrets_xor, choices, final_inputs, final_output, freesni,
                                                ios, &total_iterations, &total_tuples
                                              )){

        goto process_failure;

    }
    else{
      goto process_success;
    }

    ////////////////////////////////////////////////////////////////////
    // Tuple is a failure
    ////////////////////////////////////////////////////////////////////
    process_failure:
    if (failure_callback) {
      SecretDep leaky_inputs[2] = { 0 };
      Dependency secret_deps[2] = { 0 };
      if (!has_random) {
        printf("A failure was found. Some randoms might be missing from the tuple you get.\n");
        failure_callback(circuit, curr_comb, comb_len, leaky_inputs, data);
      }
      else{
          if (dim_red_data) {
          expand_tuple_to_failure(circuit, -1, 0,
                                  curr_comb, comb_len, leaky_inputs, secret_deps,
                                  comb_len, dim_red_data, failure_callback, data);
          // if((!freesni) && (!ios)){
          //   expand_tuple_to_failure(circuit, -1, 0,
          //                           prefix->content, prefix->length, leaky_inputs, secret_deps,
          //                           prefix->length, dim_red_data, failure_callback, data);
          // }
        } else {
          failure_callback(circuit, curr_comb, comb_len, leaky_inputs, data);
        }
      }
    }
    failure_count++;
    if (stop_at_first_failure) {
      break;
    }

    ////////////////////////////////////////////////////////////////////
    // Tuple is not a failure, move on to the next tuple to check
    ////////////////////////////////////////////////////////////////////
    process_success:;
  } while ((first_invalid_local_deps_index = next_comb(curr_comb, comb_len, last_var, NULL)) >= 0);

  if(failure_count == 0){
     printf("Total tuples with additional iterations = %d\n", total_tuples);
     printf("Total additional iterations = %d\n", total_iterations);
     printf("Iterations per tuple =  %lf\n\n", (total_iterations * 1.0)/total_tuples);
  }

  return failure_count;
}


