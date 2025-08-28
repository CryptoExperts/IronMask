#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#include "constructive-mult_arith.h"
#include "circuit.h"
#include "combinations.h"
#include "list_tuples.h"
#include "verification_rules.h"
#include "trie.h"
#include "coeffs.h"

// For debug purposes only: the number of failures that are generated
// multiple times.
int regenerated = 0;

/* **************************************************************** */
/*                    Hash table implementation                     */
/* **************************************************************** */



// Small note regarding the size of the hash table. In the current
// implementation, the whole hash table has to be iterated through to
// compute each coefficients. This operation is a bit expensive for
// small coefficients (iterating millions of elements to find out that
// only 10 are non-null...). However, when the table starts to contain
// more elements, it's important to avoid collisions as much as
// possible, and thus to have a large table.
//
// Also, note that it would be a bit expansive to dynamically change
// the size of the hash map, since it would require recomputing the
// hash for each tuple it contains...

#define HASH_SIZE (HASH_MASK+1)
static uint64_t HASH_MASK; /* Size of the HashMap we will use */

/*
Determine the size of the HashMap we are going to use. This is the optimal 
integer int the worst-case complexity.
Input :
  -int c_len : The length of the circuit we are currently studying.
  -int coeff_max : The maximal coefficient that we have to compute.
*/

// Computes a hash for integer |x|.
// Hash function for integers from https://stackoverflow.com/a/12996028/4990392
/*static unsigned int hash_int(unsigned int x, int *num_quo) {
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = (x >> 16) ^ x;
  *num_quo = (x / HASH_MASK);
  return x % HASH_MASK;
}*/

// Computes a hash for |comb| by summing the hashes of each of its elements.
/*static unsigned int hash_comb(Comb* comb, int comb_len, int *num_quo) {
  unsigned int hash = 0;
  for (int i = 0; i < comb_len; i++) {
    int num_quo_x = 0;
    hash += hash_int(comb[i], &num_quo_x);
    *num_quo += num_quo_x;
  }
  return hash;
}*/




/*
Here I change the maneer of compute the hash. Before, we use hash_comb and 
hash_int to compute the hash of |comb|. Nevertheless, this implies to deal with 
collision because two |comb| can have the same hash. And after, when we have to 
check if a |comb| is already in the HashMap or not, we have to check coefficient
by coefficient if the |comb| in the Hash Table is the same or not, and this is 
a big lost of time in the program.
That's why we investigate another method to compute the hash. In particular, 
we want a way to have, for each |comb| a different hash. The first idea was the 
following : 
We will number the different tuples.
An example will be simple to illustrate the idea. If we are with tuples of size 
3 and suppose we have 10 index to number. Then :

 -There is 2 among 9 possible |comb| of size 3 that starts by 0. Therefore we 
  will number the |comb| that starts by 0 from 0 to 35.
  
  
 -There is 2 among 8 possible |comb| of size 3 that starts by 1 (with |comb| 
  sorted.) Therefore we will number the |comb| that starts by 1 from 36 to 
  36 + (28 - 1) = 63.
  
  -...
  
  -There is 2 among 2 possible |comb| of size 3 that starts by 7. Therefore
   we will number the |comb| that starts by 8 from to 119 to 119.
   
   Here we number the probes for the first element, but we can do it recursively
   after, for example, we see that the |comb| that starts by 0 has been numbered
   from 0 to 35, so :
    
    -There is 1 among 8 possible |comb| that starts by 0 then by 1. Therefore, 
     we will number this |comb| from 0 to 8 - 1 = 7.  
    
    -There is 1 among 7 possible |comb| that starts by 0 then by 2 (and |comb| 
     sorted). Therefore, we will number this |comb| from 8 to 8  + 7 - 1 = 14.
     
    -...
    
    -There is 1 among 1 possible |comb| that starts by 0 then by 8. Therefore,
    we will number this |comb| from 35 to 35.
    
    
Then, we can see how we can number the differents |comb| : 

-[0,1,2] ---> 0
-[0,1,3] ---> 1
...
-[0,1,9] ---> 7
-[0,2,3] ---> 8
...
-[0,2,9] ---> 14
...
-[0,8,9] ---> 35
-[1,2,3] ---> 36
...
-[1,8,9] ---> 63
...
-[7,8,9] --->119  


In this way, all the |comb| can compute an hash who are different. Nevertheless,
the numbering can become very big and surpass the size HASH_MASK. To prevent this 
issue, I have to reduce the numbering used with some modular operation, with the 
modulo HASH_MASK (TODO : why not HASH_SIZE ?). Of course, this will create 
collision too, that's why I had a component in the hashNode structure, 
named quo_hash, it gives the quotient of the numbering. And with the quotient 
and the modulo, we have unicity. Therefore , for check combs, we have just to 
compute his numbering, the modulo and the quotient.     
*/


/*
Computes the numbering for |comb|, using the method in the comments above, 
that we will use as an hash.
Input :
  -Comb *comb : The tuple we wants to number.
  -int comb_len : The size of the array |comb|.
  -int var_count : The number of different index we can have in our tuple.
  -int *num_quo : A pointer of an integer in which we will add the quotient of 
                  the numbering of the tuple.  
Output : The numbering of the tuple.
*/
static int num_tab_comb(Comb *comb, int comb_len, int var_count, int *num_quo){
  int num_tab = 0;
  int i = 0;
  int ind = 0;
  while (ind < comb_len){
    if (comb[ind] == i){
      ind += 1;
      i += 1;
    }
    
    else{
      
      num_tab += n_choose_k(comb_len - 1 - ind, var_count - 1 - i);
      *num_quo += (num_tab / HASH_SIZE);
      num_tab %= HASH_SIZE;
      i += 1;
    }
  }
  return num_tab;
}

/*
Build the tuple (|comb|, x) sorted and compute his numbering.
Input :
  -Comb *comb : The tuple in which we will add x.
  -int comb_len : The size of the array |comb|.
  -int var_count : The number of different index we can have in our tuple.
  -int x : The integer that we will add to |comb|.
  -int index_comb : the integer who is the index in the |new_comb| build 
                    from |comb| and x at which we will add x.
  -int *num_quo : A pointer of an integer in which we will add the quotient of 
                  the numbering of the tuple.                  
Output : The numbering of the tuple (|comb|, x) sorted.  
*/
static int update_num_tab(Comb *comb, int comb_len, int var_count, int x, 
                          int index_comb, int *num_quo){
  
  //Creation of the (|comb|, x)
  Comb new_comb[comb_len + 1];
  for (int i = 0; i < index_comb; i++)
    new_comb[i] = comb[i];
    
  new_comb[index_comb] = x;
  
  for (int i = index_comb + 1; i < comb_len + 1; i++)
    new_comb[i] = comb[i - 1];
  
  //Compute the numbering of (|comb|, x)
  return num_tab_comb(new_comb, comb_len + 1, var_count, num_quo);
}

// |hash| is the hash of a Comb*, and |x| an element that we would
// like to add to that Comb*. This function returns the hash of the
// new Comb* (without having to recompute it entirely)
/*static unsigned int update_hash(unsigned int hash, int x, int *num_quo) {
   return hash + hash_int(x, num_quo);
}*/

typedef struct _hashnode {
  Comb* comb;             /* tuples */
  unsigned int quo_hash;  /* See the comments above the function num_tab_comb 
                             for more details.*/
  struct _hashnode* next; /* Next element on the node */
} HashNode;

typedef struct _hashmap {
  HashNode** content;
  unsigned int comb_len; // The size of the tuples inside this hash
  int count; // The number of elements that were added to this hashmap
} HashMap;


/*Determine the size of the Hash Table we will use. it will be the minimum 
between the maximum number of failure tuple we can have "theorically" 
and a born to prevent integer overflow that I called overflow_trigger in the 
function. 
Input :
    -int c_len : The length of the circuit.
    -int coeff_max : the maximum coefficient c in the function f of random 
                     probing security notions we are studying.
*/
void determine_HASH_MASK(int c_len, int coeff_max){ 
  HASH_MASK = n_choose_k(coeff_max, c_len) - 1;
  int size = sizeof(HashNode*);
  /* To avoid overflow when I call calloc for map->content and let some free memory.*/
  uint64_t overflow_trigger = (uint64_t)((INT_MAX / (4 * size)) - 2);
  if (HASH_MASK > overflow_trigger){
    HASH_MASK = overflow_trigger;
  }
 
  printf("HASH_MASK = %lu\n", HASH_MASK);
}

// Allocates and initializes an empty hash map capable of holding
// elements of size |comb_len|.
static HashMap* init_hash(int comb_len) {
  HashMap* map = malloc(sizeof(*map));
  map->content  = calloc(HASH_SIZE, sizeof(*(map->content)));
  map->comb_len = comb_len;
  map->count    = 0;
  return map;
}

// Frees all elements contained in |map|, but does not free |map| itself.
static void empty_hash(HashMap* map, int verbose) {
  int used_buckets = 0;
  int collisions = 0;
  for (int i = 0; i < (int)(HASH_SIZE); i++) {
    HashNode* node = map->content[i];
    if (node) used_buckets++;
    while (node) {
      HashNode* next = node->next;
      free(node->comb);
      free(node);
      if (next) collisions++;
      node = next;
    }
    map->content[i] = NULL;
  }
  map->count = 0;

  if (verbose > 5) {
    printf("Comb_len=%d ---> map used at %d%%  ---  collisions:%d%%.\n", map->comb_len,
           (int)(((double)used_buckets / (HASH_SIZE)) * 100),
           (int)(((double)collisions / used_buckets) * 100));
  }
}

// Frees the content of |map|, as well as |map| itself.
static void free_hash(HashMap* map, int verbose) {
  empty_hash(map, verbose);
  free(map->content);
  free(map);
}

// Adds |comb| to |map| at index |hash|. Do not check whether it's
// already in it or not.
// Assumes that |comb| is already sorted.
static void add_to_hash_with_key(HashMap* map, Comb* comb, unsigned int hash,
                                 unsigned int hash_quo) {
  HashNode* old = map->content[hash];
  HashNode* new = malloc(sizeof(*new));
  new->comb = comb;
  new->quo_hash = hash_quo;
  new->next = old;
  map->content[hash] = new;
  map->count++;
}

//Compute the numbering of the tuple |comb|, called |num_tab| and 
//adds |comb| to |map| at index |num_tab|. Do not check whether it's
//already in it or not.
//Assumes that |comb| is already sorted.
static void add_to_hash_num_tab(HashMap* map, Comb *comb, int comb_len, 
                                int var_count){
  
  int num_quo = 0;
  int num_tab = num_tab_comb(comb, comb_len, var_count, &num_quo);
  unsigned int hash_quo = num_tab / HASH_SIZE;
  hash_quo += num_quo;
  num_tab %= HASH_SIZE;
  
  add_to_hash_with_key(map, comb, num_tab, hash_quo);

}

// Adds in |map| the incompressible tuples of size |size| contained in
// |incompr|.
static void add_incompr_to_map(HashMap* map, Trie* incompr, int size,
                               int var_count) 
{
  ListComb* incompr_list = list_from_trie(incompr, size);
  ListCombElem* curr = incompr_list->head;
  while (curr) {
    sort_comb(curr->comb, size);
    add_to_hash_num_tab(map, curr->comb, size, var_count);
    
    ListCombElem* next = curr->next;
    // Not freeing curr->comb since it now is in the hashmap
    free(curr);
    curr = next;
  }
  free(incompr_list);
}

/* **************************************************************** */
/*                         Failures generation                      */
/* **************************************************************** */

// Update the coefficients |coeffs| with the tuples contained in |map|.
void update_coeffs_with_hash(const Circuit* c, uint64_t* coeffs, HashMap* map) {
  int comb_len = map->comb_len;
  for (int i = 0; i < (int)(HASH_SIZE); i++) {
    HashNode* node = map->content[i];
    while (node) {
      update_coeff_c_single(c, coeffs, node->comb, comb_len);
      node = node->next;
    }
  }
}

/*
Check if the tuple give by the hash and the quotient is in the Hash table.
Input : 
  -HashMap *map : The hash table in which we have to check if the tuple is in 
                  or not.
  -int hash : The numbering of the tuple modulo HASH_MASK.
  -unsigned int quo_hash : The quotient of the numbering of the tuple by 
                           HASH_MASK. 
Output : True if the tuple is in map. False otherwise.
*/
bool tuple_is_in(HashMap *map, int hash, unsigned int quo_hash){
  HashNode *node = map->content[hash];
  while (node){
    if (node->quo_hash == quo_hash){ 
      //Hash and quotient are the same, so the tuple is already in map.
      return true;
    }
    node = node->next;
  }
  return false; 
}


/* Update the coefficients |coeffs| with the tuples contained in |map|.
Input :
    -const Circuit *c : The arithmetic circuit.
    -uint64_t *coeffs : The coefficients to update.
    -HashMap **map : Array of pointers of map. Each map contain the failure 
                     tuple of one subset of output index.
    -int len_map : The number of different pointers of map (i.e the number of 
                   subsets of output index of a certain size).
*/
void update_coeffs_with_hash_RPE2(const Circuit* c, uint64_t* coeffs, 
                                  HashMap** map, int len_map) {
  //Browsing the failure tuple in the first map.
  int comb_len = map[0]->comb_len;
  for (int i = 0; i < (int)(HASH_SIZE); i++) {
    HashNode* node = map[0]->content[i];
    while (node) {
      Comb *comb = node->comb;
      unsigned int hash_quo = node->quo_hash;
      bool is_good = true;
      // Verifying if the comb |comb| is in all the maps. If it is not the case,
      // then, we have an output set where this comb is not a failure tuple. 
      // By definition of RPE2, this is not a failure tuple.
      for (int idx = 1; idx < len_map && is_good; idx++){        
        is_good &= tuple_is_in(map[idx], i, hash_quo);
      }
      
      if (is_good){
        update_coeff_c_single(c, coeffs, comb, comb_len);
      }
      
      node = node->next;
    }     
  }
}



// Checks if the tuple (|comb|, |x|) is in |dst| at index |hash|. If
// not, then this tuple is added to |dst|. |comb_len| is the length of
// |comb|.
//
// The code is somewhat not straightfoward because it does not build
// the tuple (|comb|, |x|) to check whether its in |dst| or not (in
// order to avoid mallocing too much).
void check_comb_and_add(HashMap* dst, unsigned int hash,
                        Comb* comb, int x, int comb_len, int num_quo) {
  // Part 1: check if the tuple (|comb|, |x|) is in |dst|
  unsigned int hash_quo = hash / HASH_SIZE;
  hash_quo += num_quo;
  hash %= HASH_SIZE;
      
  HashNode* node = dst->content[hash];
  while (node) {
    if (hash_quo == node->quo_hash) {
      regenerated++;
      return;
    } else {
      node = node->next;
    }
  }
  
  // Part 2: the tuple is not already in the hash -> build it now.
  Comb* new_comb = malloc((comb_len+1) * sizeof(*new_comb));
  int i = 0;
  while (i < comb_len && comb[i] < x) {
    new_comb[i] = comb[i];
    i++;
  }
  new_comb[i++] = x;
  while (i < comb_len+1) {
    new_comb[i] = comb[i-1];
    i++;
  } 
  
  // Part 3: add the tuple to the hash.
  HashNode* new_node = malloc(sizeof(*new_node));
  new_node->comb     = new_comb;
  new_node->quo_hash = hash_quo;
  new_node->next     = dst->content[hash];
  dst->content[hash]  = new_node;
  dst->count++;
}

// This function considers all super-tuples of |comb| with 1 more
// element that |comb|. For each of those tuples, it calls
// check_comb_and_add, which will add them to |dst| if they are not
// already in it.
//
// The non-optimized pseudo-code of this function is:
//
//    for i = 0 to var_count-1:
//        if comb does not contain i:
//            if dst does not contain the tuple (comb, i):
//                add (comb, i) to dst
//
// We take advantage of the fact that |comb| is sorted (by
// construction) to optimize this pseudo-code:
//
//  - we can thus avoid the step "if comb does not contain i",
//    which would otherwise have a linear cost in |comb_len|
//
//  - we do not have to generate the tuple (|comb|, |i|) to check
//    if it is in |dst|.
//
void expand_tuple(HashMap *dst, Comb *comb, int comb_len, int var_count){
  if (comb_len == 0){
    // Adding elements at the start
    for (int i = 0; i < var_count; i++) {
      int num_quo = 0;
      // Creation of the hash of the super-tuple.
      int new_num_tab = update_num_tab(comb, comb_len, var_count, i, 0, &num_quo);
      check_comb_and_add(dst, new_num_tab, comb, i, comb_len, num_quo);
    }
    return;
  }
  
  
  
  int first = comb[0];
  int last = comb[comb_len-1];
      
  // Adding elements at the start
  for (int i = 0; i < first && i < var_count; i++) {
    int num_quo = 0;
    // Creation of the hash of the super-tuple.
    int new_num_tab = update_num_tab(comb, comb_len, var_count, i, 0, &num_quo);
    check_comb_and_add(dst, new_num_tab, comb, i, comb_len, num_quo);
  }
  // Adding elements in the middle
  for (int j = 0; j < comb_len-1; j++) {
    for (int i = comb[j]+1; i < comb[j+1] && i < var_count; i++) {
      int num_quo = 0;
      //Creation of the hash of the super-tuple.           
      int new_num_tab = update_num_tab(comb, comb_len, var_count, i, j + 1, &num_quo);
      check_comb_and_add(dst, new_num_tab, comb, i, comb_len, num_quo);
    }
  }
  // Adding elements at the end
  for (int i = last+1; i < var_count; i++) {
    int num_quo = 0;
    //Creation of the hash of the super-tuple.
    int new_num_tab = update_num_tab(comb, comb_len, var_count, i, comb_len, &num_quo);
    check_comb_and_add(dst, new_num_tab, comb, i, comb_len, num_quo);
  }
}



/* 
For each tuple in |curr|, this function will generate all super-tuples with 1 
more elements and add them in |next| if they are not already in it.

Input : 
  -HashMap *curr : An hash table containing error tuples of a certain size.
  -HashMap *next : An empty hash table, in which we are going to put the 
  super-tuples of a |certain size| + 1.
  -int var_count : An integer to set a limit on which variable we can add for 
  build the super-tuple of |certain size| + 1. For example, if we don't want to 
  add output index in the super-tuple, then we will have var_count = c->length,
  where  is the circuit we are studying.   
*/
void expand_tuples(HashMap* curr, HashMap* next, int var_count) {
  int comb_len = curr->comb_len;
  for (int i = 0; i < (int)(HASH_SIZE); i++) {
    HashNode* node = curr->content[i];
    while (node) {
      expand_tuple(next, node->comb, comb_len, var_count);   
      node = node->next;
    }
  }
}


// Pseudo-code:
//
//  procedure gen_failures(_incompr_):   # _incompr_ is the trie of incompressible failures
//  |
//  |   _curr_ = {}
//  |   for i = 1 to max size of tuple in |incompr|:
//  |   |   _next_ = {}
//  |   |   for failure _f_ in _curr_:
//  |   |   |   for each number _n_ in _1_.._numberOfVariables_:
//  |   |   |   |   _t_ = (_f_, _n_)
//  |   |   |   |   if _t_ \notin _next_:
//  |   |   |   |   |   add _t_ to _next_
//  |   |   add all elements of size _i_ of _incompr_ in _next_
//  |   |   Count elements in _next_    # That's the i-th coeff
//  |   |   _curr_ = _next_
//
void compute_failures_from_incompressibles(const Circuit* c, Trie* incompr,
                                           int coeff_max, int verbose) {
  int var_count = c->length;
  int concise = verbose < 5;

  uint64_t coeffs[c->total_wires+1];
  for (int i = 0; i <= c->total_wires; i++) {
    coeffs[i] = 0;
  }
  if (coeff_max == c->total_wires) coeff_max = c->total_wires - 1;

  if (concise) {
    printf("[ ");
    fflush(stdout);
  }

  HashMap* curr = init_hash(1);
  HashMap* next = init_hash(0);
  for (int i = 0; i < coeff_max; i++) {
    next->comb_len = i + 1;
    expand_tuples(curr, next, var_count);
    add_incompr_to_map(next, incompr, i + 1, var_count);
    
    update_coeffs_with_hash(c, coeffs, next);
    if (concise) {
      printf("%lu, ", coeffs[i + 1]);
      fflush(stdout);
    } else {
      printf("c%d = %lu\n", i+1, coeffs[i + 1]);

      printf("Regenerated: %d%% (%d / %d)\n",
             (int)((double)regenerated/next->count*100),
             regenerated, next->count);
      regenerated = 0;
    }

    empty_hash(curr, verbose);
    HashMap* tmp = curr;
    curr = next;
    next = tmp;
  }

  if (concise) {
    for (int i = coeff_max + 1; i < c->total_wires; i++) {
      printf("%lu, ", coeffs[i]);
    }
    printf("%lu ]\n", coeffs[c->total_wires]);
  } else {
    for (int i = coeff_max+1; i < c->total_wires; i++) {
      printf("c%d = %lu\n", i, coeffs[i]);
    }
  }
  
  free_hash(curr, verbose);
  free_hash(next, verbose);



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
}


/*
Compute the coefficient for the property RPC/RPE1, for a secret value or the 
intersection between 2 secrets values.

Input : 
  -const Circuit* c : The arithmetic circuit we are currently studying.
  -Trie *incompr : An array of incompressible error tuples for the secret value 
                   we are studying.
  -Trie *incompr2 : Useful only when we are computing the coefficents of the 
                    intersection between two secret values (in the RPE1 
                    property). Otherwise, this is set to NULL. This is an array 
                    of incompressible error tuples for another secret values.                     
  -int coeff_max : The maximal coefficient that we have to compute.
  -int verbose : For the level of verbosity.
  -uint64_t *coeffs : The array of coefficients in which we are going to write 
                      the coefficients of error.
  -bool RPE_and : A boolean who indicates if we are computing the coefficients 
                  for the intersection of 2 secrets values or not.  
                    
*/
void compute_failures_from_incompressibles_RPC(const Circuit* c, Trie *incompr,
                                               Trie *incompr2, int coeff_max, 
                                               int verbose, uint64_t *coeffs, 
                                               bool RPE_and) {
  int var_count = c->length;
  if (coeff_max == -1) coeff_max = c->total_wires+1;

  // Creation and Initialisation of the HashMap.
  HashMap* curr = init_hash(1);
  HashMap* next = init_hash(0);
  HashMap* curr2 = NULL;
  HashMap* next2 = NULL;
  HashMap* inter = NULL;
  if (RPE_and){
    curr2 = init_hash(1);
    next2 = init_hash(0);
    inter = init_hash(0);
  }
  
  //Filling the "next" HashMap with errors tuples of size  i + 1 from errors 
  //tuples of size i existing in "curr" HashMap.
  for (int i = -1; i < coeff_max; i++) {
    next->comb_len = i + 1;
    expand_tuples(curr, next, var_count);
    add_incompr_to_map(next, incompr, i + 1, var_count);
    
    if(RPE_and){
      /* Compute the intersection between I1 and I2 */
      inter->comb_len = i + 1;
      next2->comb_len = i + 1;
      expand_tuples(curr2, next2, var_count);
      add_incompr_to_map(next2, incompr2, i + 1, var_count);
      
      for(int j = 0; j < (int)(HASH_SIZE); j++){
        if(next->content[j] && next2->content[j]){
          HashNode* node = next->content[j];
          while (node){
            HashNode* node2 = next2->content[j];
            while (node2){
              if (node->quo_hash == node2->quo_hash){
                Comb *new_comb = malloc(inter->comb_len * sizeof(*new_comb));
                memcpy(new_comb, node->comb, inter->comb_len * sizeof(*new_comb));
                add_to_hash_with_key(inter, new_comb, j, node2->quo_hash);
              }
              node2 = node2->next;
            }
            node = node->next;
          }
        }
      } 
    }
    
    //Updating coefficients with all the rrors of size i + 1 we found.
    if (!RPE_and) update_coeffs_with_hash(c, coeffs, next);
    else update_coeffs_with_hash(c,coeffs, inter);
    
    //Remove the error tuples of size i in "curr" HashMap and add all the errors
    //tuples of size  i + 1 in "curr" HashMap.
    empty_hash(curr, verbose);
    HashMap* tmp = curr;
    curr = next;
    next = tmp;
    
    if (RPE_and){
      empty_hash(curr2, verbose);
      empty_hash(inter, verbose);
      HashMap* tmp = curr2;
      curr2 = next2;
      next2 = tmp;  
    }
    
  }
  
  //Freeing HashMap
  free_hash(curr, verbose);
  free_hash(next, verbose);
  if (RPE_and){
    free_hash(curr2, verbose);
    free_hash(next2, verbose);
    free_hash(inter, verbose);
  }  
}   

/*
Compute the coefficient for the property RPE2, for the first secret value, 
the second secret value and the intersection between the secrets values.

Input : 
  -const Circuit* c : The arithmetic circuit we are currently studying.
  -Trie **incompr : An array of array of incompressible error tuples for the 
                    first secret value. Each line of this array represents an 
                    array of incompressible error tuples for a certain output 
                    index set.
  -Trie **incompr2 : Same as Trie **incompr but with the second secret value.
  -int coeff_max : The maximal coefficient that we have to compute.
  -int verbose : For the level of verbosity.
  -uint64_t *coeffs : The array of coefficients in which we are going to write 
                      the coefficients of error for the first secret value.
  -uint64_t *coeffs2 : Same as uint64_t *coeffs but with the second secret 
                       value.
  -uint64_t *coeffs_and : Same as uint64_t *coeffs but with the intersection 
                          between the first ans the second secret value.                     
*/
void compute_failures_from_incompressibles_RPE2(const Circuit* c, Trie **incompr,
                                                Trie **incompr2, int coeff_max, 
                                                int verbose, uint64_t *coeffs,
                                                uint64_t *coeffs2, 
                                                uint64_t *coeffs_and) {
  int var_count = c->length;
  int nb_output = c->deps->length - var_count;
  
  if (coeff_max == -1) coeff_max = c->total_wires+1;

  //Creation of all the HashMap.
  HashMap *curr[nb_output];
  HashMap *next[nb_output];  
  HashMap *curr2[nb_output];
  HashMap *next2[nb_output];
  HashMap *inter[nb_output]; 
  
  // Initialisation of all the HashMap.
  for (int i = 0; i < nb_output; i++){
    curr[i] = init_hash(1);
    next[i] = init_hash(0);
    curr2[i] = init_hash(1);
    next2[i] = init_hash(0);
    inter[i] = init_hash(0);
  }
  
  //Filling all the HashMap
  for (int i = -1; i < coeff_max; i++) {
    
    //Filling all the "next" and "inter" HashMap with the errors tuples 
    //of size i + 1 from the errors tuples of size i in "curr" HashMap.
    for (int j = 0 ; j < nb_output; j++){
      next[j]->comb_len = i + 1;
      expand_tuples(curr[j], next[j], var_count);
      add_incompr_to_map(next[j], incompr[j], i + 1, var_count);
        
      next2[j]->comb_len = i + 1;
      expand_tuples(curr2[j], next2[j], var_count);
      add_incompr_to_map(next2[j], incompr2[j], i + 1, var_count);
        
      inter[j]->comb_len = i + 1;
      for (int k = 0; k < (int)(HASH_SIZE); k++){
        if (next[j]->content[k] && next2[j]->content[k]){
          HashNode *node = next[j]->content[k];
          while(node){
            HashNode *node2 = next2[j]->content[k];
            while(node2){
              if (node->quo_hash == node2->quo_hash){
                Comb *new_comb = malloc(inter[j]->comb_len * sizeof(*new_comb));
                memcpy(new_comb, node2->comb, inter[j]->comb_len * sizeof(*new_comb));
                add_to_hash_with_key(inter[j], new_comb, k, node2->quo_hash);  
              }
              node2 = node2->next;
            }
            node = node->next;
          }
        }
      }
    }
      
    //Updating the coefficients in each array(coeffs, coeffs2 and coeffs_and) 
    //with all the errors of size i + 1 we found.
    update_coeffs_with_hash_RPE2(c, coeffs, next, nb_output);
    update_coeffs_with_hash_RPE2(c, coeffs2, next2, nb_output);
    update_coeffs_with_hash_RPE2(c, coeffs_and, inter, nb_output);
    
    //Remove the tuples of size i in "curr" HashMap and add the errors tuples 
    //of size i + 1 in "curr" HashMap.
    for (int j = 0; j < nb_output; j++){ 
      empty_hash(curr[j], verbose);
      HashMap* tmp = curr[j];
      curr[j] = next[j];
      next[j] = tmp;
      
      empty_hash(curr2[j], verbose);
      HashMap* tmp2 = curr2[j];
      curr2[j] = next2[j];
      next2[j] = tmp2;
      
      empty_hash(inter[j], verbose);
    }
  }
  
  //Free HashMap
  for (int i = 0; i < nb_output; i++){
    free_hash(curr[i], verbose);
    free_hash(next[i], verbose);
    free_hash(curr2[i], verbose);
    free_hash(next2[i], verbose);
    free_hash(inter[i], verbose);
  }
}

//Parallelisation structure used to fill all the Hashmap
struct steps_i_args{
  int i;
  int j;
  HashMap **curr;
  HashMap **next;
  HashMap **curr2;
  HashMap **next2;
  HashMap **inter;
  int var_count;
  Trie **incompr;
  Trie **incompr2;
};

//Starting thread for the fill of the HashMAp.
void *start_thread_steps_i(void *void_args){
  struct steps_i_args *args = (struct steps_i_args *) void_args;
  
  int i = args->i;
  int j = args->j;
  HashMap **curr = args->curr;
  HashMap **next = args->next; 
  HashMap **curr2 = args->curr2;
  HashMap **next2 = args->next2; 
  HashMap **inter = args->inter;  
  int var_count = args->var_count;

  Trie **incompr = args->incompr;
  Trie **incompr2 = args->incompr2;
  
  
  next[j]->comb_len = i + 1;
  expand_tuples(curr[j], next[j], var_count);
  add_incompr_to_map(next[j], incompr[j], i + 1, var_count);
        
  next2[j]->comb_len = i + 1;
  expand_tuples(curr2[j], next2[j], var_count);
  add_incompr_to_map(next2[j], incompr2[j], i + 1, var_count);
        
  inter[j]->comb_len = i + 1;
  for (int k = 0; k < (int)(HASH_SIZE); k++){
    if (next[j]->content[k] && next2[j]->content[k]){
      HashNode *node = next[j]->content[k];
      while(node){
        HashNode *node2 = next2[j]->content[k];
        while(node2){
          if (node->quo_hash == node2->quo_hash){
            Comb *new_comb = malloc(inter[j]->comb_len * sizeof(*new_comb));
            memcpy(new_comb, node2->comb, inter[j]->comb_len * sizeof(*new_comb));
            add_to_hash_with_key(inter[j], new_comb, k, node2->quo_hash);  
          }
          node2 = node2->next;
        }
        node = node->next;
      }
    }
  }
  
  free(args);
  return NULL;
}

struct update_coeffs_with_hash_RPE2_args{
  const Circuit* c; 
  uint64_t* coeffs; 
  HashMap** map;
  int len_map;  
};

void * start_thread_update_coeffs_with_hash_RPE2 (void *void_args){
  struct update_coeffs_with_hash_RPE2_args *args = 
    (struct update_coeffs_with_hash_RPE2_args *)void_args;
    
  update_coeffs_with_hash_RPE2(args->c, args->coeffs, args->map, args->len_map);

  free(args);
  return NULL;
}

//Parallel version of |compute_failures_from_incompressibles_RPE2|.
//The goal of the parallelisation is to fill all the different map created 
//for each output subset in the same time instead of one by one.
void compute_failures_from_incompressibles_RPE2_parallel(const Circuit* c, 
                                                         Trie **incompr,
                                                         Trie **incompr2, 
                                                         int coeff_max, 
                                                         int verbose, 
                                                         uint64_t *coeffs,
                                                         uint64_t *coeffs2, 
                                                         uint64_t *coeffs_and,
                                                         int cores){
  if (cores == 1 || cores == 0){
    compute_failures_from_incompressibles_RPE2(c, incompr, incompr2, coeff_max,
                                               verbose, coeffs, coeffs2, 
                                               coeffs_and);
    return;
  }
                                                         
                                                         
  int var_count = c->length;
  int nb_output = c->deps->length - var_count;
  
  if (coeff_max == -1) coeff_max = c->total_wires+1;

  //Creation of all the HashMap.
  HashMap *curr[nb_output];
  HashMap *next[nb_output];  
  HashMap *curr2[nb_output];
  HashMap *next2[nb_output];
  HashMap *inter[nb_output]; 
  
  // Initialisation of all the HashMap.
  for (int i = 0; i < nb_output; i++){
    curr[i] = init_hash(1);
    next[i] = init_hash(0);
    curr2[i] = init_hash(1);
    next2[i] = init_hash(0);
    inter[i] = init_hash(0);
  }
  
  //Filling all the HashMap
  for (int i = -1; i < coeff_max; i++) {
  
    pthread_t threads[nb_output];
    //Filling all the "next" and "inter" HashMap with the errors tuples 
    //of size i + 1 from the errors tuples of size i in "curr" HashMap.
    for (int j = 0 ; j < nb_output; j++){
      struct steps_i_args *args = malloc (sizeof(*args));
      args->i = i;
      args->j = j;
      args->curr = curr;
      args->next = next;
      args->curr2 = curr2;
      args->next2 = next2;
      args->inter = inter;
      args->var_count = var_count;
      args->incompr = incompr;
      args->incompr2 = incompr2;
      

      int ret = pthread_create(&threads[j], NULL, start_thread_steps_i, args);
      
      if (ret)
        start_thread_steps_i(args);
      
    }
    
    for (int j = 0; j < nb_output; j++){
      pthread_join(threads[j], NULL);
    }
      
    //Updating the coefficients in each array(coeffs, coeffs2 and coeffs_and) 
    //with all the errors of size i + 1 we found. Updatings the different *
    //coefficients in parallel.
    
    pthread_t threads2[2];
    struct update_coeffs_with_hash_RPE2_args *args1 = malloc (sizeof(*args1));
    args1->c = c;
    args1->coeffs = coeffs;
    args1->map = next;
    args1->len_map = nb_output;
    
    int ret = pthread_create(&threads2[0], NULL, 
                             start_thread_update_coeffs_with_hash_RPE2, args1);
                   
    if (ret)
      start_thread_update_coeffs_with_hash_RPE2(args1);  
    
    
    struct update_coeffs_with_hash_RPE2_args *args2 = malloc (sizeof(*args2));
    args2->c = c;
    args2->coeffs = coeffs2;
    args2->map = next2;
    args2->len_map = nb_output;
    
    ret = pthread_create(&threads2[1], NULL, 
                         start_thread_update_coeffs_with_hash_RPE2, args2);

    if (ret)
      start_thread_update_coeffs_with_hash_RPE2(args2);  
    
    update_coeffs_with_hash_RPE2(c, coeffs_and, inter, nb_output);
    
    for (int m = 0; m < 2; m++)
      pthread_join(threads2[m], NULL);
    
    
    //Remove the tuples of size i in "curr" HashMap and add the errors tuples 
    //of size i + 1 in "curr" HashMap.
    for (int j = 0; j < nb_output; j++){ 
      empty_hash(curr[j], verbose);
      HashMap* tmp = curr[j];
      curr[j] = next[j];
      next[j] = tmp;
      
      empty_hash(curr2[j], verbose);
      HashMap* tmp2 = curr2[j];
      curr2[j] = next2[j];
      next2[j] = tmp2;
      
      empty_hash(inter[j], verbose);
    }
  }
  
  //Free HashMap
  for (int i = 0; i < nb_output; i++){
    free_hash(curr[i], verbose);
    free_hash(next[i], verbose);
    free_hash(curr2[i], verbose);
    free_hash(next2[i], verbose);
    free_hash(inter[i], verbose);
  }
}



/*
Compute the coefficient for the property RPE2 for the copy gadget 
(a special case) and write them in uint64_t *coeffs.

Input : 
  -const Circuit* c : The arithmetic circuit we are currently studying.
  -Trie **incompr : An array of array of incompressible error tuples for the 
                    first secret value. Each line of this array represents an 
                    array of incompressible error tuples for a certain output 
                    index set.
  -int len_incompr : the number of line of Trie **incompr.
  -int coeff_max : The maximal coefficient that we have to compute.
  -int verbose : For the level of verbosity.
  -uint64_t *coeffs : The array of coefficients in which we are going to write 
                      the coefficients of error for the first secret value.                   
*/
void compute_failures_from_incompressibles_RPE2_single(const Circuit* c, 
                                                       Trie **incompr,
                                                       int len_incompr, 
                                                       int coeff_max, 
                                                       int verbose, 
                                                       uint64_t *coeffs) {
  int var_count = c->length;
  if (coeff_max == -1) coeff_max = c->total_wires+1;
  
  //Creation of the HashMap.
  HashMap *curr[len_incompr];
  HashMap *next[len_incompr];
  
  //Initialisation of the HashMap.
  for (int i = 0; i < len_incompr; i++){
    curr[i] = init_hash(1);
    next[i] = init_hash(0);
  }
  
  //Filling the errors tuples in the HashMap
  for (int i = 0; i < coeff_max; i++) {
    //Filling all the errors tuples of size  i + 1 in the "next" HashMap from 
    //the errors tuples of size i in "curr" HashMap.
    for (int j = 0 ; j < len_incompr; j++){
      next[j]->comb_len = i + 1;
      expand_tuples(curr[j], next[j], var_count);
      add_incompr_to_map(next[j], incompr[j], i + 1, var_count);
    }
    
    //Updating coefficients with all the errors tuples of size i + 1 we found.
    update_coeffs_with_hash_RPE2(c, coeffs, next, len_incompr);
    
    //Remove the errors tuples of size i in "curr" HashMap and add the errors 
    //tuples of size  i + 1 in "curr" HashMap.
    for (int j = 0; j < len_incompr; j++){ 
      empty_hash(curr[j], verbose);
      HashMap* tmp = curr[j];
      curr[j] = next[j];
      next[j] = tmp;
    }
  }
  
  //Freeing HashMap.
  for (int i = 0; i < len_incompr; i++){
    free_hash(curr[i], verbose);
    free_hash(next[i], verbose);
  }
}  
