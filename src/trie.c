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

void insert_in_trie_merge(Trie* trie, Comb* comb, int comb_len,
                          SecretDep* secret_deps, int secret_deps_len) {
  _insert_in_trie(trie->head, trie->childs_len, comb, comb_len,
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
}

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
