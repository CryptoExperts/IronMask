#pragma once

// This file provides ListComb, a list of tuples. It was used in the
// early days of IronMask, and is kept around mostly by nostalgia.

#include <stdlib.h>
#include <stdint.h>

#include "combinations.h"

typedef char SecretDep;

typedef struct _list_comb_elem {
  Comb* comb;
  SecretDep* secret_deps; // The secret inputs this element depends
                          // on. Initially empty (as we do not know), but
                          // later filled by the verification rules.
  struct _list_comb_elem* next;
  struct _list_comb_elem* prev;
} ListCombElem;

typedef struct _list_comb {
  ListCombElem* head;
} ListComb;

ListComb* make_empty_list();
void delete(ListCombElem* e);
void delete_free_dep(ListCombElem* e);
void add_with_deps(ListComb* l, Comb* arr, SecretDep* secret_deps);
void add(ListComb* l, Comb* arr);
ListComb* list_from_array(Comb** arr, uint64_t length);
int free_list(ListComb* l);
