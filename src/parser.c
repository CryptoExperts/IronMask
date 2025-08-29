#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <inttypes.h>

#include "parser.h"
#include "circuit.h"
#include "vectors.h"
#include "utils.h"

/* ***************************************************** */
/*              File parsing                             */
/* ***************************************************** */


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


/*
Expr* parse_expr(char* line, char* str, int characteristic) {
  Expr* ret_e = malloc(sizeof(*ret_e));
  ret_e->left = NULL;
  ret_e->right = NULL;

  skip_spaces(str);

  int end = 0;
  while (!is_eol(str[end]) && !is_space(str[end]) && !is_operator(str[end])) end++;

  if(is_not(str[end])){
    ret_e->op = Add;
    ret_e->right = strndup("1", 1);
    str += end + 1;
    end = 0;

    if(is_eol(str[end]) || is_space(str[end])){
      fprintf(stderr, "Error in line '%s': variable expected after ~ operator, got '%c'. Exiting.\n",
            line, *str);
      exit(EXIT_FAILURE);
    }

    while (!is_eol(str[end]) && !is_space(str[end])) end++;
    ret_e->left = strndup(str, end);
  }
  else{
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
  }

  return ret_e;
}*/



Expr* parse_expr(char* line, char* str, int characteristic) {
  Expr* ret_e = malloc(sizeof(*ret_e));
  ret_e->left = NULL;
  ret_e->right = NULL;

  skip_spaces(str);
  
  int coeff = 1; // Bydefault, the coefficient of a variable is set to 1.
  int end = 0;
  while (!is_eol(str[end]) && !is_space(str[end]) && !is_operator(str[end]) && !is_coeff(str,end)) end++;

  if (is_coeff(str,end)){
    while (is_number(str[end])) end++;
    char *str_coeff = strndup(str, end);
    sscanf(str_coeff, "%d", &coeff);
    coeff %= characteristic;
    coeff = coeff < 0 ? coeff + characteristic: coeff;   
    free(str_coeff);
    
    str += end;
    skip_spaces(str);
    end = 0;
    while (!is_eol(str[end]) && !is_space(str[end]) && !is_operator(str[end])) end++;  
  } 
  
  
  ret_e->left = strndup(str, end);
  ret_e->coeff_left = coeff;
  
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
    coeff = 1;
    while (!is_eol(str[end]) && !is_space(str[end]) && !is_operator(str[end]) && !is_coeff(str,end)) end++;
    
    if (is_coeff(str,end)) {
      while (is_number(str[end])) end++;
      char *str_coeff = strndup(str, end);
      sscanf(str_coeff, "%d", &coeff);
      free(str_coeff);
      coeff %= characteristic;
      coeff = coeff < 0 ? coeff + characteristic: coeff;   
      str += end;
      skip_spaces(str);
      end = 0;
      while (!is_eol(str[end]) && !is_space(str[end]) && !is_operator(str[end])) end++;  
    } 
    
    ret_e->right = strndup(str, end);
    ret_e->coeff_right = coeff;
  }

  return ret_e;
}

void parse_eq_str(EqList* eqs, char* str, int characteristic) {
  bool anti_glitch = false;
  bool correction_output = false;
  bool correction = false;
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
  char * str_tmp = str;
  if (*str == '!' && *(str+1) == '[') {
    anti_glitch = true;
    str += 2;
    skip_spaces(str);
    // Removing the final ']'
    size_t idx = 0;
    while ((idx < strlen(str)) && (str[idx] != ']')) idx++;
    if (idx == strlen(str)) {
      fprintf(stderr, "Invalid line: '![' without matching ']'.\n"
              "Reminder: the closing ']' must be the last non-space character of the line.\n"
              "Exiting.");
      exit(EXIT_FAILURE);
    }
    str_tmp = str + idx + 1;
    str[idx] = '\0'; // truncating the end of the string
  }

  while(!is_eol(*str_tmp)) str_tmp++;
  if(*str_tmp == '#'){
    if(str_equals_nocase(str_tmp+1, "correction_o", 12)){
      correction_output = true;
      correction = true;
    }
    else if(str_equals_nocase(str_tmp+1, "correction", 10)){
      correction = true;
    }
  }

  Expr* e = parse_expr(str_start, str, characteristic);
  add_eq_list(eqs, dst, e, anti_glitch, correction, correction_output);

  //if(correction_output) printf("%s is correction output\n", dst);
}

ParsedFile* parse_file(char* filename) {
  FILE* f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "Cannot open file '%s'.\n", filename);
    exit(EXIT_FAILURE);
  }

  int order = -1, shares = -1, nb_duplications = 1, characteristic = 2;
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
      } else if (str_equals_nocase(&line[i], "DUPLICATIONS", 12)) {
        if (sscanf(&line[i+12], "%d", &nb_duplications) != 1) {
          fprintf(stderr, "Missing number on line '%s'.\n", line);
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
      } else if (str_equals_nocase(&line[i], "CAR", 3)) {  
        if (sscanf(&line[i + 3], "%d", &characteristic) != 1) {
          fprintf(stderr, "Missing number on line '%s'.\n", line);
          exit(EXIT_FAILURE);
        }
      } else if (str_equals_nocase(&line[i], "CHARACTERISTIC", 14)) {
        if (sscanf(&line[i + 14], "%d", &characteristic) != 1) {
          fprintf(stderr, "Missing number on line '%s'.\n", line);
          exit(EXIT_FAILURE);
        }
      } else {
        fprintf(stderr, "Unrecognized line '%s'. Ignoring it.\n", line);
      }
    } else {
      // Equation line
      parse_eq_str(eqs, line, characteristic);
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

  print_str_map(in);
  print_str_map(randoms);
  print_str_map(out);
  //print_eq_list(eqs);

  ParsedFile * pf = malloc(sizeof(*pf));
  pf->eqs = eqs;
  pf->in = in;
  pf->out = out;
  pf->randoms = randoms;
  pf->nb_duplications = nb_duplications;
  pf->shares = shares;
  pf->filename = strdup(filename);
  pf->characteristic = characteristic;

  return pf;
}

void free_parsed_file(ParsedFile * pf){
  free_str_map(pf->in);
  free_str_map(pf->randoms);
  free_str_map(pf->out);
  free_eq_list(pf->eqs);
  free(pf->filename);
  free(pf);
}


/* ***************************************************** */
/*                 Circuit building                      */
/* ***************************************************** */

typedef struct mult_ptrs_oririginal{
  Dependency * left_original;
  Dependency * right_original;
  int idx_same_dependencies;
}OriginalMultPtrs;

typedef struct _original_deps{
  Dependency ** original_deps;
  OriginalMultPtrs ** original_mult_ptrs;
  int original_deps_len;
  int deps_size;
  int mult_len;
}OriginalDeps;

OriginalDeps * init_original_deps(int deps_len, int mult_count, int deps_size){
  OriginalDeps * original_deps = malloc(sizeof(*original_deps));
  original_deps->deps_size = deps_size;
  original_deps->mult_len = mult_count;
  original_deps->original_deps_len = deps_len;

  original_deps->original_deps = malloc(deps_len * sizeof(*(original_deps->original_deps)));
  for(int i=0; i< deps_len;i++){
    original_deps->original_deps[i] = calloc(deps_size, sizeof(*(original_deps->original_deps[i])));
  }

  original_deps->original_mult_ptrs = malloc(mult_count * sizeof(*(original_deps->original_mult_ptrs)));
  for(int i=0; i<mult_count;i ++){
    original_deps->original_mult_ptrs[i] = malloc(sizeof(*(original_deps->original_mult_ptrs[i])));
  }

  return original_deps;
}

void free_original_deps(OriginalDeps * original_deps){
  for(int i=0; i< original_deps->original_deps_len;i++){
    free(original_deps->original_deps[i]);
  }
  free(original_deps->original_deps);

  for(int i=0; i< original_deps->mult_len ;i ++){
    free(original_deps->original_mult_ptrs[i]);
  }
  free(original_deps->original_mult_ptrs);

  free(original_deps);
}




bool vec_contains_dep(DepArrVector* dep_vec, Dependency * dep, int size){

  for(int i=0; i<dep_vec->length; i++){
    bool eq = true;
    for(int j=0; j<size; j++){
      eq = eq && (dep[j] == dep_vec->content[i][j]);
    }

    if(eq){
      return true;
    }
  }

  return false;
}

int count_mults(EqList* eqs) {
  int total = 0;
  for (EqListElem* e = eqs->head; e != NULL; e = e->next) {
    if ((e->expr->op == Mult) && (!e->correction) && (!(e->correction_output)) ) total += 1;
  }
  return total;
}

int count_correction_outputs(EqList* eqs) {
  int total = 0;
  for (EqListElem* e = eqs->head; e != NULL; e = e->next) {
    if (e->correction_output) total += 1;
  }
  return total;
}

bool is_dep_constant(Dependency * dep, int length){
  bool is_constant = true;
  for(int i=0; i<length-1; i++){
    is_constant = is_constant && (dep[i] == 0);
  }
  return is_constant;
}

bool is_var_constant(char * name){
  return (strcmp(name, "1") == 0) || (strcmp(name, "0") == 0);
}

bool are_dep_equal(Dependency * dep1, Dependency * dep2, int deps_size){
  bool eq = true;
  for(int i=0; i< deps_size; i++){
    eq = eq && (dep1[i] == dep2[i]);
  }
  return eq;
}

bool are_dep_equal_with_mult(Dependency * dep1, Dependency * dep2, int deps_size,
                             int non_mult_deps_count,
                             int mult_deps_count, MultDependencyList * mult_deps){
  bool eq = true;
  for(int i=0; i< non_mult_deps_count; i++){
    eq = eq && (dep1[i] == dep2[i]);
  }
  for(int i=non_mult_deps_count+mult_deps_count; i< deps_size; i++){
    eq = eq && (dep1[i] == dep2[i]);
  }

  int * eq_mult = calloc(mult_deps_count, sizeof(int));
  for(int i=0; i< mult_deps_count; i++){
    if(dep1[i+non_mult_deps_count]){
      eq_mult[mult_deps->deps[i]->idx_same_dependencies] ++;
    }
    if(dep2[i+non_mult_deps_count]){
      eq_mult[mult_deps->deps[i]->idx_same_dependencies] ++;
    }
  }
  for(int i=0; i< mult_deps_count; i++){
    eq = eq && (eq_mult[i] % 2 == 0);
  }
  free(eq_mult);

  return eq;
}

bool are_dep_equal_with_mult_original(Dependency * dep1, Dependency * dep2, int deps_size,
                                      int non_mult_deps_count,
                                      int mult_deps_count, OriginalMultPtrs ** original_mult_ptrs){
  bool eq = true;
  for(int i=0; i< non_mult_deps_count; i++){
    eq = eq && (dep1[i] == dep2[i]);
  }
  for(int i=non_mult_deps_count+mult_deps_count; i< deps_size; i++){
    eq = eq && (dep1[i] == dep2[i]);
  }

  int * eq_mult = calloc(mult_deps_count, sizeof(int));
  for(int i=0; i< mult_deps_count; i++){
    if(dep1[i+non_mult_deps_count]){
      eq_mult[original_mult_ptrs[i]->idx_same_dependencies] ++;
    }
    if(dep2[i+non_mult_deps_count]){
      eq_mult[original_mult_ptrs[i]->idx_same_dependencies] ++;
    }
  }
  for(int i=0; i< mult_deps_count; i++){
    eq = eq && (eq_mult[i] % 2 == 0);
    //printf("%d\n", eq_mult[i]);
  }
  free(eq_mult);

  return eq;
}

bool are_dep_inverse(Dependency * dep1, Dependency * dep2, int deps_size){
  bool eq = true;
  for(int i=0; i< deps_size-1; i++){
    eq = eq && (dep1[i] == dep2[i]);
  }
  return eq && (dep1[deps_size-1] + dep2[deps_size-1] == 1);
}


void update_same_dependencies_idx_last_mult(MultDependencyList * mult_deps,
                                            int mult_length, int deps_size,
                                            OriginalMultPtrs ** original_mult_ptrs){
  

  MultDependency * mult_dep = mult_deps->deps[mult_length-1];
  for(int k=0; k<mult_length-1; k++){
    MultDependency * iter_mult_dep = mult_deps->deps[k];
    if(are_dep_equal(mult_dep->left_ptr, iter_mult_dep->left_ptr, deps_size) &&
        are_dep_equal(mult_dep->right_ptr, iter_mult_dep->right_ptr, deps_size)){
        mult_dep->idx_same_dependencies = k;
        break;
    }
    else if(are_dep_equal(mult_dep->left_ptr, iter_mult_dep->right_ptr, deps_size) &&
            are_dep_equal(mult_dep->right_ptr, iter_mult_dep->left_ptr, deps_size)){
      mult_dep->idx_same_dependencies = k;
      break;
    }
  }

  OriginalMultPtrs * original = original_mult_ptrs[mult_length-1];
  for(int k=0; k<mult_length-1; k++){
    OriginalMultPtrs * iter_mult_dep = original_mult_ptrs[k];
    if(are_dep_equal(original->left_original, iter_mult_dep->left_original, deps_size) &&
        are_dep_equal(original->right_original, iter_mult_dep->right_original, deps_size)){

      original->idx_same_dependencies = k;
      //printf("original %d is same as original %d\n", mult_length-1, k);
      break;
    }
    else if(are_dep_equal(mult_dep->left_ptr, iter_mult_dep->right_original, deps_size) &&
            are_dep_equal(mult_dep->right_ptr, iter_mult_dep->left_original, deps_size)){
      original->idx_same_dependencies = k;
      //printf("original %d is same as original %d\n", mult_length-1, k);
      break;
    }
  }
}


void handle_faulted_correction_output(EqList* eqs,
                                uint64_t * fault_idx, bool * split, int add_idx, int corr_output_idx,
                                Dependency * dep, DependencyList* deps,
                                Faults * fv, Dependency ** original_deps, int deps_size){

  //printf("%s\n", e->dst);
  if(!split[add_idx]) return;

  bool fault_in_correction = false;
  uint64_t f_idx = fault_idx[add_idx];
  while (f_idx != 0) {
    int fault_idx_in_elem = __builtin_ia32_lzcnt_u64(f_idx);
    f_idx &= ~(1ULL << (63-fault_idx_in_elem));
    int f_elem_idx = 63-fault_idx_in_elem;
    EqListElem * eq = get_eq_list(eqs, fv->vars[f_elem_idx]->name);
    if(eq && eq->correction && (!eq->correction_output)){
      //printf("%s DETECTED\n", e->dst);
      fault_in_correction = true;
      break;
    }
  }
  if(!fault_in_correction){
    split[add_idx] = false;
    fault_idx[add_idx] = 0;
    memcpy(dep, original_deps[add_idx], deps_size * sizeof(*dep));
  }else{
    memset(dep, 0, deps_size*sizeof(*dep));
    dep[deps->first_correction_idx + corr_output_idx] = 1;
  }
}

inline void add_elem_dep(char * name, int dep_idx, Dependency d,
                          int deps_size, int add_idx, DepMap* deps_map,
                          StrMap* positions_map, bool * split,
                          Dependency ** original_deps, DependencyList* deps){

  Dependency* dep = calloc(deps_size, sizeof(*dep));
  dep[dep_idx] = d;
  DepArrVector* dep_arr = DepArrVector_make();
  DepArrVector_push(dep_arr, dep);
  
  deps->deps[add_idx]       = dep_arr;
  deps->deps_exprs[add_idx] = dep;
  deps->names[add_idx]      = strdup(name);

  memcpy(original_deps[add_idx], dep, deps_size*sizeof(*dep));
  dep_map_add(deps_map, strdup(name), dep, dep_arr, original_deps[add_idx]);
  str_map_add(positions_map, strdup(name));
  if (split)
    split[add_idx] = false;
}


Circuit* gen_circuit(ParsedFile * pf, bool glitch, bool transition, Faults * fv) {

  StrMap* in = pf->in;
  StrMap* randoms = pf->randoms;
  StrMap* out = pf->out;
  EqList* eqs = pf->eqs;
  int nb_duplications = pf->nb_duplications;
  int shares = pf->shares;

  bool faults_on_inputs = false;
  if(fv){
    for(int i=0; i< fv->length; i++){
      if(fv->vars[i]->fault_on_input){
        faults_on_inputs = true;
        break;
      }
    }
  }
  // if(faults_on_inputs){
  //   fprintf(stderr, "There are faults on inputs, exiting...\n");
  //   exit(EXIT_FAILURE);
  // }

  Circuit* c = malloc(sizeof(*c));

  int circuit_size = eqs->size;

  int mult_count = count_mults(eqs);
  int correction_outputs_count = count_correction_outputs(eqs);
  int linear_deps_size = in->next_val 
                         + (faults_on_inputs ? in->next_val * shares : 0)
                         + randoms->next_val;

  //printf("There are %d multiplications\n", mult_count);

  int deps_size = in->next_val
                  + (faults_on_inputs ? in->next_val * shares : 0)
                  + randoms->next_val
                  + mult_count
                  + correction_outputs_count
                  + 1;                        // for the constant term "1" or "0"

  MultDependencyList* mult_deps = malloc(sizeof(*mult_deps));
  mult_deps->length = mult_count;
  mult_deps->deps = malloc(mult_count * sizeof(*(mult_deps->deps)));

  int ** temporary_mult_idx = malloc(mult_count * sizeof(*temporary_mult_idx));

  DependencyList* deps = malloc(sizeof(*deps));
  deps->length         = circuit_size + randoms->next_val +
                          in->next_val * shares * nb_duplications;
  deps->deps_size      = deps_size;

  deps->first_rand_idx = in->next_val + (faults_on_inputs ? in->next_val * shares : 0);
  deps->first_mult_idx = deps->first_rand_idx + randoms->next_val;
  deps->first_correction_idx = (correction_outputs_count == 0) ? -1 : (deps->first_mult_idx + mult_count);

  deps->deps           = malloc(deps->length * sizeof(*deps->deps));
  deps->deps_exprs     = malloc(deps->length * sizeof(*deps->deps_exprs));
  deps->names          = malloc(deps->length * sizeof(*deps->names));
  deps->mult_deps      = mult_deps;

  CorrectionOutputs * correction_outputs = malloc(sizeof(*correction_outputs));
  if(correction_outputs_count != 0){
    correction_outputs->correction_outputs_deps = malloc(correction_outputs_count * sizeof(*correction_outputs->correction_outputs_deps));
    correction_outputs->correction_outputs_names = malloc(correction_outputs_count * sizeof(*correction_outputs->correction_outputs_names));
  }
  correction_outputs->length = correction_outputs_count;
  deps->correction_outputs = correction_outputs;

  bool * split = malloc(deps->length * sizeof(*split));
  uint64_t * fault_idx = calloc(deps->length, sizeof(*fault_idx));

  OriginalDeps * orig_deps_struct = init_original_deps(deps->length, mult_count, deps_size);
  Dependency ** original_deps = orig_deps_struct->original_deps;
  OriginalMultPtrs ** original_mult_ptrs = orig_deps_struct->original_mult_ptrs;
   
  DepMap* deps_map = make_dep_map("Dependencies");

  int* weights = calloc(deps->length, sizeof(*weights));
  StrMap* positions_map = make_str_map("Positions");

  int add_idx = 0, mult_idx = 0, corr_output_idx = 0;

  // Initializing "0" and "1" dependencies
  for(int i=0; i<2; i++){
    char* name = malloc(2 * sizeof(*name));
    snprintf(name, 2, "%d", i);
    Dependency* dep = calloc(deps_size, sizeof(*dep));
    dep[deps_size-1] = i;
    DepArrVector* dep_arr = DepArrVector_make();
    dep_map_add(deps_map, name, dep, dep_arr, dep);
  }

  // Initializing dependencies with inputs
  int inp_idx = 0;
  for (StrMapElem* e = in->head; e != NULL; e = e->next) {
    int len = strlen(e->key) + 1 + 2 + (nb_duplications <= 1 ? 0 : 3);  // +1 for '\0' and +2 for share number and +1 for "_" and +2 for dup number
    for (int i = 0; i < shares; i++) {
      if(nb_duplications <= 1){
        char* name = malloc(len * sizeof(*name));
        snprintf(name, len, "%s%d", e->key, i);
        add_elem_dep(name, inp_idx, 1 << i, deps_size, add_idx, deps_map, positions_map, split, original_deps, deps);
        add_idx += 1;
        free(name);
      }
      else{
        for(int j=0; j< nb_duplications; j++){
          char* name = malloc(len * sizeof(*name));
          snprintf(name, len, "%s%d_%d", e->key, i, j);
          add_elem_dep(name, inp_idx, 1 << i, deps_size, add_idx, deps_map, positions_map, split, original_deps, deps);
          add_idx += 1;
          free(name);
        }
      }
    }
    inp_idx++;
  }

  // Initializing random dependencies
  int rand_idx = 0;
  for (StrMapElem* e = randoms->head; e != NULL; e = e->next, add_idx++) {
    add_elem_dep(e->key, deps->first_rand_idx+rand_idx, 1, deps_size, add_idx, deps_map, positions_map, split, original_deps, deps);
    rand_idx++;
  }

  if(fv){
    for(int i=0; i<fv->length; i++){
      if(str_map_contains(positions_map, fv->vars[i]->name)){
        int idx = str_map_get(positions_map, fv->vars[i]->name);
        split[idx] = false;
        fault_idx[idx] = 1ULL << i;

        if(fv->vars[i]->fault_on_input){
          if(deps->deps_exprs[idx][0]){
            deps->deps_exprs[idx][0] = 0;
            deps->deps_exprs[idx][in->next_val+fv->vars[i]->share] = 1ULL << fv->vars[i]->duplicate;
          }
          else{
            assert((deps->deps_exprs[idx][1]) && (in->next_val==2));
            deps->deps_exprs[idx][1] = 0;
            deps->deps_exprs[idx][in->next_val+shares+fv->vars[i]->share] = 1ULL << fv->vars[i]->duplicate;
          }
        }
        else{
          memset(deps->deps_exprs[idx], 0, deps_size * sizeof(*deps->deps_exprs[idx]));
          memset(deps->deps[idx]->content[0], 0, deps_size * sizeof(*deps->deps[idx]->content[0]));
          if(fv->vars[i]->set){
            deps->deps_exprs[idx][deps_size-1] = 1;
            deps->deps[idx]->content[0][deps_size-1] = 1;
          }
        }
      }
    }
  }

  // Adding dependencies of other instructions
  for (EqListElem* e = eqs->head; e != NULL; e = e->next, add_idx++) {    
    Dependency* dep;
    DepMapElem* left  = dep_map_get(deps_map, e->expr->left);
    DepMapElem* right = e->expr->op != Asgn ? dep_map_get(deps_map, e->expr->right) : NULL;
    split[add_idx] = false;
    fault_idx[add_idx] = 0;

    int str_pos_left = str_map_contains(positions_map, e->expr->left) ? str_map_get(positions_map, e->expr->left) : -1;
    int str_pos_right = (right != NULL) ? (str_map_contains(positions_map, e->expr->right) ? str_map_get(positions_map, e->expr->right) : -1) : -1;
    if(str_pos_left != -1){
      split[add_idx] = split[add_idx] || split[str_pos_left];
      fault_idx[add_idx] = fault_idx[add_idx] | fault_idx[str_pos_left];
    }
    if(str_pos_right != -1){
      split[add_idx] = split[add_idx] || split[str_pos_right];
      fault_idx[add_idx] = fault_idx[add_idx] | fault_idx[str_pos_right];
    }

    if(!e->correction){
      if(e->expr->op == Asgn){
        dep = left->std_dep;
        memcpy(original_deps[add_idx], left->original_dep, deps_size * sizeof(*original_deps[add_idx]));
      } 
      else if (e->expr->op == Add) {
        dep = calloc(deps_size, sizeof(*dep));
        for (int i = 0; i < deps_size; i++) {
          dep[i] = left->std_dep[i] ^ right->std_dep[i];
          original_deps[add_idx][i] = left->original_dep[i] ^ right->original_dep[i];
        }
      }
      else{
        MultDependency* mult_dep = malloc(sizeof(*mult_dep));
        mult_dep->left_ptr  = left->std_dep;
        mult_dep->right_ptr = right->std_dep;
        mult_dep->name = strdup(e->dst);
        mult_dep->name_left = strdup(e->expr->left);
        mult_dep->name_right = strdup(e->expr->right);
        mult_dep->contained_secrets = NULL;
        mult_dep->idx_same_dependencies = mult_idx;

        mult_deps->deps[mult_idx] = mult_dep;

        dep = calloc(deps_size, sizeof(*dep));
        dep[linear_deps_size + mult_idx] = 1;
        original_deps[add_idx][linear_deps_size + mult_idx] = 1;

        original_mult_ptrs[mult_idx]->left_original = left->original_dep;
        original_mult_ptrs[mult_idx]->right_original = right->original_dep;
        original_mult_ptrs[mult_idx]->idx_same_dependencies = mult_idx;

        for(int k=linear_deps_size; k<linear_deps_size+mult_count; k++){
          if(mult_dep->left_ptr[k] || mult_dep->right_ptr[k]){
            fprintf(stderr, "Unsupported mult. variable %s. Multiplicative depth > 1. Exiting...\n", e->dst);
            exit(EXIT_FAILURE);
          }
        }

        temporary_mult_idx[mult_idx] = malloc(2 * sizeof(*temporary_mult_idx[mult_idx]));
        assert((str_pos_left != -1) && (str_pos_right != -1));
        temporary_mult_idx[mult_idx][0] = str_pos_left;
        temporary_mult_idx[mult_idx][1] = str_pos_right;

        mult_idx++;

        update_same_dependencies_idx_last_mult(mult_deps, mult_idx, deps_size, original_mult_ptrs);

        if(is_dep_constant(left->std_dep, deps_size)){
          if(left->std_dep[deps_size-1]){
            split[add_idx] = split[str_pos_right];
          }
          else{
            split[add_idx] = false;
          }
        }
        else if(is_dep_constant(right->std_dep, deps_size)){
          if(right->std_dep[deps_size-1]){
            split[add_idx] = split[str_pos_left];
          }
          else{
            split[add_idx] = false;
          }
        }    
      }
    }
    else{
      if(e->expr->op == Asgn){
        dep = left->std_dep;
        memcpy(original_deps[add_idx], left->original_dep, deps_size * sizeof(*original_deps[add_idx]));
        if(e->correction_output){
          // fprintf(stderr, "Unsupported format for assignment correction output variable %s\n", e->dst);
          // exit(EXIT_FAILURE);
          handle_faulted_correction_output(eqs, fault_idx, split, 
                                         add_idx, corr_output_idx, dep, deps,
                                         fv, original_deps, deps_size);
        }
      }
      else if (e->expr->op == Add) {
        dep = calloc(deps_size, sizeof(*dep));
        for (int i = 0; i < deps_size; i++) {
          dep[i] = left->std_dep[i] ^ right->std_dep[i];
          original_deps[add_idx][i] = left->original_dep[i] ^ right->original_dep[i];
        }
        if(e->correction_output){
          handle_faulted_correction_output(eqs, fault_idx, split, 
                                         add_idx, corr_output_idx, dep, deps,
                                         fv, original_deps, deps_size);
        }
      }
      else{
        dep = calloc(deps_size, sizeof(*dep));

        // assert(are_dep_equal(left->original_dep, right->original_dep, deps_size) ||
        //        are_dep_equal_with_mult_original(left->original_dep, right->original_dep,
        //        deps_size, linear_deps_size, mult_count, original_mult_ptrs));

        memcpy(original_deps[add_idx], left->original_dep, deps_size * sizeof(*original_deps[add_idx]));

        if(!split[add_idx]){
          //printf("%s\n", e->dst);
          if(are_dep_equal_with_mult(left->std_dep, right->std_dep,
             deps_size, linear_deps_size, mult_count, mult_deps)){

            memcpy(dep, left->std_dep, deps_size * sizeof(*dep));

          }
          else{
            split[add_idx] = true;
          }
        }
        else{
          if(are_dep_equal_with_mult(left->std_dep, right->std_dep,
             deps_size, linear_deps_size, mult_count, mult_deps)){

            memcpy(dep, left->std_dep, deps_size * sizeof(*dep));
            split[add_idx] = false;
          }
          else if(is_dep_constant(left->std_dep, deps_size)){
            if(left->std_dep[deps_size-1]){
              memcpy(dep, right->std_dep, deps_size * sizeof(*dep));
              split[add_idx] = split[str_pos_right];
            }
            else{
              memset(dep, 0, deps_size * sizeof(*dep));
              split[add_idx] = false;
            }
          }
          else if(is_dep_constant(right->std_dep, deps_size)){
            if(right->std_dep[deps_size-1]){
              memcpy(dep, left->std_dep, deps_size * sizeof(*dep));
              split[add_idx] = split[str_pos_left];
            }
            else{
              memset(dep, 0, deps_size * sizeof(*dep));
              split[add_idx] = false;
            }
          }    
        }
        
        if(e->correction_output){
          handle_faulted_correction_output(eqs, fault_idx, split, 
                                            add_idx, corr_output_idx, dep, deps,
                                            fv, original_deps, deps_size);
        }
        
      }
    }

    
    // Taking glitches and transitions into account. We ignore the
    // interaction between glitches and transitions are assume that
    // either glitches or (exclusively) transitions are to be
    // considered.
    DepArrVector* dep_arr = DepArrVector_make();
    if ((!glitch || e->anti_glitch) && (!split[add_idx])) {
      DepArrVector_push(dep_arr, dep);
    } else {
      //printf("SPLITTING %s\n", e->dst);
      for (int i = 0; i < left->glitch_trans_dep->length; i++) {
        if (!vec_contains_dep(dep_arr, left->glitch_trans_dep->content[i], deps_size)) {
          DepArrVector_push(dep_arr, left->glitch_trans_dep->content[i]);
        }
      }
      if (right) {
        for (int i = 0; i < right->glitch_trans_dep->length; i++) {
          // Avoiding duplicates, which might occur if a dependency is
          // in both operands.
          if (!vec_contains_dep(dep_arr, right->glitch_trans_dep->content[i], deps_size)) {
            DepArrVector_push(dep_arr, right->glitch_trans_dep->content[i]);
          }
        }
      }
    }
    if (transition) {
      DepMapElem* prev_value = dep_map_get(deps_map, e->dst);

      if(fv){
        fprintf(stderr, "Unsupported combination of transitions and faults in current implementation\n");
        exit(EXIT_FAILURE);
      }
      else{
        DepArrVector_push(dep_arr, prev_value->std_dep);
      }
    }

    if(e->correction_output){
      DepArrVector* dep_arr_corr = DepArrVector_make();

      if(!split[add_idx]){
        assert(!dep[deps->first_correction_idx + corr_output_idx]);
      }
      else{
        assert(dep[deps->first_correction_idx + corr_output_idx]);
        for (int i = 0; i < left->glitch_trans_dep->length; i++) {
          if (!vec_contains_dep(dep_arr_corr, left->glitch_trans_dep->content[i], deps_size)) {
            DepArrVector_push(dep_arr_corr, left->glitch_trans_dep->content[i]);
          }
        }
        if (right) {
          for (int i = 0; i < right->glitch_trans_dep->length; i++) {
            if (!vec_contains_dep(dep_arr_corr, right->glitch_trans_dep->content[i], deps_size)) {
              DepArrVector_push(dep_arr_corr, right->glitch_trans_dep->content[i]);
            }
          }
        }
      }

      correction_outputs->correction_outputs_deps[corr_output_idx] = dep_arr_corr;
      correction_outputs->correction_outputs_names[corr_output_idx] = strdup(e->dst);
      corr_output_idx++;
    }

    if(fv){
      for(int i=0; i<fv->length; i++){
        if(strcmp(e->dst, fv->vars[i]->name) == 0){
          memset(dep, 0, deps_size * sizeof(*dep));
          dep[deps_size-1] = fv->vars[i]->set;

          DepArrVector_shallow_free(dep_arr);
          dep_arr = DepArrVector_make();

          DepArrVector_push(dep_arr, dep);

          split[add_idx] = false;
          fault_idx[add_idx] |= 1ULL << i;

          if(e->correction_output){
            DepArrVector* dep_arr_corr = correction_outputs->correction_outputs_deps[corr_output_idx-1]; 
            DepArrVector_shallow_free(dep_arr_corr);
            correction_outputs->correction_outputs_deps[corr_output_idx-1] = DepArrVector_make();
          }

          break;
        }
      }
    }

    // Updating weights
    if (str_pos_left != -1){
      weights[str_pos_left] += weights[str_pos_left] == 0 ? 1 : 2;
    }
    if (e->expr->op != Asgn) {
      if(str_pos_right != -1){
        weights[str_pos_right] += weights[str_pos_right] == 0 ? 1 : 2;
      }      
    }

    // Adding to deps
    deps->deps[add_idx]       = dep_arr;
    deps->deps_exprs[add_idx] = dep;
    deps->names[add_idx]      = strdup(e->dst);
    dep_map_add(deps_map, strdup(e->dst), dep, dep_arr, original_deps[add_idx]);
    str_map_add(positions_map, strdup(e->dst));
  }

  // Moving outputs to the end
  StrMap* outputs_map = make_str_map("outputs expanded");
  if(nb_duplications <= 1){
    for (StrMapElem* e = out->head; e != NULL; e = e->next) {
      int len = strlen(e->key) + 1 + 3; // +1 for '\0' and +3 for share number
      for (int i = 0; i < shares; i++) {
        char* name = malloc(len * sizeof(*name));
        snprintf(name, len, "%s%d", e->key, i);
        str_map_add_with_val(outputs_map, name, 1);
      }
    }
  }
  else{
    for (StrMapElem* e = out->head; e != NULL; e = e->next){
      int len = strlen(e->key) + 1 + 2 + 1 + 2; // +1 for '\0' and +2 for share number and +1 for "_" and +2 for duplicate number
      for (int i = 0; i < shares; i++) {
        for(int j=0; j< nb_duplications; j++){
          char* name = malloc(len * sizeof(*name));
          snprintf(name, len, "%s%d_%d", e->key, i, j);
          str_map_add_with_val(outputs_map, name, 1);
        }
      }
    }
  }

  // Finding first index (from the end) that does not contain an output
  int end_idx = add_idx-1;
  while (str_map_contains(outputs_map, deps->names[end_idx])) {
    str_map_remove(outputs_map, deps->names[end_idx]);
    end_idx--;
  }

  // Finding outputs and swapping them to the end
#define SWAP(_type, _v1, _v2) {               \
  _type _tmp = _v1;                           \
  _v1 = _v2;                                  \
  _v2 = _tmp;                                 \
}
#define SWAP_DEPS(i1,i2) {                                        \
  SWAP(char*, deps->names[i1], deps->names[i2]);                  \
  SWAP(DepArrVector*, deps->deps[i1], deps->deps[i2]);            \
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
    // weights[i] = 1;
    if (!weights[i]) weights[i] = 1;
  }

  c->length          = deps->length - out->next_val * shares * nb_duplications;
  c->nb_duplications = nb_duplications;
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
  c->faults_on_inputs = faults_on_inputs;
  c->characteristic = 2;


  compute_total_wires(c);
  compute_rands_usage(c);
  compute_contained_secrets(c, temporary_mult_idx);
  compute_bit_deps(c, temporary_mult_idx);
  compute_total_correction_bit_deps(c);


  //print_circuit(c);

  // print_eq_full_expr(eqs, "temp150");
  // printf("\n\n");
  // print_eq_full_expr(eqs, "temp60");
  // printf("\n\n");
  // print_eq_full_expr(eqs, "temp61");
  // printf("\n\n");
  //print_eq_full_expr(eqs, "temp204");
  //printf("\n\n");

  for(int i=0; i<mult_count; i++){
    free(temporary_mult_idx[i]);
  }
  free(temporary_mult_idx);
  free(split);
  free(fault_idx);
  free_original_deps(orig_deps_struct);


  free_str_map(outputs_map);
  free_str_map(positions_map);
  free_dep_map(deps_map);

  //exit(EXIT_FAILURE);


  return c;
}





Circuit* gen_circuit_arith (ParsedFile * pf, int characteristic){
  
  StrMap* in = pf->in;
  StrMap* randoms = pf->randoms;
  StrMap* out = pf->out;
  EqList* eqs = pf->eqs;
  bool glitch = false; //TODO : Remove this.
  bool transition = false; //TODO : Remove this.
  int shares = pf->shares;

  
  Circuit* c = malloc(sizeof(*c));  
  
  int circuit_size = eqs->size;
  int mult_count = count_mults(eqs);
  int linear_deps_size = in->next_val * shares + randoms->next_val;
  int deps_size = linear_deps_size + mult_count;

  MultDependencyList* mult_deps = malloc(sizeof(*mult_deps));
  mult_deps->length = mult_count;
  mult_deps->deps = malloc(mult_count * sizeof(*(mult_deps->deps)));

  DependencyList* deps = malloc(sizeof(*deps));
  deps->length         = circuit_size + randoms->next_val +
                         in->next_val * shares;
  deps->deps_size      = deps_size;
  deps->first_rand_idx = in->next_val * shares;
  deps->first_mult_idx = deps->first_rand_idx + randoms->next_val;
  
  deps->deps           = malloc(deps->length * sizeof(*deps->deps));
  deps->deps_exprs     = malloc(deps->length * sizeof(*deps->deps_exprs));
  deps->names          = malloc(deps->length * sizeof(*deps->names));
  deps->mult_deps      = mult_deps;

  OriginalDeps * orig_deps_struct = init_original_deps(deps->length, mult_count, deps_size);
  Dependency ** original_deps = orig_deps_struct->original_deps;
  OriginalMultPtrs ** original_mult_ptrs = orig_deps_struct->original_mult_ptrs;

  DepMap* deps_map = make_dep_map("Dependencies");

  int* weights = calloc(deps->length, sizeof(*weights));
  StrMap* positions_map = make_str_map("Positions");

  int add_idx = 0, mult_idx = 0;
  // Initializing dependencies with inputs
  for (StrMapElem* e = in->head; e != NULL; e = e->next) {
    int len = strlen(e->key) + 1 + 2; // +1 for '\0' and +2 for share number
    for (int i = 0; i < shares; i++) {
      char* name = malloc(len * sizeof(*name));
      snprintf(name, len, "%s%d", e->key, i);
      add_elem_dep(name, add_idx, 1, deps_size, add_idx, deps_map, positions_map, NULL, original_deps, deps);
      //Dependency* dep = calloc(deps_size, sizeof(*dep));
      //dep[add_idx] = 1;
      //DepArrVector* dep_arr = DepArrVector_make();
      //DepArrVector_push(dep_arr, dep);
      //deps->deps[add_idx]       = dep_arr;
      //deps->deps_exprs[add_idx] = dep;
      //deps->names[add_idx]      = strdup(name);
      //dep_map_add(deps_map, name, dep, dep_arr);
      //str_map_add(positions_map, strdup(name));
      add_idx += 1;
      free(name);
    }
  }
  
  
  // Initializing random dependencies
  for (StrMapElem* e = randoms->head; e != NULL; e = e->next, add_idx++) {
    add_elem_dep(e->key, add_idx, 1, deps_size, add_idx, deps_map, positions_map, NULL, original_deps, deps);
    //Dependency* dep = calloc(deps_size, sizeof(*dep));
    //dep[add_idx] = 1;
    //DepArrVector* dep_arr = DepArrVector_make();
    //DepArrVector_push(dep_arr, dep);
    //deps->deps[add_idx]       = dep_arr;
    //deps->deps_exprs[add_idx] = dep;
    //deps->names[add_idx]      = strdup(e->key);
    //dep_map_add(deps_map, strdup(e->key), dep, dep_arr);
    //str_map_add(positions_map, strdup(e->key));
  }


  // Adding dependencies of other instructions
  for (EqListElem* e = eqs->head; e != NULL; e = e->next, add_idx++) {
    Dependency* dep;
    DepMapElem* left  = dep_map_get(deps_map, e->expr->left);
    DepMapElem* right = e->expr->op != Asgn ? dep_map_get(deps_map, e->expr->right) : NULL;
    
    
    // Computing dependency |dep|
    if (e->expr->op == Asgn) {
      dep = calloc(deps_size, sizeof(*dep));
      for (int i = 0; i < deps_size; i++) {
        dep[i] = (int) (e->expr->coeff_left * left->std_dep[i]) % characteristic;
      }
    } else if (e->expr->op == Add) {
      dep = calloc(deps_size, sizeof(*dep));
      for (int i = 0; i < deps_size; i++) {
        dep[i] = (int) ((e->expr->coeff_left * left->std_dep[i] + e->expr->coeff_right * right->std_dep[i])) % characteristic; //TODO : Rajouter des coefficients à Eq
      }
    } else { // multiplication
      MultDependency* mult_dep = malloc(sizeof(*mult_dep));
      mult_dep->left_ptr  = left->std_dep;
      mult_dep->right_ptr = right->std_dep;
      mult_dep->name = strdup(e->dst);
      mult_dep->name_left = strdup(e->expr->left);
      mult_dep->name_right = strdup(e->expr->right);
      mult_dep->left_idx  = str_map_get(positions_map, e->expr->left);
      mult_dep->right_idx = str_map_get(positions_map, e->expr->right);
      mult_dep->contained_secrets = NULL;
      mult_dep->idx_same_dependencies = mult_idx;
      mult_deps->deps[mult_idx] = mult_dep;

      dep = calloc(deps_size, sizeof(*dep));
      dep[linear_deps_size + mult_idx] = (e->expr->coeff_left * e->expr->coeff_right) % characteristic;
      mult_idx++;  
    }

    // Taking glitches and transitions into account. We ignore the
    // interaction between glitches and transitions are assume that
    // either glitches or (exclusively) transitions are to be
    // considered.
    DepArrVector* dep_arr = DepArrVector_make();
    DepArrVector_push(dep_arr, dep);

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
    dep_map_add(deps_map, strdup(e->dst), dep, dep_arr, NULL);
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
  c->characteristic  = characteristic;
  
  if(pf->nb_duplications)
    c->nb_duplications = pf->nb_duplications;

  compute_total_wires(c);
  compute_rands_usage_arith(c);
  compute_contained_secrets_arith(c);

  //print_circuit(c);


  free_original_deps(orig_deps_struct);
  //free_str_map(in);
  //free_str_map(randoms);
  //free_str_map(out);
  free_str_map(outputs_map);
  free_str_map(positions_map);
  free_dep_map(deps_map);
  //free_eq_list(eqs);

  return c;
}


