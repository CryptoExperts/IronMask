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
#include "constructive-mult-compo.h"
#include "vectors.h"

#define INIT_ARR_SIZE 10

// The code of constructive-mult.c is out-of-date since
// verification_rules.c uses bitvectors for the dependencies instead
// of standard Dependency*. However, since constructive-mult.c is
// generally slower than verification_rules.c (for multiplication
// gadgets), I didn't bother to update it for now. Switch to the
// an older commit to use constructive-mult.c.

#if 0

/* Optimization idea (that does not work):

   During main recursion, skip indices that contain already revealed
   secret. For instance, in the tuple:

      (a0, a0 ^ r0, a1 ^ r2)

   We could skip "a0 ^ r0" since "a0" is already revealed. However,
   this causes some issues in some cases. Consider for instance:

      (a0, a0 ^ r0 ^ r1, a1 ^ r0 ^ r2)

   After Guass elimination, that gives us:

      (a0, r0, r1)

   We would be tempted to skip the 2nd element (since a0 is already
   revealed), and thus try to unmask only the last element. To do so,
   we need to add a variable containing r1. However, by doing so, we
   would miss the fact that adding "r0 ^ r2" also unmasks the 3rd
   element: the tuple

      (a0, a0 ^ r0 ^ r1, a1 ^ r0 ^ r2, r0 ^ r2)

   Becomes, after Gauss elimination:

      (a0, r0, r1, a1)

   TODO: This optimization improves _greatly_ performance. Would be
   nice to find a condition that allows to perform it.

 */


/************************************************
          Constructing the columns
*************************************************/

static void get_deps(const Circuit* c,
                     Dependency* dep,
                     Dependency* dst) {
  int deps_size           = c->deps->deps_size;
  int non_mult_deps_count = c->deps->deps_size - c->deps->mult_deps->length;

  for (int j = 0; j < non_mult_deps_count; j++) {
    dst[j] |= dep[j];
  }
  for (int j = non_mult_deps_count; j < deps_size; j++) {
    if (dep[j]) {
      MultDependency* mult_dep = c->deps->mult_deps->deps[j-non_mult_deps_count];
      get_deps(c, mult_dep->left_ptr, dst);
      get_deps(c, mult_dep->right_ptr, dst);
    }
    }
}

// Populates the arrays |secrets| and |randoms| as follows:
//
//    - variables that depend on the k-th share of the i-th input are
//      put in the k+i*share_count-th case of the |secrets| array.
//
//    - variables that depend on the i-th random are put in the i-th
//      case of the |randoms| array.
//
// Note that variables can be put in multiples arrays.
static void update_arrs_with_deps(const Circuit* c,
                                  int main_dep_idx,
                                  DepArrVector* dep_arr,
                                  VarVector** secrets,
                                  VarVector** randoms) {
  int secret_count        = c->secret_count;
  int share_count         = c->share_count;
  int deps_size           = c->deps->deps_size;
  int non_mult_deps_count = c->deps->deps_size - c->deps->mult_deps->length;

  // Computing all dependencies contained in |dep_arr|.
  Dependency dep[deps_size];
  memset(dep, 0, deps_size * sizeof(*dep));
  for (int dep_idx = 0; dep_idx < dep_arr->length; dep_idx++) {
    get_deps(c, dep_arr->content[dep_idx], dep);
  }

  for (int i = 0; i < secret_count; i++) {
    for (int j = 0; j < share_count; j++) {
      if (dep[i] & (1 << j)) {
        VarVector_push(secrets[i*share_count + j], main_dep_idx);
      }
    }
  }

  for (int i = secret_count; i < non_mult_deps_count; i++) {
    if (dep[i]) {
      VarVector_push(randoms[i], main_dep_idx);
    }
  }
}

void build_dependency_arrays_mult(const Circuit* c,
                                    VarVector*** secrets,
                                    VarVector*** randoms,
                                    int include_outputs,
                                    int verbose) {
  int secret_count        = c->secret_count;
  int share_count         = c->share_count;
  int random_count        = c->random_count;
  int total_secrets       = secret_count * share_count;
  DependencyList* deps = c->deps;

  *secrets = malloc(total_secrets * sizeof(*secrets));
  for (int i = 0; i < total_secrets; i++) {
    (*secrets)[i] = VarVector_make();
  }
  *randoms = malloc((secret_count + random_count) * sizeof(*randoms));
  for (int i = secret_count; i < secret_count + random_count; i++) {
    (*randoms)[i] = VarVector_make();
  }

  int length = include_outputs ? deps->length : c->length;
  for (int i = 0; i < length; i++) {
    update_arrs_with_deps(c, i, deps->deps[i], *secrets, *randoms);
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

static void add_tuple_to_trie(Trie* incompr_tuples, Tuple* curr_tuple,
                              const Circuit* c, int secret_idx, int revealed_secret) {
  SecretDep* secret_deps = calloc(c->secret_count, sizeof(*secret_deps));
  secret_deps[secret_idx] = revealed_secret;
  if (is_sorted_comb(curr_tuple->content, curr_tuple->length)) {
    insert_in_trie(incompr_tuples, curr_tuple->content, curr_tuple->length, secret_deps);
  } else {
    Comb sorted_comb[curr_tuple->length];
    memcpy(sorted_comb, curr_tuple->content, curr_tuple->length * sizeof(*sorted_comb));
    sort_comb(sorted_comb, curr_tuple->length);
    insert_in_trie(incompr_tuples, sorted_comb, curr_tuple->length, secret_deps);
  }
}

static int tuple_is_not_incompr(Trie* incompr_tuples, Tuple* curr_tuple) {
  if (is_sorted_comb(curr_tuple->content, curr_tuple->length)) {
    return trie_contains_subset(incompr_tuples, curr_tuple->content, curr_tuple->length) ? 1 : 0;
  } else {
    Comb sorted_comb[curr_tuple->length];
    memcpy(sorted_comb, curr_tuple->content, curr_tuple->length * sizeof(*sorted_comb));
    sort_comb(sorted_comb, curr_tuple->length);
    return trie_contains_subset(incompr_tuples, sorted_comb, curr_tuple->length) ? 1 : 0;
  }
}


// Returns true if |deps| is a failure for input |secret_idx|. This
// function should be called on tuples that do not contain
// multiplications (ie, after factorization).
/* static int is_failure(Dependency** deps, int deps_len, */
/*                       int secret_idx, int share_count) { */
/*   int secret_dep = 0; */
/*   for (int i = 0; i < deps_len; i++) { */
/*     secret_dep |= deps[i][secret_idx]; */
/*   } */
/*   return __builtin_popcount(secret_dep) == share_count; */
/* } */

// Returns the secret shares in the dependencies |deps| at index
// |secret_idx| that are not masked by randoms. Note that this
// function is meant to be called on tuples that do not contain
// multiplications (ie, after factorization).
/* static int get_revealed_secret(Dependency** deps, int deps_length, */
/*                         int secret_idx, int first_rand_idx, */
/*                         int non_mult_deps_count) { */
/*   int revealed_secret = 0; */
/*   for (int i = 0; i < deps_length; i++) { */
/*     if (!deps[i][secret_idx]) continue; */
/*     for (int j = first_rand_idx; i < non_mult_deps_count; i++) { */
/*       if (deps[i][j]) { */
/*         goto next_outer; */
/*       } */
/*     } */
/*     revealed_secret |= deps[i][secret_idx]; */

/*   next_outer:; */
/*   } */
/*   return revealed_secret; */
/* } */

// Returns the secret shares contained in |dep| at index
// |secret_idx|. Follows multiplications. Note that this does not
// return the secret leaked by |dep|, but only the secret it contains
// (ie, this secret can be masked by some randoms).
//
// TODO: this does not take multiplication factorization into
// account... Is that an issue?
//
// TODO: this code is outdated now that we have DepArrVectors instead
// of Dependency*
//
/* static int get_contained_secret(const Circuit* c, Dependency* dep, int secret_idx) { */
/*   DependencyList* deps    = c->deps; */
/*   int deps_size           = deps->deps_size; */
/*   int non_mult_deps_count = deps_size - deps->mult_deps->length; */

/*   int secret = dep[secret_idx]; */


/*   for (int i = non_mult_deps_count; i < deps_size; i++) { */
/*     if (dep[i]) { */
/*       MultDependency* mult_dep = deps->mult_deps->deps[i-non_mult_deps_count]; */
/*       secret |= get_contained_secret(c, mult_dep->left_ptr,  secret_idx); */
/*       secret |= get_contained_secret(c, mult_dep->right_ptr, secret_idx); */
/*     } */
/*   } */

/*   return secret; */
/* } */

static int get_initial_revealed_secret(const Circuit* c, int size,
                                       Dependency** gauss_deps,
                                       int secret_idx) {
  int non_mult_deps_count = c->deps->deps_size - c->deps->mult_deps->length;
  int revealed_secret = 0;
  for (int i = 0; i < size; i++) {
    if (!gauss_deps[i][secret_idx]) continue;
    int is_masked = 0;
    for (int j = c->deps->first_rand_idx; j < non_mult_deps_count; j++) {
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

static int get_initial_shares(int size,
                              Dependency** gauss_deps,
                              int secret_idx) {
  int shares = 0;
  for (int i = 0; i < size; i++) {
    shares |= gauss_deps[i][secret_idx];
  }
  return shares;
}

// Returns the first random in |dep| (between indices |first_rand_idx|
// and |deps_size| for which |allowed_rand| is true. If |allowed_rand|
// is NULL, then just returns the first random of |dep|. In practice,
// |allowed_rand| should be either circuit->out_rands or
// circuit->i1_rands (or i2_rands), thus returning a random that is
// suitable for the current Gauss elimination: either an output random
// or an input random.
static int get_first_rand_mult(Dependency* dep, int deps_size, int first_rand_idx,
                               bool* allowed_rand) {
  for (int i = first_rand_idx; i < deps_size; i++) {
    if (dep[i]) {
      if (!allowed_rand || allowed_rand[i]) {
        return i;
      }
    }
  }
  return 0;
}

// TODO: remove (if not useful), or adapt with DepArrVector instead of
// Dependency* (if useful)
/* static int get_first_in_rand(const Circuit* c, Dependency* dep, bool* disallowed_rand) { */
/*   int deps_size           = c->deps->deps_size; */
/*   int first_rand_idx      = c->deps->first_rand_idx; */
/*   int non_mult_deps_count = deps_size - c->deps->mult_deps->length; */

/*   for (int i = first_rand_idx; i < non_mult_deps_count; i++) { */
/*     if (dep[i] && !disallowed_rand[i]) { */
/*       return i; */
/*     } */
/*   } */

/*   for (int i = non_mult_deps_count; i < deps_size; i++) { */
/*     if (dep[i]) { */
/*       MultDependency* mult_dep = c->deps->mult_deps->deps[i-non_mult_deps_count]; */
/*       int left_rand = get_first_in_rand(c, mult_dep->left_ptr, disallowed_rand); */
/*       if (left_rand == -1) { */
/*         int right_rand = get_first_in_rand(c, mult_dep->right_ptr, disallowed_rand); */
/*         if (right_rand != -1) return right_rand; */
/*       } else { */
/*         return left_rand; */
/*       } */
/*     } */
/*   } */

/*   return -1; */
/* } */


static int tot_adds = 0;

// About the |out_rec| param: this function is recursive on both
// |unmask_idx_out| and |unmask_idx_in|. This causes some issues, as
// can be seen on the following toy example (where "a" and "b"
// represent |unmask_idx_out| and |unmask_idx_in|):
//
//                       a,b
//                    /      \
//                  /          \
//                /              \
//           a+1,b                 a,b+1
//          /    \                 /    \
//         /      \               /      \
//        /        \             /        \
//     a+2,b    a+1,b+1        a+1,b+1    a,b+1
//
// As you can see, we end up twice on "a+1,b+1", and going further
// would result in even more nodes being reached multiple times. To
// prevent this from happening, we forbid left-then-right recursion,
// ie, if we recursed on the left (ie, incremented "a"), then the next
// recursion cannot be to the right (ie, it cannot increment "b"). In
// our case, we take "left" recursion as being on |unmask_idx_out| and
// "right" recursion as being on |unmask_idx_in|. If |out_rec| is
// true, then the previous recursion was on |unmask_idx_out|, and the
// next one cannot be on |unmask_idx_in|. Note that this only stands
// for recursive calls that "do nothing", ie, not for recursive calls
// that extend the main tuple (in the code, that's where "!out_rec &&"
// is used).
//
//
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
//  |gauss_deps_o|: The tuple after gauss elimination on output
//      randoms. However, not that the dependencies are kept in their
//      unmasked form, in order to update the Gauss elimination when
//      adding variables to the tuple.
//
//  |gauss_rands_o|: The indices of the randoms used for the Gauss
//      elimination on output randoms. When an element of the tuple
//      contains no random, 0 is put in |gauss_rands_o|.
//
//  |gauss_deps_i|: Same as |gauss_deps_o|, but for Gauss elimination
//      on the input of interest after factorization.
//
//  |gauss_rands_i|: Same as |gauss_rands_o| but for |gauss_deps_i|.
//
//  |secret_idx|: The index of the secret that we are trying to reveal.
//
//  |unmask_idx_out|: the current index for the recursion on
//      |gauss_deps_o|. It corresponds to the element that we want to
//      unmask at the current stage of the recursion.
//
//  |unmask_idx_in|: same as |unmask_idx_out|, but for |gauss_deps_i|.
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
//  |available_shares|: the secret shares that are in
//      |gauss_deps_i|. If |available_shares| is not equal to
//      "all_shares_mask", then there is no need to recurse on
//      |unmask_idx_in| yet: we only need to keep recursing on
//      |unmask_idx_out| until at least one of each secret share is
//      in |gauss_deps_i|.
//
//  |out_rec|: see above
//
//  |debug|: if true, then some debuging information are printed.
//
static void randoms_step(const Circuit* c,
                         Trie* incompr_tuples,
                         int max_size,
                         const VarVector** randoms,
                         Dependency** gauss_deps_o,
                         Dependency* gauss_rands_o,
                         int gauss_length_o,
                         Dependency** gauss_deps_i,
                         Dependency* gauss_rands_i,
                         int gauss_length_i,
                         int secret_idx,
                         int unmask_idx_out,
                         int unmask_idx_in,
                         Tuple* curr_tuple,
                         int revealed_secret,
                         int available_shares,
                         bool out_rec,
                         int debug) {
  int deps_size           = c->deps->deps_size;
  int first_rand_idx      = c->deps->first_rand_idx;
  int non_mult_deps_count = deps_size - c->deps->mult_deps->length;

  //bool* input_rands = secret_idx == 0 ? c->i1_rands : c->i2_rands;

  if (curr_tuple->length > max_size) return;

  if (debug) {
    printf("randoms_step: [ ");
    for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]);
    printf("] (unmask_idx_out=%d -- unmask_idx_in=%d)\n", unmask_idx_out, unmask_idx_in);

    printf(" Gauss deps main:\n");
    for (int i = 0; i < curr_tuple->length; i++) {
      printf("   [ ");
      for (int j = 0; j < deps_size; j++) printf("%d ", gauss_deps_o[i][j]);
      printf("] -- mask: %d\n", gauss_rands_o[i]);
    }
    printf(" Gauss deps inner:\n");
    for (int i = 0; i < gauss_length_i; i++) {
      printf("   [ ");
      for (int j = 0; j < non_mult_deps_count; j++) printf("%d ", gauss_deps_i[i][j]);
      printf("] -- mask: %d\n", gauss_rands_i[i]);
    }
  }

  if (revealed_secret == c->all_shares_mask) {
    tot_adds++;
    if (! tuple_is_not_incompr(incompr_tuples, curr_tuple)) {
      add_tuple_to_trie(incompr_tuples, curr_tuple, c, secret_idx, revealed_secret);
    }
    return;
  }

  if (curr_tuple->length == max_size) {
    // Tuple has maximal size and is not a failure -> abandoning this path.
    return;
  }

  // Skipping the current element (once in "out" and once in "in")
  if (unmask_idx_out < gauss_length_o-1 && available_shares != c->all_shares_mask) {
    // Finding out the index of the next potential element of |curr_tuple| to unmask.
    int next_unmask_idx_out = unmask_idx_out+1;
    // INVALID; see begining of the file
    /* while (next_unmask_idx_out < curr_tuple->length) { */
    /*   if (gauss_rands_o[next_unmask_idx_out] == 0) { */
    /*     next_unmask_idx_out++; */
    /*   } else { */
    /*     // Making sure that the random at index |next_unmask_idx_out| */
    /*     // actually needs to be unmasked. */
    /*     int this_secret = get_contained_secret(c, gauss_deps_o[next_unmask_idx_out], secret_idx); */
    /*     if ((this_secret & revealed_secret) == this_secret) { */
    /*       next_unmask_idx_out++; */
    /*     } else { */
    /*       break; */
    /*     } */
    /*   } */
    /* } */
    /* int this_secret = get_contained_secret(c, gauss_deps_o[unmask_idx_out], secret_idx); */
    /* int secret_is_somewhere_else = 0; */
    /* for (int i = unmask_idx_out+1; i < curr_tuple->length-1; i++) { */
    /*   int that_secret = get_contained_secret(c, gauss_deps_o[i], secret_idx); */
    /*   if ((that_secret & this_secret) != 0) { */
    /*     secret_is_somewhere_else = 1; */
    /*     break; */
    /*   } */
    /* } */
    /* if (secret_is_somewhere_else) { */
      randoms_step(c, incompr_tuples, max_size, randoms,
                   gauss_deps_o, gauss_rands_o, gauss_length_o,
                   gauss_deps_i, gauss_rands_i, gauss_length_i,
                   secret_idx,
                   next_unmask_idx_out, unmask_idx_in, curr_tuple,
                   revealed_secret, available_shares, true, debug);
    /* } */
  }
  if (!out_rec && unmask_idx_in < gauss_length_i-1 && available_shares == c->all_shares_mask
      && c->has_input_rands) {
    int next_unmask_idx_in = unmask_idx_in+1;
    // INVALID; see begining of the file
    /* while (next_unmask_idx_in < gauss_length_i && */
    /*        (gauss_rands_i[next_unmask_idx_in] == 0 || */
    /*         (gauss_deps_i[next_unmask_idx_in][secret_idx] & revealed_secret) */
    /*         == gauss_deps_i[next_unmask_idx_in][secret_idx])) { */
    /*   next_unmask_idx_in++; */
    /* } */
    // INVALID as well, pretty much for the same reason I think
    /* int secret_is_somewhere_else = 0; */
    /* for (int i = 0; i < gauss_length_i; i++) { */
    /*   if (i == unmask_idx_in) continue; */
    /*   if ((gauss_deps_i[i][secret_idx] & gauss_deps_i[unmask_idx_in][secret_idx]) != 0) { */
    /*     secret_is_somewhere_else = 1; */
    /*     break; */
    /*   } */
    /* } */
    /* if (secret_is_somewhere_else) { */
      randoms_step(c, incompr_tuples, max_size, randoms,
                   gauss_deps_o, gauss_rands_o, gauss_length_o,
                   gauss_deps_i, gauss_rands_i, gauss_length_i,
                   secret_idx,
                   unmask_idx_out, next_unmask_idx_in, curr_tuple,
                   revealed_secret, available_shares, false, debug);
    /* } */
  }


  if (unmask_idx_out < gauss_length_o && available_shares != c->all_shares_mask) {
    // Unmasking on output random (based on |unmask_idx_out|)

    /* if (gauss_rands_o[unmask_idx_out] == 0) goto unmask_input_rand; */
    /* int rand_idx = gauss_rands_o[unmask_idx_out]; */

    // Finding which random to unmask
    int rand_idx = 0;
    /* printf(" Unmasking main: about to chose random for index %d\n", unmask_idx_out); */
    while (rand_idx == 0 && unmask_idx_out < gauss_length_o) {
      /* secret_to_unmask = get_contained_secret(c, gauss_deps_o[unmask_idx_out], secret_idx); */
      /* if ((revealed_secret & secret_to_unmask) == secret_to_unmask) { */
      /*   // The secret in this element does not need to be unmasked */
      /*   /\* printf(" Unasmking main: element %d reveals nothing. Moving on.\n", unmask_idx_out); *\/ */
      /*   unmask_idx_out++; */
      /*   continue; */
      /* } */
      rand_idx = gauss_rands_o[unmask_idx_out];
      if (rand_idx == 0) unmask_idx_out++;
    }

    if (debug) {
        printf("(recall 1) randoms_step: [ ");
        for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]);
        printf("] (unmask_idx_out=%d -- unmask_idx_in=%d)\n", unmask_idx_out, unmask_idx_in);

        printf(" Gauss deps main:\n");
        for (int i = 0; i < curr_tuple->length; i++) {
          printf("   [ ");
          for (int j = 0; j < deps_size; j++) printf("%d ", gauss_deps_o[i][j]);
          printf("] -- mask: %d\n", gauss_rands_o[i]);
        }
        printf(" Unmasking main: selecting random %d (btw, now index=%d)\n", rand_idx, unmask_idx_out);
      }

    if (unmask_idx_out >= gauss_length_o || rand_idx == 0) {
      // No more output randoms to unmask:
      //   * all output randoms have been unmasked
      //   * or, the secrets masked by output randoms are already revealed
      // If we are here though, the tuple is not a failure yet, which
      // means that we still need to continue to unmask the input
      // randoms.
      // TODO: this should be a return, right?
      goto unmask_input_rand;
    }

    const VarVector* dep_array = randoms[rand_idx];
    for (int j = 0; j < dep_array->length; j++) {
      Var dep = dep_array->content[j];
      if (Tuple_contains(curr_tuple, dep)) continue;


      if (debug) {
        printf("(recall 2) randoms_step: [ ");
        for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]);
        printf("] (unmask_idx_out=%d -- unmask_idx_in=%d)\n", unmask_idx_out, unmask_idx_in);

        printf(" Gauss deps main:\n");
        for (int i = 0; i < curr_tuple->length; i++) {
          printf("   [ ");
          for (int j = 0; j < deps_size; j++) printf("%d ", gauss_deps_o[i][j]);
          printf("] -- mask: %d\n", gauss_rands_o[i]);
        }
        printf(" Gauss deps inner:\n");
        for (int i = 0; i < gauss_length_i; i++) {
          printf("   [ ");
          for (int j = 0; j < non_mult_deps_count; j++) printf("%d ", gauss_deps_i[i][j]);
          printf("] -- mask: %d\n", gauss_rands_i[i]);
        }

        printf(" Unmasking main random %d\n", rand_idx);
      }
      if (debug) {
        printf(" Unmasking main: about to add %d (tuple length=%d; unmask_idx_out=%d; unmask_idx_in=%d\n",
               dep, curr_tuple->length, unmask_idx_out, unmask_idx_in);
      }

      // TODO: checking if |dep| does not have exactly the same
      // multiplications as the element it's supposed to unmask: if
      // so, they will cancel out and |dep| will not unmask that
      // element. For instance, if we want to unmask "r ^ a&b",
      // chosing "r ^ a&b ^ r2" would achieve nothing since the Gauss
      // elimination would cancel both "r" and "a&b".
      // TODO: reimplement this
      /* int have_same_mults = 1; */
      /* for (int i = non_mult_deps_count; i < deps_size; i++) { */
      /*   if (c->deps->deps[dep][i] != gauss_deps_o[unmask_idx_out][i]) { */
      /*     have_same_mults = 0; */
      /*     break; */
      /*   } */
      /* } */
      /* if (have_same_mults) continue; */

      Tuple_push(curr_tuple, dep);

      int new_gauss_length_o = gauss_length_o;
      DepArrVector* dep_arr = c->deps->deps[dep];
      for (int dep_idx = 0; dep_idx < dep_arr->length; dep_idx++) {
        apply_gauss(c->deps->deps_size, dep_arr->content[dep_idx],
                    gauss_deps_o, gauss_rands_o, new_gauss_length_o);
        int first_rand = get_first_rand_mult(gauss_deps_o[new_gauss_length_o],
                                             non_mult_deps_count,
                                             first_rand_idx, c->out_rands);
        gauss_rands_o[new_gauss_length_o] = first_rand;
        new_gauss_length_o++;
      }

      // Factorize new dependency
      Dependency** deps1 = secret_idx == 0 ? &gauss_deps_i[gauss_length_i] : NULL;
      Dependency** deps2 = secret_idx == 0 ? NULL : &gauss_deps_i[gauss_length_i];
      int deps_length = 0;
      int* deps1_length = secret_idx == 0 ? &deps_length : NULL;
      int* deps2_length = secret_idx == 0 ? NULL : &deps_length;
      factorize_mults(c, &gauss_deps_o[gauss_length_o], deps1, deps2,
                      deps1_length, deps2_length, new_gauss_length_o - gauss_length_o);

      // Gauss elimination on new factorized deps
      for (int k = 0; k < deps_length; k++) {
        apply_gauss(c->deps->deps_size, gauss_deps_i[gauss_length_i + k],
                    gauss_deps_i, gauss_rands_i, gauss_length_i + k);
        gauss_rands_i[gauss_length_i+k] =
          get_first_rand_mult(gauss_deps_i[gauss_length_i+k],
                              non_mult_deps_count, first_rand_idx, NULL);
      }

      if (debug) {
        printf("Factorization added %d deps\n", deps_length);

        printf("  After adding %d: [ ", dep);
        for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]);
        printf("] (unmask_idx_out=%d -- unmask_idx_in=%d)\n", unmask_idx_out, unmask_idx_in);

        printf(" Gauss deps main:\n");
        for (int i = 0; i < curr_tuple->length; i++) {
          printf("   [ ");
          for (int j = 0; j < deps_size; j++) printf("%d ", gauss_deps_o[i][j]);
          printf("] -- mask: %d\n", gauss_rands_o[i]);
        }
        printf(" Gauss deps inner:\n");
        for (int i = 0; i < gauss_length_i+deps_length; i++) {
          printf("   [ ");
          for (int j = 0; j < non_mult_deps_count; j++) printf("%d ", gauss_deps_i[i][j]);
          printf("] -- mask: %d\n", gauss_rands_i[i]);
        }

        printf(" Unmasking main random %d\n", rand_idx);
      }


      int additional_revealed_secret =
        get_initial_revealed_secret(c, deps_length, &gauss_deps_i[gauss_length_i], secret_idx);
      int new_revealed_secret = revealed_secret | additional_revealed_secret;

      int new_available_shares = available_shares |
        get_initial_shares(deps_length, &gauss_deps_i[gauss_length_i], secret_idx);

      // Computing new unmask_idx_out
      int new_unmask_idx_out = unmask_idx_out + 1;
      // INVALID; see begining of the file
      /* while (new_unmask_idx_out < curr_tuple->length) { */
      /*   if (gauss_rands_o[new_unmask_idx_out] == 0) { */
      /*     new_unmask_idx_out++; */
      /*   } else { */
      /*     // Making sure that the random at index |new_unmask_idx_out| */
      /*     // actually needs to be unmasked. */
      /*     int this_secret = get_contained_secret(c, gauss_deps_o[new_unmask_idx_out], secret_idx); */
      /*     if ((this_secret & new_revealed_secret) == this_secret) { */
      /*       new_unmask_idx_out++; */
      /*     } else { */
      /*       break; */
      /*     } */
      /*   } */
      /* } */

      // Computing new unmask_idx_in: since we might have revealed
      // some new secret shares, it's possible that the current share
      // at |unmask_idx_in| does not need to be unmasked.
      int new_unmask_idx_in = unmask_idx_in;
      // INVALID; see begining of the file
      /* while (new_unmask_idx_in < gauss_length_i+deps_length-1 && */
      /*        (gauss_rands_i[new_unmask_idx_in] == 0 || */
      /*         (gauss_deps_i[new_unmask_idx_in][secret_idx] & new_revealed_secret) */
      /*           == gauss_deps_i[new_unmask_idx_in][secret_idx])) { */
      /*   new_unmask_idx_in++; */
      /* } */

      if (debug) {
        printf(" And now, new_unmask_idx_in=%d (old unmask_idx_in=%d, additional_revealed_secret=0x%x, new_revealed_secret=0x%x)\n", new_unmask_idx_in, unmask_idx_in, additional_revealed_secret, new_revealed_secret);
      }

      // Recursing
      if (debug) {
        printf("Calling randoms_step with unmask_idx_out=%d and unmask_idx_in=%d\n",
               new_unmask_idx_out, new_unmask_idx_in);
      }
      randoms_step(c, incompr_tuples, max_size, randoms,
                   gauss_deps_o, gauss_rands_o, new_gauss_length_o,
                   gauss_deps_i, gauss_rands_i, gauss_length_i+deps_length,
                   secret_idx,
                   new_unmask_idx_out, new_unmask_idx_in, curr_tuple,
                   new_revealed_secret, new_available_shares, false, debug);


      Tuple_pop(curr_tuple);
    }
    // TODO: find why/if this return makes sense. It improves
    // performance and yields seemingly correct results, but I'm not
    // sure it's actually correct.
    return;
  }


 unmask_input_rand:
  if (unmask_idx_in < gauss_length_i && available_shares == c->all_shares_mask
      && c->has_input_rands) {
      // Unmasking an input random (based on |unmask_idx_in|)
    int secret_to_unmask = 0;

    int rand_idx = 0;

    if (debug) {
      printf(" Unmasking inner: about to chose random for index %d\n", unmask_idx_in);
    }
    while (rand_idx == 0 && unmask_idx_in < gauss_length_i) {
      secret_to_unmask = gauss_deps_i[unmask_idx_in][secret_idx];
      /* if ((revealed_secret & secret_to_unmask) != secret_to_unmask) { */
      /*   rand_idx = gauss_rands_i[unmask_idx_in]; */
      /* } */
      rand_idx = gauss_rands_i[unmask_idx_in];
      unmask_idx_in++;
    }
    if (debug) {
      printf(" Unmasking inner: chose random for index %d: %d\n", unmask_idx_in-1, rand_idx);
    }

    if (rand_idx == 0) {
      // No random found _and_ end of tuple reached -> we're done.
      return;
    }

    const VarVector* dep_array = randoms[rand_idx];
    // TODO: factorize this loop: it's almost the same as above...
    for (int j = 0; j < dep_array->length; j++) {
      Var dep = dep_array->content[j];
      if (Tuple_contains(curr_tuple, dep)) continue;
      if (debug) {
        printf("  Unmasking inner: trying with dep %d (to unmask dep %d, rand %d)\n",
               dep, unmask_idx_in-1, rand_idx);
      }

      // TODO: re-add ?
      /* int this_secret = get_contained_secret(c, c->deps->deps[dep], secret_idx); */
      /* if (this_secret == secret_to_unmask) { */
      /*   if (debug) { */
      /*     printf("   -> this_secret == secret_to_unmask ---> Moving to the next elem.\n"); */
      /*   } */
      /*   //continue; */
      /* } */

      if (debug) {
        printf(" Unmasking inner with %d:\n", rand_idx);
        printf("    Before elim, main deps:\n");
        for (int i = 0; i < curr_tuple->length; i++) {
          printf("      [ ");
          for (int j = 0; j < deps_size; j++) printf("%d ", gauss_deps_o[i][j]);
          printf("] -- mask: %d\n", gauss_rands_o[i]);
        }
        printf("    Before elim, inner deps:\n");
        for (int i = 0; i < gauss_length_i; i++) {
          printf("      [ ");
          for (int j = 0; j < non_mult_deps_count; j++) printf("%d ", gauss_deps_i[i][j]);
          printf("] -- mask: %d\n", gauss_rands_i[i]);
        }
      }

      Tuple_push(curr_tuple, dep);

      int new_gauss_length_o = gauss_length_o;
      DepArrVector* dep_arr = c->deps->deps[dep];
      for (int dep_idx = 0; dep_idx < dep_arr->length; dep_idx++) {
        apply_gauss(c->deps->deps_size, dep_arr->content[dep_idx],
                    gauss_deps_o, gauss_rands_o, new_gauss_length_o);
        int first_rand = get_first_rand_mult(gauss_deps_o[new_gauss_length_o],
                                             non_mult_deps_count,
                                             first_rand_idx, c->out_rands);
        gauss_rands_o[new_gauss_length_o] = first_rand;
        new_gauss_length_o++;
      }

      // Note: the test bellow is not correct: it would only make
      // sense after the second gauss elimination (on the input
      // randoms)
      /* int this_secret_after_gauss = */
      /*   get_contained_secret(c, gauss_deps_o[gauss_length_o], secret_idx); */
      /* if (this_secret_after_gauss == 0) { */
      /*   // TODO: use "& secret_to_unmask != secret_to_unmask" rather than "== 0" ? */
      /*   continue; */
      /* } */

      // Factorize new dependency
      Dependency** deps1 = secret_idx == 0 ? &gauss_deps_i[gauss_length_i] : NULL;
      Dependency** deps2 = secret_idx == 0 ? NULL : &gauss_deps_i[gauss_length_i];
      int deps_length = 0;
      int* deps1_length = secret_idx == 0 ? &deps_length : NULL;
      int* deps2_length = secret_idx == 0 ? NULL : &deps_length;

      factorize_mults(c, &gauss_deps_o[gauss_length_o], deps1, deps2,
                      deps1_length, deps2_length,
                      new_gauss_length_o - gauss_length_o);

      // Gauss elimination on new factorized deps
      for (int k = 0; k < deps_length; k++) {
        apply_gauss(c->deps->deps_size, gauss_deps_i[gauss_length_i + k],
                    gauss_deps_i, gauss_rands_i, gauss_length_i + k);
        gauss_rands_i[gauss_length_i+k] =
          get_first_rand_mult(gauss_deps_i[gauss_length_i+k],
                              non_mult_deps_count, first_rand_idx, NULL);
      }
      int additional_revealed_secret =
        get_initial_revealed_secret(c, deps_length, &gauss_deps_i[gauss_length_i], secret_idx);
      int new_revealed_secret = revealed_secret | additional_revealed_secret;

      int new_available_shares = available_shares |
        get_initial_shares(deps_length, &gauss_deps_i[gauss_length_i], secret_idx);

      // Computing new unmask_idx_out
      int new_unmask_idx_out = unmask_idx_out;
      // INVALID; see begining of the file
      /* while (new_unmask_idx_out < curr_tuple->length) { */
      /*   if (gauss_rands_o[new_unmask_idx_out] == 0) { */
      /*     new_unmask_idx_out++; */
      /*   } else { */
      /*     // Making sure that the random at index |new_unmask_idx_out| */
      /*     // actually needs to be unmasked. */
      /*     int this_secret = get_contained_secret(c, gauss_deps_o[new_unmask_idx_out], secret_idx); */
      /*     if ((this_secret & new_revealed_secret) == this_secret) { */
      /*       new_unmask_idx_out++; */
      /*     } else { */
      /*       break; */
      /*     } */
      /*   } */
      /* } */

      // Computing new unmask_idx_in
      int new_unmask_idx_in = unmask_idx_in;
      // INVALID; see begining of the file
      /* while (new_unmask_idx_in < gauss_length_i+deps_length && */
      /*        (gauss_rands_i[new_unmask_idx_in] == 0 || */
      /*         (gauss_deps_i[new_unmask_idx_in][secret_idx] & revealed_secret) */
      /*            == gauss_deps_i[new_unmask_idx_in][secret_idx])) { */
      /*   new_unmask_idx_in++; */
      /* } */

      // Recursing
      randoms_step(c, incompr_tuples, max_size, randoms,
                   gauss_deps_o, gauss_rands_o, new_gauss_length_o,
                   gauss_deps_i, gauss_rands_i, gauss_length_i+deps_length,
                   secret_idx,
                   new_unmask_idx_out, new_unmask_idx_in, curr_tuple,
                   new_revealed_secret, new_available_shares, false, debug);

      curr_tuple->length--;
    }
  }
}

static void initial_gauss_and_fact(const Circuit* c,
                                   Trie* incompr_tuples,
                                   int max_size,
                                   const VarVector** randoms,
                                   Dependency** gauss_deps_o,
                                   Dependency* gauss_rands_o,
                                   Dependency** gauss_deps_i,
                                   Dependency* gauss_rands_i,
                                   int secret_idx,
                                   Tuple* curr_tuple,
                                   int debug) {
  const DependencyList* deps = c->deps;
  int deps_size = deps->deps_size;
  int non_mult_deps_count = deps_size - deps->mult_deps->length;
  int first_rand_idx = c->deps->first_rand_idx;

  Comb nat_comb[deps_size];
  for (int i = 0; i < deps_size; i++) nat_comb[i] = i;


  // Gauss elimination on output randoms
  int gauss_length_o = 0;
  for (int i = 0; i < curr_tuple->length; i++) {
    DepArrVector* dep_arr = deps->deps[curr_tuple->content[i]];
    for (int dep_idx = 0; dep_idx < dep_arr->length; dep_idx++) {
      Dependency* real_dep = dep_arr->content[dep_idx];
      apply_gauss(deps_size, real_dep, gauss_deps_o, gauss_rands_o, gauss_length_o);
      gauss_rands_o[gauss_length_o] =
        get_first_rand_mult(gauss_deps_o[gauss_length_o], non_mult_deps_count,
                            first_rand_idx, c->out_rands);
      gauss_length_o++;
    }
  }

  // Extracting "clean" dependencies from |gauss_deps_o| and
  // |gauss_rands_o| in order to do the factorization (|gauss_deps_o|
  // does not only contain randoms in order to enable further
  // elimination, but it should, at least for the factorization)
  Dependency* local_deps[gauss_length_o];
  for (int i = 0; i < gauss_length_o; i++) {
    local_deps[i] = alloca(deps_size * sizeof(**local_deps));
    if (gauss_rands_o[i] != 0) {
      // TODO: we could skip this case altogether, right?
      memset(local_deps[i], 0, deps_size * sizeof(*local_deps[i]));
      local_deps[i][gauss_rands_o[i]] = 1;
    } else {
      memcpy(local_deps[i], gauss_deps_o[i], deps_size * sizeof(*local_deps[i]));
    }
  }

  // Factorization
  int deps12_length = gauss_length_o * c->deps->mult_deps->length * 2; // TODO: smaller?
  Dependency* deps_target[deps12_length];
  int deps_target_length = 0;
  for (int i = 0; i < deps12_length; i++) {
    deps_target[i] = alloca(deps_size * sizeof(**deps_target));
  }
  Dependency** deps1 = secret_idx == 0 ? deps_target : NULL;
  Dependency** deps2 = secret_idx == 0 ? NULL : deps_target;
  int* deps1_length = secret_idx == 0 ? &deps_target_length : NULL;
  int* deps2_length = secret_idx == 0 ? NULL : &deps_target_length;

  factorize_mults(c, local_deps, deps1, deps2,
                  deps1_length, deps2_length,
                  gauss_length_o);

  // Gauss elimination on input randoms
  for (int i = 0; i < deps_target_length; i++) {
    Dependency* real_dep = deps_target[i];
    apply_gauss(deps->deps_size, real_dep, gauss_deps_i, gauss_rands_i, i);
    gauss_rands_i[i] = get_first_rand_mult(gauss_deps_i[i], non_mult_deps_count,
                                           first_rand_idx, NULL);
  }

  int revealed_secret = get_initial_revealed_secret(c, deps_target_length,
                                                    gauss_deps_i, secret_idx);
  int available_shares = get_initial_shares(deps_target_length,
                                            gauss_deps_i, secret_idx);

  // Identifying the first element of |gauss_deps_o| and
  // |gauss_rands_o| that need to be unmasked.
  int unmask_idx_out = 0;
  while (unmask_idx_out < curr_tuple->length && gauss_rands_o[unmask_idx_out] == 0) {
    unmask_idx_out++;
  }
  int unmask_idx_in = 0;
  // TODO: is the stop condition in the while correct?!
  while (unmask_idx_in < deps_target_length &&
         (gauss_rands_i[unmask_idx_in] == 0 ||
          (gauss_deps_i[unmask_idx_in][secret_idx] & revealed_secret)
            == gauss_deps_i[unmask_idx_in][secret_idx])) {
    unmask_idx_in++;
  }

  randoms_step(c, incompr_tuples, max_size, randoms,
               gauss_deps_o, gauss_rands_o, gauss_length_o,
               gauss_deps_i, gauss_rands_i, deps_target_length,
               secret_idx, unmask_idx_out, unmask_idx_in,
               curr_tuple, revealed_secret, available_shares, false, debug);
}

static void secrets_step(const Circuit* c,
                         Trie* incompr_tuples,
                         int max_size,
                         const VarVector** secrets,
                         const VarVector** randoms,
                         Dependency** gauss_deps_o,
                         Dependency* gauss_rands_o,
                         Dependency** gauss_deps_i,
                         Dependency* gauss_rands_i,
                         int secrets_count,
                         int secret_idx,
                         Tuple* curr_tuple,
                         int debug) {
  if (secrets_count == -1 || curr_tuple->length == max_size) {
    if (debug) {
      printf("\n\nBase tuple: [ ");
      for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]);
      printf("] (size_max = %d)\n", max_size);
    }

    initial_gauss_and_fact(c, incompr_tuples, max_size, randoms,
                           gauss_deps_o, gauss_rands_o, gauss_deps_i, gauss_rands_i,
                           secret_idx, curr_tuple, debug);

  } else {
    const VarVector* dep_array = secrets[secrets_count];
    int tuple_idx = curr_tuple->length;
    for (int i = 0; i < dep_array->length; i++) {
      Var dep = dep_array->content[i];
      if (Tuple_contains(curr_tuple, dep)) {
        // This variable of the gadget contains multiple shares of the
        // same input. No need to add it multiple times to the tuples,
        // just recusring further.
        secrets_step(c, incompr_tuples, max_size, secrets, randoms,
                     gauss_deps_o, gauss_rands_o, gauss_deps_i, gauss_rands_i,
                     secrets_count-1, secret_idx, curr_tuple, debug);
      } else {
        curr_tuple->content[tuple_idx] = dep;
        curr_tuple->length++;
        secrets_step(c, incompr_tuples, max_size, secrets, randoms,
                     gauss_deps_o, gauss_rands_o, gauss_deps_i, gauss_rands_i,
                     secrets_count-1, secret_idx, curr_tuple, debug);
        curr_tuple->length--;
      }
    }
  }
}


static Trie* build_incompr_tuples(const Circuit* c,
                                  const VarVector** secrets,
                                  const VarVector** randoms,
                                  int coeff_max,
                                  int debug) {
  // TODO: compute more precisely what size is needed
  int max_deps_length = c->deps->length * 20;

  Tuple* curr_tuple = Tuple_make_size(c->deps->length);
  // |gauss_deps_o| and |gauss_rands_o|: for main Gauss eliminiation,
  // on Output randoms.
  Dependency** gauss_deps_o= malloc(max_deps_length * sizeof(*gauss_deps_o));
  for (int i = 0; i < max_deps_length; i++) {
    gauss_deps_o[i] = malloc(c->deps->deps_size * sizeof(*gauss_deps_o[i]));
  }
  Dependency* gauss_rands_o = malloc(max_deps_length * sizeof(*gauss_rands_o));
  // |gauss_deps_i| and |gauss_rands_i|: for secondary Gauss
  // elimination, on Input randoms.
  // TODO: what size for gauss_deps_i and gauss_rands_i ?!
  Dependency** gauss_deps_i= malloc(max_deps_length * 10 * sizeof(*gauss_deps_i));
  for (int i = 0; i < max_deps_length; i++) {
    gauss_deps_i[i] = malloc(c->deps->deps_size * sizeof(*gauss_deps_i[i]));
  }
  Dependency* gauss_rands_i = malloc(max_deps_length * 10 * sizeof(*gauss_rands_i));

  int share_count = c->share_count;
  // TODO: one trie per input?
  Trie* incompr_tuples = make_trie(c->deps->length);

  int refresh_i1 = 0, refresh_i2 = 0;
  int non_mult_deps_count = c->deps->deps_size - c->deps->mult_deps->length;
  for (int i = c->deps->first_rand_idx; i < non_mult_deps_count; i++) {
    refresh_i1 += c->i1_rands[i];
    refresh_i2 += c->i2_rands[i];
  }

  int max_incompr_size = c->share_count + c->random_count;
  max_incompr_size = coeff_max == -1 ? max_incompr_size :
    coeff_max > max_incompr_size ? max_incompr_size : coeff_max;
  //max_incompr_size = 8;
  for (int max_size = 1; max_size <= max_incompr_size; max_size++) {
    for (int i = 0; i < c->secret_count; i++) {
      // Randoms refreshing the other input cannot interfer with the
      // current one. The "real" maximal size is thus smaller than
      // |max_incompr_size|. Adjusting here with those two ifs.
      if (i == 0 && max_size > c->share_count + c->random_count - refresh_i2) continue;
      if (i == 1 && max_size > c->share_count + c->random_count - refresh_i1) continue;
      secrets_step(c, incompr_tuples, max_size, &secrets[share_count * i], randoms,
                   gauss_deps_o, gauss_rands_o, gauss_deps_i, gauss_rands_i,
                   share_count-1, i, curr_tuple, debug);
    }
    printf("Size %d: %d tuples\n", max_size, trie_tuples_size(incompr_tuples, max_size));
  }

  Tuple_free(curr_tuple);

  return incompr_tuples;
}

Trie* compute_incompr_tuples_mult(const Circuit* c, int coeff_max, int verbose) {
  // Uncomment the following 2 lines to use compositional approach (experimental).
  //constructive_v2(c, coeff_max, verbose);
  //exit(1);
  VarVector** secrets;
  VarVector** randoms;
  build_dependency_arrays_mult(c, &secrets, &randoms, false, verbose);
  /* for (int i = 0; i < c->secret_count; i++) { */
  /*   for (int j = 0; j < c->share_count; j++) { */
  /*     printf("Input %d - share %d: [ ", i, j); */
  /*     for (int k = 0; k < secrets[i*c->share_count+j]->length; k++) { */
  /*       int idx = secrets[i*c->share_count+j]->content[k]; */
  /*       printf("%d (%s), ", idx, c->deps->names[idx]); */
  /*     } */
  /*     printf("]\n"); */
  /*   } */
  /* } */

  Trie* incompr_tuples = build_incompr_tuples(c, (const VarVector**)secrets,
                                              (const VarVector**)randoms,
                                              coeff_max, verbose);

  /* printf("\n\nAll tuples:\n"); */
  /* print_all_tuples(incompr_tuples); */
  /* printf("\n"); */

  printf("\nTotal incompr: %d\n", trie_size(incompr_tuples));

  for (int size = c->share_count; size < 100; size++) {
    int tot = trie_tuples_size(incompr_tuples, size);
    printf("Incompr of size %d: %d\n", size, tot);
    if (tot == 0) break;
  }

  /* print_all_tuples_size(incompr_tuples, 3); */
  /* print_all_tuples_size(incompr_tuples, 4); */
  /* print_all_tuples(incompr_tuples); */

  printf("Generated %d tuples.\n\n", tot_adds);
  //return;

  // Generating failures from incompressible tuples, and computing coefficients.
  compute_failures_from_incompressibles(c, incompr_tuples, coeff_max, verbose);

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
#endif

Trie* compute_incompr_tuples_mult(const Circuit* c, int coeff_max, int verbose) {
  (void) c;
  (void) coeff_max;
  (void) verbose;
  return NULL;
}
