#include <stdlib.h>
#include <stdint.h>

#include "list_tuples.h"


void delete(ListCombElem* e) {
  if (e->prev) {
    e->prev->next = e->next;
  }
  if (e->next) {
    e->next->prev = e->prev;
  }
  // Not freeing e->secret_deps, because ownership is always in the trie
  free(e);
}

void delete_free_dep(ListCombElem* e) {
  free(e->secret_deps);
  delete(e);
}

void add_with_deps(ListComb* l, Comb* arr, SecretDep* secret_deps) {
  ListCombElem* e = malloc(sizeof(*e));
  e->comb = arr;
  e->secret_deps = secret_deps;
  e->next = l->head;
  e->prev = NULL;
  if (l->head) {
    l->head->prev = e;
  }
  l->head = e;
}

void add(ListComb* l, Comb* arr) {
  add_with_deps(l, arr, NULL);
}

ListComb* list_from_array(Comb** arr, uint64_t length) {
  ListComb* l = malloc(sizeof(*l));
  l->head = NULL;
  for (uint64_t i = 0; i < length; i++) {
    add(l,arr[i]);
  }
  return l;
}

ListComb* make_empty_list() {
  ListComb* l = malloc(sizeof(*l));
  l->head = NULL;
  return l;
}

int free_list(ListComb* l) {
  ListCombElem* curr = l->head;
  int count = 0;
  while (curr) {
    ListCombElem* next = curr->next;
    count++;
    // Note: curr->comb is never owned by the list ==> not freeing it
    free(curr);
    curr = next;
  }
  free(l);
  return count;
}
