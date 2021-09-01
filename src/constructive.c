#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "constructive.h"
#include "constructive-mult.h"
#include "circuit.h"
#include "combinations.h"
#include "list_tuples.h"
#include "verification_rules.h"
#include "trie.h"
#include "failures_from_incompr.h"
#include "vectors.h"


// Note: using a hash to avoid recomputing the same tuples multiple
// times does not work, because 2 tuples can contain the same
// elements, but used to unmask different randoms. I think.


/************************************************
          Constructing the columns
*************************************************/

// Populates the arrays |secrets| and |randoms| as follows:
//
//    - variables that depend on the k-th share of the i-th input are
//      put in the k+i*share_count-th case of the |secrets| array.
//
//    - variables that depend on the i-th random are put in the i-th
//      case of the |randoms| array.
//
// Note that variables can be put in multiples arrays.
void build_dependency_arrays(const Circuit* c,
                             VarVector*** secrets,
                             VarVector*** randoms,
                             bool include_outputs,
                             int verbose) {
  int secret_count     = c->secret_count;
  int share_count      = c->share_count;
  int total_secrets    = c->secret_count * c->share_count;
  int random_count     = c->random_count;
  DependencyList* deps = c->deps;
  int deps_size        = deps->deps_size;

  *secrets = malloc(total_secrets * sizeof(*secrets));
  for (int i = 0; i < total_secrets; i++) {
    (*secrets)[i] = VarVector_make();
  }
  *randoms = malloc((secret_count + random_count) * sizeof(*randoms));
  for (int i = secret_count; i < secret_count + random_count; i++) {
    (*randoms)[i] = VarVector_make();
  }

  // Replace c->length by deps->length to include outputs
  int length = include_outputs ? deps->length : c->length;
  for (int i = 0; i < length; i++) {
    DepArrVector* dep_arr = deps->deps[i];

    // Computing all dependencies contained in |dep_arr|
    Dependency dep[deps_size];
    memset(dep, 0, deps_size * sizeof(*dep));
    for (int dep_idx = 0; dep_idx <  dep_arr->length; dep_idx++) {
      for (int j = 0; j < deps_size; j++) {
        dep[j] |= dep_arr->content[dep_idx][j];
      }
    }

    // Updating |secrets| and |randoms| based on |dep|
    for (int j = 0; j < secret_count; j++) {
      for (int k = 0; k < share_count; k++) {
        if (dep[j] & (1 << k)) {
          VarVector_push((*secrets)[j*share_count + k], i);
        }
      }
    }
    for (int j = secret_count; j < secret_count + random_count; j++) {
      if (dep[j]) {
        VarVector_push((*randoms)[j], i);
      }
    }
  }

  if (verbose) {
    printf("***Size of columns***\n");
    for (int i = 0; i < total_secrets; i++) {
      printf(" - Secret %d, share %d: %d\n", i / share_count, i % share_count,
             (*secrets)[i]->length);
    }
    for (int i = secret_count; i < secret_count + random_count; i++) {
      printf(" - Random %d: %d\n", i, (*randoms)[i]->length);
    }
    printf("\n");
  }
}

/************************************************
             Building the tuples
*************************************************/

int get_first_rand(Dependency* dep, int deps_size, int first_rand_idx) {
  for (int i = first_rand_idx; i < deps_size; i++) {
    if (dep[i]) return i;
  }
  return 0;
}

// |gauss_deps|: the dependencies after gauss elimination up to index
//               |idx| (excluded). Note that even if an element is //
//               fully masked by a random, we keep its other
//               dependencies in order to be able to use them to
//               perform subsequent eliminations.
//
// |gauss_rands|: the randoms used for the gauss elimination. Contains
//                0 when an element does not contain any random.
//
// |real_dep|: the dependency to add to |gauss_deps|, at index |idx|.
//
// |deps_size|: the size of the dependencies in |gauss_deps| and of |real_dep|.
//
// This function adds |real_dep| to |gauss_deps|, and performs a Gauss
// elimination on this element: all previous elements of |gauss_deps|
// have already been eliminated, and we xor them as needed with |real_dep|.
void apply_gauss(int deps_size,
                 Dependency* real_dep,
                 Dependency** gauss_deps,
                 Dependency* gauss_rands,
                 int idx) {
  Dependency* dep_target = gauss_deps[idx];
  if (dep_target != real_dep) {
    memcpy(dep_target, real_dep, deps_size * sizeof(*dep_target));
  }
  for (int i = 0; i < idx; i++) {
    int r = gauss_rands[i];
    if (r != 0 && dep_target[r]) {
      for (int j = 0; j < deps_size; j++) {
        dep_target[j] ^= gauss_deps[i][j];
      }
    }
  }
}

// Adds the tuple |curr_tuple| to the trie |incompr_tuples|.
void add_tuple_to_trie(Trie* incompr_tuples, Tuple* curr_tuple,
                       const Circuit* c, int secret_idx, int revealed_secret) {
  SecretDep* secret_deps = calloc(c->secret_count, sizeof(*secret_deps));
  secret_deps[secret_idx] = revealed_secret;
  // Note that tuples in the trie are sorted (ie, the variables they
  // contain are in ascending order) -> we might need to sort the
  // current tuple.
  if (is_sorted_comb(curr_tuple->content, curr_tuple->length)) {
    insert_in_trie(incompr_tuples, curr_tuple->content, curr_tuple->length, secret_deps);
  } else {
    Comb sorted_comb[curr_tuple->length];
    memcpy(sorted_comb, curr_tuple->content, curr_tuple->length * sizeof(*sorted_comb));
    sort_comb(sorted_comb, curr_tuple->length);
    insert_in_trie(incompr_tuples, sorted_comb, curr_tuple->length, secret_deps);
  }
}

// Checks if |curr_tuple| is incompressible by checking if any of its
// subtuple is incompressible itself (ie, if any of its subtuples is
// in |incompr_tuples|): if any of its subtuple is incompressible,
// then |curr_tuple| is a simple failure, but not an incompressible one.
int tuple_is_not_incompr(Trie* incompr_tuples, Tuple* curr_tuple) {
  if (is_sorted_comb(curr_tuple->content, curr_tuple->length)) {
    return trie_contains_subset(incompr_tuples, curr_tuple->content, curr_tuple->length) ? 1 : 0;
  } else {
    Comb sorted_comb[curr_tuple->length];
    memcpy(sorted_comb, curr_tuple->content, curr_tuple->length * sizeof(*sorted_comb));
    sort_comb(sorted_comb, curr_tuple->length);
    return trie_contains_subset(incompr_tuples, sorted_comb, curr_tuple->length) ? 1 : 0;
  }
}
// Parameters:
//
//  |c|: the circuit
//
//  |incompr_tuples|: the trie of incompressible tuples (contain
//      already computed incompressible tuples, and new ones are added
//      in this trie as well.
//
//  |max_size|: the maximal size of tuples allowed.
//
//  |randoms|: the array of array containing, for all randoms, which
//      variables depend on that random.
//
//  |gauss_deps|: The tuple after gauss elimination on output
//      randoms. However, not that the dependencies are kept in their
//      unmasked form, in order to update the Gauss elimination when
//      adding variables to the tuple.
//
//  |gauss_rands|: The indices of the randoms used for the Gauss
//      elimination on output randoms. When an element of the tuple
//      contains no random, 0 is put in |gauss_rands_o|.
//
//  |secret_idx|: The index of the secret that we are trying to reveal.
//
//  |unmask_idx|: the current index for the recursion on
//      |gauss_deps|. It corresponds to the element that we want to
//      unmask at the current stage of the recursion.
//
//  |factorized_size|: the length of the input dependencies after
//      factorization, or, put otherwise, the length of
//      |gauss_deps_i|. (note that there is no equivalent for
//      |gauss_deps_o| since its length is the length of the current tuple).
//
//  |curr_tuple|: the tuple that is being built.
//
//  |revealed_secret|: the secret that is currently being revealed by
//      |curr_tuple|. This is used to prune out of some branches of the
//      recursion: we never need to unmask an element whose secret
//      shares are already being revealed.
//
//  |debug|: if true, then some debuging information are printed.
//
int tot_adds = 0;
void randoms_step(const Circuit* c,
                  int t_in,
                  bool include_outputs,
                  int required_outputs_remaining,
                  Trie* incompr_tuples,
                  int target_size,
                  bool* to_skip,
                  VarVector** randoms,
                  bool* randoms_added,
                  Dependency** gauss_deps,
                  Dependency* gauss_rands,
                  int gauss_length,
                  int secret_idx,
                  int unmask_idx,
                  Tuple* curr_tuple,
                  int revealed_secret,
                  int debug) {
  if (curr_tuple->length > target_size) return;
  if (include_outputs && required_outputs_remaining < 0) return;

  if (debug) {
    printf("randoms_step(unmask_idx=%d). Tuple: [ ", unmask_idx);
    for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]);
    printf("]  -- unmask_idx = %d\n", unmask_idx);
  }

  // Checking if secret is revealed
  if (__builtin_popcount(revealed_secret) == t_in) {
    tot_adds++;
    if (include_outputs && required_outputs_remaining != 0) return;
    if (tuple_is_not_incompr(incompr_tuples, curr_tuple)) {
      return;
    }
    add_tuple_to_trie(incompr_tuples, curr_tuple, c, secret_idx, revealed_secret);
    return;
  }

  // Checking if reached the end of the tuple.
  if (unmask_idx == gauss_length) {
    return;
  }

  // Checking if |target_size| is reached.
  if (curr_tuple->length == target_size) {
    return;
  }

  // Checking if the element at index |unmask_idx| needs to be
  // unmasked. If not, skipping to the next element.
  // TODO: this recursion step is probably wrong. See comment at the
  // top of constructive-mult.c
  /* int secret_to_unmask = gauss_deps[unmask_idx][secret_idx]; */
  /* if ((revealed_secret & secret_to_unmask) == secret_to_unmask) { */
  /*   randoms_step(c, incompr_tuples, target_size, to_skip, randoms, randoms_added, */
  /*                gauss_deps, gauss_rands, secret_idx, */
  /*                unmask_idx+1, curr_tuple, revealed_secret, debug); */
  /*   return; */
  /* } */


  // Check if some later element of the tuple contains the same secret
  // shares. Sometimes, there is no need to unmask the current
  // element. Consider for instance (share_count = 3):
  //
  //     [ 2 0 0 1 0 0 0 1 ]
  //     [ 1 1 1 1 0 0 1 0 ]
  //     [ 4 0 0 0 0 0 0 0 ]
  //
  // As it turns out, the 1st element is already somewhat unmasked
  // by the 2nd one before adding any randoms.
  // (example from `Crypto2020_Gadgets/gadget_add_2_o2.sage`,
  //  incompressible tuple with indices [ 2 6 10 15 16 ])
  // TODO: this recursion step is a bit expensive. Would be nice to do
  // it only when necessary..
  // TODO: check that this is OK.
  /* int secret_is_somewhere_else = 0; */
  /* for (int i = unmask_idx+1; i < curr_tuple->length; i++) { */
  /*   if ((gauss_deps[i][secret_idx] & gauss_deps[unmask_idx][secret_idx]) != 0) { */
  /*     secret_is_somewhere_else = 1; */
  /*     break; */
  /*   } */
  /* } */
  /* if (secret_is_somewhere_else) { */
  randoms_step(c, t_in, include_outputs, required_outputs_remaining,
               incompr_tuples, target_size, to_skip, randoms, randoms_added,
               gauss_deps, gauss_rands, gauss_length, secret_idx,
               unmask_idx+1, curr_tuple, revealed_secret, debug);
  /* } */

  int curr_size = curr_tuple->length;
  int rand_idx = gauss_rands[unmask_idx];

  if (rand_idx == 0) {
    // if |secret_is_somewhere_else| is true, then this recursive call
    // has already been performed.
    // TODO: uncomment if using the "secret_is_somewhere_else" opti
    /* if (!secret_is_somewhere_else) { */
    /*   randoms_step(c, t_in, include_outputs, required_outputs_remaining, */
    /*                incompr_tuples, target_size, to_skip, randoms, randoms_added, */
    /*                gauss_deps, gauss_rands, gauss_length, secret_idx, */
    /*                unmask_idx+1, curr_tuple, revealed_secret, debug); */
    /* } */
    return;
  } else {
    randoms_added[rand_idx] = 1;
    VarVector* dep_array = randoms[rand_idx];
    for (int j = 0; j < dep_array->length; j++) {
      Var dep = dep_array->content[j];
      if (to_skip[dep]) continue;

      if (Tuple_contains(curr_tuple, dep)) {
        // Do not add an element that is already in the tuple
        continue;
      }

      // TODO: re-enable this optimization
      /* if (c->deps->deps[dep][secret_idx] == gauss_deps[unmask_idx][secret_idx]) { */
      /*   // This element contains the same secret share as the element */
      /*   // at index |unmask_idx|, which means that adding it to the */
      /*   // tuple will loose the secret share at index |unmask_idx| */
      /*   // rather than unmasking it. For instance, if we had the tuple: */
      /*   //   (a0 ^ r0) */
      /*   // and we tried to unmask it by adding */
      /*   //   (a0 ^ r0 ^ r1) */
      /*   // We would thus end up with */
      /*   //   (a0 ^ r0, a0 ^ r0 ^ r1) */
      /*   // Which, after Gauss elimination _on the 1st element only_, produces: */
      /*   //   (r0, r1) */
      /*   continue; */
      /* } */

      curr_tuple->content[curr_size] = dep;

      int new_gauss_length = gauss_length;
      const DepArrVector* dep_arr = c->deps->deps[dep];
      for (int dep_idx = 0; dep_idx < dep_arr->length; dep_idx++) {
        apply_gauss(c->deps->deps_size, dep_arr->content[dep_idx],
                    gauss_deps, gauss_rands, new_gauss_length++);
      }

      int has_secret_share = 0;
      for (int l = gauss_length; l < new_gauss_length; l++) {
        if (gauss_deps[l][secret_idx]) {
          has_secret_share = 1;
          break;
        }
      }
      if (!has_secret_share) {
          // This element does not contain any random share. It can
          // happen if when applying gauss elimination, some
          // expressions cancelled out the secret shares of the
          // current expression.
          continue;
      }

      int new_required_outputs_remaining = required_outputs_remaining;
      if (dep >= c->length) {
        new_required_outputs_remaining++;
      }

      int new_revealed_secret = revealed_secret;
      for (int l = gauss_length; l < new_gauss_length; l++) {
        int first_rand = get_first_rand(gauss_deps[l], c->deps->deps_size,
                                        c->deps->first_rand_idx);
        gauss_rands[l] = first_rand;
        if (first_rand == 0) {
          new_revealed_secret |= gauss_deps[l][secret_idx];
        }
      }
      curr_tuple->length++;
      randoms_step(c, t_in, include_outputs, new_required_outputs_remaining,
                   incompr_tuples, target_size, to_skip, randoms, randoms_added,
                   gauss_deps, gauss_rands, new_gauss_length,
                   secret_idx, unmask_idx+1,
                   curr_tuple, new_revealed_secret, debug);
      curr_tuple->length--;
    }

    randoms_added[rand_idx] = 0;
  }
}

int get_initial_revealed_secret(const Circuit* c, int size,
                                Dependency** gauss_deps,
                                int secret_idx) {
  int revealed_secret = 0;
  for (int i = 0; i < size; i++) {
    int is_masked = 0;
    for (int j = c->deps->first_rand_idx; j < c->deps->deps_size; j++) {
      if (gauss_deps[i][j]) {
        is_masked = 1;
        break;
      }
    }
    if (!is_masked) {
      revealed_secret |= gauss_deps[i][secret_idx];
    }
  }
  return revealed_secret;
}

void initial_gauss_elimination(const Circuit* c,
                               int t_in,
                               bool include_outputs,
                               int required_outputs_remaining,
                               Trie* incompr_tuples,
                               int target_size,
                               bool* to_skip,
                               VarVector** randoms,
                               Dependency** gauss_deps,
                               Dependency* gauss_rands,
                               int secret_idx,
                               Tuple* curr_tuple,
                               int debug) {
  int gauss_length = 0;
  DependencyList* deps = c->deps;
  for (int i = 0; i < curr_tuple->length; i++) {
    DepArrVector* dep_arr = deps->deps[curr_tuple->content[i]];
    for (int j = 0; j < dep_arr->length; j++) {
      Dependency* real_dep = dep_arr->content[j];
      apply_gauss(deps->deps_size, real_dep, gauss_deps, gauss_rands, gauss_length);
      gauss_rands[gauss_length] = get_first_rand(gauss_deps[gauss_length], deps->deps_size,
                                                 deps->first_rand_idx);
      gauss_length++;
    }
  }

  bool* randoms_added = calloc(c->deps->length, sizeof(*randoms_added));
  int revealed_secret = get_initial_revealed_secret(c, gauss_length, gauss_deps, secret_idx);
  randoms_step(c, t_in, include_outputs, required_outputs_remaining,
               incompr_tuples, target_size, to_skip, randoms, randoms_added,
               gauss_deps, gauss_rands, gauss_length,
               secret_idx, 0, curr_tuple, revealed_secret, debug);
  free(randoms_added);
}

void secrets_step(const Circuit* c,
                  int t_in,
                  bool include_outputs,
                  int required_outputs_remaining,
                  Trie* incompr_tuples,
                  int target_size,
                  bool* to_skip,
                  VarVector** secrets,
                  VarVector** randoms,
                  Dependency** gauss_deps,
                  Dependency* gauss_rands,
                  int next_secret_share_idx, // Index of the next secret share to consider
                  int selected_secret_shares_count, // Number of secret shares already taken
                  int secret_idx, // Index of the secret that the current tuple should reveal
                  Tuple* curr_tuple,
                  int debug) {
  if (next_secret_share_idx == -1 || curr_tuple->length == target_size || selected_secret_shares_count == t_in) {
    if (selected_secret_shares_count != t_in) return;
    if (include_outputs && required_outputs_remaining < 0) return;
    if (debug) {
      printf("\n\nBase tuple: [ ");
      for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]);
      printf("] (size_max = %d)\n", target_size);
    }
    initial_gauss_elimination(c, t_in, include_outputs, required_outputs_remaining,
                              incompr_tuples, target_size, to_skip, randoms,
                              gauss_deps, gauss_rands,
                              secret_idx, curr_tuple, debug);
  } else {
    // Skipping the current share if there are enough shares remaining
    if (next_secret_share_idx >= t_in - selected_secret_shares_count) {
      secrets_step(c, t_in, include_outputs, required_outputs_remaining, incompr_tuples,
                   target_size, to_skip, secrets, randoms,
                   gauss_deps, gauss_rands,
                   next_secret_share_idx-1, selected_secret_shares_count,
                   secret_idx, curr_tuple, debug);
    }

    VarVector* dep_array = secrets[next_secret_share_idx];
    for (int i = 0; i < dep_array->length; i++) {
      Comb dep_idx = dep_array->content[i];
      /* if (to_skip[dep_idx]) continue; */
      /* to_skip[dep_idx] = true; */
      if (Tuple_contains(curr_tuple, dep_idx)) {
        // This variable of the gadget contains multiple shares of the
        // same input. No need to add it multiple times to the tuples,
        // just recusring further.
        secrets_step(c, t_in, include_outputs, required_outputs_remaining, incompr_tuples,
                     target_size, to_skip, secrets, randoms,
                     gauss_deps, gauss_rands,
                     next_secret_share_idx-1, selected_secret_shares_count+1,
                     secret_idx, curr_tuple, debug);
      } else {
        Tuple_push(curr_tuple, dep_idx);
        int new_required_outputs_remaining = required_outputs_remaining;
        if (dep_idx >= c->length) {
          new_required_outputs_remaining++;
        }
        secrets_step(c, t_in, include_outputs, new_required_outputs_remaining, incompr_tuples,
                     target_size, to_skip, secrets, randoms,
                     gauss_deps, gauss_rands,
                     next_secret_share_idx-1, selected_secret_shares_count+1,
                     secret_idx, curr_tuple, debug);
        Tuple_pop(curr_tuple);
      }
      //to_skip[dep] = false;
    }
  }
  // Reset upper level of |to_skip|
  //if (secrets_count != c->share_count-1) {
  /* if (secrets_count != -1) { */
  /*   VarVector* dep_array = secrets[secrets_count]; */
  /*   for (int i = 0; i < dep_array->length; i++) { */
  /*     Var dep_idx = dep_array->content[i]; */
  /*     to_skip[dep_idx] = false; */
  /*   } */
  /* } */
}


Trie* build_incompr_tuples(const Circuit* c,
                           VarVector** secrets,
                           VarVector** randoms,
                           int t_in,
                           VarVector* prefix,
                           int max_size,
                           bool include_outputs,
                           int required_outputs,
                           int debug) {
  // TODO: compute more precisely what size is needed
  int max_deps_length = c->deps->length * 20;

  Tuple* curr_tuple = Tuple_make_size(c->deps->length);
  for (int i = 0; i < prefix->length; i++) {
    Tuple_push(curr_tuple, prefix->content[i]);
  }
  Dependency** gauss_deps = malloc(max_deps_length * sizeof(*gauss_deps));
  for (int i = 0; i < max_deps_length; i++) {
    gauss_deps[i] = malloc(c->deps->deps_size * sizeof(*gauss_deps[i]));
  }
  Dependency* gauss_rands = malloc(max_deps_length * sizeof(*gauss_rands));
  int share_count = c->share_count;
  // TODO: one trie per input?
  Trie* incompr_tuples = make_trie(c->deps->length);

  int max_incompr_size = c->share_count + c->random_count;
  max_incompr_size = max_size == -1 ? max_incompr_size :
    max_size < max_incompr_size ? max_size : max_incompr_size;
  for (int target_size = 1; target_size <= max_incompr_size; target_size++) {
    for (int i = 0; i < c->secret_count; i++) {
      bool to_skip[c->deps->length];
      memset(to_skip, 0, c->deps->length * sizeof(*to_skip));
      secrets_step(c, t_in, include_outputs, required_outputs, incompr_tuples, target_size,
                   to_skip, &secrets[share_count * i],
                   randoms, gauss_deps, gauss_rands,
                   c->share_count-1, // next_secret_share_idx
                   0, // selected_secret_shares_count
                   i, // secret_idx
                   curr_tuple, debug);
    }
    printf("Size %d: %d tuples\n", target_size, trie_tuples_size(incompr_tuples, target_size));
  }

  Tuple_free(curr_tuple);
  for (int i = 0; i < c->deps->length; i++) {
    free(gauss_deps[i]);
  }
  free(gauss_deps);
  free(gauss_rands);

  return incompr_tuples;
}


Trie* compute_incompr_tuples(const Circuit* c,
                             int t_in,  // The number of shares that must be
                                        // leaked for a tuple to be a failure
                             VarVector* prefix, // Prefix to add to all the tuples
                             int max_size, // The maximal size of the incompressible tuples
                             bool include_outputs, // if true, includes outputs
                             int required_outputs, // number of outputs required in each tuple
                             int verbose) {
  if (c->contains_mults) {
    return compute_incompr_tuples_mult(c, max_size, verbose);
  }
  VarVector** secrets;
  VarVector** randoms;
  build_dependency_arrays(c, &secrets, &randoms, include_outputs, verbose);

  if (t_in == -1) t_in = c->share_count;
  prefix = prefix ? prefix : &empty_VarVector;

  // As a parameter to compute_incompr_tuples, |include_outputs|
  // instructs on whether build_dependency_arrays should take outputs
  // into account. However, as a parameter to build_incompr_tuples
  // (and the functions called by it), |include_outputs| is used to
  // indicate if the number of outputs present in the tuples is fixed
  // by |required_outputs| or not.
  include_outputs = required_outputs > 0 ? true : false;

  Trie* incompr_tuples = build_incompr_tuples(c, secrets, randoms, t_in,
                                              prefix, max_size, include_outputs,
                                              required_outputs, verbose);

  // The following code was useful to identify incompressible tuples
  // whose sum didn't cancel all randoms.
  /* for (int size = 1; size < max_size; size++) { */
  /*   ListComb* incompr_list = list_from_trie(incompr_tuples, size); */
  /*   ListCombElem* curr = incompr_list->head; */
  /*   while (curr) { */
  /*     Dependency summed[c->deps->deps_size]; */
  /*     memset(summed, 0, c->deps->deps_size * sizeof(*summed)); */
  /*     for (int i = 0; i < size; i++) { */
  /*       int dep_idx = curr->comb[i]; */
  /*       Dependency* dep = c->deps->deps[dep_idx]->content[0]; */
  /*       for (int j = 0; j < c->deps->deps_size; j++) { */
  /*         summed[j] ^= dep[j]; */
  /*       } */
  /*     } */

  /*     /\* printf("[ "); *\/ */
  /*     /\* for (int i = 0; i < c->deps->deps_size; i++) printf("%d ", summed[i]); *\/ */
  /*     /\* printf("]\n"); *\/ */

  /*     ListCombElem* next = curr->next; */
  /*     free(curr->comb); */
  /*     free(curr); */
  /*     curr = next; */
  /*   } */
  /* } */

  if (verbose) {
    printf("\nTotal incompr: %d\n", trie_size(incompr_tuples));

    for (int size = c->share_count; size < 100; size++) {
      int tot = trie_tuples_size(incompr_tuples, size);
      printf("Incompr of size %d: %d\n", size, tot);
      if (tot == 0) break;
    }

    printf("Generated %d tuples.\n\n", tot_adds);
  }

  // Freeing stuffs
  {
    int secret_count     = c->secret_count;
    int total_secrets    = c->secret_count * c->share_count;
    int random_count     = c->random_count;

    for (int i = 0; i < total_secrets; i++) {
      VarVector_free(secrets[i]);
    }
    free(secrets);
    for (int i = secret_count; i < secret_count + random_count; i++) {
      VarVector_free(randoms[i]);
    }
    free(randoms);
  }

  return incompr_tuples;
}

void compute_RP_coeffs_incompr(const Circuit* c, int coeff_max, int verbose) {
  Trie* incompr_tuples = compute_incompr_tuples(c, c->share_count,
                                                NULL, coeff_max, false, 0, verbose);

  // Generating failures from incompressible tuples, and computing coefficients.
  compute_failures_from_incompressibles(c, incompr_tuples, coeff_max, verbose);

  free_trie(incompr_tuples);
}
