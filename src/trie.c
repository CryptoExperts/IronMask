#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "trie.h"
#include "combinations.h"



Trie* make_trie(int childs_len) {
  Trie* trie = malloc(sizeof(*trie));
  trie->childs_len = childs_len;
  TrieNode* head = malloc(sizeof(*head));
  // Mallocing the head's childs so that |trie_contains| does not
  // return true as soon as it visits the head. (Since nodes with an
  // empty childs array are leafs)
  head->childs = calloc(childs_len, sizeof(*head->childs));
  head->secret_deps = NULL;
  trie->head = head;
  return trie;
}

void free_trie_node(TrieNode* trie, int childs_len) {
  free(trie->secret_deps);
  if (!trie->childs) {
    free(trie);
    return;
  }
  for (int i = 0; i < childs_len; i++) {
    if (trie->childs[i]) {
      free_trie_node(trie->childs[i], childs_len);
    }
  }
  free(trie->childs);
  free(trie);
}

void free_trie(Trie* trie) {
  free_trie_node(trie->head, trie->childs_len);
  free(trie);
}

int trie_node_size(TrieNode* trie, int childs_len) {
  if (!trie->childs) return 1; // a leaf
  int total = 0;
  for (int i = 0; i < childs_len; i++) {
    if (trie->childs[i]) {
      total += trie_node_size(trie->childs[i], childs_len);
    }
  }
  return total;
}

int trie_size(Trie* trie) {
  return trie_node_size(trie->head, trie->childs_len);
}

int trie_node_tuples_size(TrieNode* trie, int childs_len, int size) {
  if (!trie->childs) return size == 0;
  int total = 0;
  for (int i = 0; i < childs_len; i++) {
    if (trie->childs[i]) {
      total += trie_node_tuples_size(trie->childs[i], childs_len, size-1);
    }
  }
  return total;
}

int trie_tuples_size(Trie* trie, int size) {
  return trie_node_tuples_size(trie->head, trie->childs_len, size);
}

void _insert_in_trie(TrieNode* trie, int childs_len,
                     Comb* comb, int comb_len,
                     SecretDep* secret_deps, int secret_deps_len) {
  if (comb_len == 0) {
    if (secret_deps_len && trie->secret_deps) {
      for (int i = 0; i < secret_deps_len; i++) {
        trie->secret_deps[i] |= secret_deps[i];
      }
      free(secret_deps);
    } else {
      trie->secret_deps = secret_deps;
    }
    return;
  }
  if (! trie->childs) {
    trie->childs = calloc(childs_len, sizeof(*trie->childs));
  }
  if (!trie->childs[*comb]) {
    trie->childs[*comb] = calloc(1, sizeof(*trie->childs[*comb]));
  }
  _insert_in_trie(trie->childs[*comb], childs_len, comb+1, comb_len-1,
                  secret_deps, secret_deps_len);
  return;
}

void insert_in_trie(Trie* trie, Comb* comb, int comb_len, SecretDep* secret_deps) {
  _insert_in_trie(trie->head, trie->childs_len, comb, comb_len,
                  secret_deps, 0);
}

void _insert_in_trie_arith(TrieNode* trie, int childs_len,
                     Comb* comb, int comb_len,
                     SecretDep* secret_deps, int secret_deps_len) {
                     
  if (comb_len == 0) {
    if (secret_deps_len && trie->secret_deps) {
      for (int i = 0; i < secret_deps_len; i++) {
        trie->secret_deps[i] |= secret_deps[i];
      }
      free(secret_deps);
    } else {
      trie->secret_deps = secret_deps;
      
      /* To prevent bug from the form : tuple in [9,10,13,17] and I want to 
         add [9,10,13], so I have to remove the last element 17.*/
      if (trie->childs){  
        for (int i = 0; i < childs_len; i++){
          if (trie->childs[i]){
            free_trie_node(trie->childs[i], childs_len);
          }
        }
        free(trie->childs);
        trie->childs = NULL; 
      }
    }
    return;
  }
  if (!trie->childs) {
    trie->childs = calloc(childs_len, sizeof(*trie->childs));
  }
  if (!trie->childs[*comb]) {
    trie->childs[*comb] = calloc(1, sizeof(*trie->childs[*comb]));
  }
  _insert_in_trie_arith(trie->childs[*comb], childs_len, comb+1, comb_len-1,
                  secret_deps, secret_deps_len);
  return;
}

void insert_in_trie_arith(Trie* trie, Comb* comb, int comb_len, 
                    SecretDep* secret_deps) {
  
  _insert_in_trie_arith(trie->head, trie->childs_len, comb, comb_len,
                  secret_deps, 0);  
}





void insert_in_trie_merge(Trie* trie, Comb* comb, int comb_len,
                          SecretDep* secret_deps, int secret_deps_len) {
  _insert_in_trie(trie->head, trie->childs_len, comb, comb_len,
                  secret_deps, secret_deps_len);
}

void insert_in_trie_merge_arith(Trie* trie, Comb* comb, int comb_len,
                          SecretDep* secret_deps, int secret_deps_len) {
  _insert_in_trie_arith(trie->head, trie->childs_len, comb, comb_len,
                  secret_deps, secret_deps_len);
}


int _trie_contains(TrieNode* trie, Comb* comb, int comb_len) {
    if (!trie->childs) return 1;
    if (comb_len == 0) return 0;
    if (!trie->childs[*comb]) return 0;
    return _trie_contains(trie->childs[*comb], comb+1, comb_len-1);
}

int trie_contains(Trie* trie, Comb* comb, int comb_len) {
  return _trie_contains(trie->head, comb, comb_len);
}

SecretDep *_is_in_trie(TrieNode *trie, Comb* comb, int comb_len){
  if (trie && !trie->childs && comb_len == 0) return trie->secret_deps;
  if (comb_len == 0) return NULL;
  if (!trie->childs) return NULL;
  if (!trie->childs[*comb]) return NULL;
  return _is_in_trie(trie->childs[*comb], comb+1, comb_len-1);
   
}

SecretDep *is_in_trie (Trie *trie, Comb *comb, int comb_len){
  return _is_in_trie(trie->head, comb, comb_len);  
}



SecretDep* _trie_contains_subset(TrieNode* trie, Comb* comb, int comb_len) {
  if (!trie->childs) {
    return trie->secret_deps;
  }
  if (comb_len == 0) return NULL;
  char* secret_deps = _trie_contains_subset(trie, comb+1, comb_len-1);
  if (secret_deps) return secret_deps;
  if (!trie->childs[*comb]) return NULL;
  return _trie_contains_subset(trie->childs[*comb], comb+1, comb_len-1);
}

SecretDep* trie_contains_subset(Trie* trie, Comb* comb, int comb_len) {
  return _trie_contains_subset(trie->head, comb, comb_len);
}

// This function generates all combinations of |comb| of size 1 to
// |comb_len-1|, and checks if one of them is in |trie|. Returns 1 if
// so, 0 otherwise.
// It is slower than |trie_contains_subset|, because the latter does
// not actually generate combinations, but achieves a similar effect
// through its recursion.
int trie_contains_subset_slow(Trie* trie, Comb* comb, int comb_len) {
  int k = 1;
  Comb indices[comb_len];
  while (k < comb_len) {
    for (int i = 0; i < k; i++) {
      indices[i] = i;
    }

    while (1) {
      Comb subset[comb_len];
      for (int j = 0; j < k; j++) subset[j] = comb[indices[j]];
      if (trie_contains(trie, subset, k)) return 1;
      if (!incr_comb_in_place(indices, k, comb_len)) {
        break;
      }
    }
    k++;
  }
  return 0;
}

void _get_all_tuples(VarVecVector* all_tuples, TrieNode* trie, int childs_len,
                     Comb* work_comb, int work_comb_idx) {
  if (!trie->childs) {
    VarVector* tuple = VarVector_make_size(work_comb_idx+1);
    for (int i = 0; i < work_comb_idx; i++) {
      VarVector_push(tuple, work_comb[i]);
    }
    VarVecVector_push(all_tuples, tuple);
    return;
  }

  for (int i = 0; i < childs_len; i++) {
    if (trie->childs[i]) {
      work_comb[work_comb_idx] = i;
      _get_all_tuples(all_tuples, trie->childs[i], childs_len,
                      work_comb, work_comb_idx+1);
    }
  }
}

VarVecVector* get_all_tuples(Trie* trie) {
  VarVecVector* all_tuples = VarVecVector_make();
  // Assumes that no incompressible tuple is more than 100 elements long
  Comb work_comb[100] = { 0 };
  _get_all_tuples(all_tuples, trie->head, trie->childs_len, work_comb, 0);
  return all_tuples;
}


void print_comb(Comb* comb, int comb_len) {
  printf("[");
  for (int i = 0; i < comb_len-1; i++)
    printf(" %d", comb[i]);
  if (comb_len)
    printf(" %d", comb[comb_len-1]);
  printf(" ]\n");
}

void _print_all_tuples(TrieNode* trie, int childs_len,
                       Comb* work_comb, int work_comb_idx) {
  if (!trie->childs) {
    print_comb(work_comb, work_comb_idx);
    return;
  }
  for (int i = 0; i < childs_len; i++) {
    if (trie->childs[i]) {
      work_comb[work_comb_idx] = i;
      _print_all_tuples(trie->childs[i], childs_len, work_comb, work_comb_idx+1);
    }
  }
}

void print_all_tuples(Trie* trie) {
  // Assumes that no incompressible tuple is more than 100 elements long
  Comb work_comb[100] = { 0 };
  _print_all_tuples(trie->head, trie->childs_len, work_comb, 0);
}

void _print_all_tuples_size(TrieNode* trie, int childs_len,
                            Comb* work_comb, int work_comb_idx, int size) {
  if (!trie->childs) {
    if (size == 0) {
      print_comb(work_comb, work_comb_idx);
    }
    return;
  }
  for (int i = 0; i < childs_len; i++) {
    if (trie->childs[i]) {
      work_comb[work_comb_idx] = i;
      _print_all_tuples_size(trie->childs[i], childs_len, work_comb, work_comb_idx+1, size-1);
    }
  }
}

void print_all_tuples_size(Trie* trie, int size) {
  // Assumes that no incompressible tuple is more than 100 elements long
  Comb work_comb[100] = { 0 };
  _print_all_tuples_size(trie->head, trie->childs_len, work_comb, 0, size);
}
void _list_from_trie(TrieNode* trie, int childs_len,
                     ListComb* list,
                     Comb* comb, int idx, int comb_len) {
  if (!trie->childs && idx == comb_len) {
    add_with_deps(list, comb, trie->secret_deps);
    return;
  }
  if (idx == comb_len || !trie->childs) {
    return;
  }
  int used = 0;
  for (int i = 0; i < childs_len; i++) {
    if (trie->childs[i]) {
      if (used) {
        Comb* comb_copy = malloc(comb_len * sizeof(*comb_copy));
        memcpy(comb_copy, comb, idx * sizeof(*comb));
        comb_copy[idx] = i;
        _list_from_trie(trie->childs[i], childs_len, list, comb_copy, idx+1, comb_len);
      } else {
        used = 1;
        comb[idx] = i;
        _list_from_trie(trie->childs[i], childs_len, list, comb, idx+1, comb_len);
      }
    }
  }
}



/*
Recursive algorithm that evaluate if the |comb| contains in |trie| respect the 
condition described in the function derive_trie_from_subset. If it's the case, 
we add the tuples in |new_trie|.
Input : 
  -TrieNode *trie : The current Node in the Trie that we want to add tuples.
  -int childs_len : Maximal number of childs that |trie| can have.
  -int *subset : An array of index between |circuit_length| and 
                |idx_output_end|.
  -int len_subset : The size of the array.
  -int circuit_length : Integer that is the start index that |subset| can have 
                        in his element.
  -Trie *new_trie : The trie in which we will be add the good tuple.
  -Comb *work_comb : Tuple used recursively to found the tuples existing in 
                     |trie|.
  -int work_comb_idx : Integer used recursively with |work_comb|, 
                       it is currently the size of |work_comb|.
  -int nb_output : Integer used recursively with |work_comb|, it is the number
                   output index existing in |work_comb|. 
  -int secret_count : Number of secret for this gadget. 
  -int idx_output_end : Integer that is the end index that |subset| can have 
                        in his element.   
*/
void _derive_trie_from_subset(TrieNode *trie, int childs_len, int *subset, 
                              int len_subset, int circuit_length, 
                              Trie* new_trie, Comb *work_comb, 
                              int work_comb_idx, int nb_output, 
                              int secret_count, int idx_output_end){
  if(!trie->childs){
    SecretDep *secret_deps = calloc(secret_count, sizeof(*secret_deps)); 
    memcpy(secret_deps, trie->secret_deps, secret_count * sizeof(*secret_deps));
    Comb work_comb_cpy[work_comb_idx];
    
    for (int i = 0; i < work_comb_idx; i++){
      if (work_comb[i] >= idx_output_end){
        work_comb_cpy[i - nb_output] = work_comb[i];
      }
      else {
        work_comb_cpy[i] = work_comb[i];
      }
    }
    
    insert_in_trie_arith(new_trie, work_comb_cpy, work_comb_idx - nb_output, 
                   secret_deps);
    
    return;
  }
  
  for (int i = 0; i < childs_len; i++){
    if (trie->childs[i]){
      int new_nb_output = nb_output;
      if (i >= circuit_length){
        if (i < idx_output_end){
          bool is_good = false;
          for (int j = 0; j < len_subset; j++) {
            if (i == subset[j]) {
              is_good = true;
              new_nb_output++;    
              break;
            }
          }
          if (!(is_good)) continue;/* Le tuple n'est pas bon, ne pas l'ajouter*/
        }
      }
      
      work_comb[work_comb_idx] = i;
      _derive_trie_from_subset(trie->childs[i], childs_len, subset, len_subset, 
                               circuit_length, new_trie, work_comb, 
                               work_comb_idx + 1, new_nb_output, secret_count,
                               idx_output_end);                           
    }
  }   
}

/*
Recursive algorithm who adds tuples of size |size| existing into the node |trie| 
to the Trie |new_trie|.
Input : 
  -TrieNode *trie : The current Node in the Trie that we want to add tuples.
  -Trie *new_trie : The trie in which we will be add the good tuple.
  -int size : The size of the tuples we want to add.
  -int childs_len : Maximal number of childs that |trie| can have.
  -Comb *work_comb : Tuple used recursively to found the tuples existing in 
                     |trie|.
  -int work_comb_idx : Integer used recursively with |work_comb|, 
                       it is currently the size of |work_comb|.
  -int secret_count : Number of secret for this gadget. 
*/
static void _add_tuples_to_trie_size(TrieNode *trie, Trie *new_trie, int size,
                                     int childs_len, Comb *work_comb, 
                                     int work_comb_idx, int secret_count){

  if (!trie->childs){
    //We found a |comb| existing in the original Trie. 
    if (size == 0) {
      //If the size is different from 0, then the tuples mustn't be add.
      if (!trie_contains_subset(new_trie, work_comb, work_comb_idx)){
        //If the |new_trie| contains a subset of our |comb|, then the tuples
        //mustn't be add.  
        SecretDep *secret_deps = calloc(secret_count, sizeof(*secret_deps)); 
        memcpy(secret_deps, trie->secret_deps, secret_count * 
               sizeof(*secret_deps));
        insert_in_trie_arith(new_trie, work_comb, work_comb_idx, secret_deps);
      }
    }  
    return;
  }
  
  for (int i = 0; i < childs_len; i++) {
    if (trie->childs[i]) {
      work_comb[work_comb_idx] = i;
      _add_tuples_to_trie_size(trie->childs[i], new_trie, size - 1, childs_len, 
                               work_comb, work_comb_idx+1, secret_count);
    }
  }
}

/*
Compute a new Trie from another Trie by only adding tuples who contains a 
subset of |subset| between the index >= |circuit_length| and 
index <= |idx_output_end|. And in this case, removing the subset.
Input :
  -Trie *trie :The original Trie.
  -int *subset : An array of index between |circuit_length| and |idx_output_end|
  -int len_subset : The size of the array.
  -int circuit_length : Integer that is the start index that |subset| can have 
                        in his element.
  -int secret_count : Number of secret for this gadget.
  -int idx_output_end : Integer that is the end index that |subset| can have 
                        in his element.
  -int coeff_max : The maximal coefficient that we have to compute.
Output : A new Trie which only have tuples of the forme described above. 
*/
Trie *derive_trie_from_subset(Trie *trie, int *subset, int len_subset, 
                              int circuit_length, int secret_count, 
                              int idx_output_end, int coeff_max){
  
  Trie *new_trie = make_trie(trie->childs_len);
  // Assumes that no incompressible tuple is more than 100 elements long
  Comb work_comb[100] = { 0 };
  _derive_trie_from_subset(trie->head, trie->childs_len, subset, len_subset, 
                           circuit_length, new_trie, work_comb, 0, 0, 
                           secret_count, idx_output_end);
                           
  
  /* The trie here is not incompressible, 
     for example we can have 3 output sets possible : [24, 25], [24,26], 
     [25,26].
     Then, we can have in our first trie the following tuples :
     -  [6, 16, 19, 24]
     -  [0, 6, 16, 17, 19, 23, 25]
     
     When we apply _derive_trie_from_subset to this trie with the output set 
     [24,25]. It returns a trie containing :
     -  [6, 16, 19]
     -  [0, 6, 16, 17, 19, 23]
     
     We see that [0, 6, 16, 17, 19] is include in [6, 16, 19]. So the trie 
     obtained from _derive_trie_from_subset is not full of incompressible 
     tuples.
     We have to do one more step to create the trie of incompressible tuples 
     that we want. 
  */
  
  Trie *final_trie = make_trie(trie->childs_len);
  for (int size = 0; size <= coeff_max ; size++){
    _add_tuples_to_trie_size(new_trie->head, final_trie, size, 
                             trie->childs_len, work_comb, 0, secret_count);
  }
    
  free_trie(new_trie); 

  return final_trie;
} 

ListComb* list_from_trie(Trie* trie, int comb_len) {
  ListComb* list = make_empty_list();
  TrieNode* head = trie->head;
  if (head->secret_deps && comb_len == 0){
    Comb *comb = malloc(comb_len * sizeof(*comb));
    add_with_deps(list, comb, head->secret_deps);
  }
  else if (comb_len == 0) return list;
  
  else if (head->childs){
    for (int i = 0; i < trie->childs_len; i++) {
      if (head->childs[i]) {
        Comb* comb = malloc(comb_len * sizeof(*comb));
        comb[0] = i;
        _list_from_trie(head->childs[i], trie->childs_len, list, comb, 1, 
                        comb_len);            
      }
    }
  }
  return list;
}

/*
Recursive algorithm that adds all the informations contained in the TrieNode 
*trie in the Trie *trie_copy.
Input : 
  -TrieNode *trie : The current Node in the Trie we want to copy.
  -Trie *trie_copy : The trie in which we will be copying |trie|.
  -Comb *work_comb : Tuple used recursively to found the tuples existing in 
                     |trie|.
  -int work_comb_idx : Integer used recursively with |work_comb|, 
                       it is currently the size of |work_comb|.
  -int childs_len : Maximal number of childs that |trie| can have.
  -int secret_count : Number of secret for this gadget.     
*/
void _trie_copy(TrieNode *trie, Trie *trie_copy, Comb *work_comb, 
                int work_comb_idx, int childs_len, int secret_count){
  if (!trie->childs){
    SecretDep *secret_deps_copy = malloc(secret_count * 
                                         sizeof(*secret_deps_copy));
    
    memcpy(secret_deps_copy, trie->secret_deps, 
           secret_count * sizeof(*secret_deps_copy));
    
    insert_in_trie_arith(trie_copy, work_comb, work_comb_idx, secret_deps_copy);
  }
  
  else {
    for (int i = 0; i < childs_len; i++){
      if (trie->childs[i]){
        work_comb[work_comb_idx] = i;
        _trie_copy(trie->childs[i], trie_copy, work_comb, work_comb_idx + 1, 
                   childs_len, secret_count);
      }
    }
  }
}

/*
Make a copy from an original Trie.
Input : 
  - Trie *trie : The original Trie.
  - int secret_count : The number of secrets for this gadget.
Output : A copy of Trie *trie. 
*/
Trie *trie_copy(Trie *trie, int secret_count){
  Trie *trie_copy = make_trie(trie->childs_len);
  Comb work_comb[100] = { 0 };
  _trie_copy(trie->head, trie_copy, work_comb, 0, trie->childs_len, 
             secret_count);
  return trie_copy;
}




/*
ListComb* list_from_trie(Trie* trie, int comb_len) {
  ListComb* list = make_empty_list();
  TrieNode* head = trie->head;
  for (int i = 0; i < trie->childs_len; i++) {
    if (head->childs[i]) {
      Comb* comb = malloc(comb_len * sizeof(*comb));
      comb[0] = i;
      _list_from_trie(head->childs[i], trie->childs_len, list, comb, 1, comb_len);
    }
  }
  return list;
}*/

// Main for debug
/* int main() { */
/*   Comb x1[3] = {1, 5, 16}; */
/*   Comb x2[3] = {1, 14, 18}; */
/*   Comb x3[3] = {4, 14, 18}; */
/*   Comb x4[8] = {0, 1, 2, 3, 4, 5, 14, 16}; */

/*   Trie* trie = make_trie(19); */
/*   insert_in_trie(trie, x1, 3); */
/*   insert_in_trie(trie, x2, 3); */

/*   printf("contains x1 ? %d\n", trie_contains(trie, x1, 3)); */
/*   printf("contains x2 ? %d\n", trie_contains(trie, x2, 3)); */
/*   printf("contains x3 ? %d\n", trie_contains(trie, x3, 2)); */
/*   printf("contains x4 ? %d\n", trie_contains_subset(trie, x4, 8)); */
/* } */
