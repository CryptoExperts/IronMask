#pragma once

// This file offers Trie, an implementation of a trie containing
// incompressible tuples. Unexpected things will happen if this trie
// is used for non-incompressible tuples!
//
// Because this trie only stores incompressible tuples, it cannot
// contain a tuple t and a tuple t' that is a subtuple of t. Thus, the
// criteria we use to know where to stop in the sub-tries is: if
// |childs| is NULL, then we are on a leaf.

#include "combinations.h"
#include "list_tuples.h"
#include "vectors.h"


typedef struct _trie_node {
  struct _trie_node** childs;
  SecretDep* secret_deps;
} TrieNode;

typedef struct _trie {
  int childs_len;
  TrieNode* head;
} Trie;

void free_trie(Trie* trie);
Trie* make_trie(int childs_len);
int trie_contains(Trie* trie, Comb* comb, int comb_len);
int trie_size(Trie* trie);
int trie_tuples_size(Trie* trie, int size);
void insert_in_trie(Trie* trie, Comb* comb, int comb_len, SecretDep* secret_deps);
void insert_in_trie_merge(Trie* trie, Comb* comb, int comb_len,
                          SecretDep* secret_deps, int secret_deps_len);
void insert_in_trie_merge_arith(Trie* trie, Comb* comb, int comb_len,
                          SecretDep* secret_deps, int secret_deps_len);
SecretDep *is_in_trie(Trie *trie, Comb *comb, int comb_len);                          
SecretDep* trie_contains_subset(Trie* trie, Comb* comb, int comb_len);
void print_all_tuples(Trie* trie);
void print_all_tuples_size(Trie* trie, int size);
Trie *derive_trie_from_subset(Trie *trie, int *subset, int len_subset, 
                              int circuit_length, int secret_count,
                              int idx_output_end, int coeff_max);
                              
ListComb* list_from_trie(Trie* trie, int comb_len);
VarVecVector* get_all_tuples(Trie* trie);
Trie *trie_copy(Trie *trie, int secret_count);
