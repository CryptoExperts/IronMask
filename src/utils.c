#include "utils.h"


/* ***************************************************** */
/*              File parsing                             */
/* ***************************************************** */

#define uppercase(c) ((c) >= 97 && (c) <= 122 ? (c) - 32 : (c))
int str_equals_nocase(char* s1, char* s2, int len) {
  for (int i = 0; i < len; i++) {
    if (uppercase(*s1) != uppercase(*s2)) return 0;
    s1++; s2++;
  }
  return 1;
}

#define skip_spaces(_str) while (*_str && is_space(*_str)) _str++

int is_space(char c) {
  return c == ' ' || c == '\t' || c == '\n';
}

int is_eol(char c) {
  return c == '\0' || c == '#';
}

int is_number (char c){
  return 48 <= c && c <= 57; 
}

int is_coeff (char* s, int index){
  return (is_number(s[index]) && (index == 0 || is_space(s[index - 1]) || s[index - 1] == '-'));
}

/* ***************************************************** */
/*              String/Int map utilities                 */
/* ***************************************************** */

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
  printf("%s with %d elements: {",map->name, map->next_val);
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
/*              String/String Vectors map utilities      */
/* ***************************************************** */

StrVecMap* make_str_vec_map(char* name) {
  StrVecMap* map = malloc(sizeof(*map));
  map->name = strdup(name);
  map->head = NULL;
  map->length = 0;
  return map;
}

void str_vec_map_add(StrVecMap* map, char* key, char* var) {
  StrVecMapElem* curr = map->head;

  while (curr) {
    if (strcmp(curr->key, key) == 0) {
      StringVector_push(curr->vec, strdup(var));
      return;
    }
    curr = curr->next;
  }

  StrVecMapElem* e = malloc(sizeof(*e));
  e->key = strdup(key);
  e->vec = StringVector_make();
  StringVector_push(e->vec, strdup(var));
  e->next = map->head;
  map->head = e;
  map->length++;
  
}

void free_str_vec_map(StrVecMap* map) {
  StrVecMapElem* e = map->head;
  while (e) {
    free(e->key);
    StringVector_deep_free(e->vec);
    StrVecMapElem* next = e->next;
    free(e);
    e = next;
  }
  free(map->name);
  free(map);
}

void print_str_vec_map(StrVecMap* map) {
  printf("%s with %d elements: {\n",map->name, map->length);
  StrVecMapElem* e = map->head;
  while (e) {
    printf("%s:",e->key);
    for(int i=0; i< e->vec->length; i++){
      printf("%s, ", e->vec->content[i]);
    }
    printf("\n");
    e = e->next;
  }
  printf("}\n");
}




/* ***************************************************** */
/*             Utilities related to equations            */
/* ***************************************************** */

EqList* make_eq_list() {
  EqList* l = malloc(sizeof(*l));
  l->size = 0;
  l->head = NULL;
  return l;
}

void add_eq_list(EqList* l, char* dst, Expr* e, bool anti_glitch, bool correction, bool correction_output) {
  EqListElem* el = malloc(sizeof(*el));
  el->expr = e;
  el->dst  = dst;
  el->anti_glitch = anti_glitch;
  el->correction = correction;
  el->correction_output = correction_output;
  el->next = l->head;
  l->head = el;
  l->size++;
}

void free_eq_list(EqList* l) {
  EqListElem* el = l->head;
  while (el) {
    free(el->expr->left);
    if(el->expr->right){
      free(el->expr->right);
    }
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
      printf("%s = %d %s\n", el->dst, el->expr->coeff_left, el->expr->left);
    } else {
      printf("%s = %d %s %s %d %s\n", el->dst, el->expr->coeff_left, el->expr->left,
             el->expr->op == Add ? "+" : "*", el->expr->coeff_right, el->expr->right);
    }
  }
}

EqListElem * get_eq_list(EqList* l, char* dst) {
  EqListElem * h = l->head;
  while(h){
    if(strcmp(h->dst, dst) == 0){
      return h;
    }
    h = h->next;
  }
  return NULL;
}

void print_eq_full_expr(EqList* l, char* dst){
  EqListElem * x = get_eq_list(l, dst);
  if(x){
    if ((x->expr->op != Asgn) && (x->expr->op != Mult))
      printf(" ( ");

    print_eq_full_expr(l, x->expr->left);

    if(x->expr->op != Asgn){
      if(x->expr->op == Add){
        printf(" + ");
      }
      else{
        printf(" * ");
      }
      print_eq_full_expr(l, x->expr->right);
    }

    if ((x->expr->op != Asgn) && (x->expr->op != Mult))
      printf(" ) ");

    return;
  }

  printf("%s ", dst);
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
/*                   Deps map utilities                  */
/* ***************************************************** */

DepMap* make_dep_map(char* name) {
  DepMap* map = malloc(sizeof(*map));
  map->name = strdup(name);
  map->head = NULL;
  return map;
}

// Warning: takes ownership of |dep|; don't free it after calling map_add!
void dep_map_add(DepMap* map, char* str, Dependency* std_dep, DepArrVector* glitch_trans_dep, Dependency* original_dep) {
  DepMapElem* e = malloc(sizeof(*e));
  e->key = str;
  e->std_dep = std_dep;
  e->glitch_trans_dep = glitch_trans_dep;
  e->original_dep = original_dep;
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

char* dep_get_from_expr_nofail(DependencyList* deps, int length, Dependency* dep, DepArrVector* dep_arr, int deps_size) {
  if(dep_arr->length > 1){
    return NULL;
  }
  for(int i=0; i<length; i++){
    if(deps->deps[i]->length == 1){
      bool eq = true;
      for(int j=0; j<deps_size; j++){
        eq = eq && (dep[j] == deps->deps_exprs[i][j]);
        eq = eq && (dep[j] == deps->deps[i]->content[0][j]);
      }

      if(eq){
        return deps->names[i];
      }
    }
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

void print_dep_map(DepMap* map, int deps_size) {
  printf("%s: {",map->name); 
  DepMapElem* e = map->head; 
  while (e) { 
    printf("  %s: [ ",e->key); 
    for (int i = 0; i < deps_size; i++) { 
      printf("%d ", e->std_dep[i]); 
    } 
    printf("]\n"); 
    e = e->next; 
  } 
  printf("}\n"); 
} 


void free_faults_combs(FaultsCombs * fc){
  if(!fc) return;
  for(int i=0; i< fc->length; i++){
    FaultsComb * f = fc->fc[i];
    for(int j=0; j<f->length; j++){
      free(f->names[j]);
    }
    free(f);
  }
  free(fc->fc);
  free(fc);
}

void print_faults_combs(FaultsCombs * fc){
  if(!fc) return;
  printf("%d\n", fc->length);
  for(int i=0; i< fc->length; i++){
    FaultsComb * f = fc->fc[i];
    printf("%d, ", f->length);
    for(int j=0; j<f->length-1; j++){
      printf("%s, ", f->names[j]);
    }
    printf("%s\n", f->names[f->length-1]);
  }
}

bool ignore_faulty_scenario(Faults * fv, FaultsCombs * fc){
  if(!fc) return false;
  for(int i=0; i<fc->length; i++){
    if(fc->fc[i]->length == fv->length){

      int * cpt = calloc(fv->length, sizeof(*cpt));

      for(int j=0; j< fv->length; j++){
        char * e = fc->fc[i]->names[j];

        for(int k=0; k< fv->length; k++){
          if(strcmp(e, fv->vars[k]->name) == 0){
            cpt[k]++;
            break;
          }
        }
      }
      bool equal = true;
      for(int j = 0; j< fv->length; j++){
        equal = equal & (cpt[j] == 1);
      }
      free(cpt);

      if(equal) return true;
    }
  }

  return false;
}
