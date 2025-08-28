#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include "constructive_arith.h"
#include "constructive-mult_arith.h"
#include "circuit.h"
#include "combinations.h"
#include "list_tuples.h"
#include "verification_rules.h"
#include "trie.h"
#include "failures_from_incompr.h"
#include "constructive-mult-compo.h"
#include "vectors.h"

#define INIT_ARR_SIZE 10
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) > (b) ? (b) : (a))


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
    dst[j] |= (dep[j] != 0);
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
  int total_secrets       = secret_count * share_count;
  int deps_size           = c->deps->deps_size;
  int non_mult_deps_count = c->deps->deps_size - c->deps->mult_deps->length;

  // Computing all dependencies contained in |dep_arr|.
  Dependency dep[deps_size];
  memset(dep, 0, deps_size * sizeof(*dep));
  for (int dep_idx = 0; dep_idx < dep_arr->length; dep_idx++) {
    get_deps(c, dep_arr->content[dep_idx], dep);
  }
    
  for (int i = 0; i < total_secrets; i++) {
    if (dep[i]) {
      VarVector_push(secrets[i], main_dep_idx);
    }
  }
  

  for (int i = secret_count * share_count; i < non_mult_deps_count; i++) {
    if (dep[i]) {
      VarVector_push(randoms[i], main_dep_idx);
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
  *randoms = malloc((total_secrets + random_count) * sizeof(*randoms));
  for (int i = total_secrets; i < total_secrets + random_count; i++) {
    (*randoms)[i] = VarVector_make();
  }
  
  int length = include_outputs ? deps->length : c->length;
  for (int i = length - 1; i >= 0; i--) {
    update_arrs_with_deps(c, i, deps->deps[i], *secrets, *randoms);
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


/************************************************
             Building the tuples
*************************************************/

//See function in "constructive.c"
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

//See function in "constructive.c"
static int get_initial_revealed_secret(const Circuit* c, int size,
                                       Dependency** gauss_deps,
                                       int secret_idx) {
  int non_mult_deps_count = c->deps->deps_size - c->deps->mult_deps->length;
  int nb_shares = c->share_count;                              
  int revealed_secret = 0;
  for (int i = 0; i < size; i++) {
    int is_masked = 0;
    for (int j = c->deps->first_rand_idx; j < non_mult_deps_count; j++) {
      if (gauss_deps[i][j]) {
        is_masked = 1;
        break;
      }
    }
    if (!is_masked) {
      for (int j = 0; j < nb_shares; j++){
        if (gauss_deps[i][secret_idx * nb_shares +j]) revealed_secret |= 1 << j;
      }
    }
  }
  return revealed_secret;
}

/*
Compute the bit value of the line of dependencies given in arguments.
Input : 
    -int size : The number of shares we have to consider.
    -int secret_idx : The index of the secret value we consider.
    -Dependency *gauss_line_deps : The line of dependencies.
    
Example : If we want to compute a line of dependencies l = [0,1,1, ...] for the
          first secret value (i.e secret_idx = 0) and for 3-share gadget 
          (i.e size = 3). Then the bit value of the line wille be 2¹ + 2².
*/
static int bit_value_gauss_line(int size, Dependency *gauss_line_deps, int secret_idx){
  int res = 0;
  for (int i = 0; i < size; i++){
    if (gauss_line_deps[secret_idx * size + i]) res |= 1 << i; 
  }
  return res;
}

/* Compute the available shares in |gauss_deps| that aren't masked by a random 
or in a multiplication.
Input : 
  -const Circuit *c : The arithmetic circuit.
  -int size : The number of probes in |gauss_deps|.
  -Dependency **gauss_deps : Array 2D of dependencies of probes containing in 
                             our actual tuple.
  -int secret_idx : The index of the secret value we study.                                                        
*/
static int get_initial_shares(const Circuit *c,
                              int size,
                              int nb_shares,
                              Dependency** gauss_deps,
                              int secret_idx) {
  
  DependencyList *deps = c->deps;
  int first_rand_idx = deps->first_rand_idx;
  int non_mult_deps_count = deps->deps_size - deps->mult_deps->length;
  int shares = 0;                   
  for (int i = 0; i < size; i++) {
    bool masked_line = false;
    
    for (int j = first_rand_idx;j < non_mult_deps_count; j++){
      if (gauss_deps[i][j]) {
        masked_line = true;
        break;
      }
    }
    
    bool has_mult_wire = false;
    for (int j = non_mult_deps_count; j < deps->deps_size; j++){
      if (gauss_deps[i][j]){
        has_mult_wire = true;
        break;
      }
    }
    
    if (!masked_line || !has_mult_wire) 
      shares |= bit_value_gauss_line(nb_shares, gauss_deps[i], secret_idx);
  
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

/* 
Factorize a probe m1 * m2 with reference to the first secret values for |deps1|
and second secret values  for |deps2|.
Input :
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -Dependency **deps1 : Array of dependencies of probes contained in the tuples,
                        dependecies factorized by the first secret value, i.e
                        (shares of first secret value, input random with 
                        reference to the first secret value, 1). Each line of 
                        the array is of size |non_mutl_deps_count|.
  -Dependency **deps2 : Array of dependencies of probes contained in the tuples,
                        dependecies factorized by the second secret value, i.e
                        (shares of second secret value, input random with 
                        reference to the second secret value, 1). Each line of 
                        the array is of size |non_mutl_deps_count|.
  -int left_idx : index of |m1| in |c->deps|.
  -int right_idx : index of |m2| in |c->deps|.
  -int index_probes : The number of the probes in the tuples evaluated.
  -int coeff : Coefficient of the probes.
  -int i1_length : Size of the tuples (shares of first secret value, 
                   input random with reference to the first secret value, 1).
  -int i2_length : Size of the tuples (shares of second secret value, 
                   input random with reference to the second secret value, 1).
*/
void factorize_inner_mults_arith(const Circuit* c, Dependency** deps1,
                                 Dependency** deps2, int left_idx, int right_idx, 
                                 int index_probes, int coeff, int i1_length, 
                                 int i2_length) {
  
  DependencyList *deps    = c->deps;
  int characteristic      = c->characteristic;
  int non_mult_deps_count = deps->deps_size - deps->mult_deps->length;
  int share_count         = c->share_count;
  int first_rand_idx      = deps->first_rand_idx;
  bool *i1_rands          = c->i1_rands;
  bool *i2_rands          = c->i2_rands;
  
  //Complex index, we have still to recurse on left_idx or right_idx to have 
  //index in |non_mult_deps_count|.
  if (left_idx >= non_mult_deps_count) {
    Dependency* dep = deps->deps[left_idx]->content[0];
    for (int i = 0; i < deps->deps_size; i++){
      if (dep[i]){
        int new_left_idx = i;
        int elem = (int)((coeff * dep[i])) % characteristic;
        if(elem < 0) elem += characteristic; 
        factorize_inner_mults_arith(c, deps1, deps2, new_left_idx, right_idx, index_probes, elem, i1_length, i2_length);
      }
    }
    return;   
  }
  
  if (right_idx >= non_mult_deps_count) {
    Dependency* dep = deps->deps[right_idx]->content[0];
    for (int i = 0; i < deps->deps_size; i++){
      if (dep[i]){
        int new_right_idx = i;
        int elem = (int)((coeff * dep[i])) % characteristic;
        if (elem < 0) elem += characteristic; 
        factorize_inner_mults_arith(c, deps1, deps2, left_idx, new_right_idx, index_probes, elem, i1_length, i2_length);
      }
    }
    return;   
  }
  
  
  // Inputs
  if (left_idx < share_count) {
    if (right_idx < 2 * share_count){
      /* Case a * b */
      deps1[index_probes * i1_length + left_idx][right_idx] += coeff;
      deps1[index_probes * i1_length + left_idx][right_idx] %= characteristic;
      deps2[index_probes * i2_length + (right_idx - share_count)][left_idx] += coeff;
      deps2[index_probes * i2_length + (right_idx - share_count)][left_idx] %= characteristic;
      return;        
    }
      
    int cpt = -1;
    for (int k = first_rand_idx; k < non_mult_deps_count; k++){
      if (i2_rands[k]) cpt++;
      if (right_idx == k){
        /* Case a * r_b */
        deps1[index_probes * i1_length + left_idx][k] += coeff;
        deps1[index_probes * i1_length + left_idx][k] %= characteristic;
        deps2[index_probes * i2_length + cpt + share_count][left_idx] += coeff;
        deps2[index_probes * i2_length + cpt + share_count][left_idx] %= characteristic;
        return; 
      }
    }  
  }
    
  if (left_idx < 2 * share_count){
    if (right_idx < share_count){
      /* Case b * a */
      deps1[index_probes * i1_length + right_idx][left_idx] += coeff;
      deps1[index_probes * i1_length + right_idx][left_idx] %= characteristic;
      deps2[index_probes * i2_length + (left_idx - share_count)][right_idx] += coeff;
      deps2[index_probes * i2_length + (left_idx - share_count)][right_idx] %= characteristic;
      return; 
    }
      
    int cpt = -1;
    for (int k = first_rand_idx; k < non_mult_deps_count; k++){
      if (i1_rands[k]) cpt++;
      if (right_idx == k){
        /* Case b * r_a */
        deps1[index_probes * i1_length + cpt + share_count][left_idx] += coeff;
        deps1[index_probes * i1_length + cpt + share_count][left_idx] %= characteristic;
        deps2[index_probes * i2_length + left_idx - share_count][k] += coeff;
        deps2[index_probes * i2_length + left_idx - share_count][k] %= characteristic;
        return; 
      }
    }  
  }
  
  
  //Randoms
  /* This is the last case, so left_index have to be a random. */
  if (i1_rands[left_idx]) {
    int cpt = -1;
    for (int j = first_rand_idx; j <= left_idx; j++){
      if (i1_rands[j]) cpt++;
    }
    
    if (right_idx < share_count){
      /*Case r_a * a ---> nothing to factorize */
      return; 
    }
    
    if (right_idx < first_rand_idx){
      /*Case r_a * b */
      deps1[index_probes * i1_length + cpt + share_count][right_idx] += coeff;
      deps1[index_probes * i1_length + cpt + share_count][right_idx] %= characteristic;
      deps2[index_probes * i2_length + right_idx - share_count][left_idx] += coeff;
      deps2[index_probes * i2_length + right_idx - share_count][left_idx] %= characteristic;
      return;
    }
    
    else {
      /*Case r_a * r_b */
      int cpt1 = -1; 
      for (int j = first_rand_idx; j <= right_idx; j++){
        if (i2_rands[j]) cpt1++;
      }
      
      deps1[index_probes * i1_length + cpt + share_count][right_idx] += coeff;
      deps1[index_probes * i1_length + cpt + share_count][right_idx] %= characteristic;
      deps2[index_probes * i2_length + cpt1 + share_count][left_idx] += coeff;
      deps2[index_probes * i2_length + cpt1 + share_count][left_idx] %= characteristic;
      return;
    }
  }
  
  else {
    int cpt = -1;
    for (int j = first_rand_idx; j <= left_idx; j++){
      if (i2_rands[j]) cpt++;
    }
    
    if (right_idx < share_count){
      /*Case r_b * a*/
      deps1[index_probes * i1_length + right_idx][left_idx] += coeff;
      deps1[index_probes * i1_length + right_idx][left_idx] %= characteristic;
      deps2[index_probes * i2_length + cpt + share_count][right_idx] += coeff;
      deps2[index_probes * i2_length + cpt + share_count][right_idx] %= characteristic;
      return;
    }
    
    if (right_idx < 2 * share_count){
      /*Case r_b * b ---> nothing to factorize */
      return;
    }
    
    else {
      /*Case r_b * r_a */
      int cpt1 = -1; 
      for (int j = first_rand_idx; j <= right_idx; j++){
        if (i1_rands[j]) cpt1++;
      }
      
      deps1[index_probes * i1_length + cpt1 + share_count][left_idx] += coeff;
      deps1[index_probes * i1_length + cpt1 + share_count][left_idx] %= characteristic;
      deps2[index_probes * i2_length + cpt + share_count][right_idx] += coeff;
      deps2[index_probes * i2_length + cpt + share_count][right_idx] %= characteristic;
      return;
    }
  }  
}


/*
Count the number of not zero elements in |i_rands| between |index| and |len|.
Input :
  -bool* i_rands : Array in which we will count the element.
  -int len : index at which we stop our counting.
  -int index : index at which we start countting.
Output : The counting.
*/

static int count_rands (bool* i_rands, int len, int index){
  int cnt = 0;
  for (int i = index; i < len; i++){
    if (i_rands[i]) cnt += 1;
  }
  return cnt;
}



/* 
Factorizes the dependencies in |comb|.
Input :
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -Dependency **local_deps : Clean Dependencies from |gauss_deps_o|.
  -Dependency **deps1 : Array of dependencies of probes contained in the tuples,
                        dependecies factorized by the first secret value, i.e
                        (shares of first secret value, input random with 
                        reference to the first secret value, 1). Each line of 
                        the array is of size |non_mutl_deps_count|.
  -Dependency **deps2 : Array of dependencies of probes contained in the tuples,
                        dependecies factorized by the second secret value, i.e
                        (shares of second secret value, input random with 
                        reference to the second secret value, 1). Each line of 
                        the array is of size |non_mutl_deps_count|.
  -int i1_length : Size of the tuples (shares of first secret value, 
                   input random with reference to the first secret value, 1)
  -int i2_length : Size of the tuples (shares of second secret value, 
                   input random with reference to the second secret value, 1)
  -int local_deps_len : Number of line in |local_deps|.
  -int secret_idx : Current secret value we want to reveal.
*/
void factorize_mults_arith(const Circuit* c, Dependency** local_deps,
                     Dependency** deps1, Dependency** deps2,
                     int i1_length, int i2_length,
                     int local_deps_len, int secret_idx) {
  
  DependencyList* deps       = c->deps;
  int deps_size              = deps->deps_size;
  int nb_shares              = c->share_count;
  int inputs_real_count      = c->secret_count * nb_shares;
  int factorized_deps_length = inputs_real_count + c->random_count;
  int mult_count             = deps->mult_deps->length;
  int non_mult_deps_count    = deps_size - mult_count;
  int first_rand_idx         = deps->first_rand_idx;  
  
  bool* i1_rands             = c->i1_rands;
  bool* i2_rands             = c->i2_rands;
  bool* out_rands            = c->out_rands;
  
  //printf("Has i1_rands : %d\n", *i1_rands);
  //printf("Has i2_rands : %d\n", *i2_rands);
  //printf("Has out_rands : %d\n", *out_rands);
  
  //Factorizing every probes in |comb| after gaussian elimination on output 
  //randoms one by one. The factorisation is done one time for the first secret 
  //value and a second time for the second secret value.
  for (int i = 0; i < local_deps_len; i++) {
    //Factorizing the i-th probes.
    Dependency* dep   = local_deps[i];
    int has_i1_rand   = 0;
    int has_i2_rand   = 0;
    int has_out_rand  = 0;
    bool has_mult     = false;
    
    for (int j = first_rand_idx; j < factorized_deps_length ; j++) {
      if ((dep[j]!=0) & i1_rands[j]) {
        has_i1_rand = 1;
        break;
      } else if ((dep[j]!=0) & i2_rands[j]) {
        has_i2_rand = 1;
        break;
      } else if ((dep[j]!=0) & out_rands[j]) {
        has_out_rand = 1;
        break;
      }
    }        
    
    //If has output randoms, then by construction, it contains one output 
    //random who doesn't exist anywhere else. So we can't have information on 
    //this probe and we skip to next element of the tuple.
    if (has_out_rand) {
      continue;
    }
      
    // By construction, if an element contains a random related to
    // either input, it cannot contain a multiplication, and thus
    // there is nothing to factorize. We can thus skip to next
    // element of the tuple.
    if (has_i1_rand || has_i2_rand){ 
      for (int j = 0; j < non_mult_deps_count; j++){
        if (secret_idx == 0) {
          deps2[i * i2_length + i2_length - 1][j] = dep[j];
        }
            
        if (secret_idx == 1)  {
          deps1[i * i1_length + i1_length - 1][j] = dep[j];
        }  
      }
      continue;
    }
    
    for (int j = first_rand_idx; j < factorized_deps_length; j++) assert(!dep[j]);
    
    //Factorizing multiplication probe if we find one.
    for (int j = deps_size - mult_count; j < deps_size; j++) {
      int mult_elem = dep[j];
      if (mult_elem != 0) {
        //Multiplication probe find --->factorization.
        has_mult = true;
        int mult_idx = j - non_mult_deps_count;
        MultDependency* mult = deps->mult_deps->deps[mult_idx];
        
        //The index of the left operand in the multiplication probe.
        int left_idx = mult->left_idx;
        
        //The index of the right operand in the multiplication probe.
        int right_idx = mult->right_idx;
        
        //Factorization.
        factorize_inner_mults_arith(c, deps1, deps2, left_idx,right_idx, i, mult_elem, i1_length, i2_length);
      }
    }
    
    if (!has_mult){
      /* The probes is a single input ---> Factorization with reference to 1. */
      for (int j = 0; j < first_rand_idx; j++){
        if (dep[j]) {
          if (j < nb_shares && secret_idx == 0) {
            deps2[i * i2_length + i2_length - 1][j] =  dep[j];
          }
          
          if (j >= nb_shares && secret_idx == 1)  {
            deps1[i * i1_length + i1_length - 1][j] = dep[j];
          }
        }
      }
    }
  }
}


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
//  |gauss_length_o| : Number of line in |gauss_deps_o|.
//
//  |gauss_deps_i|: Same as |gauss_deps_o|, but for Gauss elimination
//      on the input of interest after factorization.
//
//  |gauss_rands_i|: Same as |gauss_rands_o| but for |gauss_deps_i|.
//
//  |gauss_length_i| : Number of line in |gauss_deps_i|.
//
//  |secret_idx|: The index of the secret that we are trying to reveal.
//
//  |unmask_idx_out|: the current index for the recursion on
//      |gauss_deps_o|. It corresponds to the element that we want to
//      unmask at the current stage of the recursion.
//
//  |unmask_idx_in|: same as |unmask_idx_out|, but for |gauss_deps_i|.
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
//  |t_in| : Number of shares we have to reveal to consider a tuple as an error.   
//
//  |out_rec|: see above
//
//  |include_output| : Boolean to know if we have to include the outputs index 
//                     in the tuple |curr_tuple|.
//
//  |required_output_remaining| : Useful only if |include_outputs| is true, it 
//      indicates the number of outputs we can had in |curr_tuple| at most.
//
//  |fill_only_with_output| : See the comments above randoms_step in the file 
//                            "constructive.c".
//
//  |RPC| : Indicates if we are in RPC/RPE mode or not.
//
//  |mutex|: For parallelisation purpose. We have to lock the thread when we add
//           tuple into |incompr_tuples|. 
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
                         int t_in,
                         bool out_rec,
                         bool include_output,
                         int required_output_remaining,
                         bool fill_only_with_output,
                         bool RPC,
                         pthread_mutex_t *mutex,
                         int debug) {
  
  int deps_size           = c->deps->deps_size;
  int first_rand_idx      = c->deps->first_rand_idx;
  int nb_shares           = c->share_count;
  int non_mult_deps_count = deps_size - c->deps->mult_deps->length;
  bool* i1_rands          = c->i1_rands;
  int nb_rands_i1         = count_rands(i1_rands, non_mult_deps_count, first_rand_idx);
  bool* i2_rands          = c->i2_rands;
  int nb_rands_i2         = count_rands(i2_rands, non_mult_deps_count, first_rand_idx); 
  
  //Impossible case for a tuples to be a failure.
  if (curr_tuple->length > max_size) return;
  if (include_output && required_output_remaining < 0) return;
  
  if (debug) {
    printf("randoms_step: [ ");
    for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]);
    printf("] (unmask_idx_out=%d -- unmask_idx_in=%d, gauss_lenght_i = %d)\n", unmask_idx_out, unmask_idx_in, gauss_length_i);

    /*printf(" Gauss deps main:\n");
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
    }*/
  }
  
  //Check if the current tuple is a failure or not. If the number of bits set 
  //to 1 in revealed_secret is >= t_in, this is the case. 
  if (__builtin_popcount(revealed_secret) >= t_in) {
    
   
    // If parallelisation is done, we have to lock this part. Otherwise, 
    // we can add two failures tuples at the same time.
    if(mutex)
      pthread_mutex_lock(mutex);

    tot_adds++;
    
    //Check if it doesn't exist an incompressible tuple in |incompr_tuples| that 
    //contains this tuple.
    if (!tuple_is_not_incompr(incompr_tuples, curr_tuple)) {
      add_tuple_to_trie_arith(incompr_tuples, curr_tuple, c, secret_idx, 
                        revealed_secret);
    }
    
    //Unlock the mutex
    if(mutex)
      pthread_mutex_unlock(mutex);
    
    
    return;
  }


  if (curr_tuple->length == max_size) {
    // If we are in RPC/RPE mode, the output probe doesn't count as a probe. 
    //So we can add it for free.
    if (include_output)  
      fill_only_with_output = true;

    else{
      // Tuple has maximal size and is not a failure -> abandoning this path.  
      return;
    }
  }

  // Skipping the current element (once in "out" and once in "in")
  if (unmask_idx_out < gauss_length_o-1 && available_shares != c->all_shares_mask) {
    // Finding out the index of the next potential element of |curr_tuple| to unmask.
    int next_unmask_idx_out = unmask_idx_out+1;
    randoms_step(c, incompr_tuples, max_size, randoms,
                 gauss_deps_o, gauss_rands_o, gauss_length_o,
                 gauss_deps_i, gauss_rands_i, gauss_length_i,
                 secret_idx,
                 next_unmask_idx_out, unmask_idx_in, curr_tuple,
                 revealed_secret, available_shares, t_in, true, include_output,
                 required_output_remaining, fill_only_with_output, RPC, mutex, 
                 debug);
  }
  
  if (!out_rec && unmask_idx_in < gauss_length_i-1 && available_shares == c->all_shares_mask
      && c->has_input_rands) {
    // Finding out the index of the next potential element of |curr_tuple| to unmask.
    int next_unmask_idx_in = unmask_idx_in+1;
    randoms_step(c, incompr_tuples, max_size, randoms,
                 gauss_deps_o, gauss_rands_o, gauss_length_o,
                 gauss_deps_i, gauss_rands_i, gauss_length_i,
                 secret_idx,
                 unmask_idx_out, next_unmask_idx_in, curr_tuple,
                 revealed_secret, available_shares, t_in, false, include_output,
                 required_output_remaining, fill_only_with_output, RPC, mutex, 
                 debug);
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
    
    //Add a probe to unmask the random we want to unmask.
    const VarVector* dep_array = randoms[rand_idx];
    for (int j = 0; j < dep_array->length; j++) {
      Var dep = dep_array->content[j];
      if (Tuple_contains(curr_tuple, dep)) continue;
      if (fill_only_with_output && dep < c->length) continue;
             
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
      
      //Apply Gaussian elimination on the last probe we just add. 
      int new_gauss_length_o = gauss_length_o;
      DepArrVector* dep_arr = c->deps->deps[dep];
      for (int dep_idx = 0; dep_idx < dep_arr->length; dep_idx++) {
        apply_gauss_arith(c->deps->deps_size, dep_arr->content[dep_idx],
                    gauss_deps_o, gauss_rands_o, new_gauss_length_o, c->characteristic);
        int first_rand = get_first_rand_mult(gauss_deps_o[new_gauss_length_o],
                                             non_mult_deps_count,
                                             first_rand_idx, c->out_rands);
        gauss_rands_o[new_gauss_length_o] = first_rand;
        new_gauss_length_o++;
      }

      
      // Factorize new dependency, and only new dependency.
      
      /*Dependency** deps1 = secret_idx == 0 ? &gauss_deps_i[gauss_length_i] : NULL;
      Dependency** deps2 = secret_idx == 0 ? NULL : &gauss_deps_i[gauss_length_i];
      int deps_length = 0;
      int* deps1_length = secret_idx == 0 ? &deps_length : NULL;
      int* deps2_length = secret_idx == 0 ? NULL : &deps_length; */

      int i1_length   = c->share_count + nb_rands_i1 + 1;
      int i2_length   = c->share_count + nb_rands_i2 + 1;
      int deps_length = secret_idx == 1 ? i1_length * new_gauss_length_o : i2_length * new_gauss_length_o;
      int length = secret_idx == 1? i1_length * (new_gauss_length_o - gauss_length_o): i2_length * (new_gauss_length_o - gauss_length_o);
  
      /* TODO Victor: Actually, I was computing one of the two array for 
      nothing. Maybe I have to do something. */
      
      //Creating arrays for dependencies on the last probe with reference to the
      //first secret value (deps1) and w.r.t the second secret value (deps2).
      Dependency* deps1[(new_gauss_length_o - gauss_length_o) * i1_length];
      Dependency* deps2[(new_gauss_length_o - gauss_length_o) * i2_length]; 
  
      
      for (int i = 0; i < (new_gauss_length_o - gauss_length_o)  * i1_length; i++){
          deps1[i] = calloc(non_mult_deps_count, sizeof(Dependency));
        }  
    
      for (int i = 0; i < (new_gauss_length_o - gauss_length_o) * i2_length; i++){
          deps2[i] = calloc(non_mult_deps_count, sizeof(Dependency));
      }
      
      factorize_mults_arith(c, &gauss_deps_o[gauss_length_o], deps1, deps2,
                      i1_length, i2_length, new_gauss_length_o - gauss_length_o,
                      secret_idx);
                      
      //Choose the dependency we are interested in.
      Dependency **study_deps = secret_idx == 1? deps1 : deps2; 
      
      // Gauss elimination on input randoms                                   
      for (int i = 0; i < length; i++) {
        Dependency* real_dep = study_deps[i];
        apply_gauss_arith(non_mult_deps_count, real_dep, gauss_deps_i, gauss_rands_i, 
                    i + gauss_length_i, c->characteristic);
        
        gauss_rands_i[gauss_length_i + i] = 
            get_first_rand_mult(gauss_deps_i[gauss_length_i + i], 
                                non_mult_deps_count,
                                first_rand_idx, NULL);       
      }
      
      //Freeing stuffs                
      for (int i = 0; i < (new_gauss_length_o - gauss_length_o) * i1_length; i++){
          free(deps1[i]);
      }  
    
      for (int i = 0; i < (new_gauss_length_o - gauss_length_o) * i2_length; i++){
          free(deps2[i]);
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
      
      //Compute new revealed secret & availables shares.
      int additional_revealed_secret =
        get_initial_revealed_secret(c, deps_length - gauss_length_i, &gauss_deps_i[gauss_length_i], secret_idx);
      
      int new_revealed_secret = revealed_secret | additional_revealed_secret;

      int new_available_shares = available_shares |
        get_initial_shares(c, deps_length - gauss_length_i , nb_shares, &gauss_deps_i[gauss_length_i], secret_idx);

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
      
      int new_required_output_remaining = required_output_remaining;
      int new_max_size = max_size;
      if (dep >= c->length) {
        new_required_output_remaining--;
        if(RPC) new_max_size++;
      }
     
      randoms_step(c, incompr_tuples, new_max_size, randoms,
                   gauss_deps_o, gauss_rands_o, new_gauss_length_o,
                   gauss_deps_i, gauss_rands_i, deps_length,
                   secret_idx,
                   new_unmask_idx_out, new_unmask_idx_in, curr_tuple,
                   new_revealed_secret, new_available_shares, t_in, false,
                   include_output, new_required_output_remaining, 
                   fill_only_with_output, RPC, mutex, debug);


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
      for(int i = 0; i < nb_shares; i++){
        if (gauss_deps_i[unmask_idx_in][nb_shares * secret_idx + i])
          secret_to_unmask |= 1 >> i;
      }
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
    
    //Add a probe to unmask the random we want to unmask. 
    const VarVector* dep_array = randoms[rand_idx];
    for (int j = 0; j < dep_array->length; j++) {
      Var dep = dep_array->content[j];
      if (Tuple_contains(curr_tuple, dep)) continue;
      if (fill_only_with_output && dep < c->length) continue; 
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
      
      //Gaussian elimination on output random
      int new_gauss_length_o = gauss_length_o;
      DepArrVector* dep_arr = c->deps->deps[dep];
      for (int dep_idx = 0; dep_idx < dep_arr->length; dep_idx++) {
        apply_gauss_arith(c->deps->deps_size, dep_arr->content[dep_idx],
                    gauss_deps_o, gauss_rands_o, new_gauss_length_o, c->characteristic);
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

      // Factorize new dependency, and only new dependency.
      
      int i1_length    = c->share_count + nb_rands_i1 + 1;
      int i2_length    = c->share_count + nb_rands_i2 + 1;
      int diff_len_o = new_gauss_length_o - gauss_length_o;
      int deps_length  = secret_idx == 1 ? i1_length * new_gauss_length_o : i2_length * new_gauss_length_o;
      int length = secret_idx == 1? i1_length * diff_len_o : i2_length * diff_len_o;     
      
      /* TODO : Same issue as above, I was computing so much factorization.*/ 
      Dependency* deps1[diff_len_o * i1_length]; 
      Dependency* deps2[diff_len_o * i2_length];
      
      for (int i = 0; i < diff_len_o * i1_length; i++){
          deps1[i] = calloc(non_mult_deps_count, sizeof(Dependency));
      }  
    
      for (int i = 0; i < diff_len_o * i2_length; i++){
          deps2[i] = calloc(non_mult_deps_count, sizeof(Dependency));
      }

      factorize_mults_arith(c, &gauss_deps_o[gauss_length_o], deps1, deps2,
                      i1_length, i2_length, diff_len_o, secret_idx);
     
      //Choose the dependency we are interested in.
      Dependency **study_deps = secret_idx == 1? deps1 : deps2;
  
      // Gauss elimination on input randoms                                       
      for (int i = 0; i < length; i++) {
        Dependency* real_dep = study_deps[i];
        apply_gauss_arith(non_mult_deps_count, real_dep, gauss_deps_i, gauss_rands_i,
                    i + gauss_length_i, c->characteristic);
        gauss_rands_i[gauss_length_i + i] = 
            get_first_rand_mult(gauss_deps_i[gauss_length_i + i],
                                non_mult_deps_count, first_rand_idx, NULL);       
      }
      
      //Freeing stuffs.
      for (int i = 0; i < diff_len_o * i1_length; i++){
          free(deps1[i]);
      }  
    
      for (int i = 0; i < diff_len_o * i2_length; i++){
          free(deps2[i]);
      }
      
      //Compute new revealed secret & availables shares.
      int additional_revealed_secret =
        get_initial_revealed_secret(c, deps_length - gauss_length_i, &gauss_deps_i[gauss_length_i], secret_idx);
      int new_revealed_secret = revealed_secret | additional_revealed_secret;

      int new_available_shares = available_shares |
        get_initial_shares(c, deps_length - gauss_length_i, nb_shares, &gauss_deps_i[gauss_length_i], secret_idx);

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

      int new_required_output_remaining = required_output_remaining;
      int new_max_size = max_size;
      if (dep >= c->length) {
        new_required_output_remaining--;
        if(RPC) new_max_size++;
      }

      // Recursing
      randoms_step(c, incompr_tuples, new_max_size, randoms,
                   gauss_deps_o, gauss_rands_o, new_gauss_length_o,
                   gauss_deps_i, gauss_rands_i, deps_length,
                   secret_idx,
                   new_unmask_idx_out, new_unmask_idx_in, curr_tuple,
                   new_revealed_secret, new_available_shares, t_in, false, 
                   include_output, new_required_output_remaining, 
                   fill_only_with_output, RPC, mutex, debug);

      curr_tuple->length--;
    }
  }
}

/*
Do the first Gauss elimination & factorization on the probes in |curr_tuple|. 
Then, calls random_step.
Input :
  -const Circuit *c: the circuit

  -Trie *incompr_tuples: the trie of incompressible tuples (contain already 
                         computed incompressible tuples, and new ones are added 
                         in this trie as well.

  -int max_size: the maximal size of tuples allowed.

  -VarVector **randoms: the array of array containing, for all randoms, which
                        variables depend on that random.

  -Dependency **gauss_deps_o: The tuple after gauss elimination on output
                              randoms. However, not that the dependencies are 
                              kept in their unmasked form, in order to update 
                              the Gauss elimination when adding variables to the
                              tuple.

  -Dependency *gauss_rands_o: The indices of the randoms used for the Gauss
                              elimination on output randoms. When an element of 
                              the tuple contains no random, 0 is put in 
                              |gauss_rands_o|.

  -Dependency **gauss_deps_i: Same as |gauss_deps_o|, but for Gauss elimination
                              on the input of interest after factorization.

  -Dependency *gauss_rands_i : Same as |gauss_rands_o| but for |gauss_deps_i|.



  -int secret_idx: The index of the secret that we are trying to reveal.


  -Tuple *curr_tuple: the tuple that is being built.

  -int t_in : Number of shares we have to reveal to consider a tuple as an error.   

  -bool include_output : Boolean to know if we have to include the outputs index 
                         in the tuple |curr_tuple|.

  -int required_output_remaining : Useful only if |include_outputs| is true, it 
                                    indicates the number of outputs we can had 
                                    in |curr_tuple| at most.

  -bool RPC : Indicates if we are in RPC/RPE mode or not.
  
  -pthread_mutex_t *mutex : For parallelisation purpose. We have to give it to 
                            the function randoms_step. 
  
  -int debug: if true, then some debuging information are printed.
*/
void initial_gauss_and_fact(const Circuit* c,
                            Trie* incompr_tuples,
                            int max_size,
                            const VarVector** randoms,
                            Dependency** gauss_deps_o,
                            Dependency* gauss_rands_o,
                            int* gauss_length_o,
                            Dependency** gauss_deps_i,
                            Dependency* gauss_rands_i,
                            int secret_idx,
                            Tuple* curr_tuple,
                            int t_in,
                            bool include_output, 
                            int required_output_remaining,
                            bool RPC,
                            pthread_mutex_t *mutex,
                            int debug) {                               
  if (debug) {
      printf("\n\nBase tuple: [ ");
      for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]);
      printf("] (size_max = %d), gauss_length = %d\n", max_size, *gauss_length_o);
      printf("Secret idx : %d\n", secret_idx);
  }
                                 
  const DependencyList* deps = c->deps;
  int deps_size = deps->deps_size;
  int nb_shares = c->share_count;
  int non_mult_deps_count = deps_size - deps->mult_deps->length;
  int first_rand_idx = c->deps->first_rand_idx;

  // Gauss elimination on output randoms
  for (int i = *gauss_length_o; i < curr_tuple->length; i++) {
    
    DepArrVector* dep_arr = deps->deps[curr_tuple->content[i]];
    for (int dep_idx = 0; dep_idx < dep_arr->length; dep_idx++) {
      Dependency* real_dep = dep_arr->content[dep_idx];
      apply_gauss_arith(deps_size, real_dep, gauss_deps_o, gauss_rands_o, *gauss_length_o, c->characteristic);
      gauss_rands_o[*gauss_length_o] =
        get_first_rand_mult(gauss_deps_o[*gauss_length_o], non_mult_deps_count,
                            first_rand_idx, c->out_rands);
      (*gauss_length_o)++;
    }
  }
  
  // Extracting "clean" dependencies from |gauss_deps_o| and
  // |gauss_rands_o| in order to do the factorization (|gauss_deps_o|
  // does not only contain randoms in order to enable further
  // elimination, but it should, at least for the factorization)
  Dependency* local_deps[*gauss_length_o];
  for (int i = 0; i < *gauss_length_o; i++) {
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
  bool* i1_rands             = c->i1_rands;
  int nb_rands_i1            = count_rands(i1_rands, non_mult_deps_count, first_rand_idx);
  bool* i2_rands             = c->i2_rands;
  int nb_rands_i2            = count_rands(i2_rands, non_mult_deps_count, first_rand_idx);

  int i1_length = c->share_count + nb_rands_i1 + 1;
  int i2_length = c->share_count + nb_rands_i2 + 1;
  int length = secret_idx == 1? i1_length * (*gauss_length_o) : i2_length * (*gauss_length_o);
  
  /* TODO Victor : Same issue as above, I was computing so much factorization.*/
  Dependency* deps1[*gauss_length_o * i1_length]; 
  Dependency* deps2[*gauss_length_o * i2_length]; 
  
  for (int i = 0; i < *gauss_length_o * i1_length; i++){
      deps1[i] = calloc(non_mult_deps_count, sizeof(Dependency));
    }  
    
  for (int i = 0; i < *gauss_length_o * i2_length; i++){
      deps2[i] = calloc(non_mult_deps_count, sizeof(Dependency));
  }
  
  factorize_mults_arith(c, local_deps, deps1, deps2,
                  i1_length, i2_length,
                  *gauss_length_o, secret_idx);
  
  Dependency **study_deps = secret_idx == 1? deps1 : deps2;
  
  
  
  
  // Gauss elimination on input randoms                                       
  for (int i = 0; i < length; i++) {
    Dependency* real_dep = study_deps[i];
    apply_gauss_arith(non_mult_deps_count, real_dep, gauss_deps_i, gauss_rands_i, i, c->characteristic);
    gauss_rands_i[i] = get_first_rand_mult(gauss_deps_i[i], non_mult_deps_count,
                                           first_rand_idx, NULL);       
  }
  
  //Computing revealed secret and available shares.
  int revealed_secret = get_initial_revealed_secret(c, length, gauss_deps_i, secret_idx);
  int available_shares = get_initial_shares(c, length, nb_shares, gauss_deps_i, secret_idx);
  
  /* Freeing stuff */
  for (int i = 0; i < *gauss_length_o * i1_length; i++){
    free(deps1[i]);
  }  
    
  for (int i = 0; i < *gauss_length_o * i2_length; i++){
    free(deps2[i]);
  }
  
  
  
  // Identifying the first element of |gauss_deps_o| and
  // |gauss_rands_o| that need to be unmasked.
  int unmask_idx_out = 0;
  while (unmask_idx_out < curr_tuple->length && gauss_rands_o[unmask_idx_out] == 0) {
    unmask_idx_out++;
  }
  
  // Identifying the first element of |gauss_deps_i| and
  // |gauss_rands_i| that need to be unmasked.
  int unmask_idx_in = 0;
  while (unmask_idx_in < length && 
          (gauss_rands_i[unmask_idx_in] == 0 ||
            (bit_value_gauss_line(nb_shares, gauss_deps_i[unmask_idx_in], secret_idx) & revealed_secret)
              == bit_value_gauss_line(nb_shares, gauss_deps_i[unmask_idx_in], secret_idx))) {
    unmask_idx_in++;
  }
  
  //Calls Random Step
  randoms_step(c, incompr_tuples, max_size, randoms,
               gauss_deps_o, gauss_rands_o, *gauss_length_o,
               gauss_deps_i, gauss_rands_i, length,
               secret_idx, unmask_idx_out, unmask_idx_in,
               curr_tuple, revealed_secret, available_shares, t_in, false, 
               include_output, required_output_remaining, false, RPC, mutex, 
               debug);
}


// Parallelisation of the function secrets_steps.
struct sec_step_args {
  const Circuit* c;
  Trie* incompr_tuples;
  int max_size;
  const VarVector** secrets;
  const VarVector** randoms;
  Tuple *curr_tuple;
  int next_secret_share_idx;
  int secrets_count;
  int secret_idx;
  int required_outputs_remaining;
  bool RPC;
  int t_in;
  pthread_mutex_t *mutex;
  int debug; 
};

/* Function use to start the tread in the paralellisation of the function 
secrets_step */
void *start_thread_sec_step_mult (void *void_args);

/*
Found all the incompressibles tuples of size |target_size| (without counting the 
output index in the size of a tuple) and adds them in the trie |incompr_tuples|.
Input :
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -Trie *incomrp_tuples : The trie in which we will add the incompressibles 
                          tuples. 
  -int max_size : The maximal size of the incompressible tuples 
                  we want to observe.                        
  -VarVector **secrets : Array 2D of integer. Each line contains an array of 
                         index of probes that contains the line-th share we 
                         found in the |c->deps| array. For example, 
                         the first line contains the index of the probes who can
                         leak the first share of the first secret value.   
                          
  -VarVector **randoms : Array 2D of integer. Each line contains an array of
                         index of probes that contains the line-th randoms.
  -Dependency **gauss_deps_o : The tuple after gauss elimination on output
                              randoms. However, not that the dependencies are 
                              kept in their unmasked form, in order to update 
                              the Gauss elimination when adding variables to the
                              tuple.
  -Dependeny *gauss_rands_o : The indices of the randoms used for the Gauss
                              elimination on output randoms. When an element of 
                              the tuple contains no random, 0 is put in 
                              |gauss_rands_o|.

  -Dependency **gauss_deps_i : Same as |gauss_deps_o|, but for Gauss elimination
                               on the input of interest after factorization.
  -Dependeny *gauss_rands_i : Same as |gauss_rands_o| but for |gauss_deps_i|.

  -int next_secret_share_idx : Index of the next secret share to consider.
  -int secret_count : Number of secret shares already taken.
  -int secret_idx : Index of the secret that the current tuple should reveal.
  -int required_outputs_remaining : Number of outputs required in each tuple.  
  -Tuple *curr_tuple : the tuple that is being built.                            
  -bool RPC : If true, we are checking the RPC (or RPE1 sometimes) property.
  -int t_in : The number of shares that must be leaked for a tuple to be a 
              failure.
  -int debug : if true, then some debuging information are printed.
*/
static void secrets_step(const Circuit* c,
                         Trie* incompr_tuples,
                         int max_size,
                         const VarVector** secrets,
                         const VarVector** randoms,
                         Dependency** gauss_deps_o,
                         Dependency* gauss_rands_o,
                         int *gauss_length_o,
                         Dependency** gauss_deps_i,
                         Dependency* gauss_rands_i,
                         int next_secret_share_idx,
                         int secrets_count,
                         int secret_idx,
                         Tuple* curr_tuple,
                         int required_outputs_remaining,
                         bool RPC,
                         int t_in,
                         int debug) {                  
  //Stop condition : |curr_tuple| is at maximal size or we have enough secret 
  //                 shares to leak.                                               
  if (next_secret_share_idx == -1 || secrets_count == t_in 
                                  || curr_tuple->length == max_size) { 
    
    //We don't have enough secret shares.
    if (secrets_count != t_in){ 
      //We just have to check if we have check all the possible shares. Because 
      //if it's not the case, we don't have the good secret_counts. 
      //We know we can't have more probes to add, but we have to update 
      //secrets count.
      if (next_secret_share_idx != -1){
        const VarVector* dep_array = secrets[next_secret_share_idx];
        bool already_in = false;
        for (int i = 0; i < dep_array->length; i++) {
          Var dep = dep_array->content[i];
          if (Tuple_contains(curr_tuple, dep)) {
            if(already_in)
              continue;
            
            already_in = true;
            // This variable of the gadget contains multiple shares of the
            // same input. No need to add it multiple times to the tuples,
            // just recusring further.
            secrets_step(c, incompr_tuples, max_size, secrets, randoms,
                         gauss_deps_o, gauss_rands_o, gauss_length_o, 
                         gauss_deps_i, gauss_rands_i, next_secret_share_idx - 1, 
                         secrets_count + 1, secret_idx, curr_tuple, 
                         required_outputs_remaining, RPC, t_in, debug);
            
          }
          
          else if (dep >= c->length && RPC && required_outputs_remaining > 0){
            curr_tuple->content[curr_tuple->length] = dep;
            curr_tuple->length++;
            //Probes added, Recursion
            secrets_step(c, incompr_tuples, max_size + 1, secrets, randoms,
                         gauss_deps_o, gauss_rands_o, gauss_length_o, 
                         gauss_deps_i, gauss_rands_i, next_secret_share_idx - 1, 
                         secrets_count + 1, secret_idx, curr_tuple, 
                         required_outputs_remaining - 1, RPC, t_in, debug);
                      
            curr_tuple->length--;
            *gauss_length_o = min(*gauss_length_o, curr_tuple->length);    
          } 
        }
        secrets_step(c, incompr_tuples, max_size, secrets, randoms,
                     gauss_deps_o, gauss_rands_o, gauss_length_o,
                     gauss_deps_i, gauss_rands_i, next_secret_share_idx - 1, 
                     secrets_count, secret_idx, curr_tuple, 
                     required_outputs_remaining, RPC, t_in, debug);
      }
      //If we have checked all share index, skip this probes.
      return;
    }
    
    //SNI case and we don't have enough output in the probe, so we skip.
    if (!RPC && required_outputs_remaining > 0) return;
    
    //Go to Gauss elimination on random output, then factorisation, then gauss
    //elimination on input random.  
    initial_gauss_and_fact(c, incompr_tuples, max_size, randoms,
                           gauss_deps_o, gauss_rands_o, gauss_length_o, 
                           gauss_deps_i, gauss_rands_i, secret_idx, curr_tuple, 
                           t_in, (required_outputs_remaining > 0),
                           required_outputs_remaining, RPC, NULL, debug); 
  } else {
    // Skipping the current share if there are enough shares remaining
    if (next_secret_share_idx >= t_in - secrets_count) {
          secrets_step(c, incompr_tuples, max_size, secrets, randoms,
                       gauss_deps_o, gauss_rands_o, gauss_length_o, 
                       gauss_deps_i, gauss_rands_i, next_secret_share_idx - 1, 
                       secrets_count, secret_idx, curr_tuple, 
                       required_outputs_remaining, RPC, t_in, debug);
    }
    
    //Add one probe that leaks the secrets of next_secret_idx.
    const VarVector* dep_array = secrets[next_secret_share_idx];
    int tuple_idx = curr_tuple->length;
    bool already_in = false;
    for (int i = 0; i < dep_array->length; i++) {
      Var dep = dep_array->content[i];
      if (Tuple_contains(curr_tuple, dep)) {
        // This variable of the gadget contains multiple shares of the
        // same input. No need to add it multiple times to the tuples,
        // just recusring further.
        if (already_in)
          continue;
        
        secrets_step(c, incompr_tuples, max_size, secrets, randoms,
                     gauss_deps_o, gauss_rands_o, gauss_length_o, gauss_deps_i, 
                     gauss_rands_i, next_secret_share_idx - 1, 
                     secrets_count + 1, secret_idx, curr_tuple, 
                     required_outputs_remaining, RPC, t_in, debug);
        
        already_in = true;
      } else {
        int new_required_outputs_remaining = required_outputs_remaining;
        int new_max_size = max_size;
        if (dep >= c->length && required_outputs_remaining >= 0){
          if (required_outputs_remaining == 0) continue; 
          new_required_outputs_remaining--;
          if (RPC) new_max_size++;
        }
      
        curr_tuple->content[tuple_idx] = dep;
        curr_tuple->length++;
        //Probes added, Recursion
        secrets_step(c, incompr_tuples, new_max_size, secrets, randoms,
                     gauss_deps_o, gauss_rands_o, gauss_length_o, gauss_deps_i, 
                     gauss_rands_i, next_secret_share_idx - 1, 
                     secrets_count + 1, secret_idx, curr_tuple, 
                     new_required_outputs_remaining, RPC, t_in, debug);
                      
        curr_tuple->length--;
        *gauss_length_o = min(*gauss_length_o, curr_tuple->length);    
      }
    }
  }
}


//Parallel version of secrets_step.
static void secrets_step_parallel(const Circuit* c,
                                  Trie* incompr_tuples,
                                  int max_size,
                                  const VarVector** secrets,
                                  const VarVector** randoms,
                                  Dependency** gauss_deps_o,
                                  Dependency* gauss_rands_o,
                                  int* gauss_length_o,
                                  Dependency** gauss_deps_i,
                                  Dependency* gauss_rands_i,
                                  int next_secret_share_idx,
                                  int secrets_count,
                                  int secret_idx,
                                  Tuple* curr_tuple,
                                  int required_outputs_remaining,
                                  bool RPC,
                                  int t_in,
                                  pthread_mutex_t *mutex,
                                  int debug) {                      
 
  //Stop condition : |curr_tuple| is at maximal size or we have enough secret 
  //                 shares to leak.                                               
  if (next_secret_share_idx == -1 || secrets_count == t_in 
                                  || curr_tuple->length == max_size) { 
    //We don't have enough secret shares.
    if (secrets_count != t_in){ 
      //We just have to check if we have check all the possible shares. Because 
      //if it's not the case, we don't have the good secret_counts. 
      //We know we can't have more probes to add, but we have to update 
      //secrets count.
      if (next_secret_share_idx != -1){
        const VarVector* dep_array = secrets[next_secret_share_idx];
        for (int i = 0; i < dep_array->length; i++) {
          Var dep = dep_array->content[i];
          bool already_in = false;
          if (Tuple_contains(curr_tuple, dep)) {
            if(already_in)
              continue;
            
            already_in = true;
            // This variable of the gadget contains multiple shares of the
            // same input. No need to add it multiple times to the tuples,
            // just recusring further.
            secrets_step_parallel(c, incompr_tuples, max_size, secrets, randoms,
                         gauss_deps_o, gauss_rands_o, gauss_length_o, 
                         gauss_deps_i, gauss_rands_i, next_secret_share_idx - 1, 
                         secrets_count + 1, secret_idx, curr_tuple, 
                         required_outputs_remaining, RPC, t_in, mutex, debug);
          }
          
          else if (dep >= c->length && RPC && required_outputs_remaining > 0){
            curr_tuple->content[curr_tuple->length] = dep;
            curr_tuple->length++;
            //Probes added, Recursion
            secrets_step_parallel(c, incompr_tuples, max_size + 1, secrets, 
                                  randoms, gauss_deps_o, gauss_rands_o, 
                                  gauss_length_o, gauss_deps_i, gauss_rands_i, 
                                  next_secret_share_idx - 1, secrets_count + 1, 
                                  secret_idx, curr_tuple, 
                                  required_outputs_remaining - 1, RPC, t_in, 
                                  mutex, debug);
                      
            curr_tuple->length--;
            *gauss_length_o = min(*gauss_length_o, curr_tuple->length); 
          } 
        }
        secrets_step_parallel(c, incompr_tuples, max_size, secrets, randoms,
                     gauss_deps_o, gauss_rands_o, gauss_length_o,
                     gauss_deps_i, gauss_rands_i, next_secret_share_idx - 1, 
                     secrets_count, secret_idx, curr_tuple, 
                     required_outputs_remaining, RPC, t_in, mutex, debug);
      }
      //If we have checked all share index, skip this probes.
      return;
    }
    
    //SNI case and we don't have enough output in the probe, so we skip.
    if (!RPC && required_outputs_remaining > 0) return;

    //Go to Gauss elimination on random output, then factorisation, then gauss
    //elimination on input random.  
    initial_gauss_and_fact(c, incompr_tuples, max_size, randoms,
                           gauss_deps_o, gauss_rands_o, gauss_length_o,
                           gauss_deps_i, gauss_rands_i, secret_idx, curr_tuple, 
                           t_in, (required_outputs_remaining > 0),
                           required_outputs_remaining, RPC, mutex, debug); 
  } else {
    pthread_t threads1;
    int threads_used_1 = 0;
    // Skipping the current share if there are enough shares remaining
    if (next_secret_share_idx >= t_in - secrets_count) {
          if (curr_tuple->length >= max_size - 1){
            secrets_step_parallel(c, incompr_tuples, max_size, secrets, randoms,
                                  gauss_deps_o, gauss_rands_o, gauss_length_o, 
                                  gauss_deps_i, gauss_rands_i, 
                                  next_secret_share_idx - 1, secrets_count, 
                                  secret_idx, curr_tuple, 
                                  required_outputs_remaining, RPC, t_in, mutex, 
                                  debug);
          }
          else {
            //Parallelisation step
            Tuple *curr_tuple_cpy = Tuple_make_size(c->deps->length);
            memcpy(curr_tuple_cpy->content, curr_tuple->content, 
                   curr_tuple->length * sizeof(*curr_tuple_cpy->content));
            curr_tuple_cpy->length = curr_tuple->length;
          
            struct sec_step_args *args = malloc(sizeof(*args));
            args->c = c; 
            args->incompr_tuples = incompr_tuples;
            args->max_size = max_size;
            args->secrets = secrets;
            args->randoms = randoms;
            args->next_secret_share_idx = next_secret_share_idx - 1;
            args->secrets_count = secrets_count;
            args->secret_idx = secret_idx;
            args->required_outputs_remaining = required_outputs_remaining;
            args->RPC = RPC;
            args->t_in = t_in;
            args->mutex = mutex;
            args->debug = debug;
            args->curr_tuple = curr_tuple_cpy;
            
            pthread_mutex_lock(mutex);
            int ret = pthread_create(&threads1, NULL, 
                                     start_thread_sec_step_mult, args);
            pthread_mutex_unlock(mutex);
            
            if(ret){
              Tuple_free(curr_tuple_cpy);
              free(args);
              secrets_step_parallel(c, incompr_tuples, max_size, secrets, 
                                    randoms, gauss_deps_o, gauss_rands_o, 
                                    gauss_length_o, gauss_deps_i, gauss_rands_i,
                                    next_secret_share_idx - 1, secrets_count, 
                                    secret_idx, curr_tuple, 
                                    required_outputs_remaining, RPC, t_in, 
                                    mutex,debug);
            }
            
            else 
              threads_used_1 += 1;
          }
          
    }
    
    //Add one probe that leaks the secrets of next_secret_idx.
    const VarVector* dep_array = secrets[next_secret_share_idx];
    int tuple_idx = curr_tuple->length;
    pthread_t threads[dep_array->length];
    int threads_used = 0;
    bool already_in = false;
    for (int i = 0; i < dep_array->length; i++) {
      Var dep = dep_array->content[i];
      if (Tuple_contains(curr_tuple, dep)) {
        if (already_in)
          continue;
        
        already_in = true;
        
        // This variable of the gadget contains multiple shares of the
        // same input. No need to add it multiple times to the tuples,
        // just recusring further.
        if (curr_tuple->length >= max_size - 1 || (i == (dep_array->length - 1) 
            && threads_used == dep_array->length - 2)){
          
          secrets_step_parallel(c, incompr_tuples, max_size, secrets, randoms,
                       gauss_deps_o, gauss_rands_o, gauss_length_o, 
                       gauss_deps_i, gauss_rands_i, next_secret_share_idx - 1, 
                       secrets_count + 1, secret_idx, curr_tuple, 
                       required_outputs_remaining, RPC, t_in, mutex, debug);
        }
        else {
          //Parallelisation step.
          Tuple *curr_tuple_cpy = Tuple_make_size(c->deps->length);
          memcpy(curr_tuple_cpy->content, curr_tuple->content, 
                 curr_tuple->length * sizeof(*curr_tuple_cpy->content));
          curr_tuple_cpy->length = curr_tuple->length;
          
          struct sec_step_args *args = malloc(sizeof(*args));
          args->c = c; 
          args->incompr_tuples = incompr_tuples;
          args->max_size = max_size;
          args->secrets = secrets;
          args->randoms = randoms;
          args->next_secret_share_idx = next_secret_share_idx - 1;
          args->secrets_count = secrets_count + 1;
          args->secret_idx = secret_idx;
          args->required_outputs_remaining = required_outputs_remaining;
          args->RPC = RPC;
          args->t_in = t_in;
          args->mutex = mutex;
          args->debug = debug;
          args->curr_tuple = curr_tuple_cpy;
          
          pthread_mutex_lock(mutex);
          int ret = pthread_create(&threads[threads_used], NULL, 
                                   start_thread_sec_step_mult, args);
          pthread_mutex_unlock(mutex);
          
          if(ret){
            
            Tuple_free(curr_tuple_cpy);
            free(args);
            secrets_step_parallel(c, incompr_tuples, max_size, secrets, randoms,
                                  gauss_deps_o, gauss_rands_o, gauss_length_o, 
                                  gauss_deps_i, gauss_rands_i, 
                                  next_secret_share_idx - 1, secrets_count + 1, 
                                  secret_idx, curr_tuple, 
                                  required_outputs_remaining, RPC, t_in, mutex,
                                  debug);
          }
          else 
            threads_used += 1;
           
        }  
      } else {
        int new_required_outputs_remaining = required_outputs_remaining;
        int new_max_size = max_size;
        if (dep >= c->length && required_outputs_remaining >= 0){
          if (required_outputs_remaining == 0) continue; 
          new_required_outputs_remaining--;
          if (RPC) new_max_size++;
        }
        curr_tuple->content[tuple_idx] = dep;
        curr_tuple->length++;
        //Probes added, Recursion
        if (curr_tuple->length >= max_size - 1 || (i == (dep_array->length - 1) 
            && threads_used == dep_array->length - 2)){
          
          secrets_step_parallel(c, incompr_tuples, new_max_size, secrets, randoms,
                                gauss_deps_o, gauss_rands_o, gauss_length_o,
                                gauss_deps_i, gauss_rands_i, 
                                next_secret_share_idx - 1, secrets_count + 1, 
                                secret_idx, curr_tuple, 
                                new_required_outputs_remaining, RPC, t_in, 
                                mutex, debug);
                                
          *gauss_length_o = min(*gauss_length_o, curr_tuple->length - 1);
        } else {
          //Parallelisation step.
          Tuple *curr_tuple_cpy = Tuple_make_size(c->deps->length);
          memcpy(curr_tuple_cpy->content, curr_tuple->content, 
                 curr_tuple->length * sizeof(*curr_tuple_cpy->content));
          curr_tuple_cpy->length = curr_tuple->length;
          
          struct sec_step_args *args = malloc(sizeof(*args));
          args->c = c; 
          args->incompr_tuples = incompr_tuples;
          args->max_size = new_max_size;
          args->secrets = secrets;
          args->randoms = randoms;
          args->next_secret_share_idx = next_secret_share_idx - 1;
          args->secrets_count = secrets_count + 1;
          args->secret_idx = secret_idx;
          args->required_outputs_remaining = new_required_outputs_remaining;
          args->RPC = RPC;
          args->t_in = t_in;
          args->mutex = mutex;
          args->debug = debug;
          args->curr_tuple = curr_tuple_cpy;
          
          pthread_mutex_lock(mutex);
          int ret = pthread_create(&threads[threads_used], NULL, 
                                   start_thread_sec_step_mult, args);
          pthread_mutex_unlock(mutex);
          
          if(ret){
            Tuple_free(curr_tuple_cpy);
            free(args);
            secrets_step_parallel(c, incompr_tuples, new_max_size, secrets, randoms,
                                  gauss_deps_o, gauss_rands_o, gauss_length_o,
                                  gauss_deps_i, gauss_rands_i, 
                                  next_secret_share_idx - 1, secrets_count + 1, 
                                  secret_idx, curr_tuple, 
                                  new_required_outputs_remaining, RPC, t_in, 
                                  mutex, debug);
            
          }
          else{
            threads_used += 1;
          }
        }
        curr_tuple->length--;
      }
    }
    for(int m = 0; m < threads_used; m++){
      pthread_join(threads[m], NULL);
    } 
    if (threads_used_1 == 1)
      pthread_join(threads1, NULL);
    
  }
}


void *start_thread_sec_step_mult (void *void_args){
  
  struct sec_step_args *args = (struct sec_step_args *) void_args;  
  int max_deps_length = args->max_size + args->required_outputs_remaining + 2; // TODO Victor : Normally +1 but sometiomes in randoms step it creates bug.
  // |gauss_deps_o| and |gauss_rands_o|: for main Gauss eliminiation,
  // on Output randoms.
  Dependency** gauss_deps_o = malloc(max_deps_length * sizeof(*gauss_deps_o));
  for (int i = 0; i < max_deps_length; i++) {
    gauss_deps_o[i] = calloc(args->c->deps->deps_size, sizeof(*gauss_deps_o[i]));
  }
  Dependency* gauss_rands_o = malloc(max_deps_length * sizeof(*gauss_rands_o));
  
  //Compute the number of refresh made on the multiplication.
  int refresh_i1 = 0, refresh_i2 = 0;
  int non_mult_deps_count = args->c->deps->deps_size - args->c->deps->mult_deps->length;
  for (int i = args->c->deps->first_rand_idx; i < non_mult_deps_count; i++) {
    refresh_i1 += args->c->i1_rands[i];
    refresh_i2 += args->c->i2_rands[i];
  }
  
  int max_deps_length_i = max_deps_length * (max(refresh_i1, refresh_i2) + 1 + args->c->share_count);
  // |gauss_deps_i| and |gauss_rands_i|: for secondary Gauss
  // elimination, on Input randoms.
  Dependency** gauss_deps_i= malloc(max_deps_length_i * sizeof(*gauss_deps_i));
  for (int i = 0; i < max_deps_length_i; i++) {
    gauss_deps_i[i] = calloc(args->c->deps->deps_size, sizeof(*gauss_deps_i[i]));
  }
  Dependency* gauss_rands_i = malloc(max_deps_length_i * sizeof(*gauss_rands_i));
  
  int *gauss_length_o = malloc(sizeof(int));
  *gauss_length_o = 0;
  
  secrets_step_parallel(args->c, args->incompr_tuples, args->max_size, 
                        args->secrets, args->randoms, gauss_deps_o, 
                        gauss_rands_o, gauss_length_o, gauss_deps_i, gauss_rands_i, 
                        args->next_secret_share_idx, args->secrets_count,
                        args->secret_idx, args->curr_tuple, 
                        args->required_outputs_remaining, args->RPC, args->t_in,
                        args->mutex, args->debug);
  
  free(gauss_rands_o);
  free(gauss_rands_i);
  for (int i = 0; i < max_deps_length; i++)
    free(gauss_deps_o[i]);
  
  for (int i = 0; i < max_deps_length_i; i++)
    free(gauss_deps_i[i]);
  
  free(gauss_deps_o);
  free(gauss_deps_i);
  
  free(gauss_length_o);
  Tuple_free(args->curr_tuple);
  free(args);
  
  return NULL;
}


/*
Build the trie of incompr_tuples for the multiplication gadget.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -VarVector **secrets : Array 2D of integer. Each line contains an array of 
                         index of probes that contains the line-th share we 
                         found in the |c->deps| array. For example, 
                         the first line contains the index of the probes who can
                         leak the first share of the first secret value.   
                          
  -VarVector **randoms : Array 2F of integer. Each line contains an array of
                         index of probes that contains the line-th randoms.
                         
  -int coeff_max : The maximal size of the incompressible tuples.
  -int required_outputs : Number of outputs required in each tuple.
  -bool RPC : If true, we are checking the RPC (or RPE1 sometimes) property.
  -int t_in : The number of shares that must be leaked for a tuple to be a 
              failure.
  -int cores : If cores !=1, we use parallelization every time we can.
  -bool one_failure : Indicates if we have to find one failure tuple or all 
                      the failure tuples.
  -int debug : if true, then some debuging information are printed.
*/
static Trie* build_incompr_tuples(const Circuit* c,
                                  const VarVector** secrets,
                                  const VarVector** randoms,
                                  int coeff_max,
                                  int required_outputs,
                                  bool RPC,
                                  int t_in,
                                  int cores,
                                  bool one_failure,
                                  int debug) {
  //TODO Victor: Possibly some bugs but normally this size is the maimum possible one.                                
  int max_deps_length = coeff_max + required_outputs + 2;
  Tuple* curr_tuple = Tuple_make_size(c->deps->length);
  
  // |gauss_deps_o| and |gauss_rands_o|: for main Gauss eliminiation,
  // on Output randoms.
  Dependency** gauss_deps_o = malloc(max_deps_length * sizeof(*gauss_deps_o));
  for (int i = 0; i < max_deps_length; i++) {
    gauss_deps_o[i] = calloc(c->deps->deps_size, sizeof(*gauss_deps_o[i]));
  }
  Dependency* gauss_rands_o = malloc(max_deps_length * sizeof(*gauss_rands_o));

  int share_count = c->share_count;
  // Creation of the trie.
  Trie* incompr_tuples = make_trie(c->deps->length);

  //Compute the number of refresh made on the multiplication.
  int refresh_i1 = 0, refresh_i2 = 0;
  int non_mult_deps_count = c->deps->deps_size - c->deps->mult_deps->length;
  for (int i = c->deps->first_rand_idx; i < non_mult_deps_count; i++) {
    refresh_i1 += c->i1_rands[i];
    refresh_i2 += c->i2_rands[i];
  }
  
  //TODO Victor: Possibly some bugs but normally this size is the maximum 
  //possible one.                                
  int max_deps_length_i = max_deps_length * 
                          (max(refresh_i1,refresh_i2) + 1 + share_count);
  
  // |gauss_deps_i| and |gauss_rands_i|: for secondary Gauss
  // elimination, on Input randoms.
  Dependency** gauss_deps_i= malloc(max_deps_length_i * sizeof(*gauss_deps_i));
  for (int i = 0; i < max_deps_length_i; i++) {
    gauss_deps_i[i] = calloc(c->deps->deps_size, sizeof(*gauss_deps_i[i]));
  }
  Dependency* gauss_rands_i = malloc(max_deps_length_i * sizeof(*gauss_rands_i));
  
  int max_incompr_size = share_count + c->random_count;
  max_incompr_size = coeff_max == -1 ? max_incompr_size :
    coeff_max > max_incompr_size ? max_incompr_size : coeff_max;
  
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    
  for (int max_size = 1; max_size <= max_incompr_size; max_size++) {
    if (cores == 1){
      for (int i = 0; i < c->secret_count; i++) {
        // Randoms refreshing the other input cannot interfer with the
        // current one. The "real" maximal size is thus smaller than
        // |max_incompr_size|. Adjusting here with those two ifs.
        if (i == 0 && max_size > share_count + c->random_count - refresh_i2) continue;
        if (i == 1 && max_size > share_count + c->random_count - refresh_i1) continue;
        
        int gauss_length_o = 0;
        secrets_step(c, incompr_tuples, max_size, &secrets[share_count * i], randoms,
                     gauss_deps_o, gauss_rands_o, &gauss_length_o, gauss_deps_i, 
                     gauss_rands_i, share_count-1, 0, i, curr_tuple, 
                     required_outputs, RPC, t_in, debug);
      }
    } else {
      pthread_t threads;
      Tuple* curr_tuple_bis = Tuple_make_size(c->deps->length);
      struct sec_step_args *args = malloc(sizeof (*args));
      args->c = c; 
      args->incompr_tuples = incompr_tuples;
      args->max_size = max_size;
      args->secrets = secrets;
      args->randoms = randoms;
      args->next_secret_share_idx = share_count - 1;
      args->secrets_count = 0;
      args->secret_idx = 0;
      args->required_outputs_remaining = required_outputs;
      args->RPC = RPC;
      args->t_in = t_in;
      args->mutex = &mutex;
      args->debug = debug;
      args->curr_tuple = curr_tuple_bis;
        
      int ret = pthread_create(&threads, NULL, start_thread_sec_step_mult, args);
      
      int *gauss_length_o = calloc(1, sizeof(int));
      secrets_step_parallel(c, incompr_tuples, max_size, &secrets[share_count], 
                            randoms, gauss_deps_o, gauss_rands_o, 
                            gauss_length_o, gauss_deps_i, gauss_rands_i, 
                            share_count-1, 0, 1, curr_tuple, required_outputs, 
                            RPC, t_in, &mutex, debug);
     free(gauss_length_o);
      
      if (ret){
        gauss_length_o = calloc(1, sizeof(int));
        secrets_step_parallel(c, incompr_tuples, max_size, secrets, randoms, 
                              gauss_deps_o, gauss_rands_o, gauss_length_o, 
                              gauss_deps_i, gauss_rands_i, share_count-1, 0, 0, 
                              curr_tuple, required_outputs, RPC, t_in, &mutex, 
                              debug);
        free(gauss_length_o); 
      }
      else 
        pthread_join(threads, NULL);
    }
    
    //If we only have to find one failure and incompr_tuples is not empty, 
    //we can stop.
    if (one_failure && trie_tuples_size(incompr_tuples, max_size))
      break;
    
  }
  
  //Freeing stuff 
  free(gauss_rands_o);
  free(gauss_rands_i);
  for (int i = 0; i < max_deps_length; i++)
    free(gauss_deps_o[i]);
  
  for (int i = 0; i < max_deps_length_i; i++)
    free(gauss_deps_i[i]);
  
  free(gauss_deps_o);
  free(gauss_deps_i);
  
  Tuple_free(curr_tuple);
  return incompr_tuples;
}

/*
Compute the incompressible tuples we found for the multiplication gadget we are 
studying.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int coeff_max : The maximal size of the incompressible tuples.
  -bool include_outputs : if true, we can include outputs index in the array 
                          secrets and randoms.
  -int required_outputs : Number of outputs required in each tuple.
  -bool RPC : If true, we are checking the RPC (or RPE1 sometimes) property.
  -int t_in : The number of shares that must be leaked for a tuple to be a 
              failure.
  -int cores : If cores !=1, we use parallelization every time we can.
  -bool one_failure : Indicates if we have to find one failure tuple or all 
                      the failure tuples.
  -int verbose : Set a level of verbosity.
  
Output : A trie of all the incompressibles tuples of size <= |coeff_max|.
*/
Trie* compute_incompr_tuples_mult_arith(const Circuit* c, int coeff_max, 
                                        bool include_outputs, int required_outputs, 
                                        bool RPC, int t_in, int cores,
                                        bool one_failure, int verbose) {
  VarVector** secrets;
  VarVector** randoms;
  build_dependency_arrays_mult(c, &secrets, &randoms, include_outputs, verbose);
  
  Trie *incompr_tuples = build_incompr_tuples(c, (const VarVector**)secrets,
                                              (const VarVector**)randoms,
                                              coeff_max, required_outputs, RPC, 
                                              t_in, cores, one_failure, verbose);

  
  // Freeing stuffs
  {
    int total_secrets    = c->secret_count * c->share_count;
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
Like build_incompr_tuples but for the property RPE,
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
  -int coeff_max : The maximal size of the incompressible tuples.
  -int required_outputs : Number of outputs required in each tuple.
  -bool RPC : If true, we are checking the RPC (or RPE1 sometimes) property.
  -int t_in : The number of shares that must be leaked for a tuple to be a 
              failure.
  -int secret_idx : The secret value we are currently studying.
  -pthread_mutex_t *mutex : Mutex for parallelization purpose.
  -int cores : If cores !=1, we use parallelization every time we can.
  -int debug : if true, then some debuging information are printed.
*/ 
static Trie* build_incompr_tuples_RPE(const Circuit* c,
                                      const VarVector** secrets,
                                      const VarVector** randoms,
                                      int coeff_max,
                                      int required_outputs,
                                      bool RPC,
                                      int t_in,
                                      int secret_idx,
                                      pthread_mutex_t *mutex,
                                      int cores,
                                      int debug) {
  int max_deps_length = c->deps->length;

  Tuple* curr_tuple = Tuple_make_size(c->deps->length);
  
  // |gauss_deps_o| and |gauss_rands_o|: for main Gauss eliminiation,
  // on Output randoms.
  Dependency** gauss_deps_o = malloc(max_deps_length * sizeof(*gauss_deps_o));
  for (int i = 0; i < max_deps_length; i++) {
    gauss_deps_o[i] = calloc(c->deps->deps_size, sizeof(*gauss_deps_o[i]));
  }
  Dependency* gauss_rands_o = malloc(max_deps_length * sizeof(*gauss_rands_o));
  
  // |gauss_deps_i| and |gauss_rands_i|: for secondary Gauss
  // elimination, on Input randoms.
  Dependency** gauss_deps_i= malloc(max_deps_length * sizeof(*gauss_deps_i));
  for (int i = 0; i < max_deps_length; i++) {
    gauss_deps_i[i] = calloc(c->deps->deps_size, sizeof(*gauss_deps_i[i]));
  }
  Dependency* gauss_rands_i = malloc(max_deps_length * sizeof(*gauss_rands_i));

  int share_count = c->share_count;
  //Creation of the trie.
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
    
  for (int max_size = 1; max_size <= max_incompr_size; max_size++) {
    // Randoms refreshing the other input cannot interfer with the
    // current one. The "real" maximal size is thus smaller than
    // |max_incompr_size|. Adjusting here with those two ifs.
    if (secret_idx == 0 && max_size > share_count + c->random_count - refresh_i2) continue;
    if (secret_idx == 1 && max_size > share_count + c->random_count - refresh_i1) continue;
    
    if(cores == 1){
      int gauss_length_o = 0;
      secrets_step(c, incompr_tuples, max_size, 
                   &secrets[share_count * secret_idx], randoms, gauss_deps_o, 
                   gauss_rands_o, &gauss_length_o, gauss_deps_i, gauss_rands_i, 
                   share_count-1, 0, secret_idx, curr_tuple, required_outputs, 
                   RPC, t_in, debug);
    }
    else{
      //Enter in the parallelized version of |secrets_step|.
      int* gauss_length_o = calloc(1, sizeof(int));
      secrets_step_parallel(c, incompr_tuples, max_size, 
                            &secrets[share_count * secret_idx], randoms, 
                            gauss_deps_o, gauss_rands_o, gauss_length_o, 
                            gauss_deps_i, gauss_rands_i, share_count-1, 0, 
                            secret_idx, curr_tuple, required_outputs, RPC, t_in,
                            mutex, debug);
      free(gauss_length_o);
    }
  }
  
  //Freeing stuff
  Tuple_free(curr_tuple);
  free(gauss_rands_i);
  free(gauss_rands_o);
  for (int i = 0 ; i < max_deps_length; i++){
    free(gauss_deps_i[i]);
    free(gauss_deps_o[i]);
  }
  free(gauss_deps_i);
  free(gauss_deps_o);
  
  return incompr_tuples;
}

//Parallelisation on the build of incompressible tuples in RPE mode. We 
//parallelize according to secret_idx.
struct b_icpr_RPE_args {
  const Circuit* c;
  const VarVector** secrets;
  const VarVector** randoms;
  int coeff_max;
  int required_outputs;
  bool RPC;
  int t_in;
  int secret_idx;
  pthread_mutex_t mutex;
  int cores;
  int debug; 
};

//Function to start thread.
void *start_thread_b_icpr_RPE(void *void_args){
  struct b_icpr_RPE_args *args = (struct b_icpr_RPE_args *) void_args;
  Trie *incompr_tuples = build_incompr_tuples_RPE(args->c, args->secrets, 
                                                  args->randoms, 
                                                  args->coeff_max, 
                                                  args->required_outputs, 
                                                  args->RPC, args->t_in,
                                                  args->secret_idx,
                                                  &(args->mutex),
                                                  args->cores, 
                                                  args->debug);
  free(args);
  return incompr_tuples;
}



/*
Compute the incompressible tuples we found for the multiplication gadget we are 
studying for every secret value.
Input : 
  -const Circuit *c : The arithmetic circuit we are currently studying.
  -int coeff_max : The maximal size of the incompressible tuples.
  -bool include_outputs : if true, we can include outputs index in the array 
                          secrets and randoms.
  -int required_outputs : Number of outputs required in each tuple.
  -int t_in : The number of shares that must be leaked for a tuple to be a 
              failure.
  -int cores : If cores !=1, we use parallelization every time we can.
  -int verbose : Set a level of verbosity.
  
Output : An array of trie of all the incompressibles tuples of 
         size <= |coeff_max| by secret values evaluated.
*/
Trie** compute_incompr_tuples_mult_RPE_arith(const Circuit* c, int coeff_max, 
                                             bool include_outputs, 
                                             int required_outputs, int t_in, 
                                             int cores, int verbose) {
  VarVector** secrets;
  VarVector** randoms;
  build_dependency_arrays_mult(c, &secrets, &randoms, include_outputs, verbose);
  Trie** incompr_tuples_tab  = malloc(c->secret_count * sizeof(*incompr_tuples_tab));
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  
  if (cores == 1){
    for (int secret_idx = 0; secret_idx < c->secret_count; secret_idx++) { 
      Trie* incompr_tuples = 
        build_incompr_tuples_RPE(c, (const VarVector**) secrets,
                                 (const VarVector**) randoms, coeff_max, 
                                 required_outputs, true, 
                                 t_in, secret_idx, &mutex, cores, verbose);
    
      incompr_tuples_tab[secret_idx] = incompr_tuples;
    }
  }
  
  else {
    //Parallelisation step.
    pthread_t thread;
    Trie *incompr_tuples;
    
    struct b_icpr_RPE_args *args = malloc(sizeof(*args));
    args->c = c;
    args->secrets = (const VarVector**) secrets;
    args->randoms = (const VarVector**) randoms;
    args->coeff_max = coeff_max;
    args->required_outputs = required_outputs;
    args->RPC = true;
    args->t_in = t_in;
    args->secret_idx = 0;
    args->mutex = mutex;
    args->cores = cores;
    args->debug = verbose;
      
    int ret = pthread_create(&thread, NULL, start_thread_b_icpr_RPE, 
                             (void *) args);
    
    
    Trie *incompr_tuples_1 = 
      build_incompr_tuples_RPE(c, (const VarVector**) secrets,
                               (const VarVector**) randoms, coeff_max, 
                               required_outputs, true, 
                               t_in, 1, &mutex, cores, verbose);
                               
      
    if (ret){
      incompr_tuples = 
        build_incompr_tuples_RPE(c, (const VarVector**) secrets,
                                 (const VarVector**) randoms, coeff_max, 
                                 required_outputs, true, 
                                 t_in, 0, &mutex, cores, verbose);      
    }
    
    else{
      void *return_value;
      pthread_join(thread, &return_value);
      incompr_tuples = (Trie *) return_value;
    }
    incompr_tuples_tab[0] = incompr_tuples;
    incompr_tuples_tab[1] = incompr_tuples_1;
    
  }
  
  {
    int total_secrets    = c->secret_count * c->share_count;
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
  
  return incompr_tuples_tab;
}



