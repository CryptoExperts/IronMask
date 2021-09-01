#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "parser.h"
#include "circuit.h"
#include "vectors.h"


typedef struct _StrMap StrMap;
typedef struct _EqList EqList;


Circuit* gen_circuit(int shares, EqList* eqs,
                     StrMap* in, StrMap* randoms, StrMap* out,
                     bool glitch, bool transition);


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

StrMap* make_str_map(char* name) {
  StrMap* map = malloc(sizeof(*map));
  map->name = strdup(name);
  map->head = NULL;
  map->next_val = 0;
  return map;
}

// Warning: takes ownership of |str|; don't free it after calling map_add!
void str_map_add_with_val(StrMap* map, char* str, int val) {
  StrMapElem* e = malloc(sizeof(*e));
  e->key = str;
  e->val = val;
  e->next = map->head;
  map->head = e;
}

void str_map_add(StrMap* map, char* str) {
  str_map_add_with_val(map, str, map->next_val++);
}

void str_map_remove(StrMap* map, char* str) {
  StrMapElem* e = map->head;
  if (!e) return;
  if (strcmp(e->key, str) == 0) {
    map->head = e->next;
    free(e->key);
    free(e);
    return;
  }
  StrMapElem* prev = e;
  e = e->next;
  while (e) {
    if (strcmp(e->key, str) == 0) {
      prev->next = e->next;
      free(e->key);
      free(e);
      return;
    }
    prev = e;
    e = e->next;
  }
}

int str_map_get(StrMap* map, char* str) {
  StrMapElem* curr = map->head;
  while (curr) {
    if (strcmp(curr->key, str) == 0) {
      return curr->val;
    }
    curr = curr->next;
  }
  fprintf(stderr, "Elem '%s' not found in map '%s'.\n", str, map->name);
  exit(EXIT_FAILURE);
}

int str_map_contains(StrMap* map, char* str) {
  StrMapElem* curr = map->head;
  while (curr) {
    if (strcmp(curr->key, str) == 0) {
      return 1;
    }
    curr = curr->next;
  }
  return 0;
}

void free_str_map(StrMap* map) {
  StrMapElem* e = map->head;
  while (e) {
    free(e->key);
    StrMapElem* next = e->next;
    free(e);
    e = next;
  }
  free(map->name);
  free(map);
}

void print_str_map(StrMap* map) {
  printf("%s: {",map->name);
  StrMapElem* e = map->head;
  while (e) {
    printf("%s:%d, ",e->key,e->val);
    e = e->next;
  }
  printf("}\n");
}

StrMapElem* _reverse_str_map(StrMapElem* e, StrMapElem* prev) {
  if (!e->next) {
    e->next = prev;
    return e;
  }
  StrMapElem* new = _reverse_str_map(e->next, e);
  e->next = prev;
  return new;
}

void reverse_str_map(StrMap* map) {
  StrMapElem* e = map->head;
  map->head = _reverse_str_map(e, NULL);
}


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
  struct _EqListElem* next;
} EqListElem;

typedef struct _EqList {
  int size;
  EqListElem* head;
} EqList;

EqList* make_eq_list() {
  EqList* l = malloc(sizeof(*l));
  l->size = 0;
  l->head = NULL;
  return l;
}

void add_eq_list(EqList* l, char* dst, Expr* e, bool anti_glitch) {
  EqListElem* el = malloc(sizeof(*el));
  el->expr = e;
  el->dst  = dst;
  el->anti_glitch = anti_glitch;
  el->next = l->head;
  l->head = el;
  l->size++;
}

void free_eq_list(EqList* l) {
  EqListElem* el = l->head;
  while (el) {
    free(el->expr->left);
    free(el->expr->right);
    free(el->expr);
    free(el->dst);
    EqListElem* next = el->next;
    free(el);
    el = next;
  }
  free(l);
}

void print_eq_list(EqList* l) {
  for (EqListElem* el = l->head; el != NULL; el = el->next) {
    if (el->expr->op == Asgn) {
      printf("%s = %s\n", el->dst, el->expr->left);
    } else {
      printf("%s = %s %s %s\n", el->dst, el->expr->left,
             el->expr->op == Add ? "+" : "*", el->expr->right);
    }
  }
}

EqListElem* _reverse_eq_list(EqListElem* el, EqListElem* prev) {
  if (!el->next) {
    el->next = prev;
    return el;
  }
  EqListElem* new = _reverse_eq_list(el->next, el);
  el->next = prev;
  return new;
}

void reverse_eq_list(EqList* l) {
  EqListElem* el = l->head;
  l->head = _reverse_eq_list(el, NULL);
}


/* ***************************************************** */
/*              File parsing                             */
/* ***************************************************** */
#define skip_spaces(_str) while (*_str && is_space(*_str)) _str++

int is_space(char c) {
  return c == ' ' || c == '\t' || c == '\n';
}

int is_eol(char c) {
  return c == '\0' || c == '#';
}

void parse_idents(StrMap* map, char* str) {
  skip_spaces(str);

  while (*str) {
    int end = 0;
    while (!is_eol(str[end]) && !is_space(str[end])) end++;
    str_map_add(map, strndup(str,end));
    str += end + 1;
    skip_spaces(str);
  }
}

#define is_operator(c) (c == '+' || c == '^' || c == '*' || c == '&')
#define is_add(c) (c == '+' || c == '^')
#define is_mult(c) (c == '*' || c == '&')
Expr* parse_expr(char* line, char* str) {
  Expr* ret_e = malloc(sizeof(*ret_e));

  skip_spaces(str);

  int end = 0;
  while (!is_eol(str[end]) && !is_space(str[end]) && !is_operator(str[end])) end++;
  ret_e->left = strndup(str, end);
  str += end;

  skip_spaces(str);

  if (is_eol(*str)) {
    ret_e->op = Asgn;
  } else if (is_add(*str)) {
    ret_e->op = Add;
  } else if (is_mult(*str)) {
    ret_e->op = Mult;
  } else {
    fprintf(stderr, "Error in line '%s': operator expected, got '%c'. Exiting.\n",
            line, *str);
    exit(EXIT_FAILURE);
  }

  if (ret_e->op != Asgn) {
    str++;
    skip_spaces(str);

    end = 0;
    while (!is_eol(str[end]) && !is_space(str[end]) && !is_operator(str[end])) end++;

    ret_e->right = strndup(str, end);
  }

  return ret_e;
}


void parse_eq_str(EqList* eqs, char* str) {
  bool anti_glitch = false;
  char* str_start = str;
  // Skipping whitespaces
  while (*str && is_space(*str)) str++;
  if (! *str) return;

  int end = 0;
  while (str[end] && !is_space(str[end]) && str[end] != '=') end++;
  char* dst = strndup(str, end);
  str += end;

  skip_spaces(str);
  if (*str != '=') {
    fprintf(stderr, "Invalid line at character %lu: '%s'. Exiting.\n",
            str-str_start, str_start);
    exit(EXIT_FAILURE);
  }
  str++;

  skip_spaces(str);
  if (*str == '!' && *(str+1) == '[') {
    anti_glitch = true;
    str += 2;
    skip_spaces(str);
    // Removing the final ']'
    int idx = strlen(str)-1;
    while (idx > 0 && is_space(str[idx])) idx--;
    if (str[idx] != ']') {
      fprintf(stderr, "Invalid line: '![' without matching ']'.\n"
              "Reminder: the closing ']' must be the last non-space character of the line.\n"
              "Exiting.");
      exit(EXIT_FAILURE);
    }
    str[idx] = '\0'; // truncating the end of the string
  }

  Expr* e = parse_expr(str_start, str);
  add_eq_list(eqs, dst, e, anti_glitch);
}

#define uppercase(c) ((c) >= 97 && (c) <= 122 ? (c) - 32 : (c))
int str_equals_nocase(char* s1, char* s2, int len) {
  for (int i = 0; i < len; i++) {
    if (uppercase(*s1) != uppercase(*s2)) return 0;
    s1++; s2++;
  }
  return 1;
}

Circuit* parse_file(char* filename, bool glitch, bool transition) {
  FILE* f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "Cannot open file '%s'.\n", filename);
    exit(EXIT_FAILURE);
  }

  int order = -1, shares = -1;
  StrMap* in = make_str_map("in");
  StrMap* randoms = make_str_map("randoms");
  StrMap* out = make_str_map("out");
  EqList* eqs = make_eq_list();

  char* line = NULL;
  size_t len;
  while(getline(&line,&len,f) != -1) {
    unsigned int i = 0;
    // Skipping whitespaces
    while (i < len && is_space(line[i])) i++;
    if (i >= len) continue;

    if (line[i] == '#') {
      i += 1;
      // Config line
      if (str_equals_nocase(&line[i], "ORDER", 5)) {
        if (sscanf(&line[i+5], "%d", &order) != 1) {
          fprintf(stderr, "Missing number on line '%s'.\n", line);
          exit(EXIT_FAILURE);
        }
      } else if (str_equals_nocase(&line[i], "SHARES", 6)) {
        if (sscanf(&line[i+6], "%d", &shares) != 1) {
          fprintf(stderr, "Missing number on line '%s'.\n", line);
          exit(EXIT_FAILURE);
        }
        if (shares > 99) {
          fprintf(stderr, "Error: this tool does not support more than 99 shares (> %d).\n", shares);
          exit(EXIT_FAILURE);
        }
      } else if (str_equals_nocase(&line[i], "INPUT", 5)) {
        parse_idents(in, &line[i+5]);
      } else if (str_equals_nocase(&line[i], "IN", 2)) {
        parse_idents(in, &line[i+2]);
      } else if (str_equals_nocase(&line[i], "RANDOMS", 7)) {
        parse_idents(randoms, &line[i+7]);
      } else if (str_equals_nocase(&line[i], "OUTPUT", 6)) {
        parse_idents(out, &line[i+6]);
      } else if (str_equals_nocase(&line[i], "OUT", 3)) {
        parse_idents(out, &line[i+3]);
      } else {
        fprintf(stderr, "Unrecognized line '%s'. Ignoring it.\n", line);
      }
    } else {
      // Equation line
      parse_eq_str(eqs, line);
    }
  }
  free(line);
  fclose(f);

  // Reversing |in|, |randoms| and |out| isn't really necessary, but
  // produces dependencies that are visually identical to what the old
  // VRAPS tool did. Reversing |eqs|, on the other hand, is required
  // to have the equations sorted according to their dependencies.
  reverse_str_map(in);
  reverse_str_map(randoms);
  reverse_str_map(out);
  reverse_eq_list(eqs);

  /* print_str_map(in); */
  /* print_str_map(randoms); */
  /* print_str_map(out); */

  Circuit* c = gen_circuit(shares, eqs, in, randoms, out, glitch, transition);


  return c;
}

/* ***************************************************** */
/*                   Deps map utilities                  */
/* ***************************************************** */

typedef struct _DepMapElem {
  char* key;
  Dependency*  std_dep;
  DepArrVector* glitch_trans_dep;
  struct _DepMapElem* next;
} DepMapElem;

typedef struct _DepMap {
  char* name;
  DepMapElem* head;
} DepMap;

DepMap* make_dep_map(char* name) {
  DepMap* map = malloc(sizeof(*map));
  map->name = strdup(name);
  map->head = NULL;
  return map;
}

// Warning: takes ownership of |dep|; don't free it after calling map_add!
void dep_map_add(DepMap* map, char* str, Dependency* std_dep, DepArrVector* glitch_trans_dep) {
  DepMapElem* e = malloc(sizeof(*e));
  e->key = str;
  e->std_dep = std_dep;
  e->glitch_trans_dep = glitch_trans_dep;
  e->next = map->head;
  map->head = e;
}

DepMapElem* dep_map_get(DepMap* map, char* dep) {
  DepMapElem* curr = map->head;
  while (curr) {
    if (strcmp(curr->key, dep) == 0) {
      return curr;
    }
    curr = curr->next;
  }
  fprintf(stderr, "Elem '%s' not found in map '%s'.\n", dep, map->name);
  exit(EXIT_FAILURE);
}

// Same as dep_map_get, but if |dep| is not found in |map|, returns
// NULL instead of crashing.
DepMapElem* dep_map_get_nofail(DepMap* map, char* dep) {
  DepMapElem* curr = map->head;
  while (curr) {
    if (strcmp(curr->key, dep) == 0) {
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

void free_dep_map(DepMap* map) {
  DepMapElem* e = map->head;
  while (e) {
    free(e->key);
    // Not freing e->std_dep and e->glitch_trans_dep because they are
    // returned by this module
    DepMapElem* next = e->next;
    free(e);
    e = next;
  }
  free(map->name);
  free(map);
}

/* void print_dep_map(DepMap* map, int deps_size) { */
/*   printf("%s: {",map->name); */
/*   DepMapElem* e = map->head; */
/*   while (e) { */
/*     printf("  %s: [ ",e->key); */
/*     for (int i = 0; i < deps_size; i++) { */
/*       printf("%d ", e->val[i]); */
/*     } */
/*     printf("]\n"); */
/*     e = e->next; */
/*   } */
/*   printf("}\n"); */
/* } */


/* ***************************************************** */
/*                 Circuit building                      */
/* ***************************************************** */


int count_mults(EqList* eqs) {
  int total = 0;
  for (EqListElem* e = eqs->head; e != NULL; e = e->next) {
    if (e->expr->op == Mult) total += 1;
  }
  return total;
}


Circuit* gen_circuit(int shares, EqList* eqs,
                     StrMap* in, StrMap* randoms, StrMap* out,
                     bool glitch, bool transition) {
  Circuit* c = malloc(sizeof(*c));

  int circuit_size = eqs->size;// - out->next_val;
  int mult_count = count_mults(eqs);
  int linear_deps_size = in->next_val + randoms->next_val;
  int deps_size = linear_deps_size + mult_count;

  MultDependencyList* mult_deps = malloc(sizeof(*mult_deps));
  mult_deps->length = mult_count;
  mult_deps->deps = malloc(mult_count * sizeof(*(mult_deps->deps)));

  DependencyList* deps = malloc(sizeof(*deps));
  deps->length         = circuit_size + randoms->next_val +
                         in->next_val * shares;
  deps->deps_size      = deps_size;
  deps->first_rand_idx = in->next_val;
  deps->deps           = malloc(deps->length * sizeof(*deps->deps));
  deps->deps_exprs     = malloc(deps->length * sizeof(*deps->deps_exprs));
  deps->names          = malloc(deps->length * sizeof(*deps->names));
  deps->mult_deps      = mult_deps;

  DepMap* deps_map = make_dep_map("Dependencies");

  int* weights = calloc(deps->length, sizeof(*weights));
  StrMap* positions_map = make_str_map("Positions");

  int add_idx = 0, mult_idx = 0, dep_bit_idx = 0;
  // Initializing dependencies with inputs
  for (StrMapElem* e = in->head; e != NULL; e = e->next, dep_bit_idx++) {
    int len = strlen(e->key) + 1 + 2; // +1 for '\0' and +2 for share number
    for (int i = 0; i < shares; i++) {
      char* name = malloc(len * sizeof(*name));
      snprintf(name, len, "%s%d", e->key, i);
      Dependency* dep = calloc(deps_size, sizeof(*dep));
      dep[dep_bit_idx] = 1 << i;
      DepArrVector* dep_arr = DepArrVector_make();
      DepArrVector_push(dep_arr, dep);
      deps->deps[add_idx]       = dep_arr;
      deps->deps_exprs[add_idx] = dep;
      deps->names[add_idx]      = strdup(name);
      dep_map_add(deps_map, name, dep, dep_arr);
      str_map_add(positions_map, strdup(name));
      add_idx += 1;
    }
  }

  // Initializing random dependencies
  for (StrMapElem* e = randoms->head; e != NULL; e = e->next, dep_bit_idx++, add_idx++) {
    Dependency* dep = calloc(deps_size, sizeof(*dep));
    dep[dep_bit_idx] = 1;
    DepArrVector* dep_arr = DepArrVector_make();
    DepArrVector_push(dep_arr, dep);
    deps->deps[add_idx]       = dep_arr;
    deps->deps_exprs[add_idx] = dep;
    deps->names[add_idx]      = strdup(e->key);
    dep_map_add(deps_map, strdup(e->key), dep, dep_arr);
    str_map_add(positions_map, strdup(e->key));
  }


  // Adding dependencies of other instructions
  for (EqListElem* e = eqs->head; e != NULL; e = e->next, add_idx++) {
    Dependency* dep;
    DepMapElem* left  = dep_map_get(deps_map, e->expr->left);
    DepMapElem* right = e->expr->op != Asgn ? dep_map_get(deps_map, e->expr->right) : NULL;

    // Computing dependency |dep|
    if (e->expr->op == Asgn) {
      dep = left->std_dep;
    } else if (e->expr->op == Add) {
      dep = calloc(deps_size, sizeof(*dep));
      for (int i = 0; i < deps_size; i++) {
        dep[i] = left->std_dep[i] ^ right->std_dep[i];
      }
    } else { // multiplication
      MultDependency* mult_dep = malloc(sizeof(*mult_dep));
      mult_dep->left_ptr  = left->std_dep;
      mult_dep->right_ptr = right->std_dep;
      mult_dep->left_idx  = str_map_get(positions_map, e->expr->left);
      mult_dep->right_idx = str_map_get(positions_map, e->expr->right);
      mult_deps->deps[mult_idx] = mult_dep;

      dep = calloc(deps_size, sizeof(*dep));
      dep[linear_deps_size + mult_idx] = 1;
      mult_idx++;
    }

    // Taking glitches and transitions into account. We ignore the
    // interaction between glitches and transitions are assume that
    // either glitches or (exclusively) transitions are to be
    // considered.
    DepArrVector* dep_arr = DepArrVector_make();
    if (!glitch || e->anti_glitch) {
      DepArrVector_push(dep_arr, dep);
    } else {
      for (int i = 0; i < left->glitch_trans_dep->length; i++) {
        DepArrVector_push(dep_arr, left->glitch_trans_dep->content[i]);
      }
      if (right) {
        for (int i = 0; i < right->glitch_trans_dep->length; i++) {
          // Avoiding duplicates, which might occur if a dependency is
          // in both operands.
          if (!DepArrVector_contains(dep_arr, right->glitch_trans_dep->content[i])) {
            DepArrVector_push(dep_arr, right->glitch_trans_dep->content[i]);
          }
        }
      }
    }
    if (transition) {
      DepMapElem* prev_value = dep_map_get(deps_map, e->dst);
      DepArrVector_push(dep_arr, prev_value->std_dep);
    }


    // Updating weights
    int left_idx = str_map_get(positions_map, e->expr->left);
    weights[left_idx] += weights[left_idx] == 0 ? 1 : 2;
    if (e->expr->op != Asgn) {
      int right_idx = str_map_get(positions_map, e->expr->right);
      weights[right_idx] += weights[right_idx] == 0 ? 1 : 2;
    }

    // Adding to deps
    deps->deps[add_idx]       = dep_arr;
    deps->deps_exprs[add_idx] = dep;
    deps->names[add_idx]      = strdup(e->dst);
    dep_map_add(deps_map, strdup(e->dst), dep, dep_arr);
    str_map_add(positions_map, strdup(e->dst));
  }

  // Moving outputs to the end
  StrMap* outputs_map = make_str_map("outputs expanded");
  for (StrMapElem* e = out->head; e != NULL; e = e->next) {
    int len = strlen(e->key) + 1 + 3; // +1 for '\0' and +3 for share number
    for (int i = 0; i < shares; i++) {
      char* name = malloc(len * sizeof(*name));
      snprintf(name, len, "%s%d", e->key, i);
      str_map_add_with_val(outputs_map, name, 1);
    }
  }
  // Finding first index (from the end) that does not contain an output
  int end_idx = add_idx-1;
  while (str_map_contains(outputs_map, deps->names[end_idx])) {
    str_map_remove(outputs_map, deps->names[end_idx]);
    end_idx--;
  }
  // Finding outputs and swapping them to the end
#define SWAP(_type, _v1, _v2) {                 \
    _type _tmp = _v1;                           \
    _v1 = _v2;                                  \
    _v2 = _tmp;                                 \
  }
#define SWAP_DEPS(i1,i2) {                                          \
    SWAP(char*, deps->names[i1], deps->names[i2]);                  \
    SWAP(DepArrVector*, deps->deps[i1], deps->deps[i2]);             \
    SWAP(Dependency*, deps->deps_exprs[i1], deps->deps_exprs[i2]);  \
    SWAP(int, weights[i1], weights[i2]);                            \
  }
  for (int i = end_idx-1; i >= 0; i--) {
    if (str_map_contains(outputs_map, deps->names[i])) {
      // Shifting all elements between |i| and |end_idx| to the left
      for (int j = i; j < end_idx; j++) {
        SWAP_DEPS(j, j+1);
      }
      str_map_remove(outputs_map, deps->names[end_idx]);
      end_idx--;
      while (str_map_contains(outputs_map, deps->names[end_idx])) {
        str_map_remove(outputs_map, deps->names[end_idx]);
        end_idx--;
      }
    }
  }

  // Updating weights of outputs that were not used after being
  // computed (and whose weight is thus still 0)
  for (int i = 0; i < add_idx; i++) {
    if (!weights[i]) weights[i] = 1;
  }

  c->length          = deps->length - out->next_val * shares;
  c->deps            = deps;
  c->secret_count    = in->next_val;
  c->output_count    = out->next_val;
  c->share_count     = shares;
  c->random_count    = randoms->next_val;
  c->weights         = weights;
  c->all_shares_mask = (1 << shares) - 1;
  c->contains_mults  = mult_idx != 0;
  c->transition      = transition;
  c->glitch          = glitch;

  compute_total_wires(c);
  compute_rands_usage(c);
  compute_contained_secrets(c);
  compute_bit_deps(c);

  print_circuit(c);


  free_str_map(in);
  free_str_map(randoms);
  free_str_map(out);
  free_str_map(outputs_map);
  free_str_map(positions_map);
  free_dep_map(deps_map);
  free_eq_list(eqs);

  return c;
}
