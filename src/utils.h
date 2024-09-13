
#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "vectors.h"

typedef struct _StrMap StrMap;
typedef struct _EqList EqList;



/* ***************************************************** */
/*              File parsing                             */
/* ***************************************************** */
#define uppercase(c) ((c) >= 97 && (c) <= 122 ? (c) - 32 : (c))
#define skip_spaces(_str) while (*_str && is_space(*_str)) _str++
#define is_operator(c) (c == '+' || c == '^' || c == '*' || c == '&' || c == '~')
#define is_add(c) (c == '+' || c == '^')
#define is_mult(c) (c == '*' || c == '&')
#define is_not(c) (c == '~')

int str_equals_nocase(char* s1, char* s2, int len);
int is_space(char c);
int is_eol(char c);


/* ***************************************************** */
/*              String/Int map utilities                 */
/* ***************************************************** */

// For simplicity, a map is implemented using a linked-list. This is
// far from efficient, but parsing is hardly the bottleneck of IronMask.

typedef struct _StrMapElem {
  char* key;
  int   val;
  struct _StrMapElem* next;
} StrMapElem;

typedef struct _StrMap {
  char* name;
  StrMapElem* head;
  int next_val;
} StrMap;

StrMap* make_str_map(char* name);
// Warning: takes ownership of |str|; don't free it after calling map_add!
void str_map_add_with_val(StrMap* map, char* str, int val);
void str_map_add(StrMap* map, char* str);
void str_map_remove(StrMap* map, char* str);
int str_map_get(StrMap* map, char* str);
int str_map_contains(StrMap* map, char* str);
void free_str_map(StrMap* map);
void print_str_map(StrMap* map);
StrMapElem* _reverse_str_map(StrMapElem* e, StrMapElem* prev);
void reverse_str_map(StrMap* map);


/* ***************************************************** */
/*              String/String Vectors map utilities      */
/* ***************************************************** */

typedef struct _StrVecMapElem {
  char* key;
  StringVector * vec;
  struct _StrVecMapElem* next;
} StrVecMapElem;

typedef struct _StrVecMap {
  char* name;
  StrVecMapElem* head;
  int length;
} StrVecMap;

StrVecMap* make_str_vec_map(char* name);
void str_vec_map_add(StrVecMap* map, char* key, char* var);
void free_str_vec_map(StrVecMap* map);
void print_str_vec_map(StrVecMap* map);




/* ***************************************************** */
/*             Utilities related to equations            */
/* ***************************************************** */

typedef enum { Asgn, Add, Mult } Operator;

typedef struct _expr {
  Operator op;
  char* left;
  char* right;
} Expr;

typedef struct _EqListElem {
  Expr* expr;
  char* dst;
  bool anti_glitch; // True if disables glitches, false otherwise
  bool correction_output; // True if dst is the output of a correction block
  bool correction; // True if dst is an internal variable to a correction block
  struct _EqListElem* next;
} EqListElem;

typedef struct _EqList {
  int size;
  EqListElem* head;
} EqList;

EqList* make_eq_list();
void add_eq_list(EqList* l, char* dst, Expr* e, bool anti_glitch, bool correction, bool correction_output);
void free_eq_list(EqList* l);
void print_eq_list(EqList* l);
EqListElem * get_eq_list(EqList* l, char* dst);
void print_eq_full_expr(EqList* l, char* dst);
EqListElem* _reverse_eq_list(EqListElem* el, EqListElem* prev);
void reverse_eq_list(EqList* l);


/* ***************************************************** */
/*                   Deps map utilities                  */
/* ***************************************************** */

typedef struct _DepMapElem {
  char* key;
  Dependency*  std_dep;
  DepArrVector* glitch_trans_dep;
  Dependency* original_dep;
  struct _DepMapElem* next;
} DepMapElem;

typedef struct _DepMap {
  char* name;
  DepMapElem* head;
} DepMap;

DepMap* make_dep_map(char* name);
// Warning: takes ownership of |dep|; don't free it after calling map_add!
void dep_map_add(DepMap* map, char* str, Dependency* std_dep, DepArrVector* glitch_trans_dep, Dependency* original_dep);
DepMapElem* dep_map_get(DepMap* map, char* dep);
// Same as dep_map_get, but if |dep| is not found in |map|, returns
// NULL instead of crashing.
DepMapElem* dep_map_get_nofail(DepMap* map, char* dep);
char* dep_get_from_expr_nofail(DependencyList* deps, int length, Dependency* dep, DepArrVector* dep_arr, int deps_size);
void free_dep_map(DepMap* map);
void print_dep_map(DepMap* map, int deps_size);



typedef struct _parsed_file{
  char * filename;
  int shares;
  int nb_duplications;
  StrMap* in;
  StrMap* randoms;
  StrMap* out;
  EqList* eqs;
  bool glitch;
  bool transition;
}ParsedFile;

typedef struct _fault_comb{
  char ** names;
  int length;
}FaultsComb;

typedef struct _faults_combs{
  FaultsComb ** fc;
  int length;
}FaultsCombs;

void free_faults_combs(FaultsCombs * fc);
void print_faults_combs(FaultsCombs * fc);
bool ignore_faulty_scenario(Faults * fv, FaultsCombs * fc);


#endif