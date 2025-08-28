#include <gmp.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>   // For clock

#include "coeffs.h"
#include "constructive_arith.h"
#include "constructive-mult_arith.h"
#include "circuit.h"
#include "combinations.h"
#include "list_tuples.h"
#include "verification_rules.h"
#include "trie.h"
#include "failures_from_incompr.h"
#include "vectors.h"


#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) > (b) ? (b) : (a))


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
void build_dependency_arrays_arith(const Circuit* c,
                                   VarVector*** secrets,
                                   VarVector*** randoms,
                                   bool include_outputs,
                                   int verbose) {
  int total_secrets    = c->secret_count * c->share_count;
  int random_count     = c->random_count;
  DependencyList* deps = c->deps;
  int deps_size        = deps->deps_size;

  *secrets = malloc(total_secrets * sizeof(*secrets));
  for (int i = 0; i < total_secrets; i++) {
    (*secrets)[i] = VarVector_make();
  }
  
  *randoms = malloc((total_secrets + random_count) * sizeof(*randoms));
  for (int i = total_secrets; i < total_secrets + random_count; i++) {
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
        dep[j] |= (dep_arr->content[dep_idx][j] != 0);
      }
    }

    // Updating |secrets| and |randoms| based on |dep|
    for (int j = 0; j < total_secrets; j++) {
        if (dep[j]) {
          VarVector_push((*secrets)[j], i);
        }
    }
     
    for (int j = total_secrets; j < total_secrets + random_count; j++) {
      if (dep[j]) {
        VarVector_push((*randoms)[j], i);
      }
    }
  }

  if (verbose) {
    printf("***Arrays Construction***\n");
    for (int i = 0; i < total_secrets; i++) {
      printf("[%s] : ", deps->names[i]);
      for (int j = 0; j < (*secrets)[i]->length; j++){
        printf("%d, ", (*secrets)[i]->content[j]);
      }
      printf("\n");
    }
    
    for (int i = total_secrets; i < total_secrets + random_count; i++) {
      printf("[%s] : ", deps->names[i]);
      for (int j = 0; j < (*randoms)[i]->length; j++){
        printf("%d, ", (*randoms)[i]->content[j]);
      }
      printf("\n");
    }
    printf("\n");
  }
}

/*
Simple Euclidean Algorithm.
Input : 
-int x : An integer with value in [0, |q|-1]  
-int q  : A prime integer

Output : The inverse of |x| modulo |q| using Euclide Algorithm.  
*/
static int inverse_mod (int x, int q){
  int r0 = q;
  int r1 = x;
  
  int u0 = 1;
  int u1 = 0;
  int v0 = 0;
  int v1 = 1;
  
  while (r1 != 0) {
    int quo = r0 / r1;
    int remainder = r0 % r1;
    
    r0 = r1;
    r1 = remainder;
    
    int tmp = u0 - quo * u1;
    u0 = u1;
    u1 = tmp;
    
    tmp = v0 - quo * v1;
    v0 = v1;
    v1 = tmp;
   }
   return v0 < 0 ? v0 + q: v0;  
}

/*
static int inverse_mod(int x, int q){
  int a = x;
  int b = q;
  int u = 1;
  int v = 0;
  int inv_two_q = (q + 1) >> 1;

  while(a){
    if (a & 1){
      if (a < b){
        int tmp = a;
        a = (b - a) >> 1;  
        b = tmp;
        tmp = u;
        u = ((v - u) * inv_two_q) % q;
        v = tmp;
      }
      else {
        a = (a - b) >> 1;
        u = ((u - v) * inv_two_q) % q;
      }
    }
    else {
      a = a >> 1;
      u = (u * inv_two_q) % q;
    }
  }
  if (!(b == 1)){
    return 0;
  }
  return v;
}*/

/* Due to the time to convert intergers into mpz_t, it does not 
upgrade the performances. 
static int inverse_mod (int x, int q){
  mpz_t a, mod, inv;

  mpz_init_set_si(a, x);
  mpz_init_set_si(mod, q);
  mpz_init(inv);

  mpz_invert(inv, a, mod);
  
  return mpz_get_si(inv);
}*/



/* Tentative : Less efficient than Euclidean Algorithm.
//Simple Square and Multiply algorithm.
static int fast_exp(int base, int exp, int mod) {
    if (mod == 0) return 0; 
    if (mod == 1) return 0;
    int result = 1 % mod;
    base %= mod;

    while (exp) {
        if (exp & 1) result = (result * base) % mod; // multiply
        exp >>= 1;
        if (exp) base = (base * base) % mod;         // square
    }
    return result;
}


// Use the fact that x^{q - 1} = 1 mod q thanks to the Fermat Algorithm. 
//Then the inverse is x^{q - 2} that we compute thanks to fast_exp. 
static int inverse_mod (int x, int q){
  return fast_exp(x, q - 2, q);
}*/






/************************************************
             Building the tuples
*************************************************/

/*
Get the index of the first random we found in the dependencies.
Input :
  - Dependency *dep : Array of dependencies of a line in |gauss_deps|.
  - int deps_size : Size of the array |dep|.
  - int first_rand_idx : Index from which we found dependencies on randoms in 
                         |dep|.
Output : The index of the first random we found in the dependencies.
*/
int get_first_rand_arith(Dependency* dep, int deps_size, int first_rand_idx) {
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
// |idx| : Size of |gauss_deps|
//
// |characteristic| : The characteristic we are using in our computation.
//
// This function adds |real_dep| to |gauss_deps|, and performs a Gauss
// elimination on this element: all previous elements of |gauss_deps|
// have already been eliminated, and we xor them as needed with |real_dep|.
void apply_gauss_arith(int deps_size,
                 Dependency* real_dep,
                 Dependency** gauss_deps,
                 Dependency* gauss_rands,
                 int idx,
                 int characteristic) {               
  Dependency* dep_target = gauss_deps[idx];
  if (dep_target != real_dep) {
    memcpy(dep_target, real_dep, deps_size * sizeof(*dep_target));
  }
  
  for (int i = 0; i < idx; i++) {
    int r = gauss_rands[i];
    int coeff_rand = dep_target[r];
    
    if (r != 0 && coeff_rand) {
      int inverse_rands = inverse_mod(gauss_deps[i][r],characteristic);
      //if (inverse_rands < 0) inverse_rands += characteristic;  
      for (int j = 0; j < deps_size; j++) { 
        dep_target[j] -= ((((inverse_rands * coeff_rand) % characteristic) *  gauss_deps[i][j]) % characteristic); /* /!\ Overflow possible */
        if ((int)(dep_target[j]) < 0) dep_target[j] += characteristic;
      }
    }
  }
}

// Adds the tuple |curr_tuple| to the trie |incompr_tuples|.
void add_tuple_to_trie_arith(Trie* incompr_tuples, Tuple* curr_tuple,
                       const Circuit* c, int secret_idx, int revealed_secret) {
  SecretDep* secret_deps = calloc(c->secret_count, sizeof(*secret_deps));
  secret_deps[secret_idx] = revealed_secret;
  // Note that tuples in the trie are sorted (ie, the variables they
  // contain are in ascending order) -> we might need to sort the
  // current tuple.
  
  if (is_sorted_comb(curr_tuple->content, curr_tuple->length)) {
    insert_in_trie_merge_arith(incompr_tuples, curr_tuple->content, curr_tuple->length, secret_deps, c->secret_count);
  } else {
    Comb sorted_comb[curr_tuple->length];
    memcpy(sorted_comb, curr_tuple->content, curr_tuple->length * sizeof(*sorted_comb));
    sort_comb(sorted_comb, curr_tuple->length);
    
    insert_in_trie_merge_arith(incompr_tuples, sorted_comb, curr_tuple->length, secret_deps, c->secret_count);
  }
}

// Checks if |curr_tuple| is incompressible by checking if any of its
// subtuple is incompressible itself (ie, if any of its subtuples is
// in |incompr_tuples|): if any of its subtuple is incompressible,
// then |curr_tuple| is a simple failure, but not an incompressible one.
int tuple_is_not_incompr_arith(Trie* incompr_tuples, Tuple* curr_tuple) {
  if (is_sorted_comb(curr_tuple->content, curr_tuple->length)) {
    return trie_contains_subset(incompr_tuples, curr_tuple->content, curr_tuple->length) ? 1 : 0;
  } else {
    Comb sorted_comb[curr_tuple->length];
    memcpy(sorted_comb, curr_tuple->content, curr_tuple->length * sizeof(*sorted_comb));
    sort_comb(sorted_comb, curr_tuple->length);
    return trie_contains_subset(incompr_tuples, sorted_comb, curr_tuple->length) ? 1 : 0;
  }
}


//Check if tuple |curr_tuple| is in the trie |incompr_tuples|.
int tuple_is_in_incompr_tuples(Trie* incompr_tuples, Tuple* curr_tuple) {
  if (is_sorted_comb(curr_tuple->content, curr_tuple->length)) {
    return is_in_trie(incompr_tuples, curr_tuple->content, curr_tuple->length) ? 1 : 0;
  } else {
    Comb sorted_comb[curr_tuple->length];
    memcpy(sorted_comb, curr_tuple->content, curr_tuple->length * sizeof(*sorted_comb));
    sort_comb(sorted_comb, curr_tuple->length);
    return is_in_trie(incompr_tuples, sorted_comb, curr_tuple->length) ? 1 : 0;
  }
}

//During the recursion in the function randoms_step_arith, one of the stop condition 
//is when the tuple we build is of maximal size. In fact, we don't want to 
//evaluate tuples of size bigger than the maximal size. Nevertheless, in some  
//property, the output probes can be added in tuples and doesn't count as 
//a probes of the tuples if the tuples is a failure. So, even if the tuples is 
//of maximum size, we can still add probes that are outputs in RPE or RPC for 
//example. That's why we have to introduce a boolean |fill_only_with_output| in 
//randoms_step_arith.

// Parameters:
//
//  |c|: the circuit
//
//  |t_in| : The number of shares of the secret |curr_tuple| has to leak to be 
//      considered as a failure tuple.
//
//  |include_outputs| : Boolean to know if we have to include the outputs index 
//      in the tuple |curr_tuple|.
//
//  |required_outputs_remaining| : Useful only if |include_outputs| is true, it 
//      indicates the number of outputs we can had in |curr_tuple| at most.
//
//  |required_outputs_remaining_I2| : Useful only if |include_outputs| is true 
//                                    and the gadget is a copy gadget. In some 
//                                    case, we have to differentiate the outputs 
//                                    index that contains shares of the first 
//                                    output or of the second output (in RPE2 
//                                    case for example). 
//
//  |incompr_tuples|: the trie of incompressible tuples (contain
//      already computed incompressible tuples, and new ones are added
//      in this trie as well.
//
//  |target_size|: the maximal size of tuples allowed. Nevertheless, in some 
//                 case we can add output index.
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
//  |gauss_length|: Number of line in the array |gauss_deps|, also the number of
//      probes in |curr_tuple|. 
//
//  |secret_idx|: The index of the secret that we are trying to reveal.
//
//  |unmask_idx|: the current index for the recursion on
//      |gauss_deps|. It corresponds to the element that we want to
//      unmask at the current stage of the recursion.
//
//  |curr_tuple|: the tuple that is being built.
//
//  |revealed_secret|: the secret that is currently being revealed by
//      |curr_tuple|. This is used to prune out of some branches of the
//      recursion: we never need to unmask an element whose secret
//      shares are already being revealed.
//
//  |fill_only_with_output|: A boolean that indicates whether we have to add 
//      probes that are outputs in |curr_tuple|. If you don't know why we add
//      this boolean, just see the comments above.
//
//  |mutex|: A mutex to lock when we add failures tuples in |incompr_tuples|, if
//      we are parallelizing the program.
//
//  |debug|: if true, then some debuging information are printed.
//
int tot_adds_arith = 0;
void randoms_step_arith(const Circuit* c,
                  int t_in,
                  bool include_outputs,
                  int required_outputs_remaining,
                  int required_outputs_remaining_I2,
                  Trie* incompr_tuples,
                  int target_size,
                  VarVector** randoms,
                  Dependency** gauss_deps,
                  Dependency* gauss_rands,
                  int gauss_length,
                  int secret_idx,
                  int unmask_idx,
                  Tuple* curr_tuple,
                  int revealed_secret,
                  bool fill_only_with_output,
                  pthread_mutex_t *mutex,
                  int debug) { 
  if (curr_tuple->length > target_size) return;
  if (include_outputs && required_outputs_remaining < 0) return;

  if (debug) {
    printf("randoms_step_arith(unmask_idx=%d). Tuple: [ ", unmask_idx);
    for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]);
    printf("]  -- unmask_idx = %d, target_size = %d, revealed_secret = %d," 
           "t_in = %d\n", unmask_idx, target_size, revealed_secret, t_in);
  }

  // Checking if secret is revealed
  if (__builtin_popcount(revealed_secret) >= t_in) {
    
    //Lock the mutex if we do a parallelization.
    if (mutex)
      pthread_mutex_lock(mutex);
        
    if (tuple_is_not_incompr_arith(incompr_tuples, curr_tuple)) {
      
      //Unlock
      if (mutex)
        pthread_mutex_unlock(mutex); 
      
      return;
    } 
    
    tot_adds_arith++;
    
    //The tuple is an incompressible failure tuple, we add it in incompr_tuples.
    add_tuple_to_trie_arith(incompr_tuples, curr_tuple, c, secret_idx,
                            revealed_secret);
    //Unlock
    if (mutex)
      pthread_mutex_unlock(mutex);
    
    return;
  }

  // Checking if reached the end of the tuple.
  if (unmask_idx == gauss_length) {
    return;
  }

  // Checking if |target_size| is reached.
  if (curr_tuple->length == target_size) {
    
    //Checking if we can add outputs (see comments above on 
    //fill_only_with_output.
    if (include_outputs)
      fill_only_with_output = true;
    
    else 
      return;
  }

  // Checking if the element at index |unmask_idx| needs to be
  // unmasked. If not, skipping to the next element.
  // TODO: this recursion step is probably wrong. See comment at the
  // top of constructive-mult.c
  /* int secret_to_unmask = gauss_deps[unmask_idx][secret_idx]; */
  /* if ((revealed_secret & secret_to_unmask) == secret_to_unmask) { */
  /*   randoms_step_arith(c, incompr_tuples, target_size, randoms, */
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
  randoms_step_arith(c, t_in, include_outputs, required_outputs_remaining, 
               required_outputs_remaining_I2, incompr_tuples, target_size, 
               randoms, gauss_deps, gauss_rands, gauss_length, secret_idx, 
               unmask_idx+1, curr_tuple, revealed_secret, fill_only_with_output,
               mutex, debug);
  /* } */

  int curr_size = curr_tuple->length;
  int rand_idx = gauss_rands[unmask_idx];
 
  if (rand_idx == 0) {
    // if |secret_is_somewhere_else| is true, then this recursive call
    // has already been performed.
    // TODO: uncomment if using the "secret_is_somewhere_else" opti
    /* if (!secret_is_somewhere_else) { */
    /*   randoms_step_arith(c, t_in, include_outputs, required_outputs_remaining, */
    /*                incompr_tuples, target_size, randoms, */
    /*                gauss_deps, gauss_rands, gauss_length, secret_idx, */
    /*                unmask_idx+1, curr_tuple, revealed_secret, debug); */
    /* } */
    return;
  } else {
    VarVector* dep_array = randoms[rand_idx];
    for (int j = 0; j < dep_array->length; j++) {
      Var dep = dep_array->content[j];
      
      if (Tuple_contains(curr_tuple, dep)) {
        // Do not add an element that is already in the tuple
        continue;
      }
                  
      if (fill_only_with_output && dep < c->length){
        //Do not add an internal probe if we can just fill it with output 
        //probes.
        continue;
      }
     
      
      int new_required_outputs_remaining = required_outputs_remaining;
      int new_required_outputs_remaining_I2 = required_outputs_remaining_I2;
      int new_target_size = target_size;
      if (dep >= c->length) {
        //If it's an output probes that we will add, we have to reduce 
        //the number of outputs we can add after this addition, and we have to 
        //increase the size of the tuples because outputs probes doesn't count
        //for a probe in final.
        if (new_required_outputs_remaining_I2 == -1){
          if (new_required_outputs_remaining <=0) continue;
            new_required_outputs_remaining--;
        }
        
        else {
          if (dep < c->length + c->share_count){ 
            if (new_required_outputs_remaining <=0) continue;
            new_required_outputs_remaining--;
          }
          else{
            if (new_required_outputs_remaining_I2 <=0) continue;
            new_required_outputs_remaining_I2--;
          }
        }
        new_target_size++;
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
      
      //Add the probes n°dep and apply the reduction of Gauss to this probes.
      curr_tuple->content[curr_size] = dep;

      int new_gauss_length = gauss_length;
      const DepArrVector* dep_arr = c->deps->deps[dep];
      for (int dep_idx = 0; dep_idx < dep_arr->length; dep_idx++) {
        apply_gauss_arith(c->deps->deps_size, dep_arr->content[dep_idx],
                    gauss_deps, gauss_rands, new_gauss_length++,
                    c->characteristic);
      }
      
      int nb_shares = c->share_count;
      int start = secret_idx * nb_shares;
      int has_secret_share = 0;
      for (int l = gauss_length; l < new_gauss_length; l++) {
        for (int k = start; k < start + nb_shares; k++){
          if (gauss_deps[l][k]) {
            has_secret_share |= 1;
            break;
          }
        }
      }
      
      if (!has_secret_share) {
          // This element does not contain any secret share. It can
          // happen if when applying gauss elimination, some
          // expressions cancelled out the secret shares of the
          // current expression.
          continue;
      }

      //Compute the new revealed secret.
      int new_revealed_secret = revealed_secret;
      for (int l = gauss_length; l < new_gauss_length; l++) {
        int first_rand = get_first_rand_arith(gauss_deps[l], c->deps->deps_size,
                                        c->deps->first_rand_idx);
        gauss_rands[l] = first_rand;
        if (first_rand == 0) {
          for (int k = start; k < start + nb_shares; k++){ 
            if (gauss_deps[l][k])  
              new_revealed_secret |= 1 << (k - start);
          }
        }
      }
   
      curr_tuple->length++;
   
      randoms_step_arith(c, t_in, include_outputs, new_required_outputs_remaining,
                   new_required_outputs_remaining_I2, incompr_tuples, 
                   new_target_size, randoms, gauss_deps, gauss_rands, 
                   new_gauss_length, secret_idx, unmask_idx+1, curr_tuple, 
                   new_revealed_secret, fill_only_with_output, mutex, debug);
      curr_tuple->length--;
    }
  }
}


/*
Compute the revealed_secret from the dependencies who are in |gauss_deps|.
Input :
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int size : Number of line of |gauss_deps|
  -Dependency **gauss_deps : Array of dependencies of each probes containing in 
                             the tuple |curr_tuple| after Gaussian Elimination.
  -int secret_idx : Index of the secret that the current tuple should reveal.
Output : The share who leaks in base 2. Example : if the first(index = 0) and 
         the third (index = 2) shares leaks, then the revealed secret will be 
         2⁰ + 2² = 5.
*/
int get_initial_revealed_secret_arith(const Circuit* c, int size,
                                Dependency** gauss_deps,
                                int secret_idx) {
  int nb_shares = c->share_count;                              
  int revealed_secret = 0;
  for (int i = 0; i < size; i++) {
    int is_masked = 0;
    for (int j = c->deps->first_rand_idx; j < c->deps->deps_size; j++) {
      if (gauss_deps[i][j]) {
        //There is one random in the dependencies, and according to the Gaussian
        //elimination, this random is not in the previous probes.
        //In this case, this random mask the other dependencies and become the 
        //pivot for this probes. The line is masked. 
        is_masked = 1;
        break;
      }
    }
    if (!is_masked) {
      // The line is not masked, and we recover all the shares we have in the 
      // dependencies as information that we add to |revealed_secret|
      for (int j = 0; j < nb_shares; j++){
        if (gauss_deps[i][secret_idx * nb_shares +j])
          revealed_secret |= (1 << j);
      }
    }
  }
  return revealed_secret;
}

/*
Make the first Gaussian Elimination on all the probes contains in |curr_tuple|. 
Then go to the randoms step.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int t_in : The number of shares that must be leaked for a tuple to be a 
              failure.
  -bool include_outputs : if true, we can include outputs index in the tuples.
  -int required_outputs_remaining : Number of outputs required in each tuple.
  -int required_outputs__remaining_I2 : See the comments of the function 
                                        |randoms_step_arith| for this variable.
  -Trie *incompr_tuples : The trie in which we will add the incompressibles 
                          tuples. 
  -int target_size : The maximal size of the incompressible tuples 
                     we want to observe.
  -VarVector **randoms : Array 2D of integer. Each line contains an array of
                         index of probes that contains the line-th randoms.
  -Dependency **gauss_deps : Array of dependencies of each probes containing in 
                             the tuple |curr_tuple| after Gaussian Elimination.
  -Dependeny *gauss_rands : Array of random index. Each line has the value of 
                            the pivot random used for this probe.
  -int secret_idx : Index of the secret that the current tuple should reveal.                        
  -Tuple *curr_tuple : the tuple that is being built.                            
  -int debug : if true, then some debuging information are printed.
  -pthread_mutex_t : mutex useful in |randoms_step_arith|.
*/
void initial_gauss_elimination_arith(const Circuit* c,
                               int t_in,
                               bool include_outputs,
                               int required_outputs_remaining,
                               int required_outputs_remaining_I2,
                               Trie* incompr_tuples,
                               int target_size,
                               VarVector** randoms,
                               Dependency** gauss_deps,
                               Dependency* gauss_rands,
                               int *gauss_length,
                               int secret_idx,
                               Tuple* curr_tuple,
                               int debug,
                               pthread_mutex_t *mutex) {
  if (debug) {
      printf("\n\nBase tuple:[ ");
      for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]);
      printf("] (size_max = %d), gauss_length = %d\n", target_size, *gauss_length);
  }
    
  int characteristic = c->characteristic;                             
  DependencyList* deps = c->deps;
  
  //Fill the |gauss_deps| and |gauss_rands| array.
  for (int i = *gauss_length; i < curr_tuple->length; i++) {
    //Dependencies of the probes n°i of |curr_tuple| before Gauss Elimination.
    DepArrVector* dep_arr = deps->deps[curr_tuple->content[i]];
    for (int j = 0; j < dep_arr->length; j++) {
      Dependency* real_dep = dep_arr->content[j];
      //Gauss Eliminaton
      apply_gauss_arith(deps->deps_size, real_dep, gauss_deps, gauss_rands, *gauss_length, c->characteristic);
      //Set the random pivot on the line gauss_length
      gauss_rands[*gauss_length] = get_first_rand_arith(gauss_deps[*gauss_length], 
                                                        deps->deps_size, 
                                                        deps->first_rand_idx);
      if (gauss_rands[*gauss_length]) {
        int first_rand_coeff = gauss_deps[*gauss_length][gauss_rands[*gauss_length]];
        int inverse = inverse_mod(first_rand_coeff, characteristic); 
        for (int k = 0; k < deps->deps_size ; k++){
          gauss_deps[*gauss_length][k] = (int)((gauss_deps[*gauss_length][k] * inverse)) 
                                        % characteristic;
          if ((int)(gauss_deps[*gauss_length][k]) < 0) gauss_deps[*gauss_length][k] += characteristic;                                
        }                                           
      }                                          
      //Add the number of probes eliminated to |gauss_length|.
      (*gauss_length)++;
    }
  }
  
  int revealed_secret = get_initial_revealed_secret_arith(c, *gauss_length, gauss_deps, secret_idx);                     
  
  //|curr_tuple| ready to the randoms_step_arith.
  randoms_step_arith(c, t_in, include_outputs, required_outputs_remaining,
               required_outputs_remaining_I2, incompr_tuples, target_size, 
               randoms, gauss_deps, gauss_rands, *gauss_length, 
               secret_idx, 0, curr_tuple, revealed_secret, false, mutex, debug);
}

/*Optimization idea : Do "on-the-fly" Gaussian Elimination.
When we browse our tuple |curr_tuple|, we find some tuple good to eliminate, 
the goal of this optimization is to not repeat gaussian elimination on the same 
probes.

Example : 
We do the gaussian elimination for the tuple : [2,3,6].
After, we remove 6 of our tuple and add 7. Then, we want to perform gaussian 
Elimination on the tuple : [2,3,7]. Since we already do the elimination for 
[2,3,6], in particular, we know how to eliminate [2,3] and his result is kept in
|gauss_deps|. By keeping this in memory in |gauss_deps|, we just have to perform 
the elimination on 7, to perform the Gaussian Elimination on the tuple [2,3,7].

This is why we add the variable |gauss_length| in our tuple. |gauss_length| is 
the current number of probes from |curr_tuple| we have already in memory in 
|gauss_deps|. In this way, we don't have to perfom Gaussian elimination on the 
|gauss_length| first element of |gauss_deps|.
*/


/*
Found all the incompressibles tuples of size |target_size| (without counting the 
output index in the size of a tuple) and adds them in the trie |incompr_tuples|.
Input :
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int t_in : The number of shares that must be leaked for a tuple to be a 
              failure.
  -bool include_outputs : if true, we can include outputs index in the tuples.
  -int required_outputs_remaining : Number of outputs required in each tuple.
  -int required_outputs_remaining_I2 : See the comments of the function 
                                       randoms_step_arith for this variable.
  -Trie *incompr_tuples : The trie in which we will add the incompressibles 
                          tuples. 
  -int target_size : The maximal size of the incompressible tuples 
                     we want to observe.
  -VarVector **secrets : Array 2D of integer. Each line contains an array of 
                         index of probes that contains the line-th share we 
                         found in the |c->deps| array. For example, 
                         the first line contains the index of the probes who can
                         leak the first share of the first secret value.   
                          
  -VarVector **randoms : Array 2D of integer. Each line contains an array of
                         index of probes that contains the line-th randoms.
  -Dependency **gauss_deps : Array of dependencies of each probes containing in 
                             the tuple |curr_tuple| after Gaussian Elimination.
  -Dependeny *gauss_rands : Array of random index. Each line has the value of 
                            the pivot random used for this probe.
  -int* gauss_length : Size of the probes already eliminated in |gauss_deps| 
                       (for on-the-fly elimination purpose).
  -int next_secret_share_idx : Index of the next secret share to consider.
  -int selected_secret_shares_count : Number of secret shares already taken.
  -int secret_idx : Index of the secret that the current tuple should reveal.
  -Tuple *curr_tuple : the tuple that is being built.                            
  -bool RPC : If true, we are checking the RPC (or RPE1) property.
  -int debug : if true, then some debuging information are printed.
*/
void secrets_step_arith(const Circuit* c,
                  int t_in,
                  bool include_outputs,
                  int required_outputs_remaining,
                  int required_outputs_remaining_I2,
                  Trie* incompr_tuples,
                  int target_size,
                  VarVector** secrets,
                  VarVector** randoms,
                  Dependency** gauss_deps,
                  Dependency* gauss_rands,
                  int* gauss_length,
                  int next_secret_share_idx,
                  int selected_secret_shares_count,
                  int secret_idx,
                  Tuple* curr_tuple,
                  bool RPC,
                  int debug) {                             
                  
  //Stop condition : |curr_tuple| is at maximal size or we have enough secret 
  //                 shares to leak. 
  if (next_secret_share_idx == -1 || curr_tuple->length == target_size || selected_secret_shares_count == t_in) {
    // This case : We don't have enough secret shares to leak and we can't add more probes.
    if (selected_secret_shares_count != t_in){
      //We just have to check if we have check all the possible shares. Because 
      //if it's not the case, we don't have the good secret_counts. 
      //We know we can't have more probes to add, but we have to update 
      //secrets count.
      if (next_secret_share_idx != -1){
        VarVector* dep_array = secrets[next_secret_share_idx];
        bool already_in = false;
        for (int i = 0; i < dep_array->length; i++) {
          Comb dep_idx = dep_array->content[i];
          if (Tuple_contains(curr_tuple, dep_idx)) {
            if(already_in)
              continue;
            
            already_in = true;
            // This variable of the gadget contains multiple shares of the
            // same input. No need to add it multiple times to the tuples,
            // just recusring further.
            secrets_step_arith(c, t_in, include_outputs, required_outputs_remaining,
                         required_outputs_remaining_I2, incompr_tuples,
                         target_size, secrets, randoms,
                         gauss_deps, gauss_rands, gauss_length, 
                         next_secret_share_idx - 1, 
                         selected_secret_shares_count+1, secret_idx, curr_tuple,
                         RPC, debug);
          }
          else if (dep_idx >= c->length && RPC && required_outputs_remaining > 0){
            curr_tuple->content[curr_tuple->length] = dep_idx;
            curr_tuple->length++;
            //Probes added, Recursion
            secrets_step_arith(c, t_in, include_outputs, required_outputs_remaining - 1,
                         required_outputs_remaining_I2, incompr_tuples,
                         target_size + 1, secrets, randoms,
                         gauss_deps, gauss_rands, gauss_length,
                          next_secret_share_idx - 1, 
                         selected_secret_shares_count+1, secret_idx, curr_tuple,
                         RPC, debug);       
            curr_tuple->length--;
            /* Sometimes, some probes are add in |curr_tuple| and finally not 
               going to perform Gaussian Elimination. In this case, I don't add 
               these probes to |gauss_length|, it is the case where 
               |gauss_length| < length(|curr_tuple|).
               When there is an elimination, |gauss_length| is updated to the 
               length of |curr_tuple|, and when the tuple pop one element, 
               we have to remove it too, and affect the new length of 
               |curr_tuple| to |gauss_length| */
            *gauss_length = min(*gauss_length, curr_tuple->length);   
          } 
         
        }
        secrets_step_arith(c, t_in, include_outputs, required_outputs_remaining,
                     required_outputs_remaining_I2, incompr_tuples,
                     target_size, secrets, randoms,
                     gauss_deps, gauss_rands, gauss_length,
                     next_secret_share_idx - 1, 
                     selected_secret_shares_count, secret_idx, curr_tuple, RPC, 
                     debug);
      }
      //If we have checked all share index, skip this probes.
      return;
    }
    
    // This case is for the SNI property, if we have required_outputs remaining,
    // then the tuple doesn't have to be evaluate.
    if (!RPC && required_outputs_remaining > 0 && required_outputs_remaining_I2 == -1) return;
    
    if (debug) {
      printf("\n\nBase tuple:[ ");
      for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]);
      printf("] (size_max = %d)\n", target_size);
    }
    
    //The tuple has to be evaluate --->Gauss Elimination on the tuples.   
    initial_gauss_elimination_arith(c, t_in, include_outputs, 
                              required_outputs_remaining, 
                              required_outputs_remaining_I2, incompr_tuples, 
                              target_size, randoms, gauss_deps, 
                              gauss_rands, gauss_length, secret_idx, curr_tuple,
                              false, NULL);
  } else { 
    // Skipping the current share if there are enough shares remaining
    if (next_secret_share_idx >= t_in - selected_secret_shares_count) {
      secrets_step_arith(c, t_in, include_outputs, required_outputs_remaining,
                   required_outputs_remaining_I2, incompr_tuples,
                   target_size, secrets, randoms,
                   gauss_deps, gauss_rands, gauss_length,
                   next_secret_share_idx - 1, selected_secret_shares_count,
                   secret_idx, curr_tuple, RPC, debug);
    }

    //Reveal a share at index next_secret_share_idx
    VarVector* dep_array = secrets[next_secret_share_idx];
    bool already_in = false;
    for (int i = 0; i < dep_array->length; i++) {
      Comb dep_idx = dep_array->content[i];
      if (Tuple_contains(curr_tuple, dep_idx)) {
        if(already_in) 
          continue;
        
        already_in = true; 
        // This variable of the gadget contains multiple shares of the
        // same input. No need to add it multiple times to the tuples,
        // just recusring further.
        secrets_step_arith(c, t_in, include_outputs, required_outputs_remaining,
                     required_outputs_remaining_I2, incompr_tuples,
                     target_size, secrets, randoms,
                     gauss_deps, gauss_rands, gauss_length,
                     next_secret_share_idx - 1, selected_secret_shares_count+1,
                     secret_idx, curr_tuple, RPC, debug); 
      } else {
        //The tuples doesn't have the n°dep probes, it will add it and continue 
        //the secrets step for the next secret share index. 
        int new_required_output_remaining = required_outputs_remaining;
        int new_required_output_remaining_I2 = required_outputs_remaining_I2;
        int new_target_size = target_size;
        // Take care of the property if the index of the probe is an output.
        if (dep_idx >= c->length && required_outputs_remaining >= 0){
          if (c->output_count == 1 || RPC || required_outputs_remaining_I2 == -1){
            if (required_outputs_remaining == 0) continue; 
            new_required_output_remaining--;
            if (RPC) new_target_size++;
          }
          
          else {
            /* Case Copy gadget RPE */
            if (dep_idx < c->length + c->share_count){
              if (required_outputs_remaining == 0) continue; 
              new_required_output_remaining--;
            }
            else{
              if (required_outputs_remaining_I2 == 0) continue; 
              new_required_output_remaining_I2--;
            }
            new_target_size++;
          }
        }
        
        Tuple_push(curr_tuple, dep_idx);
        //Evaluate |curr_tuple| with his new probes.
        secrets_step_arith(c, t_in, include_outputs, new_required_output_remaining,
                     new_required_output_remaining_I2, incompr_tuples, 
                     new_target_size, secrets, randoms, gauss_deps, 
                     gauss_rands, gauss_length, next_secret_share_idx - 1, 
                     selected_secret_shares_count+1, secret_idx, curr_tuple, 
                     RPC, debug);
        Tuple_pop(curr_tuple);
        /* Same reason as above. */
        *gauss_length = min(curr_tuple->length, *gauss_length);
      }
    }
  }
}


struct sec_step_args {
  const Circuit* c;
  int t_in;
  bool include_outputs;
  int required_outputs_remaining;
  int required_outputs_remaining_I2;
  Trie* incompr_tuples;
  int target_size;
  VarVector** secrets;
  VarVector** randoms;
  int next_secret_share_idx;
  int secret_count;
  Tuple *curr_tuple;
  int secret_idx;
  bool RPC;
  pthread_mutex_t *mutex;
  int debug;
};

void *start_thread_sec_step(void *void_args);

//Same function than |secrets_step_arith| but with parallelisation, and |mutex\ is a 
//mutex useful in the function randoms_step_arith.
void secrets_step_arith_parallel(const Circuit* c,
                           int t_in,
                           bool include_outputs,
                           int required_outputs_remaining,
                           int required_outputs_remaining_I2,
                           Trie* incompr_tuples,
                           int target_size,
                           VarVector** secrets,
                           VarVector** randoms,
                           Dependency** gauss_deps,
                           Dependency* gauss_rands,
                           int *gauss_length,
                           int next_secret_share_idx,
                           int selected_secret_shares_count,
                           int secret_idx,
                           Tuple* curr_tuple,
                           bool RPC,
                           pthread_mutex_t *mutex,
                           int debug) {                             
  //Stop condition : |curr_tuple| is at maximal size or we have enough secret 
  //                 shares to leak. 
  if (next_secret_share_idx == -1 || curr_tuple->length == target_size || selected_secret_shares_count == t_in) {
    // This case : We don't have enough secret shares to leak and we can't add more probes.
    if (selected_secret_shares_count != t_in){
      //We just have to check if we have check all the possible shares. Because 
      //if it's not the case, we don't have the good secret_counts. 
      //We know we can't have more probes to add, but we have to update 
      //secrets count.
      if (next_secret_share_idx != -1){
        VarVector* dep_array = secrets[next_secret_share_idx];
        bool already_in = false;
        for (int i = 0; i < dep_array->length; i++) {
          Comb dep_idx = dep_array->content[i];
          if (Tuple_contains(curr_tuple, dep_idx)) {
            if(already_in)
              continue;
            
            already_in = true;  
            // This variable of the gadget contains multiple shares of the
            // same input. No need to add it multiple times to the tuples,
            // just recusring further.
            secrets_step_arith_parallel(c, t_in, include_outputs, required_outputs_remaining,
                         required_outputs_remaining_I2, incompr_tuples,
                         target_size, secrets, randoms,
                         gauss_deps, gauss_rands, gauss_length, 
                         next_secret_share_idx - 1, 
                         selected_secret_shares_count+1, secret_idx, curr_tuple,
                         RPC, mutex, debug);
          }
          else if (dep_idx >= c->length && RPC && required_outputs_remaining > 0){
            curr_tuple->content[curr_tuple->length] = dep_idx;
            curr_tuple->length++;
            //Probes added, Recursion
            secrets_step_arith_parallel(c, t_in, include_outputs, 
                                  required_outputs_remaining - 1,
                                  required_outputs_remaining_I2, incompr_tuples,
                                  target_size + 1, secrets, randoms,
                                  gauss_deps, gauss_rands, gauss_length, 
                                  next_secret_share_idx - 1, 
                                  selected_secret_shares_count+1, secret_idx, 
                                  curr_tuple, RPC, mutex, debug);
                      
            curr_tuple->length--;
            (*gauss_length) = min (*gauss_length, curr_tuple->length); 
          }
        }
        secrets_step_arith_parallel(c, t_in, include_outputs, required_outputs_remaining,
                     required_outputs_remaining_I2, incompr_tuples,
                     target_size, secrets, randoms,
                     gauss_deps, gauss_rands, gauss_length, 
                     next_secret_share_idx - 1, selected_secret_shares_count, 
                     secret_idx, curr_tuple, RPC, mutex, debug);
      }
      //If we have checked all share index, skip this tuple.
      return;
    }
    
    // This case is for the SNI property, if we have required_outputs remaining,
    // then the tuple doesn't have to be evaluate.
    if (!RPC && required_outputs_remaining > 0 && required_outputs_remaining_I2 == -1) return;
    
    if (debug) {
      printf("\n\nBase tuple:[ ");
      for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]);
      printf("] (size_max = %d), required_outputs_remaining  = %d\n", target_size, required_outputs_remaining);
    }
    
    //The tuple has to be evaluate --->Gauss Elimination on the tuples.   
    initial_gauss_elimination_arith(c, t_in, include_outputs, 
                              required_outputs_remaining, 
                              required_outputs_remaining_I2, incompr_tuples, 
                              target_size, randoms, gauss_deps, 
                              gauss_rands, gauss_length, secret_idx, curr_tuple,
                              false, mutex);
  } else { 
    pthread_t threads1;
    int threads_used_1 = 0;
    // Skipping the current share if there are enough shares remaining
    if (next_secret_share_idx >= t_in - selected_secret_shares_count) {
      if (curr_tuple->length >= target_size - 1) {
        secrets_step_arith_parallel(c, t_in, include_outputs, required_outputs_remaining,
                     required_outputs_remaining_I2, incompr_tuples,
                     target_size, secrets, randoms,
                     gauss_deps, gauss_rands, gauss_length,
                     next_secret_share_idx - 1, selected_secret_shares_count,
                     secret_idx, curr_tuple, RPC, mutex, debug);
      }
      
      else {
        //Parallelisation step on the recursion.
        Tuple *curr_tuple_cpy = Tuple_make_size(c->deps->length);
        memcpy(curr_tuple_cpy->content, curr_tuple->content, 
               curr_tuple->length * sizeof(*curr_tuple_cpy->content));
        curr_tuple_cpy->length = curr_tuple->length;
           
        struct sec_step_args *args = malloc(sizeof(*args));
        args->c = c;
        args->t_in = t_in;
        args->include_outputs = include_outputs;
        args->required_outputs_remaining = required_outputs_remaining; 
        args->required_outputs_remaining_I2 = required_outputs_remaining_I2;
        args->incompr_tuples = incompr_tuples;
        args->target_size = target_size;
        args->secrets = secrets;
        args->randoms = randoms;
        args->next_secret_share_idx = next_secret_share_idx - 1;
        args->secret_count = selected_secret_shares_count;
        args->secret_idx = secret_idx;
        args->curr_tuple = curr_tuple_cpy;
        args->RPC = RPC;
        args->mutex = mutex;
        args->debug = debug;
            
        pthread_mutex_lock(mutex);
        int ret = pthread_create(&threads1, NULL, 
                                 start_thread_sec_step, args);
        pthread_mutex_unlock(mutex);
            
        if(ret){
          Tuple_free(curr_tuple_cpy);
          free(args);
          secrets_step_arith_parallel(c, t_in, include_outputs, required_outputs_remaining,
                     required_outputs_remaining_I2, incompr_tuples,
                     target_size, secrets, randoms,
                     gauss_deps, gauss_rands, gauss_length,
                     next_secret_share_idx - 1, selected_secret_shares_count,
                     secret_idx, curr_tuple, RPC, mutex, debug);
        }
        
        else
          threads_used_1 += 1;
      }
    }

    //Reveal a share at index next_secret_share_idx
    VarVector* dep_array = secrets[next_secret_share_idx];
    pthread_t threads[dep_array->length];
    int threads_used = 0;
    bool already_in = false;
    for (int i = 0; i < dep_array->length; i++) {
      Comb dep_idx = dep_array->content[i];
      if (Tuple_contains(curr_tuple, dep_idx)) {
        if(already_in)
          continue;
        
        already_in = false;
        // This variable of the gadget contains multiple shares of the
        // same input. No need to add it multiple times to the tuples,
        // just recusring further.
        if (curr_tuple->length >= target_size - 1 || (i == (dep_array->length - 1) 
            && threads_used == dep_array->length - 2)){
        
          secrets_step_arith_parallel(c, t_in, include_outputs, required_outputs_remaining,
                                required_outputs_remaining_I2, incompr_tuples,
                                target_size, secrets, randoms,
                                gauss_deps, gauss_rands, gauss_length,
                                next_secret_share_idx - 1, selected_secret_shares_count+1,
                                secret_idx, curr_tuple, RPC, mutex, debug); 
        }
        
        else{
        //Parallelisation step on the recursion.
        Tuple *curr_tuple_cpy = Tuple_make_size(c->deps->length);
        memcpy(curr_tuple_cpy->content, curr_tuple->content, 
               curr_tuple->length * sizeof(*curr_tuple_cpy->content));
        curr_tuple_cpy->length = curr_tuple->length;
        

          
          
        struct sec_step_args *args = malloc(sizeof(*args));
        args->c = c;
        args->t_in = t_in;
        args->include_outputs = include_outputs;
        args->required_outputs_remaining = required_outputs_remaining; 
        args->required_outputs_remaining_I2 = required_outputs_remaining_I2;
        args->incompr_tuples = incompr_tuples;
        args->target_size = target_size;
        args->secrets = secrets;
        args->randoms = randoms;
        args->next_secret_share_idx = next_secret_share_idx - 1;
        args->secret_count = selected_secret_shares_count + 1;
        args->secret_idx = secret_idx;
        args->curr_tuple = curr_tuple_cpy;
        args->RPC = RPC;
        args->mutex = mutex;
        args->debug = debug;
            
        pthread_mutex_lock(mutex);
        int ret = pthread_create(&threads[threads_used], NULL, 
                                 start_thread_sec_step, args);
        pthread_mutex_unlock(mutex);
            
        if(ret){
          Tuple_free(curr_tuple_cpy);
          free(args);
          secrets_step_arith_parallel(c, t_in, include_outputs, required_outputs_remaining,
                                required_outputs_remaining_I2, incompr_tuples,
                                target_size, secrets, randoms,
                                gauss_deps, gauss_rands, gauss_length,
                                next_secret_share_idx - 1, selected_secret_shares_count+1,
                                secret_idx, curr_tuple, RPC, mutex, debug);
        }
        
        else
          threads_used += 1;
                  
        }  
      } else {
        //The tuples doesn't have the n°dep probes, it will add it and continue 
        //the secrets step for the next secret share index. 
        int new_required_output_remaining = required_outputs_remaining;
        int new_required_output_remaining_I2 = required_outputs_remaining_I2;
        int new_target_size = target_size;
        // Take care of the property if the index of the probe is an output.
        if (dep_idx >= c->length && required_outputs_remaining >= 0){
          if (c->output_count == 1 || RPC || required_outputs_remaining_I2 == -1){
            if (required_outputs_remaining == 0) continue; 
            new_required_output_remaining--;
            if (RPC) new_target_size++;
          }
          
          else {
            /* Case Copy gadget RPE */
            if (dep_idx < c->length + c->share_count){
              if (required_outputs_remaining == 0) continue; 
              new_required_output_remaining--;
            }
            else{
              if (required_outputs_remaining_I2 == 0) continue; 
                new_required_output_remaining_I2--;
            }
            new_target_size++;
          }
        }
        
        Tuple_push(curr_tuple, dep_idx);
        if (curr_tuple->length >= target_size - 1 || (i == (dep_array->length - 1) 
            && threads_used == dep_array->length - 2)){
          //Evaluate |curr_tuple| with his new probes.
          secrets_step_arith_parallel(c, t_in, include_outputs, new_required_output_remaining,
                       new_required_output_remaining_I2, incompr_tuples, 
                       new_target_size, secrets, randoms, gauss_deps, 
                       gauss_rands, gauss_length, next_secret_share_idx - 1, 
                       selected_secret_shares_count+1, secret_idx, curr_tuple, 
                       RPC, mutex, debug);
          (*gauss_length) = min (*gauss_length, curr_tuple->length - 1);
        }
        
        else{
          //Parallelisation step on the recursion.
          Tuple *curr_tuple_cpy = Tuple_make_size(c->deps->length);
          memcpy(curr_tuple_cpy->content, curr_tuple->content, 
                curr_tuple->length * sizeof(*curr_tuple_cpy->content));
          curr_tuple_cpy->length = curr_tuple->length;

          
          struct sec_step_args *args = malloc(sizeof(*args));
          args->c = c;
          args->t_in = t_in;
          args->include_outputs = include_outputs;
          args->required_outputs_remaining = new_required_output_remaining; 
          args->required_outputs_remaining_I2 = new_required_output_remaining_I2;
          args->incompr_tuples = incompr_tuples;
          args->target_size = new_target_size;
          args->secrets = secrets;
          args->randoms = randoms;
          args->next_secret_share_idx = next_secret_share_idx - 1;
          args->secret_count = selected_secret_shares_count + 1;
          args->secret_idx = secret_idx;
          args->curr_tuple = curr_tuple_cpy;
          args->RPC = RPC;
          args->mutex = mutex;
          args->debug = debug;
          
          pthread_mutex_lock(mutex);
          int ret = pthread_create(&threads[threads_used], NULL, 
                                   start_thread_sec_step, args);
          pthread_mutex_unlock(mutex);
            
          if(ret){
            Tuple_free(curr_tuple_cpy);
            free(args);
            secrets_step_arith_parallel(c, t_in, include_outputs, new_required_output_remaining,
                                  new_required_output_remaining_I2, incompr_tuples, 
                                  new_target_size, secrets, randoms, gauss_deps, 
                                  gauss_rands, gauss_length, next_secret_share_idx - 1, 
                                  selected_secret_shares_count+1, secret_idx, curr_tuple, 
                                  RPC, mutex, debug);
          }
        
          else
            threads_used += 1;    
        }
        Tuple_pop(curr_tuple);
      }
    }
    
    //Waiting that all the threads finished.
    for(int m = 0; m < threads_used; m++){
      pthread_join(threads[m], NULL);
    } 
    if (threads_used_1 == 1)
      pthread_join(threads1, NULL);
  }
}

/*
Function to start the thread used for |secrets_step_arith_parallel|
-void *void_args : Contain all the necessary arguments for call the 
                   function |secrets_step_arith_parallel|.
*/
void *start_thread_sec_step(void *void_args){  
  struct sec_step_args* args = (struct sec_step_args *) void_args;

  int max_deps_length = args->target_size + args->required_outputs_remaining + 1;
  max_deps_length += args->required_outputs_remaining_I2 == -1 ?
                     0 : args->required_outputs_remaining_I2;
          
  //Build the array of gaussian dependencies and the linked array of gaussian
  //randoms. It is empty now, but when we will add a probes on |curr_tuples|, 
  //we will write the dependency on the probes after gaussian elimination with
  // the other probes existing in |curr_tuples|. And we will write the index of 
  //the randoms that are the pivot for this probes in |gauss_rands|.  
  Dependency** gauss_deps = malloc(max_deps_length * sizeof(*gauss_deps));
  for (int i = 0; i < max_deps_length; i++) {
    gauss_deps[i] = malloc(args->c->deps->deps_size * sizeof(*gauss_deps[i]));
  }
  Dependency* gauss_rands = malloc(max_deps_length * sizeof(*gauss_rands));
  
  int* gauss_length = malloc(sizeof(int));
  *gauss_length = 0;
  
  secrets_step_arith_parallel(args->c, args->t_in, args->include_outputs, 
               args->required_outputs_remaining, 
               args->required_outputs_remaining_I2, args->incompr_tuples, 
               args->target_size, args->secrets, args->randoms, 
               gauss_deps, gauss_rands, gauss_length, 
               args->next_secret_share_idx, args->secret_count, 
               args->secret_idx, args->curr_tuple, args->RPC, args->mutex,
               args->debug);
  
  // Freeing stuffs 
  Tuple_free(args->curr_tuple);
  for (int i = 0; i < max_deps_length; i++)
    free(gauss_deps[i]);
  free(gauss_deps);
  free(gauss_rands);  
  free(gauss_length);
  free(args);
  return NULL; 
}


/* Build the trie of incompr_tuples.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -VarVector **secrets : Array 2D of integer. Each line contains an array of 
                         index of probes that contains the line-th share we 
                         found in the |c->deps| array. For example, 
                         the first line contains the index of the probes who can
                         leak the first share of the first secret value.   
                          
  -VarVector **randoms : Array 2D of integer. Each line contains an array of
                         index of probes that contains the line-th randoms.
                         
  -int t_in : The number of shares that must be leaked for a tuple to be a 
              failure.
  -VarVector* prefix : Prefix to add to all the tuples.
  -int max_size : The maximal size of the incompressible tuples.
  -bool include_outputs : if true, we can include outputs index in the tuples.
  -int required_outputs : Number of outputs required in each tuple.
  -int required_outputs_I2 : See the comments of the function randoms_step_arith 
                             for this variable. 
  -bool RPC : If true, we are checking the RPC (or RPE1 sometimes) property.
  -int cores : If cores != 1, we parallelize.
  -booo one_failure : Indicates wether we have to stopped after we find one 
                      failure tuple or wether we have to find all the tuple.
  -int debug : if true, then some debuging information are printed.
*/
Trie* build_incompr_tuples_arith(const Circuit* c,
                           VarVector** secrets,
                           VarVector** randoms,
                           int t_in,
                           VarVector* prefix,
                           int max_size,
                           bool include_outputs,
                           int required_outputs,
                           int required_outputs_I2,
                           bool RPC,
                           int cores,
                           bool one_failure,
                           int debug) {                         
  pthread_t threads;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  
  int max_deps_length = max_size + required_outputs + 1;
  max_deps_length += required_outputs_I2 == -1? 0 : required_outputs_I2; 
  
  
  
  int share_count = c->share_count;
  //Build the tuple |curr_tuple| that we will use for evaluate all tuples.
  Tuple* curr_tuple = Tuple_make_size(c->deps->length);
  for (int i = 0; i < prefix->length; i++) {
    Tuple_push(curr_tuple, prefix->content[i]);
  }    
  
  //Build the array of gaussian dependencies and the linked array of gaussian
  //randoms. It is empty now, but when we will add a probes on |curr_tuples|, 
  //we will write the dependency on the probes after gaussian elimination with
  // the other probes existing in |curr_tuples|. And we will write the index of 
  //the randoms that are the pivot for this probes in |gauss_rands|.  
  Dependency** gauss_deps = malloc(max_deps_length * sizeof(*gauss_deps));
  for (int i = 0; i < max_deps_length; i++) {
    gauss_deps[i] = malloc(c->deps->deps_size * sizeof(*gauss_deps[i]));
  }
  Dependency* gauss_rands = malloc(max_deps_length * sizeof(*gauss_rands));
  //Build the Trie.
  Trie* incompr_tuples = make_trie(c->deps->length);

  int max_incompr_size = c->share_count + c->random_count;
  max_incompr_size = max_size == -1 ? max_incompr_size :
    max_size < max_incompr_size ? max_size : max_incompr_size;
  
  
  //Compute the incompresible tuples for all size from 1 to |max_incompr_size|
  for (int target_size = prefix->length + 1; 
       target_size <= max_incompr_size + prefix->length; target_size++) {
    //Compute the incompressible tuples of size |target_size| for all the 
    //secret values.
    
    if (cores == 1 || c->secret_count == 1){
      for (int i = 0; i < c->secret_count; i++) {
        int gauss_length = 0;
        if(cores == 1){
          //Compute all the incompressible tuples of size |target_size| for the 
          //secret values i 
          
          secrets_step_arith(c, t_in, include_outputs, required_outputs, 
                       required_outputs_I2, incompr_tuples, target_size, 
                       &secrets[share_count * i], randoms, gauss_deps, 
                       gauss_rands,
                       &gauss_length,
                       c->share_count - 1, //next_secret_share_idx 
                       0, //selected_secret_shares_count 
                       i, //secret_idx
                       curr_tuple, RPC, debug);
        }
        
        else {
          // We use parallelization.
          secrets_step_arith_parallel(c, t_in, include_outputs, required_outputs, 
                                required_outputs_I2, incompr_tuples, target_size, 
                                secrets, randoms, gauss_deps, gauss_rands,
                                &gauss_length,
                                c->share_count - 1, //next_secret_share_idx 
                                0, //selected_secret_shares_count 
                                0, //secret_idx
                                curr_tuple, RPC, &mutex, debug);  
        }
      }
    }
      
    else {
      //We stress that c->secret_count = 2.
      
      Tuple* curr_tuple_cpy = Tuple_make_size(c->deps->length);
      memcpy(curr_tuple_cpy->content, curr_tuple->content,
             curr_tuple->length * sizeof(*curr_tuple_cpy->content));
      curr_tuple_cpy->length = curr_tuple->length;
        
      struct sec_step_args *args = malloc(sizeof(*args));
      args->c = c;
      args->t_in = t_in;
      args->include_outputs = include_outputs;
      args->required_outputs_remaining = required_outputs;
      args->required_outputs_remaining_I2 = required_outputs_I2;
      args->incompr_tuples = incompr_tuples;
      args->target_size = target_size;
      args->secrets = secrets;
      args->randoms = randoms;
      args->curr_tuple = curr_tuple_cpy;
      args->next_secret_share_idx = c->share_count - 1;
      args->secret_count = 0;
      args->secret_idx = 0;
      args->RPC = RPC;
      args->mutex = &mutex;
      args->debug = debug;
        
      int ret = pthread_create(&threads, NULL, start_thread_sec_step, 
                               (void *)args);
                     
      if (ret){
        int gauss_length = 0;
        secrets_step_arith_parallel(c, t_in, include_outputs, required_outputs, 
                              required_outputs_I2, incompr_tuples, target_size, 
                              secrets, randoms, gauss_deps, gauss_rands, 
                              &gauss_length,
                              c->share_count - 1, //next_secret_share_idx 
                              0, //selected_secret_shares_count 
                              0, //secret_idx
                              curr_tuple, RPC, &mutex, debug);
      }
      
      int gauss_length_bis = 0;  
      secrets_step_arith_parallel(c, t_in, include_outputs, required_outputs, 
                   required_outputs_I2, incompr_tuples, target_size, 
                   &secrets[share_count], randoms, gauss_deps, gauss_rands,
                   &gauss_length_bis,
                   c->share_count - 1, //next_secret_share_idx 
                   0, //selected_secret_shares_count 
                   1, //secret_idx
                   curr_tuple, RPC, &mutex, debug);
                   
      pthread_join(threads, NULL);

    }
    
    //If we have to only find one failure, cehckinf if we find one !
    if (one_failure && trie_tuples_size(incompr_tuples, target_size))
      break;
  }
  
  // Freeing stuffs 
  Tuple_free(curr_tuple);
  for (int i = 0; i < max_deps_length; i++) {
    free(gauss_deps[i]);
  }
  free(gauss_deps);
  free(gauss_rands);
  
  return incompr_tuples;
}


/*
Compute the incompressible tuples we found for the gadget we are studying.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int t_in : The number of shares that must be leaked for a tuple to be a 
              failure.
  -VarVector* prefix : Prefix to add to all the tuples.
  -int max_size : The maximal size of the incompressible tuples.
  -bool include_outputs : if true, we can include outputs index in the array 
                          secrets and randoms.
  -int required_outputs : Number of outputs required in each tuple.
  -bool RPC : If true, we are checking the RPC (or RPE1 sometimes) property.
  -int cores : If cores != 1, we parallelize.
  -booo one_failure : Indicates wether we have to stopped after we find one 
                      failure tuple or wether we have to find all the tuple.
  -int verbose : Set a level of verbosity.
  
Output : A trie of all the incompressibles tuples of size <= |max_size|.
*/
Trie* compute_incompr_tuples_arith(const Circuit* c,
                             int t_in,
                             VarVector* prefix,
                             int max_size,
                             bool include_outputs,
                             int required_outputs,
                             bool RPC,
                             int cores,
                             bool one_failure,  
                             int verbose) {
  if (c->contains_mults) {
    return compute_incompr_tuples_mult_arith(c, max_size, include_outputs, 
                                             required_outputs, RPC, t_in, cores, 
                                             one_failure, verbose);
  }
  
  VarVector** secrets;
  VarVector** randoms;
  build_dependency_arrays_arith(c, &secrets, &randoms, include_outputs, verbose);

  if (t_in == -1) t_in = c->share_count;
  prefix = prefix ? prefix : &empty_VarVector;

  // As a parameter to compute_incompr_tuples_arith, |include_outputs|
  // instructs on whether build_dependency_arrays_arith should take outputs
  // into account. However, as a parameter to build_incompr_tuples_arith
  // (and the functions called by it), |include_outputs| is used to
  // indicate if the number of outputs present in the tuples is fixed
  // by |required_outputs| or not.
  include_outputs = required_outputs > 0 ? true : false;
  
  Trie* incompr_tuples = build_incompr_tuples_arith(c, secrets, randoms, t_in,
                                              prefix, max_size, include_outputs,
                                              required_outputs, -1, RPC, cores, 
                                              one_failure, verbose);
  
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

    printf("Generated %d tuples.\n\n", tot_adds_arith);
  }
  
  // Freeing stuffs
  {
    int secret_count     = c->secret_count;
    int total_secrets    = secret_count * c->share_count;
    int random_count     = c->random_count;

    for (int i = 0; i < total_secrets; i++) {
      VarVector_free(secrets[i]);
    }
    free(secrets);
    for (int i = total_secrets; i < total_secrets + random_count; i++) {
      VarVector_free(randoms[i]);
    }
    free(randoms);
  }
  
  return incompr_tuples;
}


/*
Compute the incompressible tuples we found for the copy gadget we are studying, 
in case of the study of the RPE property.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int t_in : The number of shares that must be leaked for a tuple to be a 
              failure.
  -int max_size : The maximal size of the incompressible tuples.
  -int required_outputs_I1 : Number of outputs shares required in each tuple for
                             the first secret value.
  -int required_outputs_I2 : Number of outputs shares required in each tuple for 
                             the second secret value.
  -int cores : If cores != 1, we parallelize.
  -int verbose : Set a level of verbosity.
  
Output : A trie of all the incompressibles tuples of size <= |max_size|.
*/
Trie* compute_incompr_tuples_RPE_copy(const Circuit* c,
                                      int t_in,
                                      int max_size,
                                      int required_outputs_I1,
                                      int required_outputs_I2,
                                      int cores,
                                      int verbose) {
  
  VarVector** secrets;
  VarVector** randoms;
  build_dependency_arrays_arith(c, &secrets, &randoms, true, verbose);
  
  bool RPC = false;
  if (c->output_count == 1) RPC = true;
    
  if (t_in == -1) t_in = c->share_count;
  
  Trie* incompr_tuples = build_incompr_tuples_arith(c, secrets, randoms, t_in,
                                              &empty_VarVector, max_size, true,
                                              required_outputs_I1, 
                                              required_outputs_I2, RPC, 
                                              cores,
                                              false,
                                              verbose);
  
  // Freeing stuffs
  {
    int secret_count     = c->secret_count;
    int total_secrets    = secret_count * c->share_count;
    int random_count     = c->random_count;

    for (int i = 0; i < total_secrets; i++) {
      VarVector_free(secrets[i]);
    }
    free(secrets);
    for (int i = total_secrets; i < total_secrets + random_count; i++) {
      VarVector_free(randoms[i]);
    }
    free(randoms);
  }
  
  return incompr_tuples;
}

/*
Compute the incompressibles tuples and compute the error coefficients for the RP
property.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int coeff_max : The maximal coefficient that we have to compute.
  -int cores : If cores != 1, we parallelize.
  -int verbose : Set a level of verbosity.  
*/
void compute_RP_coeffs_incompr_arith(const Circuit* c, int coeff_max, int cores,
                               int verbose) {
  //Compute the optimal HASH_MASK for the hash_table.
  determine_HASH_MASK(c->length, coeff_max); 
  
  //Compute the trie of incompressibles tuples.
  Trie* incompr_tuples = compute_incompr_tuples_arith(c, c->share_count,
                                                NULL, coeff_max, false, 0, 
                                                false, cores, false, verbose);
  
  // Generating failures from incompressible tuples, and computing coefficients.
  compute_failures_from_incompressibles(c, incompr_tuples, coeff_max, verbose);
  free_trie(incompr_tuples);
}

/*
Create all the possible output set of size |required_output| recursively. And 
then add all the errors tuples that contains a subset of the output set in his 
output index in a trie called |incompr_tuples_output|. Then we compute the 
coefficients from this trie.
Input :
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -Trie *incompr_tuples : the trie of incompressible tuples.
  -int required_output : Number of outputs required in each tuple.
  -int last_index : index used to know which output_index we add in the last 
                    recursion of the function.
  -int *output_set : The output set we create recursively.
  -int len_output : The cureent size of |otuput_set|.
  -int coeff_max : The maximal coefficient that we have to compute.
  -int verbose : Set a level of verbosity.
  -uint64_t *coeffs : The list of errors coefficients in which we will write.
*/
void update_coeff_output_RPC(const Circuit* c, Trie *incompr_tuples, 
                             int required_output, int last_index, 
                             int *output_set, int len_output, int coeff_max, 
                             int verbose, uint64_t *coeffs){
  if (required_output == 0){
    //Create |incompr_tuples_output| like we said above.
    Trie *incompr_tuples_output = derive_trie_from_subset (incompr_tuples, 
                                                           output_set, 
                                                           len_output, 
                                                           c->length, 
                                                           c->secret_count,
                                                           c->deps->length,
                                                           coeff_max);
    
    // Initializing coefficients for the set.
    uint64_t coeffs_output[c->total_wires+1];
    for (int i = 0; i <= c->total_wires; i++) {
      coeffs_output[i] = 0;
    }
    
    // Generating failures from incompressible tuples, and computing coefficients.
    compute_failures_from_incompressibles_RPC(c, incompr_tuples_output, NULL,
                                              coeff_max, verbose, coeffs_output,
                                              false); 
    free_trie(incompr_tuples_output);
    
    // We take the max of the coefficients of the different subsets in RPC.
    for (int i = 0; i <= c->total_wires; i++){
      if (coeffs[i] < coeffs_output[i]) coeffs[i] = coeffs_output[i]; 
    }
  
  }
  
  else {
    //Creation of the subset is not finished.
    if (last_index == -1){
      //First recursion
      int nb_out = c->output_count * c->share_count;
      last_index = c->deps->length - nb_out;
    }
    
    for (int i = last_index; i < c->deps->length; i++){
      //Add index i to the output set, so |last_index| becomes i
      output_set[len_output] = i;
      
      //Recursion
      update_coeff_output_RPC(c, incompr_tuples, required_output - 1, i + 1, 
                          output_set, len_output + 1, coeff_max, verbose, 
                          coeffs); 
    }
  }
}

/*
Take an output set, and modify him to create a new output set. The modfication 
will be in numerical order. Here an example with 4 output index 25,26,27,28 and 
output set of size 3. Suppose that the output set is now at [25,26,27] (normally
it is the case when we call the function first). Then 1 call to the function 
will change the output set by [25,26,28] and : 
  -2 calls of the function : [25,27,28]
  -3 calls of the function : [26,27,28]
  -4 calls of the function : return false.
In fact, there is a limit of what we can modify to create a new output set 
different of all the other ones, that's why when it 's no more possible, 
we return false.

Input :
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int required_output : Size of tha array |output_set|.
  -int int index_modify : the index in which we want to add 1 to the 
                          |output_set|.
  -int *output_set : The output set we want to update.
  -int output_idx : The output value we are currently studying (0 for the first 
                    one, one for the second,...)
                    
Output : True if the modification has been made, false otherwise. 
*/
static bool update_output_set (const Circuit *c, int required_output, int index_modify,
                        int *output_set, int output_idx){
 
  if(output_set[index_modify] + 1 >= c->length + 
                                     (output_idx + 1) * c->share_count - 
                                     (required_output - 1 - index_modify)){
    //Here, we can't add 1 to index modify, because it will create output_index 
    // bigger than waht we want in the output ste for the next index of the 
    //output set.
    
    //We try to modify before in the output set if we can.
    if (index_modify != 0) {
      if(update_output_set(c, required_output, index_modify - 1, output_set, output_idx)){
        update_output_set(c, required_output, index_modify, output_set, output_idx);
      }
      
      else return false;  
    }

    else return false;
  }
  
  else {
    //Modification is OK at index |index_modify|, we add 1 to this index and 
    //add the possible minimum value for the next index (the output_set is 
    //sorted).  
    output_set[index_modify] += 1;
    for (int i = index_modify + 1 ; i < required_output; i++){
      output_set[i] = output_set[index_modify] + (i - index_modify) - 1;
    }
  }
  // Modification has been made so we return true.
  return true;
}

void update_coeffs_RPC(const Circuit *c, Trie *incompr_tuples, int *output_set,
                       int len_output, int coeff_max, uint64_t *coeffs, 
                       pthread_mutex_t *mutex, int verbose){
   
  Trie *incompr_tuples_output = derive_trie_from_subset (incompr_tuples, 
                                                         output_set, 
                                                         len_output, 
                                                         c->length, 
                                                         c->secret_count,
                                                         c->deps->length,
                                                         coeff_max);                                                      
  
  // Initializing coefficients for the set.
  uint64_t coeffs_output[c->total_wires+1];
  for (int i = 0; i <= c->total_wires; i++) {
    coeffs_output[i] = 0;
  }
    
  // Generating failures from incompressible tuples, and computing coefficients.
  compute_failures_from_incompressibles_RPC(c, incompr_tuples_output, NULL,
                                            coeff_max, verbose, coeffs_output,
                                            false); 
  free_trie(incompr_tuples_output);
    
  if (mutex)
    pthread_mutex_lock(mutex);
   
  // We take the max of the coefficients of the different subsets in RPC.
  for (int i = 0; i <= c->total_wires; i++){
    if (coeffs[i] < coeffs_output[i]) coeffs[i] = coeffs_output[i]; 
  }
  
  if(mutex)
    pthread_mutex_unlock(mutex);
}



/*
Compute the incompressibles tuples and compute the error coefficients for the 
RPC property.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int coeff_max : The maximal coefficient that we have to compute.
  -bool include_output : if true, we can include outputs index in the array 
                          secrets and randoms.
  -int required_output : Number of outputs required in each tuple
  -int cores : If cores != 1, we parallelize.
  -int verbose : Set a level of verbosity.  
*/
void compute_RPC_coeffs_incompr_arith (const Circuit* c, int coeff_max, 
                                 bool include_output, int required_output,
                                 int cores, int verbose){
  //Compute the optimal HASH_MASK for the hash_table.
  determine_HASH_MASK(c->length, coeff_max); 
  
  // Initializing coefficients
  uint64_t coeffs[c->total_wires+1];
  for (int i = 0; i <= c->total_wires; i++) {
    coeffs[i] = 0;
  }
  
  //Compute the trie of incompressible tuples.
  Trie* incompr_tuples = compute_incompr_tuples_arith(c, required_output + 1,
                                                NULL, coeff_max, include_output, 
                                                required_output, true, cores, 
                                                false, verbose);
   
  // Generating failures from incompressible tuples, and computing coefficients.
  int output_set[required_output];
  for (int i = 0 ; i < required_output; i ++){
    output_set[i] = c->length + i;
  }
  
  do {
    update_coeffs_RPC(c, incompr_tuples, output_set, required_output, 
                      coeff_max, coeffs, NULL, verbose);           
  } while (update_output_set(c, required_output, required_output - 1, output_set, 0));
  update_coeff_output_RPC(c, incompr_tuples, required_output, -1, output_set, 0,
                          coeff_max, verbose, coeffs); 
  
  printf("f(p) = [");
  for (int i = 0; i < c->total_wires; i++){
    printf("%lu, ", coeffs[i]);
  }
  printf("%lu]\n", coeffs[c->total_wires]);
  
  double p_min = compute_leakage_proba(coeffs, coeff_max,
                                       c->total_wires+1,
                                       1, // minimax
                                       false); // square root
  double p_max = compute_leakage_proba(coeffs, coeff_max,
                                       c->total_wires+1,
                                       -1, // minimax
                                       false); // square root

  printf("\n");
  printf("pmax = %.10f -- log2(pmax) = %.10f\n", p_max, log2(p_max));
  printf("pmin = %.10f -- log2(pmin) = %.10f\n", p_min, log2(p_min));
  printf("\n");
  
  free_trie(incompr_tuples);
}

/* Compute the coefficients from the intersection of the 
two Trie of incompressible tuples, and for the incompressible failure tuples 
that have output index include in |output_set|.
Input : 
    -const Circuit *c : The arithmetic circuit.
    -Trie *incompr_tuples : The trie of incompressible tuples for the first 
                            secret value (i.e failure tuples for the first 
                            secret value)
    -Trie *incompr_tuples2 : The trie of incompressible tuples for the second 
                             secret value
    -int *output_set :  The output set we are considering.
    -int len_output : The size of the output set.
    -int coeff_max : The maximal coefficient that we have to compute.
    -uint64_t *coeffs : The coeffs we have to compute.
    -pthread_mutex_t mutex : For parallelization purpose.
    -int verbose : Set a level of verbosity. 
*/
void update_coeffs_RPE_inter(const Circuit *c, Trie *incompr_tuples, 
                             Trie *incompr_tuples2, int *output_set, 
                             int len_output, int coeff_max, uint64_t *coeffs, 
                             pthread_mutex_t *mutex, int verbose){
    
    
    Trie *incompr_tuples_output = 
      derive_trie_from_subset (incompr_tuples, output_set, len_output, 
                               c->length, c->secret_count, c->deps->length, 
                               coeff_max);
    
    Trie *incompr_tuples_output2 = 
      derive_trie_from_subset (incompr_tuples2, output_set, len_output, 
                               c->length, c->secret_count, c->deps->length, 
                               coeff_max);
    
    // Initializing coefficients for the set.
    uint64_t coeffs_output[c->total_wires+1];
    for (int i = 0; i <= c->total_wires; i++) {
      coeffs_output[i] = 0;
    }
    compute_failures_from_incompressibles_RPC(c, incompr_tuples_output, 
                                              incompr_tuples_output2, coeff_max,
                                              verbose, coeffs_output, true); 
    free_trie(incompr_tuples_output);
    free_trie(incompr_tuples_output2);
    
    if (mutex)
      pthread_mutex_lock(mutex);
    
    for (int i = 0; i <= c->total_wires; i++){
      if (coeffs[i] < coeffs_output[i]) coeffs[i] = coeffs_output[i]; 
    }
    
    if (mutex)
      pthread_mutex_unlock(mutex);
}



/*Like build_incompr_tuples_arith but for the property RPE,
  the idea is to construct 2 incompressibles tuples, one depends on the first 
  secret value, the second depends on the second secret value.
  Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -VarVector **secrets : Array 2D of integer. Each line contains an array of 
                         index of probes that contains the line-th share we 
                         found in the |c->deps| array. For example, 
                         the first line contains the index of the probes who can
                         leak the first share of the first secret value.   
                          
  -VarVector **randoms : Array 2D of integer. Each line contains an array of
                         index of probes that contains the line-th randoms.
                         
  -int t_in : The number of shares that must be leaked for a tuple to be a 
              failure.
  -int max_size : The maximal size of the incompressible tuples.
  -bool include_outputs : if true, we can include outputs index in the tuples.
  -int required_outputs : Number of outputs required in each tuple.
  -int secret_idx : The secret value we are currently studying.
  -int cores : If cores != 1, we parallelize.
  -pthread_mutex_t *mutex : Useful in |randoms_step_arith|.
  -int debug : if true, then some debuging information are printed.
   */ 
Trie* build_incompr_tuples_arith_RPE(const Circuit* c,
                               VarVector** secrets,
                               VarVector** randoms,
                               int t_in,
                               int max_size,
                               bool include_outputs,
                               int required_outputs,
                               int secret_idx,
                               int cores,
                               pthread_mutex_t *mutex,
                               int debug) {
  int max_deps_length = max_size + required_outputs + 1;
  int share_count = c->share_count;
  
  //Build the tuple |curr_tuple| that we will use for evaluate all tuples.
  Tuple* curr_tuple = Tuple_make_size(c->deps->length); 
  
  //Build the array of gaussian dependencies and the linked array of gaussian
  //randoms. It is empty now, but when we will add probes on |curr_tuples|, 
  //we will write the dependency on the probes after gaussian elimination with
  //the other probes existing in |curr_tuples|. And we will write the index of 
  //the randoms that are the pivot for this probes in |gauss_rands|. 
  Dependency** gauss_deps = malloc(max_deps_length * sizeof(*gauss_deps));
  for (int i = 0; i < max_deps_length; i++) {
    gauss_deps[i] = malloc(c->deps->deps_size * sizeof(*gauss_deps[i]));
  }
  Dependency* gauss_rands = malloc(max_deps_length * sizeof(*gauss_rands));
  
  //Build the Trie.
  Trie* incompr_tuples = make_trie(c->deps->length);

  int max_incompr_size = c->share_count + c->random_count;
  max_incompr_size = max_size == -1 ? max_incompr_size :
    max_size < max_incompr_size ? max_size : max_incompr_size;
  
  pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
  if (!mutex){
    mutex = &mut;
  }
  
  //Compute the incompressible tuples for all size from 1 to |max_incompr_size|
  // and for the secret values |secret_idx|.  
  for (int target_size = 1; target_size <= max_incompr_size; target_size++) {
    int gauss_length = 0;  
    if(cores == 1){
        secrets_step_arith (c, t_in, include_outputs, required_outputs, -1, 
                      incompr_tuples, target_size,
                      &secrets[share_count * secret_idx], randoms, gauss_deps, 
                      gauss_rands,
                      &gauss_length,
                      c->share_count - 1, //next_secret_shares_idx
                      0, //selected_secret_shares_count
                      secret_idx, // secret_idx
                      curr_tuple, true, debug);       
    }
    
    else {
    
    
      secrets_step_arith_parallel (c, t_in, include_outputs, required_outputs, -1, 
                             incompr_tuples, target_size,
                             &secrets[share_count * secret_idx], randoms, 
                             gauss_deps, gauss_rands, &gauss_length,
                             c->share_count - 1, //next_secret_shares_idx
                             0, //selected_secret_shares_count
                             secret_idx, // secret_idx
                             curr_tuple, true, mutex, debug); 
    }
  }
  
  
  // Freeing stuff.
  Tuple_free(curr_tuple);
  for (int i = 0; i < max_deps_length; i++) {
    free(gauss_deps[i]);
  }
  free(gauss_deps);
  free(gauss_rands);
  return incompr_tuples;
}

// Compute the minimal value in the array |arr| of len |len|.
static double min_arr(double* arr, int len) {
  double min_val = arr[0];
  for (int i = 1; i < len; i++) min_val = min(min_val, arr[i]);
  return min_val;
}

/* Compute the coeffs RPE11 for the RPE case in the copy gadget.*/
uint64_t *compute_RPE11_coeffs(const Circuit *c, int coeff_max, 
                               int required_output, int cores, int verbose);
/* Compute the coeffs RPE21 for the RPE case in the copy gadget.*/
uint64_t *compute_RPE21_coeffs(const Circuit *c, int coeff_max, 
                               int required_output, int cores, int verbose);
/* Compute the coeffs RPE12 for the RPE case in the copy gadget.*/
uint64_t *compute_RPE12_coeffs(const Circuit *c, int coeff_max, 
                               int required_output, int cores, int verbose);
/* Compute the coeffs RPE22 for the RPE case in the copy gadget.*/
uint64_t *compute_RPE22_coeffs(const Circuit *c, int coeff_max, 
                               int required_output, int cores, int verbose);

//Struture used to parallelize the 4 function above.
struct compute_RPEii_coeffs_args{
  const Circuit *c;
  int coeff_max;
  int required_output;
  int cores;
  int verbose;
  int number_case;
};

//Function to start the parallelization of one of the 4 function above according
//to the number case in the structure. 
void *start_thread_compute_RPEii_coeffs (void *void_args){
  struct compute_RPEii_coeffs_args *args = 
                                      (struct compute_RPEii_coeffs_args *) void_args;
  uint64_t *coeffs;
  if (args->number_case == 0){
    coeffs = compute_RPE11_coeffs(args->c, args->coeff_max, args->required_output,
                                  args->cores, args->verbose);
  }
  
  else if (args->number_case == 1){
    coeffs = compute_RPE12_coeffs(args->c, args->coeff_max, args->required_output,
                                  args->cores, args->verbose);  
  }
  
  else if (args->number_case == 2){
    coeffs = compute_RPE21_coeffs(args->c, args->coeff_max, args->required_output,
                                  args->cores, args->verbose);  
  }
  
  else{
    coeffs = compute_RPE22_coeffs(args->c, args->coeff_max, args->required_output,
                                  args->cores, args->verbose);  
  }
  
  free(args);
  return coeffs;  
}


/*
Compute the incompressibles tuples and compute the error coefficients for the 
RPE property and only for the copy gadget.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int coeff_max : The maximal coefficient that we have to compute.
  -int required_outputs : Number of outputs required in each tuple
  -int cores : if cores != 1, we parallelize. 
  -int verbose : Set a level of verbosity.  
*/
void compute_RPE_coeffs_incompr_copy(const Circuit *c, int coeff_max, 
                                     int required_output, int cores, 
                                     int verbose){

  uint64_t *coeffs_RPE11 = NULL;
  uint64_t *coeffs_RPE12 = NULL;
  uint64_t *coeffs_RPE21 = NULL;                                 
  uint64_t *coeffs_RPE22 = NULL;
  
  if (cores == 1){
    coeffs_RPE11 = compute_RPE11_coeffs(c, coeff_max, required_output, 
                                                  cores, verbose);
    coeffs_RPE12 = compute_RPE12_coeffs(c, coeff_max, required_output, 
                                                  cores, verbose);
    coeffs_RPE21 = compute_RPE21_coeffs(c, coeff_max, required_output, 
                                                  cores, verbose);                                                
    coeffs_RPE22 = compute_RPE22_coeffs(c, coeff_max, required_output, 
                                                  cores, verbose);
  }
  else {
    pthread_t threads[3];
    int ret[3];
    for (int i = 0 ; i < 3; i++){
      struct compute_RPEii_coeffs_args *args = malloc(sizeof(*args));
      args->c = c;
      args->coeff_max = coeff_max;
      args->required_output = required_output;
      args->cores = cores;
      args->verbose = verbose;
      args->number_case = i;
      
      ret[i] = pthread_create(&threads[i], NULL, 
                              start_thread_compute_RPEii_coeffs, (void*) args);
                               
      if (ret[i]){
        if (i == 0){
          coeffs_RPE11 = compute_RPE11_coeffs(c, coeff_max, required_output,
                                              cores, verbose);
        }
        else if (i == 1){
          coeffs_RPE12 = compute_RPE12_coeffs(c, coeff_max, required_output,
                                              cores, verbose);          
        }
        else{
          coeffs_RPE21 = compute_RPE21_coeffs(c, coeff_max, required_output,
                                              cores, verbose);
        }              
      }
    }
    
    coeffs_RPE22 = compute_RPE22_coeffs(c, coeff_max, required_output,
                                        cores, verbose);
                                        
    if(!ret[0]){
      void *return_value;
      pthread_join(threads[0], &return_value);
      coeffs_RPE11 = (uint64_t *) return_value;
    }
    if(!ret[1]){
      void *return_value1;
      pthread_join(threads[1], &return_value1);
      coeffs_RPE12 = (uint64_t *) return_value1;
    }
    if(!ret[2]){
      void *return_value2;
      pthread_join(threads[2], &return_value2);
      coeffs_RPE21 = (uint64_t *) return_value2;
    }
  }
  
  
  //Printing the coefficients
  printf("\nCoeffs RPE11 : \n");
  printf("I = [");
  for (int i = 0; i < c->total_wires; i++){
    printf("%lu, ", coeffs_RPE11[i]);
  }
  printf("%lu]\n", coeffs_RPE11[c->total_wires]);
  
  printf("\nCoeffs RPE12 : \n");
  printf("I = [");
  for (int i = 0; i < c->total_wires; i++){
    printf("%lu, ", coeffs_RPE12[i]);
  }
  printf("%lu]\n", coeffs_RPE12[c->total_wires]);
  
  printf("\nCoeffs RPE21 : \n");
  printf("I = [");
  for (int i = 0; i < c->total_wires; i++){
    printf("%lu, ", coeffs_RPE21[i]);
  }
  printf("%lu]\n", coeffs_RPE21[c->total_wires]);

  printf("\nCoeffs RPE22 : \n");
  printf("I = [");
  for (int i = 0; i < c->total_wires; i++){
    printf("%lu, ", coeffs_RPE22[i]);
  }
  printf("%lu]\n\n", coeffs_RPE22[c->total_wires]);
  
  // Computing leakage probability from coefficients  
  double p[2];
  for (int i = 0; i < 2; i++){
    int min_max = i == 0 ? -1 : 1;
    double p_arr[4] = {
        compute_leakage_proba(coeffs_RPE11, coeff_max, c->total_wires+1, 
                              min_max, false),
        compute_leakage_proba(coeffs_RPE12, coeff_max, c->total_wires+1, 
                              min_max, false),
        compute_leakage_proba(coeffs_RPE21, coeff_max, c->total_wires+1, 
                              min_max, false),
        compute_leakage_proba(coeffs_RPE22, coeff_max, c->total_wires+1, 
                              min_max, false) 
    };
    p[i] = min_arr(p_arr, 4);
  }
  
  printf("pmax = %.10f -- log2(pmax) = %.10f\n", p[0], log2(p[0]));
  printf("pmin = %.10f -- log2(pmin) = %.10f\n", p[1], log2(p[1]));
  printf("\n");
  
  free(coeffs_RPE11);
  free(coeffs_RPE12);
  free(coeffs_RPE21);
  free(coeffs_RPE22);
}
  
  
/* Case 1 : |J1| <= t and |J2| <= t */
uint64_t *compute_RPE11_coeffs(const Circuit *c, int coeff_max, 
                               int required_output, int cores, int verbose){  
  int nb_shares = c->share_count;
  
  //Initializing coefficients
  uint64_t *coeffs_RPE11 = malloc((c->total_wires+1) * sizeof(uint64_t));
  for (int i = 0; i <= c->total_wires; i++) {
    coeffs_RPE11[i] = 0;
  }
  
  int J11[required_output];
  int J21[required_output];
  
  //Compute incompressibles tuples but with the output index.
  Trie * incompr_tuples = compute_incompr_tuples_RPE_copy(c, required_output + 1,
                                                          coeff_max,  
                                                          required_output, 
                                                          required_output,
                                                          cores, 
                                                          verbose);                                        
  
  //Create the output subset for J1 and J2 that we will update thanks to the 
  //function update_output_set.
  for (int i = 0; i < required_output; i++){
    J11[i] = c->length + i;
    J21[i] = J11[i] + nb_shares;
  }
  
  //Useful for update_output_set.
  int index_modify = required_output - 1;
  
  do {
    do {
      //Create the output_set by union of J1 and J2.
      int output_set[2 * required_output];
      memcpy(output_set, J11, required_output * sizeof(*output_set));
      memcpy(&output_set[required_output], J21, required_output * sizeof(*(&output_set[required_output])));

      //Create the trie of incompressibles errors tuples without output and who can 
      //simulate the output in J1 and J2.
      Trie *incompr_tuples_output = derive_trie_from_subset(incompr_tuples, 
                                                            output_set, 
                                                            2 * required_output, 
                                                            c->length, 
                                                            c->secret_count,
                                                            c->deps->length,
                                                            coeff_max);                                                                        
      
      //Initializing coefficients for write the errors coefficients of 
      //|incompr_tuples_output|.
      uint64_t coeffs_output[c->total_wires+1];
      for (int i = 0; i <= c->total_wires; i++) {
        coeffs_output[i] = 0;
      }      
      
      // Generating failures from incompressible tuples, and computing coefficients.
      compute_failures_from_incompressibles_RPC(c, incompr_tuples_output, NULL, 
                                                coeff_max, verbose, 
                                                coeffs_output, false);
      
      // RPE1 : Compute the max failures of all the output set.
      for (int i = 0; i <= c->total_wires; i++){
        coeffs_RPE11[i] = max(coeffs_RPE11[i], coeffs_output[i]);
      }
      
      free_trie(incompr_tuples_output);
      
    }  while (update_output_set(c, required_output, index_modify, J21, 1));
  } while (update_output_set(c, required_output, index_modify, J11, 0)); 
  
  free_trie(incompr_tuples);
  return coeffs_RPE11;
}
  
/* Case 2 : |J1| <= t and |J2| = nb_shares - 1 */
uint64_t *compute_RPE12_coeffs(const Circuit *c, int coeff_max, 
                               int required_output, int cores, int verbose){  
  int nb_shares = c->share_count;
  int var = nb_shares - 1;
  
  //Initializing coefficients
  uint64_t *coeffs_RPE12 = malloc((c->total_wires+1) * sizeof(uint64_t));
  for (int i = 0; i <= c->total_wires; i++) {
    coeffs_RPE12[i] = 0;
  }
  
  int J11[required_output];
  int J22[nb_shares -1];
  
  //Create the output subset for J1 and J2 that we will update thanks to the 
  //function update_output_set.
  for (int i = 0; i < required_output; i++){
    J11[i] = c->length + i;
    J22[i] = J11[i] + nb_shares;
  }
  
  for (int i = required_output; i < var; i++){
    J22[i] = c->length + i + nb_shares;
  }
  
  //Useful for update_output_set.
  int index_modify = required_output - 1;
  
  //Compute incompressibles tuples but with the output index.
  Trie * incompr_tuples = compute_incompr_tuples_RPE_copy(c, required_output + 1,
                                                          coeff_max, 
                                                          required_output, var,
                                                          cores, verbose);
                                                   
  do{
    //Initializing coefficients for write the errors coefficients of 
    //the property RPE2 for J2.
    uint64_t coeffs_output_I2[c->total_wires+1];
    for (int i = 0; i <= c->total_wires; i++) {
      coeffs_output_I2[i] = 0;
    }
    
    //Create the trie of incompressibles errors tuples without output in J1 from 
    //|incompr_tuples| and who can simulate the output in J1.
    Trie *incompr_tuples_I1 = derive_trie_from_subset(incompr_tuples, J11,
                                                      required_output, 
                                                      c->length,
                                                      c->secret_count,
                                                      c->length + c->share_count,
                                                      coeff_max + c->share_count);
                                                                                                      
    
    Trie **incompr_tuples_I2 = malloc(nb_shares * sizeof(*incompr_tuples_I2)); 
    int idx = 0;
    do{
      //Create the trie of incompressibles errors tuples without output in J2 
      //from |incomrp_tuples_I1| and who can simulate the output in J1 and J2.
      incompr_tuples_I2[idx] = derive_trie_from_subset(incompr_tuples_I1, 
                                                       J22,
                                                       var, 
                                                       c->length,
                                                       c->secret_count,
                                                       c->deps->length,
                                                       coeff_max);                                              
      idx += 1;                                                                    
    }  while (update_output_set(c, var, nb_shares - 2, J22, 1));
    
    //Generating RPE2 failures from |incompr_tuples_I2|, and computing 
    //coefficients.
    compute_failures_from_incompressibles_RPE2_single(c, incompr_tuples_I2,
                                                      nb_shares, coeff_max, 
                                                      verbose, 
                                                      coeffs_output_I2);
    
    for (int i = 0; i < var; i++){
      free_trie(incompr_tuples_I2[i]);
    } 
    free(incompr_tuples_I2);
    
    // RPE1 : Compute the max failure between all the J1 output set.
    for (int i = 0; i <= c->total_wires; i++){
        coeffs_RPE12[i] = max(coeffs_RPE12[i], coeffs_output_I2[i]);
    }
    
    for (int i = 0; i < var; i++){
      J22[i] = c->length + nb_shares + i;
    }
    
    free_trie(incompr_tuples_I1);
    
  } while (update_output_set(c, required_output, index_modify, J11, 0)); 
  
  free_trie(incompr_tuples);
  return coeffs_RPE12;
} 
  
/* Case 3 : |J1| = nb_shares - 1 and |J2| <= t */
uint64_t *compute_RPE21_coeffs(const Circuit *c, int coeff_max, 
                               int required_output, int cores, int verbose){ 
  int nb_shares = c->share_count;
  int var = nb_shares - 1;
  
  //Initializing coefficients
  uint64_t *coeffs_RPE21 = malloc((c->total_wires+1) * sizeof(uint64_t));
  for (int i = 0; i <= c->total_wires; i++) {
    coeffs_RPE21[i] = 0;
  }
  
  //Create the output subset for J1 and J2 that we will update thanks to the 
  //function update_output_set.
  int J21[required_output];
  int J12[nb_shares -1];
  for (int i = 0; i < required_output; i++){
    J12[i] = c->length + i;
    J21[i] = J12[i] + nb_shares;
  }
  
  for (int i = required_output; i < var; i++){
    J12[i] = c->length + i;
  }
  
  //Compute incompressibles tuples but with the output index.
  Trie *incompr_tuples = compute_incompr_tuples_RPE_copy(c, required_output + 1,
                                                         coeff_max, var, 
                                                         required_output,
                                                         cores,
                                                         verbose);
  //Useful for update_output_set.
  int index_modify = required_output - 1;
  
  do{
    //Initializing coefficients for write the errors coefficients of 
    //the property RPE2 for J1.
    uint64_t coeffs_output_I1[c->total_wires+1];
    for (int i = 0; i <= c->total_wires; i++) {
      coeffs_output_I1[i] = 0;
    }
    
    //Create the trie of incompressibles errors tuples without output in J2 from 
    //|incompr_tuples| and who can simulate the output in J2.
    Trie *incompr_tuples_I2 = derive_trie_from_subset(incompr_tuples, J21,
                                                      required_output, 
                                                      c->length + c->share_count,
                                                      c->secret_count,
                                                      c->deps->length,
                                                      coeff_max + c->share_count);
    
    
    Trie **incompr_tuples_I1 = malloc(nb_shares * sizeof(*incompr_tuples_I1)); 
    int idx = 0;
    do{
      //Create the trie of incompressibles errors tuples without output in J1 
      //from |incomrp_tuples_I2| and who can simulate the output in J1 and J2.
      incompr_tuples_I1[idx] = derive_trie_from_subset(incompr_tuples_I2, 
                                                       J12,
                                                       var, 
                                                       c->length,
                                                       c->secret_count,
                                                       c->deps->length,
                                                       coeff_max);
      
      idx += 1;                                                        
    }  while (update_output_set(c, var, nb_shares - 2, J12, 0));
    
    //Generating RPE2 failures from |incompr_tuples_I1|, and computing 
    //coefficients.
    compute_failures_from_incompressibles_RPE2_single(c, incompr_tuples_I1,
                                                      nb_shares, coeff_max, 
                                                      verbose, 
                                                      coeffs_output_I1);
    
    
    for (int i = 0; i < var; i++){
      free_trie(incompr_tuples_I1[i]);
    }
    free(incompr_tuples_I1); 
    free_trie (incompr_tuples_I2);
    
    // RPE1 : Compute the max failure between all the J2 output set.
    for (int i = 0; i <= c->total_wires; i++){
        coeffs_RPE21[i] = max(coeffs_RPE21[i], coeffs_output_I1[i]);
    }
    
    for (int i = 0; i < var; i++){
      J12[i] = c->length + i;
    }
  } while (update_output_set(c, required_output, index_modify, J21, 1)); 
  
  free_trie(incompr_tuples);
  return coeffs_RPE21;
} 
  
/* Case 4 : |J1| = nb_shares - 1 and |J2| = nb_shares - 1 */  
uint64_t *compute_RPE22_coeffs(const Circuit *c, int coeff_max, 
                               int required_output, int cores, int verbose){   
  int nb_shares = c->share_count;
  int var = nb_shares - 1;
  
  //Initializing coefficients
  uint64_t *coeffs_RPE22 = malloc((c->total_wires+1) * sizeof(uint64_t));
  for (int i = 0; i <= c->total_wires; i++){
    coeffs_RPE22[i] = 0;
  }
  
  //Create the output subset for J1 and J2 that we will update thanks to the 
  //function update_output_set.
  int J12[nb_shares -1];
  int J22[nb_shares -1];
  for (int i = 0; i < var; i++){
    J12[i] = c->length + i;
    J22[i] = J12[i] + nb_shares;
  }
  
  //Compute incompressibles tuples but with the output index.
  Trie *incompr_tuples = compute_incompr_tuples_RPE_copy(c, required_output + 1,
                                                         coeff_max, var, 
                                                         var, cores, verbose);
  
  //In this case, we will use the property RPE2 on the two output values
  //The idea is to create big output set with (nb_shares - 1) index output 
  //from the first output value and (nb_shares - 1) index output from the second
  //output value. In order to do this, we have nb_shares output set of cardinal 
  //(nb_shares - 1) for the first output value, and nb_shares  for the 
  //second output value.
  //So we have nb_shares * nb_shares big output set. Then we compute all the 
  //error tuples obtained for all the output set, in which we remove the output.
  //We obtained an array of Trie of size nb_shares * nb_shares, in each lines, 
  //we have the trie obtained for one of the output set with the output index 
  //removed.
  //Finally we search error tuples common to all the trie, if it's the case, 
  //the tuples is an error and we update the coefficients, adding the 
  //coefficients in link with this error tuples  
  int var_sq = (var + 1) * (var + 1);
  int output_set[2 * var];
  Trie **incompr_tuples_output = malloc(var_sq * sizeof(*incompr_tuples_output));
  int idx = 0;
  //Creation of all the trie, that we will add to incompr_tuples_output.
  do{
    do{
      
      memcpy(output_set, J12, var * sizeof(*output_set));
      memcpy(&output_set[var], J22, var * sizeof(*output_set));
      
      incompr_tuples_output[idx] = derive_trie_from_subset(incompr_tuples,
                                                           output_set,
                                                           var_sq, 
                                                           c->length,
                                                           c->secret_count,
                                                           c->deps->length,
                                                           coeff_max);
                                                                                                                
      idx += 1;          
    }  while (update_output_set(c, var, nb_shares - 2, J22, 1));
    
    for (int i = 0; i < var; i++){
      J22[i] = c->length + nb_shares + i;
    } 
       
  } while (update_output_set(c, var, nb_shares - 2, J12, 0)); 
  
  //Generating RPE2 failures from |incompr_tuples_output|, and computing 
  //coefficients.
  compute_failures_from_incompressibles_RPE2_single(c, incompr_tuples_output, 
                                                    var_sq, coeff_max, verbose,
                                                    coeffs_RPE22);
  

  
  for (int i = 0; i < var_sq; i++){
    free_trie(incompr_tuples_output[i]);
  }
  free(incompr_tuples_output);
  free_trie(incompr_tuples);
  return coeffs_RPE22;
}

/*
Compute the incompressibles tuples and compute the error coefficients for the 
RPE property and only for the refresh gadget.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int coeff_max : The maximal coefficient that we have to compute.
  -int required_outputs : Number of outputs required in each tuple
  -int cores : if cores != 1, we parallelize. 
  -int verbose : Set a level of verbosity.  
*/
void compute_RPE_coeffs_incompr_refresh(const Circuit *c, int coeff_max, 
                                        int required_output, int cores, 
                                        int verbose){
  Trie *incompr_tuples;
  int nb_shares = c->share_count;
  int var = nb_shares - 1;
  
  /* Case 1 : |J| <= t */
  
  //Initializing coefficients
  uint64_t coeffs_RPE1[c->total_wires+1];
  for (int i = 0; i <= c->total_wires; i++) {
    coeffs_RPE1[i] = 0;
  }
  
  int J1[required_output];
  
  //Compute incompressibles tuples but with the output index.
  incompr_tuples = compute_incompr_tuples_RPE_copy(c, required_output + 1,
                                                   coeff_max,  
                                                   required_output, 
                                                   0,
                                                   cores, 
                                                   verbose);
                                                   
                                                   
  //Create the output subset for J1 and J2 that we will update thanks to the 
  //function update_output_set.
  for (int i = 0; i < required_output; i++)
    J1[i] = c->length + i;
  
  
  //Useful for update_output_set.
  int index_modify = required_output - 1;
  
  do {
    //Create the trie of incompressibles errors tuples without output and who can 
    //simulate the output in J1 and J2.
    Trie *incompr_tuples_output = derive_trie_from_subset(incompr_tuples, 
                                                          J1, 
                                                          required_output, 
                                                          c->length, 
                                                          c->secret_count,
                                                          c->deps->length,
                                                          coeff_max);                                                                        
      
    //Initializing coefficients for write the errors coefficients of 
    //|incompr_tuples_output|.
    uint64_t coeffs_output[c->total_wires+1];
    for (int i = 0; i <= c->total_wires; i++) {
      coeffs_output[i] = 0;
    }      
      
    // Generating failures from incompressible tuples, and computing coefficients.
    compute_failures_from_incompressibles_RPC(c, incompr_tuples_output, NULL, 
                                              coeff_max, verbose, 
                                              coeffs_output, false);
      
    // RPE1 : Compute the max failures of all the output set.
    for (int i = 0; i <= c->total_wires; i++){
      coeffs_RPE1[i] = max(coeffs_RPE1[i], coeffs_output[i]);
    }
      
    free_trie(incompr_tuples_output);
            
  } while (update_output_set(c, required_output, index_modify, J1, 0)); 
  
  free_trie(incompr_tuples);
  
  //Printing the coefficients
  printf("\nCoeffs RPE1 : \n");
  printf("I = [");
  for (int i = 0; i < c->total_wires; i++){
    printf("%lu, ", coeffs_RPE1[i]);
  }
  printf("%lu]\n", coeffs_RPE1[c->total_wires]);                
  
  
  /* Case 2 : |J| = n - 1 */
  
  int J2[nb_shares -1];
  
  //Initializing coefficients
  uint64_t coeffs_RPE2[c->total_wires+1];
  for (int i = 0; i <= c->total_wires; i++){
    coeffs_RPE2[i] = 0;
  }
  
  //Create the output subset for J1 and J2 that we will update thanks to the 
  //function update_output_set.
  for (int i = 0; i < var; i++)
    J2[i] = c->length + i;
  
  
  //Compute incompressibles tuples but with the output index.
  incompr_tuples = compute_incompr_tuples_RPE_copy(c, required_output + 1,
                                                   coeff_max,  
                                                   var, 
                                                   0,
                                                   cores, 
                                                   verbose);
  
  Trie **incompr_tuples_J2 = malloc(nb_shares * sizeof(*incompr_tuples_J2)); 
  int idx = 0;
  do{
    //Create the trie of incompressibles errors tuples without output in J1 
    //from |incomrp_tuples_I2| and who can simulate the output in J1 and J2.
    incompr_tuples_J2[idx] = derive_trie_from_subset(incompr_tuples, 
                                                     J2,
                                                     var, 
                                                     c->length,
                                                     c->secret_count,
                                                     c->deps->length,
                                                     coeff_max);
      
    idx += 1;                                                        
  }  while (update_output_set(c, var, var - 1, J2, 0));
  
  //Generating RPE2 failures from |incompr_tuples_I1|, and computing 
  //coefficients.
  compute_failures_from_incompressibles_RPE2_single(c, incompr_tuples_J2,
                                                    nb_shares, coeff_max, 
                                                    verbose, 
                                                    coeffs_RPE2);
                                                    
  for (int i = 0; i < idx; i++){
    free_trie(incompr_tuples_J2[i]);
  }
  free(incompr_tuples_J2); 
  free_trie(incompr_tuples);

  //Printing the coefficients
  printf("\nCoeffs RPE2 : \n");
  printf("I = [");
  for (int i = 0; i < c->total_wires; i++){
    printf("%lu, ", coeffs_RPE2[i]);
  }
  printf("%lu]\n\n", coeffs_RPE2[c->total_wires]);
  
  // Computing leakage probability from coefficients  
  double p[2];
  for (int i = 0; i < 2; i++){
    int min_max = i == 0 ? -1 : 1;
    double p_arr[2] = {
        compute_leakage_proba(coeffs_RPE1, coeff_max,
                              c->total_wires+1, min_max, false),
        compute_leakage_proba(coeffs_RPE2, coeff_max,
                              c->total_wires+1, min_max, false) 
    };
    p[i] = min_arr(p_arr, 2);
  }
  
  printf("pmax = %.10f -- log2(pmax) = %.10f\n", p[0], log2(p[0]));
  printf("pmin = %.10f -- log2(pmin) = %.10f\n", p[1], log2(p[1]));
  printf("\n"); 
}

//Struture used to parallelize the function |build_incompr_RPE|.
struct build_incompr_RPE_args {
  const Circuit* c;
  VarVector** secrets;
  VarVector** randoms;
  int t_in;
  int max_size;
  bool include_outputs;
  int required_outputs;
  int secret_idx;
  int cores;
  pthread_mutex_t mutex;
  int debug;
};

// function to start thread on a call of |build_incompr_RPE| with the args 
//contained in |void_args|.
void *start_thread_build_incompr_RPE (void *void_args){
  struct build_incompr_RPE_args *args = 
    (struct build_incompr_RPE_args *) void_args;

  Trie *incompr_tuples = 
    build_incompr_tuples_arith_RPE(args->c, args->secrets, args->randoms, args->t_in,
    args->max_size, args->include_outputs, args->required_outputs, 
    args->secret_idx, args->cores, &(args->mutex), args->debug);
  
  free(args);
  return incompr_tuples;
  
}

/*struct update_coeffs_RPC_args {
  const Circuit *c; 
  Trie *incompr_tuples; 
  int *output_set;
  int len_output;
  int coeff_max; 
  uint64_t *coeffs;
  pthread_mutex_t mutex; 
  int verbose;  
};

void *start_thread_update_coeffs_RPC (void *void_args){
  struct update_coeffs_RPC_args *args = 
    (struct update_coeffs_RPC_args *) void_args; 
  
  update_coeffs_RPC(args->c, args->incompr_tuples, args->output_set, 
                    args->len_output, args->coeff_max, args->coeffs, 
                    &(args->mutex), args->verbose);
  
  free(args->output_set);  
  free(args);
  
  return NULL;
}

struct update_coeffs_RPE_inter_args {
  const Circuit *c; 
  Trie *incompr_tuples1;
  Trie * incompr_tuples2; 
  int *output_set;
  int len_output;
  int coeff_max; 
  uint64_t *coeffs;
  pthread_mutex_t mutex; 
  int verbose;  
};

void *start_thread_update_coeffs_RPE_inter (void *void_args){
  struct update_coeffs_RPE_inter_args *args = 
    (struct update_coeffs_RPE_inter_args *) void_args;
  
  update_coeffs_RPE_inter(args->c, args->incompr_tuples1, args->incompr_tuples2, 
                          args->output_set, args->len_output, args->coeff_max, 
                          args->coeffs, &(args->mutex), args->verbose);
  
  free(args->output_set);  
  free(args);
  
  return NULL;
}*/



// Printing the RPE1/RPE2 coefficients according to |nb_RPE| for the first 
// secret value I1, the second secret value I2 
// and the intersection of I1 and I2.
void print_coeffs_RPE(const Circuit *c, uint64_t *coeffs_I1, uint64_t *coeffs_I2, 
                      uint64_t *coeffs_I1_and_I2, int nb_RPE){
  
  printf("\nCoeffs RPE%d : \n", nb_RPE);
  printf("I1 = [");
  for (int i = 0; i < c->total_wires; i++){
    printf("%lu, ", coeffs_I1[i]);
  }
  printf("%lu]\n", coeffs_I1[c->total_wires]);
  
  printf("I2 = [");
  for (int i = 0; i < c->total_wires; i++){
    printf("%lu, ", coeffs_I2[i]);
  }
  printf("%lu]\n", coeffs_I2[c->total_wires]);
  
  printf("I1 and I2 = [");
  for (int i = 0; i < c->total_wires; i++){
    printf("%lu, ", coeffs_I1_and_I2[i]);
  }
  printf("%lu]\n\n", coeffs_I1_and_I2[c->total_wires]);
}


/* RPE1 */
/*
Compute the RPE1 coeffs for addition and multiplication gadget.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int coeff_max : The maximal coefficient that we have to compute.
  -bool include_output : if true, we can add output probe int the tuple.
  -int required_output : Number of outputs required in each tuple.
  -int cores : if cores != 1, we parallelize. 
  -int verbose : Set a level of verbosity.
*/
uint64_t** compute_RPE1_coeffs_incompr(const Circuit *c, int coeff_max, 
                                       bool include_output, int required_output,
                                       int cores, int verbose){  
  Trie *incompr_tuples_I1;
  Trie *incompr_tuples_I2;
  VarVector** secrets;
  VarVector** randoms;
  Trie **incompr_tuples_tab; 
  
  int total_secrets    = c->secret_count * c->share_count;
  int random_count     = c->random_count;
  int t_in = required_output + 1;
  
  // Initializing coefficients
  uint64_t *coeffs_RPE1_I1 = calloc((c->total_wires+1), sizeof(uint64_t));

  uint64_t *coeffs_RPE1_I2 = calloc((c->total_wires+1), sizeof(uint64_t));

  uint64_t *coeffs_RPE1_I1_and_I2 = calloc((c->total_wires+1), sizeof(uint64_t));
  
  
  //Case of the multiplication gadget.
  if(c->contains_mults){
    incompr_tuples_tab = 
      compute_incompr_tuples_mult_RPE_arith(c , coeff_max, include_output,
                                            required_output, t_in, cores, 
                                            verbose);
    
    incompr_tuples_I1 = incompr_tuples_tab[0];
    incompr_tuples_I2 = incompr_tuples_tab[1];
    free(incompr_tuples_tab);
  }
  
  //Case of the addition gadget.
  else {
    build_dependency_arrays_arith(c, &secrets, &randoms, include_output, verbose);
    if(cores == 1){
      incompr_tuples_I1 = build_incompr_tuples_arith_RPE(c, secrets, randoms, t_in, 
                                                   coeff_max, include_output, 
                                                   required_output, 0, cores, 
                                                   NULL, verbose);

      incompr_tuples_I2 = build_incompr_tuples_arith_RPE(c, secrets, randoms, t_in, 
                                                   coeff_max, include_output, 
                                                   required_output, 1, cores, 
                                                   NULL, verbose);
    }    
    else {
      pthread_t threads;
      pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
      struct build_incompr_RPE_args* args1 = malloc (sizeof(*args1));
      args1->c = c;
      args1->secrets = secrets;
      args1->randoms = randoms;
      args1->t_in = t_in;
      args1->max_size = coeff_max;
      args1->include_outputs = include_output;
      args1->required_outputs = required_output;
      args1->secret_idx = 0;
      args1->cores = cores;
      args1->mutex = mutex;
      args1->debug = verbose;
      
      void *return_value1;
      int ret1 = pthread_create(&threads, NULL, 
                                start_thread_build_incompr_RPE, (void *)args1);

      
      incompr_tuples_I2 = build_incompr_tuples_arith_RPE(c, secrets, randoms, t_in, 
                                                   coeff_max, include_output, 
                                                   required_output, 1, 
                                                   cores, 
                                                   &mutex, verbose);
      
      if (ret1){
        incompr_tuples_I1 = build_incompr_tuples_arith_RPE(c, secrets, randoms, t_in, 
                                                     coeff_max, include_output, 
                                                     required_output, 0, cores, 
                                                     NULL, verbose); 
      }
      
      else{               
        pthread_join(threads, &return_value1);
        incompr_tuples_I1 = (Trie *) return_value1;
      }
    }                                             
  }
 
  //We just have build the incompressibles tuples for the two input values.
  
  // Generating failures from incompressible tuples, and computing coefficients.
  int output_set[required_output];                          
  for (int i = 0 ; i < required_output; i ++){
    output_set[i] = c->length + i;
  }
  
  //if (cores == 1){
    do {
      update_coeffs_RPC(c, incompr_tuples_I1, output_set, required_output, 
                        coeff_max, coeffs_RPE1_I1, NULL, verbose);
      update_coeffs_RPC(c, incompr_tuples_I2, output_set, required_output, 
                        coeff_max, coeffs_RPE1_I2, NULL, verbose);
      update_coeffs_RPE_inter(c, incompr_tuples_I1, incompr_tuples_I2, output_set, 
                              required_output, coeff_max, coeffs_RPE1_I1_and_I2, 
                              NULL, verbose);                 
                                                   
    } while (update_output_set(c, required_output, required_output - 1, output_set, 0));
  //}
  
  
  /* 
  
  This parallelization doesn't work, because of stack space memory per thread 
  I think. 
  
  else {
    int cores_ite = 0;
    //int cores_per_para = (cores - 3 * required_output) / (3 * required_output);
    //if (cores_per_para < 0) cores_per_para = 0;
    pthread_t threads[cores];
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    do {    
      int *output_set_copy = malloc(required_output * sizeof(*output_set_copy));
      memcpy(output_set_copy, output_set, 
             required_output * sizeof(*output_set_copy));
      
      struct update_coeffs_RPC_args *args1 = malloc(sizeof(*args1));
      args1->c = c;
      args1->incompr_tuples = incompr_tuples_I1;
      args1->output_set = output_set_copy;
      args1->len_output = required_output;
      args1->coeff_max = coeff_max;
      args1->coeffs = coeffs_RPE1_I1;
      args1->mutex = mutex;
      args1->verbose = verbose;
      
      if (cores_ite == cores){
        for (int i = 0; i < cores; i++){
          pthread_join(threads[i],NULL);
        }
        cores_ite = 0;
      }
      
      output_set_copy = malloc(required_output * sizeof(*output_set_copy));
      memcpy(output_set_copy, output_set, 
             required_output * sizeof(*output_set_copy));
      
      pthread_create(&threads[cores_ite], NULL, start_thread_update_coeffs_RPC, 
                     args1);
      cores_ite += 1;
      
      struct update_coeffs_RPC_args *args2 = malloc(sizeof(*args2));
      args2->c = c;
      args2->incompr_tuples = incompr_tuples_I2;
      args2->output_set = output_set_copy;
      args2->len_output = required_output;
      args2->coeff_max = coeff_max;
      args2->coeffs = coeffs_RPE1_I2;
      args2->mutex = mutex;
      args2->verbose = verbose;
           
      if (cores_ite == cores){
        for (int i = 0; i < cores; i++) pthread_join(threads[i],NULL);
        cores_ite = 0;
      }
      pthread_create(&threads[cores_ite], NULL, start_thread_update_coeffs_RPC, 
                     args2);
      cores_ite += 1;
      
      
      output_set_copy = malloc(required_output * sizeof(*output_set_copy));
      memcpy(output_set_copy, output_set, 
             required_output * sizeof(*output_set_copy));
      
      struct update_coeffs_RPE_inter_args *args3 = malloc(sizeof(*args3));
      args3->c = c;
      args3->incompr_tuples1 = incompr_tuples_I1;
      args3->incompr_tuples2 = incompr_tuples_I2;
      args3->output_set = output_set_copy;
      args3->len_output = required_output;
      args3->coeff_max = coeff_max;
      args3->coeffs = coeffs_RPE1_I1_and_I2;
      args3->mutex = mutex;
      args3->verbose = verbose;
           
      if (cores_ite == cores){
        for (int i = 0; i < cores; i++) pthread_join(threads[i],NULL);
        cores_ite = 0;
      }
      pthread_create(&threads[cores_ite], NULL, 
                     start_thread_update_coeffs_RPE_inter, args3);
      cores_ite += 1;
                      
                                                   
    } while (update_output_set(c, required_output, required_output - 1, output_set, 0));    
    for (int i = 0; i < cores_ite; i++)
      pthread_join(threads[i], NULL);
    free(output_set);
  }*/
  
  free_trie(incompr_tuples_I1);
  free_trie(incompr_tuples_I2);
  
  if (!c->contains_mults){
    for (int i = 0; i < total_secrets; i++) {
      VarVector_free(secrets[i]);
    }
    free(secrets);
    for (int i = total_secrets; i < total_secrets + random_count; i++) {
      VarVector_free(randoms[i]);
    }
    free(randoms);
  }
  
  uint64_t **coeffs_RPE1 = malloc(3 * sizeof(*coeffs_RPE1));
  coeffs_RPE1[0] = coeffs_RPE1_I1;
  coeffs_RPE1[1] = coeffs_RPE1_I2;
  coeffs_RPE1[2] = coeffs_RPE1_I1_and_I2;
  
  return coeffs_RPE1;
}  



/*struct derive_trie_from_subset_args {
  Trie *incompr_tuples;
  int *subset; 
  int len_subset; 
  int circuit_length; 
  int secret_count;
  int idx_output_end; 
  int coeff_max;
  int idx;
  Trie **incompr_tuples_list;
};

void *start_thread_der_fr_sub (void *void_args){
  struct derive_trie_from_subset_args *args = 
    (struct derive_trie_from_subset_args *) void_args;
    
  Trie * icpr_tuples = derive_trie_from_subset(args->incompr_tuples, 
                                               args->subset,
                                               args->len_subset,
                                               args->circuit_length,
                                               args->secret_count,
                                               args->idx_output_end,
                                               args->coeff_max);
  
  (args->incompr_tuples_list)[args->idx] = icpr_tuples;
  
  free(args->subset);
  free (args);
  return NULL;
}*/


/* RPE2 */
/*
Compute the RPE2 coeffs for addition and multiplication gadget.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int coeff_max : The maximal coefficient that we have to compute.
  -bool include_output : if true, we can add output probe int the tuple.
  -int required_output : Number of outputs required in each tuple.
  -int cores : if cores != 1, we parallelize. 
  -int verbose : Set a level of verbosity.
*/
uint64_t **compute_RPE2_coeffs_incompr(const Circuit *c, int coeff_max, 
                                       bool include_output, int required_output,
                                       int cores, int verbose){ 
  Trie *incompr_tuples_I1;
  Trie *incompr_tuples_I2;
  VarVector** secrets;
  VarVector** randoms;
  Trie **incompr_tuples_tab;
  
  int total_secrets    = c->secret_count * c->share_count;
  int random_count     = c->random_count;
  int nb_output        = c->deps->length - c->length;
  int t_in = required_output + 1;
  
  // Initializing coefficients
  uint64_t *coeffs_RPE2_I1 = calloc((c->total_wires+1), sizeof(uint64_t));

  uint64_t *coeffs_RPE2_I2 = calloc((c->total_wires+1), sizeof(uint64_t));
  
  uint64_t *coeffs_RPE2_I1_and_I2 = calloc((c->total_wires+1), sizeof(uint64_t));
   
  required_output = c->share_count - 1;
  
  //To compute the RPE2 property, I do a list of Trie os size nb_shares 
  //(because we have nb_shares possible output set). For an output set, we 
  //create the trie of incompressible tuples obtained by searching the tuples in
  //incompr_tuples_Ix (x = 1 or 2) containing in his output index a subset of 
  //the output set, if it s the case, we add the tuples to the list after 
  //removing the output index. 
  Trie **incompr_tuples_I1_list = malloc (nb_output * sizeof(*incompr_tuples_I1_list));
  Trie **incompr_tuples_I2_list = malloc (nb_output * sizeof(*incompr_tuples_I2_list));
  int idx = 0;
  int output_set_RPE2[required_output];
  for (int i = 0; i < required_output; i++)
    output_set_RPE2[i] = c->length + i;
  
  //Creating the list of incompressible tuples with output for the first input 
  //values an dthe second input values.
  if (c->contains_mults){
    incompr_tuples_tab = 
      compute_incompr_tuples_mult_RPE_arith(c , coeff_max, include_output,
                                            required_output, t_in, cores, 
                                            verbose);
                                      
    incompr_tuples_I1 = incompr_tuples_tab[0];
    incompr_tuples_I2 = incompr_tuples_tab[1];
    
    free(incompr_tuples_tab);
  }
  
  else {
    build_dependency_arrays_arith(c, &secrets, &randoms, include_output, verbose);
    
    if (cores == 1){
    
      incompr_tuples_I1 = build_incompr_tuples_arith_RPE(c, secrets, randoms, t_in,
                                                   coeff_max, include_output,
                                                   required_output, 0, cores, 
                                                   NULL, verbose);

      incompr_tuples_I2 = build_incompr_tuples_arith_RPE(c, secrets, randoms, t_in, 
                                                   coeff_max, include_output, 
                                                   required_output, 1, cores, 
                                                   NULL, verbose);
    }
    
    else{
      pthread_t threads;
      pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
      struct build_incompr_RPE_args* args = malloc (sizeof(*args));
      args->c = c;
      args->secrets = secrets;
      args->randoms = randoms;
      args->t_in = t_in;
      args->max_size = coeff_max;
      args->include_outputs = include_output;
      args->required_outputs = required_output;
      args->secret_idx = 0;
      args->cores = cores;
      args->mutex = mutex;
      args->debug = verbose;
      
      void *return_value1;
      int ret = pthread_create(&threads, NULL, start_thread_build_incompr_RPE, 
                               (void *)args);
                     
      incompr_tuples_I2 = build_incompr_tuples_arith_RPE(c, secrets, randoms, t_in,
                                                     coeff_max, include_output, 
                                                     required_output, 1, cores, 
                                                     &mutex, verbose);  
      if(ret) {
        incompr_tuples_I1 = build_incompr_tuples_arith_RPE(c, secrets, randoms, t_in,
                                                     coeff_max, include_output, 
                                                     required_output, 0, cores, 
                                                     &mutex, verbose);
      }
      else {                              
        pthread_join(threads, &return_value1);
        incompr_tuples_I1 = (Trie *) return_value1;
      }
    }                                               
  }
  
  
  //Fill the two list incompr_tuples_I1_list and incompr_tuples_I2_list.
  //if (cores == 1){
  do {
      Trie *icpr_tpls_I1 = derive_trie_from_subset(incompr_tuples_I1, output_set_RPE2,
                                                   required_output, c->length, 
                                                   c->secret_count, 
                                                   c->deps->length,
                                                   coeff_max);
                                                   
      Trie *icpr_tpls_I2 = derive_trie_from_subset(incompr_tuples_I2, output_set_RPE2,
                                                   required_output, c->length, 
                                                   c->secret_count, 
                                                   c->deps->length,
                                                   coeff_max);
        
      incompr_tuples_I1_list[idx] = icpr_tpls_I1;
      incompr_tuples_I2_list[idx] = icpr_tpls_I2;
    
      idx += 1;
          
  } while (update_output_set(c, required_output, required_output - 1, 
                               output_set_RPE2,0));                                                                                          
  //}
  
  /*else {
    pthread_t threads_der[cores];
    int cores_rec = 0;
    
    do {
      
      int *subset_cpy = malloc (required_output * sizeof(*subset_cpy));
      memcpy(subset_cpy, output_set_RPE2, required_output * sizeof(*subset_cpy));  
    
      struct derive_trie_from_subset_args *args1 = malloc(sizeof(*args1));
      args1->incompr_tuples = incompr_tuples_I1;
      args1->subset = subset_cpy; 
      args1->len_subset = required_output; 
      args1->circuit_length = c->length; 
      args1->secret_count = c->secret_count;
      args1->idx_output_end = c->deps->length; 
      args1->coeff_max = coeff_max;
      args1->idx = idx;
      args1->incompr_tuples_list = incompr_tuples_I1_list;
      
      pthread_create(&threads_der[cores_rec], NULL, start_thread_der_fr_sub, 
                     args1);
      cores_rec += 1;
      if(cores_rec == cores){
        for (int i = 0; i < cores; i++)
          pthread_join(threads_der[i], NULL);
        cores_rec = 0;
      }
      
      int *subset_cpy2 = malloc (required_output * sizeof(*subset_cpy2));
      memcpy(subset_cpy2, output_set_RPE2, required_output * sizeof(*subset_cpy2));  
      
      struct derive_trie_from_subset_args *args2 = malloc(sizeof(*args2));
      args2->incompr_tuples = incompr_tuples_I2;
      args2->subset = subset_cpy2; 
      args2->len_subset = required_output; 
      args2->circuit_length = c->length; 
      args2->secret_count = c->secret_count;
      args2->idx_output_end = c->deps->length; 
      args2->coeff_max = coeff_max;
      args2->idx = idx;
      args2->incompr_tuples_list = incompr_tuples_I2_list;
      
      pthread_create(&threads_der[cores_rec], NULL, start_thread_der_fr_sub, 
                     args2);
      cores_rec += 1;
      if(cores_rec == cores){
        for (int i = 0; i < cores; i++)
          pthread_join(threads_der[i], NULL);
        cores_rec = 0;
      }
    
      idx += 1;
          
    } while (update_output_set(c, required_output, required_output - 1, 
                               output_set_RPE2,0));
    
    for (int i = 0; i < cores_rec; i++){
      pthread_join(threads_der[i], NULL);
    }
  }*/
  
  // Generating failures from incompressible tuples, and computing coefficients.                        
  compute_failures_from_incompressibles_RPE2_parallel(c, incompr_tuples_I1_list, 
                                                      incompr_tuples_I2_list, 
                                                      coeff_max, verbose, 
                                                      coeffs_RPE2_I1,
                                                      coeffs_RPE2_I2, 
                                                      coeffs_RPE2_I1_and_I2,
                                                      cores);                                                                                      
  
  //Freeing stuffs
  free_trie(incompr_tuples_I1);
  free_trie(incompr_tuples_I2);
  
  for (int idx = 0; idx < nb_output; idx++){
    free_trie(incompr_tuples_I1_list[idx]);
    free_trie(incompr_tuples_I2_list[idx]);
  } 
  
  if (!c->contains_mults){
    for (int i = 0; i < total_secrets; i++) {
      VarVector_free(secrets[i]);
    }
    free(secrets);
  
    for (int i = total_secrets; i < total_secrets + random_count; i++) {
      VarVector_free(randoms[i]);
    }
    free(randoms);
  }
  
  free(incompr_tuples_I1_list);
  free(incompr_tuples_I2_list);
  
  uint64_t **coeffs_RPE2 = malloc(3 * sizeof(*coeffs_RPE2));
  coeffs_RPE2[0] = coeffs_RPE2_I1;
  coeffs_RPE2[1] = coeffs_RPE2_I2;
  coeffs_RPE2[2] = coeffs_RPE2_I1_and_I2;
  
  return coeffs_RPE2;
}


//Structure for the parallelization of the computation of the RPE1 coefficients.
struct comp_RPE_args {
  const Circuit* c; 
  int coeff_max;
  bool include_output; 
  int required_output;
  int cores;
  int verbose;
};


//Start the parallelization of the computation of RPE1 coefficients with 
//arguments find in |void_args|.
void *start_thread_comp_RPE(void *void_args){
  struct comp_RPE_args *args = (struct comp_RPE_args *) void_args;
  
  uint64_t **coeffs;
  coeffs = compute_RPE1_coeffs_incompr(args->c, args->coeff_max, 
                                       args->include_output, 
                                       args->required_output, args->cores, 
                                       args->verbose);
  
  free(args);
  return coeffs;
}



/*
Compute the incompressibles tuples and compute the error coefficients for the 
RPE property.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int coeff_max : The maximal coefficient that we have to compute.
  -bool include_output : if true, we can include outputs index in the array 
                          secrets and randoms.
  -int required_output : Number of outputs required in each tuple
  -int cores : if cores != 1, we parallelize. 
  -int verbose : Set a level of verbosity.  
*/
void compute_RPE_coeffs_incompr_arith (const Circuit* c, int coeff_max, 
                                 bool include_output , int required_output,
                                 int cores, int verbose){
  
  //Compute the optimal HASH_MASK for the hash_table.
  determine_HASH_MASK(c->length, coeff_max); 
  
  //Copy gadget or refresh gadget case.
  if (c->secret_count == 1){ 
    
    if (c->output_count == 2) {
      compute_RPE_coeffs_incompr_copy(c, coeff_max, required_output, cores, 
                                      verbose);
    }
    
    else {
      compute_RPE_coeffs_incompr_refresh(c, coeff_max, required_output, cores, 
                                         verbose); 
    }
    
    return;
  }
  
  uint64_t **coeffs_RPE1;
  uint64_t **coeffs_RPE2;
  
  if (cores == 1){
  coeffs_RPE1 = compute_RPE1_coeffs_incompr(c, coeff_max, include_output, 
                                            required_output, cores, verbose);
                              
  coeffs_RPE2 = compute_RPE2_coeffs_incompr(c, coeff_max, include_output, 
                                            required_output, cores, verbose);
  }
  
  else {
    pthread_t threads;
    
    struct comp_RPE_args *args1 = malloc(sizeof(*args1));
    args1->c = c;
    args1->coeff_max = coeff_max;
    args1->include_output = include_output; 
    args1->required_output = required_output;
    args1->cores = cores;
    args1->verbose = verbose;
    
    int ret1 = pthread_create(&threads, NULL, start_thread_comp_RPE,
                              (void *)args1);
    
    coeffs_RPE2 = compute_RPE2_coeffs_incompr(c, coeff_max, include_output, 
                                              required_output, 
                                              cores , verbose);
    
    if (ret1){
      coeffs_RPE1 = compute_RPE1_coeffs_incompr(c, coeff_max, include_output, 
                                                required_output, cores, 
                                                verbose);
    }
    
    else{
      void *return_value;
      pthread_join(threads, &return_value);
      coeffs_RPE1 = (uint64_t **) return_value;
    }   
  }
  
  //Printing coefficients
  print_coeffs_RPE(c, coeffs_RPE1[0], coeffs_RPE1[1], coeffs_RPE1[2], 1);
  
  print_coeffs_RPE(c, coeffs_RPE2[0], coeffs_RPE2[1], coeffs_RPE2[2], 2);
  
  // Computing leakage probability from coefficients
  double p[2];
  for (int i = 0; i < 2; i++) {
    // i == 0 --> replace last coeffs by 0
    // i == 1 --> replace last coeffs by (n choose k)
    int min_max = i == 0 ? -1 : 1;
    double p_arr[6] = {
        compute_leakage_proba(coeffs_RPE1[0], coeff_max,
                              c->total_wires+1, min_max, false),
        compute_leakage_proba(coeffs_RPE1[1], coeff_max,
                              c->total_wires+1, min_max, false),
        compute_leakage_proba(coeffs_RPE1[2], coeff_max,
                              c->total_wires+1, min_max, true),
        compute_leakage_proba(coeffs_RPE2[0], coeff_max,
                              c->total_wires+1, min_max, false),
        compute_leakage_proba(coeffs_RPE2[1], coeff_max,
                              c->total_wires+1, min_max, false),
        compute_leakage_proba(coeffs_RPE2[2], coeff_max,
                              c->total_wires+1, min_max, true) 
    };
    p[i] = min_arr(p_arr, 6);
  }
  
  printf("pmax = %.10f -- log2(pmax) = %.10f\n", p[0], log2(p[0]));
  printf("pmin = %.10f -- log2(pmin) = %.10f\n", p[1], log2(p[1]));
  printf("\n"); 
  
  for (int i = 0 ; i < 3; i++){
    free(coeffs_RPE1[i]);
    free(coeffs_RPE2[i]);
  }
  
  free(coeffs_RPE1);
  free(coeffs_RPE2);
}
