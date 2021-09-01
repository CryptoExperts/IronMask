/* This file verifies multiplications in a compositional way: a
   multiplication is broken down into 3 refresh gadgets, all of which
   are verified separately.

   Warning 1: I did not update this file when introducing glitches and
   transitions (hence why this whole file is commented). The
   modifications should be somewhat easy to do though.

   Warning 2: There is probably a small bug somewhere: I think to
   recall that the results produced were slightly wrong.

 */

/* #include <stdlib.h> */
/* #include <stdio.h> */
/* #include <stdint.h> */
/* #include <string.h> */
/* #include <stdbool.h> */
/* #include <assert.h> */

/* #include "circuit.h" */
/* #include "trie.h" */
/* #include "constructive.h" */
/* #include "verification_rules.h" */
/* #include "vectors.h" */


/* #define SECRET_A_INDEX 0 */
/* #define SECRET_B_INDEX 1 */


/* /\***************************************************************** */

/*                 Utilitary functions */

/*  *****************************************************************\/ */

/* typedef VarVector Tuple; */

/* VarVector empty_array = { .max_size = 0, .length = 0, .content = NULL }; */



/* /\***************************************************************** */

/*            Dependency arrays (so-called "columns") */

/*  *****************************************************************\/ */
/* void build_dependency_arrays_v2(const Circuit* c, */
/*                                 VarVector deps, */
/*                                 VarVector*** secrets, */
/*                                 VarVector*** randoms, */
/*                                 bool many_secrets, // true  = multiplications are secrets */
/*                                                    // false = secrets are secrets (and no */
/*                                                    //         multiplications are present) */
/*                                 int secret_index, */
/*                                 int verbose) { */
/*   int secret_count        = c->secret_count; */
/*   int share_count         = c->share_count; */
/*   int random_count        = c->random_count; */
/*   int deps_size           = c->deps->deps_size; */
/*   int non_mult_deps_count = deps_size - c->deps->mult_deps->length; */
/*   int total_secrets = many_secrets ? c->deps->mult_deps->length : share_count; */


/*   *secrets = malloc(total_secrets * sizeof(*secrets)); */
/*   for (int i = 0; i < total_secrets; i++) { */
/*     (*secrets)[i] = VarVector_make(); */
/*   } */
/*   *randoms = malloc((secret_count + random_count) * sizeof(*randoms)); */
/*   for (int i = secret_count; i < secret_count + random_count; i++) { */
/*     (*randoms)[i] = VarVector_make(); */
/*   } */

/*   for (int i = 0; i < deps.length; i++) { */
/*     int dep_idx = deps.content[i]; */
/*     Dependency* dep = c->deps->deps[dep_idx]; */

/*     for (int j = secret_count; j < secret_count + random_count; j++) { */
/*       if (dep[j]) { */
/*         VarVector_push((*randoms)[j], dep_idx); */
/*       } */
/*     } */

/*     if (many_secrets) { */
/*       for (int j = non_mult_deps_count; j < deps_size; j++) { */
/*         if (dep[j]) { */
/*           VarVector_push((*secrets)[j-non_mult_deps_count], dep_idx); */
/*         } */
/*       } */
/*     } else { */
/*       for (int k = 0; k < share_count; k++) { */
/*         if (dep[secret_index] & (1 << k)) { */
/*           VarVector_push((*secrets)[k], dep_idx); */
/*         } */
/*       } */
/*     } */
/*   } */

/*   if (verbose) { */
/*     printf("***Size of columns***\n"); */
/*     if (many_secrets) { */
/*       for (int i = 0; i < total_secrets; i++) { */
/*         printf(" - Secret %d: %d\n", i, (*secrets)[i]->length); */
/*       } */
/*     } else { */
/*       for (int i = 0; i < total_secrets; i++) { */
/*         printf(" - Secret %d, share %d: %d\n", i / share_count, i % share_count, */
/*                (*secrets)[i]->length); */
/*       } */
/*     } */
/*     for (int i = secret_count; i < secret_count + random_count; i++) { */
/*       printf(" - Random %d: %d\n", i, (*randoms)[i]->length); */
/*     } */
/*     printf("\n"); */
/*   } */
/* } */



/* /\***************************************************************** */

/*            Circuit splitting into refresh and add gadgets */

/*  *****************************************************************\/ */

/* void split_circuit(const Circuit* circuit, */
/*                    VarVector* D_a, */
/*                    VarVector* D_b, */
/*                    VarVector* D_c) { */
/*   int first_rand_idx = circuit->deps->first_rand_idx; */
/*   int non_mult_deps_count = circuit->deps->deps_size - circuit->deps->mult_deps->length; */

/*   D_a->length = D_b->length = D_c->length = 0; */
/*   D_a->max_size = D_b->max_size = D_c->max_size = circuit->deps->length; */
/*   D_a->content = malloc(D_a->max_size * sizeof(*D_a->content)); */
/*   D_b->content = malloc(D_b->max_size * sizeof(*D_b->content)); */
/*   D_c->content = malloc(D_c->max_size * sizeof(*D_c->content)); */

/*   for (int i = 0; i < circuit->deps->length; i++) { */
/*     Dependency* dep = circuit->deps->deps[i]; */
/*     if (dep[SECRET_A_INDEX]) { */
/*       D_a->content[D_a->length++] = i; */
/*     } else if (dep[SECRET_B_INDEX]) { */
/*       D_b->content[D_b->length++] = i; */
/*     } else { */
/*       int refresh = -1; */
/*       for (int j = first_rand_idx; j < non_mult_deps_count; j++) { */
/*         if (dep[j]) { */
/*           if (!circuit->contains_mults) { */
/*             // TODO: remove */
/*             refresh = 0; */
/*             D_a->content[D_a->length++] = i; */
/*             D_b->content[D_b->length++] = i; */
/*             break; */
/*           } */
/*           if (circuit->i1_rands[j]) { */
/*             refresh = 0; */
/*             D_a->content[D_a->length++] = i; */
/*           } else if (circuit->i2_rands[j]) { */
/*             refresh = 1; */
/*             D_b->content[D_b->length++] = i; */
/*           } else { */
/*             refresh = 2; */
/*             assert(circuit->out_rands[j]); */
/*             // Not adding it to |deps_c| right away */
/*           } */
/*           break; */
/*         } */
/*       } */
/*       if (refresh == 0 || refresh == 1) continue; */

/*       // A multiplication, a xor of multiplications, or an output refresh */
/*       D_c->content[D_c->length++] = i; */
/*     } */
/*   } */

/* } */


/* /\***************************************************************** */

/*       Generation of incompressible tuples of a linear gadget */

/*  *****************************************************************\/ */

/* Tuple* extract_input(const Circuit* G, int secret_idx, VarVector* J, */
/*                      int t, */
/*                      Dependency*** gauss_deps_ret, */
/*                      Dependency** gauss_rands_ret, */
/*                      int* gauss_length) { */
/*   DependencyList* deps = G->deps; */
/*   int deps_size = deps->deps_size; */
/*   int non_mult_deps_count = deps_size - deps->mult_deps->length; */
/*   int first_rand_idx = G->deps->first_rand_idx; */


/*   // Perform gauss elimination on output rands in |J| and factorizes */
/*   // the results. */
/*   Dependency* gauss_deps_o[J->length]; */
/*   for (int i = 0; i < J->length; i++) gauss_deps_o[i] = alloca(deps_size * sizeof(**gauss_deps_o)); */
/*   Dependency gauss_rands_o[J->length]; */
/*   Dependency** gauss_deps_i = malloc(100 * sizeof(*gauss_deps_i)); // Not sure what size to choose here */
/*   for (int i = 0; i < 100; i++) gauss_deps_i[i] = malloc(deps_size * sizeof(**gauss_deps_i)); */
/*   Dependency* gauss_rands_i = malloc(100 * sizeof(*gauss_rands_i)); // (same size as above) */
/*   Dependency** deps1 = secret_idx == SECRET_A_INDEX ? gauss_deps_i : NULL; */
/*   Dependency** deps2 = secret_idx == SECRET_A_INDEX ? NULL : gauss_deps_i; */
/*   int deps1_length = 0; */
/*   int deps2_length = 0; */

/*   for (int i = 0; i < J->length; i++) { */
/*     Dependency dep = J->content[i]; */
/*     apply_gauss(G->deps->deps_size, G->deps->deps[dep], */
/*                 gauss_deps_o, gauss_rands_o, i); */
/*     int first_rand = get_first_rand(gauss_deps_o[i], non_mult_deps_count, */
/*                                     first_rand_idx); */
/*     gauss_rands_o[i] = first_rand; */
/*     if (first_rand == -1) { */
/*       int old_len = secret_idx == SECRET_A_INDEX ? deps1_length : deps2_length; */
/*       Comb comb_0 = 0; */
/*       factorize_mults(G, &gauss_deps_o[i], deps1, deps2, */
/*                       &deps1_length, &deps2_length, */
/*                       &comb_0, 1); */
/*       int new_len = secret_idx == SECRET_A_INDEX ? deps1_length : deps2_length; */
/*       for (int k = old_len; k < new_len; k++) { */
/*         apply_gauss(G->deps->deps_size, gauss_deps_i[k], */
/*                     gauss_deps_i, gauss_rands_i, k); */
/*         int first_rand = get_first_rand(gauss_deps_i[k], non_mult_deps_count, */
/*                                         first_rand_idx); */
/*         gauss_rands_i[k] = first_rand; */
/*       } */
/*     } */
/*   } */

/*   *gauss_deps_ret  = gauss_deps_i; */
/*   *gauss_rands_ret = gauss_rands_i; */
/*   *gauss_length    = secret_idx == SECRET_A_INDEX ? deps1_length : deps2_length; */

/*   Tuple* tuple = Tuple_make_size(t); */
/*   tuple->length = J->length; */
/*   memcpy(tuple->content, J->content, J->length * sizeof(*tuple->content)); */
/*   return tuple; */



/*   /\* // Extract elements related to input |a_or_b| from result of Gauss elimination *\/ */
/*   /\* Tuple* t_ret = make_VarVector(t); *\/ */

/*   /\* for (int i = 0; i < J->length; i++) { *\/ */
/*   /\*   Dependency* dep = deps->deps[J->content[i]]; *\/ */
/*   /\*   // There shouldn't be any dependency set apart from multiplications *\/ */
/*   /\*   for (int j = 0; j < non_mult_deps_count; j++) { *\/ */
/*   /\*     if (dep[j]) { *\/ */
/*   /\*       printf("Dependency %d: [ ", J->content[i]); *\/ */
/*   /\*       for (int k = 0; k < deps_size; k++) printf("%d ", dep[k]); *\/ */
/*   /\*       printf("]\n"); *\/ */
/*   /\*     } *\/ */
/*   /\*     assert(!dep[j]); *\/ */
/*   /\*   } *\/ */

/*   /\*   bool mult_found = false; *\/ */
/*   /\*   for (int j = non_mult_deps_count; j < deps_size; j++) { *\/ */
/*   /\*     if (dep[j]) { *\/ */
/*   /\*       assert(!mult_found); *\/ */
/*   /\*       mult_found = true; *\/ */
/*   /\*       MultDependency* mult_dep = deps->mult_deps->deps[j-non_mult_deps_count]; *\/ */
/*   /\*       int left_is_a = mult_dep->left_ptr[SECRET_A_INDEX]; *\/ */
/*   /\*       if (a_or_b) { *\/ */
/*   /\*         t_ret->content[t_ret->length++] = left_is_a ? mult_dep->left_idx : mult_dep->right_idx; *\/ */
/*   /\*       } else { *\/ */
/*   /\*         t_ret->content[t_ret->length++] = left_is_a ? mult_dep->right_idx : mult_dep->left_idx; *\/ */
/*   /\*       } *\/ */
/*   /\*     } *\/ */
/*   /\*   } *\/ */
/*   /\*   assert(mult_found); *\/ */
/*   /\* } *\/ */

/*   /\* return t_ret; *\/ */
/* } */


/* // Adds the tuple |curr_tuple| to the trie |incompr_tuples|. */
/* void add_tuple_to_trie_v2(Trie* incompr_tuples, Tuple* curr_tuple, */
/*                           const Circuit* c) { */
/*   SecretDep* secret_deps = malloc(1); */
/*   // Note that tuples in the trie are sorted (ie, the variables they */
/*   // contain are in ascending order) -> we might need to sort the */
/*   // current tuple. */
/*   if (is_sorted_comb(curr_tuple->content, curr_tuple->length)) { */
/*     insert_in_trie(incompr_tuples, curr_tuple->content, curr_tuple->length, secret_deps); */
/*   } else { */
/*     Comb sorted_comb[curr_tuple->length]; */
/*     memcpy(sorted_comb, curr_tuple->content, curr_tuple->length * sizeof(*sorted_comb)); */
/*     sort_comb(sorted_comb, curr_tuple->length); */
/*     insert_in_trie(incompr_tuples, sorted_comb, curr_tuple->length, secret_deps); */
/*   } */
/* } */

/* // Checks if |curr_tuple| is incompressible by checking if any of its */
/* // subtuple is incompressible itself (ie, if any of its subtuples is */
/* // in |incompr_tuples|): if any of its subtuple is incompressible, */
/* // then |curr_tuple| is a simple failure, but not an incompressible one. */
/* int tuple_is_not_incompr_v2(Trie* incompr_tuples, Tuple* curr_tuple) { */
/*   if (is_sorted_comb(curr_tuple->content, curr_tuple->length)) { */
/*     return trie_contains_subset(incompr_tuples, curr_tuple->content, curr_tuple->length) ? 1 : 0; */
/*   } else { */
/*     Comb sorted_comb[curr_tuple->length]; */
/*     memcpy(sorted_comb, curr_tuple->content, curr_tuple->length * sizeof(*sorted_comb)); */
/*     sort_comb(sorted_comb, curr_tuple->length); */
/*     return trie_contains_subset(incompr_tuples, sorted_comb, curr_tuple->length) ? 1 : 0; */
/*   } */
/* } */

/* int is_failure(Dependency** gauss_deps, Dependency* gauss_rands, int length, */
/*                int I, int non_mult_deps_count, int deps_size, */
/*                int input_type, int secret_index) { */
/*   int shares_scalar = 0; */
/*   char shares_vector[deps_size]; */
/*   memset(shares_vector, 0, deps_size * sizeof(*shares_vector)); */
/*   for (int i = 0; i < length; i++) { */
/*     if (gauss_rands[i] != -1) { */
/*       /\* printf("  + Skipping dep %d cause contains random.\n", i); *\/ */
/*       continue; */
/*     } */
/*     if (input_type == 1) { */
/*       /\* printf("  + Dep %d contains secret shares: %d\n", i, gauss_deps[i][secret_index]); *\/ */
/*       shares_scalar |= gauss_deps[i][secret_index]; */
/*     } else { */
/*       for (int j = non_mult_deps_count; j < deps_size; j++) { */
/*         shares_vector[j] |= gauss_deps[i][j]; */
/*       } */
/*     } */
/*   } */
/*   if (input_type == 1) { */
/*     /\* printf("Available shares: %d.\n", shares_scalar); *\/ */
/*     return __builtin_popcount(shares_scalar) >= I; */
/*   } else { */
/*     int count = 0; */
/*     for (int i = non_mult_deps_count; i < deps_size; i++) count += shares_vector[i]; */
/*     return count >= I; */
/*   } */
/* } */

/* void randoms_step_v2(const Circuit* G, Trie* incompr, int I, */
/*                      VarVector** randoms, int size_max, */
/*                      Dependency** gauss_deps, Dependency* gauss_rands, */
/*                      int gauss_length, */
/*                      Tuple* curr_tuple, int input_type, int secret_index, */
/*                      int unmask_idx) { */
/*   int deps_size = G->deps->deps_size; */
/*   int non_mult_deps_count = deps_size - G->deps->mult_deps->length; */
/*   int first_rand_idx = G->deps->first_rand_idx; */

/*   /\* if (input_type == 1) { *\/ */
/*   /\*   printf(" + randoms_step_v2: unmask_idx=%d  size_max=%d\n", unmask_idx, size_max); *\/ */
/*   /\*   printf("   Tuple = [ "); *\/ */
/*   /\*   for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]); *\/ */
/*   /\*   printf("]\n"); *\/ */

/*   /\*   printf("  After gauss elimination:\n"); *\/ */
/*   /\*   for (int i = 0; i < gauss_length; i++) { *\/ */
/*   /\*     printf("   [ "); *\/ */
/*   /\*     for (int j = 0; j < G->deps->deps_size; j++) { *\/ */
/*   /\*       printf("%d ", gauss_deps[i][j]); *\/ */
/*   /\*     } *\/ */
/*   /\*     printf("]  --  gauss_rand=%d\n", gauss_rands[i]); *\/ */
/*   /\*   } *\/ */
/*   /\* } *\/ */

/*   if (is_failure(gauss_deps, gauss_rands, gauss_length, I, */
/*                  non_mult_deps_count, deps_size, input_type, secret_index)) { */
/*     if (curr_tuple->length != size_max) return; */
/*     if (tuple_is_not_incompr_v2(incompr, curr_tuple)) return; */
/*     /\* if (input_type == 1) { *\/ */
/*     /\*   printf("Found incompr: [ "); *\/ */
/*     /\*   for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]); *\/ */
/*     /\*   printf("]\n"); *\/ */
/*     /\* } *\/ */
/*     add_tuple_to_trie_v2(incompr, curr_tuple, G); */
/*   } */

/*   if (unmask_idx == gauss_length || curr_tuple->length == size_max) { */
/*     /\* if (input_type == 1) { *\/ */
/*     /\*   if (unmask_idx == gauss_length) printf("    * Stopping because unmask_idx=length.\n"); *\/ */
/*     /\*   else printf("    * Stopping because length=size_max.\n"); *\/ */
/*     /\* } *\/ */
/*     return; */
/*   } */

/*   randoms_step_v2(G, incompr, I, randoms, size_max, gauss_deps, gauss_rands, gauss_length, */
/*                   curr_tuple, input_type, secret_index, unmask_idx+1); */

/*   int curr_size = curr_tuple->length; */
/*   int rand_idx  = gauss_rands[unmask_idx]; */

/*   if (rand_idx == -1) { */
/*     /\* if (input_type == 1) { *\/ */
/*     /\*   printf("    * Stopping because rand_idx[%d] == -1\n", unmask_idx); *\/ */
/*     /\* } *\/ */
/*     return; */
/*   } */

/*   VarVector* dep_array = randoms[rand_idx]; */
/*   for (int j = 0; j < dep_array->length; j++) { */
/*     Var dep = dep_array->content[j]; */
/*     if (VarVector_contains(curr_tuple, dep)) continue; */

/*     /\* if (input_type == 1) { *\/ */
/*     /\*   printf("  About to add dep %d: [ ", dep); *\/ */
/*     /\*   for (int i = 0; i < G->deps->deps_size; i++) printf("%d ", G->deps->deps[dep][i]); *\/ */
/*     /\*   printf("]\n"); *\/ */
/*     /\* } *\/ */

/*     int gauss_idx = gauss_length++; */
/*     curr_tuple->content[curr_size] = dep; */
/*     apply_gauss(G->deps->deps_size, G->deps->deps[dep], */
/*                 gauss_deps, gauss_rands, gauss_idx); */

/*     int first_rand = get_first_rand(gauss_deps[gauss_idx], non_mult_deps_count, */
/*                                     first_rand_idx); */
/*     gauss_rands[gauss_idx] = first_rand; */
/*     curr_tuple->length++; */
/*     randoms_step_v2(G, incompr, I, randoms, size_max, gauss_deps, gauss_rands, gauss_length, */
/*                     curr_tuple, input_type, secret_index, unmask_idx+1); */
/*     curr_tuple->length--; */
/*     gauss_length--; */
/*   } */
/* } */

/* void initial_gauss_elimination_v2(const Circuit* G, Trie* incompr, int I, */
/*                                   VarVector **randoms, */
/*                                   Dependency** gauss_deps, */
/*                                   Dependency* gauss_rands, */
/*                                   int gauss_length, */
/*                                   int base_tuple_length, */
/*                                   int size_max, */
/*                                   Tuple* curr_tuple, int input_type, */
/*                                   int secret_index) { */
/*   int deps_size = G->deps->deps_size; */
/*   int non_mult_deps_count = deps_size - G->deps->mult_deps->length; */
/*   DependencyList* deps = G->deps; */

/*   for (int i = base_tuple_length; i < curr_tuple->length; i++) { */
/*     int gauss_idx = gauss_length++; */
/*     Dependency* real_dep = deps->deps[curr_tuple->content[i]]; */
/*     apply_gauss(deps->deps_size, real_dep, gauss_deps, gauss_rands, gauss_idx); */
/*     gauss_rands[gauss_idx] = get_first_rand(gauss_deps[gauss_idx], */
/*                                             non_mult_deps_count, G->deps->first_rand_idx); */
/*   } */

/*   randoms_step_v2(G, incompr, I, randoms, size_max, gauss_deps, gauss_rands, gauss_length, */
/*                   curr_tuple, input_type, secret_index, 0); */
/* } */

/* void secrets_step_v2(const Circuit* G, Trie* incompr, int I, */
/*                      VarVector **secrets, VarVector **randoms, */
/*                      Dependency** gauss_deps, */
/*                      Dependency* gauss_rands, */
/*                      int gauss_length, */
/*                      int base_tuple_length, */
/*                      int size_max, */
/*                      Tuple* curr_tuple, int input_type, */
/*                      int secret_index, */
/*                      int input_index, // Index of the column of |secrets| to take the */
/*                                       // next input from */
/*                      int required_inputs_count, // Number of remaining inputs required */
/*                                                 // before moving on to the random step */
/*                      int input_count // Total number of columns in |secrets| */
/*                      ) { */
/*   int deps_size = G->deps->deps_size; */
/*   int non_mult_deps_count = deps_size - G->deps->mult_deps->length; */

/*   /\* if (input_type == 1) { *\/ */
/*   /\*   printf("secrets_step_v2: tuple = [ "); *\/ */
/*   /\*   for (int i = 0; i < curr_tuple->length; i++) printf("%d ", curr_tuple->content[i]); *\/ */
/*   /\*   printf("]\n"); *\/ */
/*   /\* } *\/ */

/*   assert(required_inputs_count <= input_count); */
/*   //assert(required_inputs_count + curr_tuple->length <= size_max); // Not sure if this assert needs to hold... */
/*     initial_gauss_elimination_v2(G, incompr, I, randoms, */
/*                                  gauss_deps, gauss_rands, gauss_length, base_tuple_length, */
/*                                  size_max, curr_tuple, input_type, secret_index); */
/*   if (required_inputs_count == 0 || input_index >= input_count || curr_tuple->length == size_max) { */
/*     return; */
/*   } else { */
/*     int remaining_inputs = input_count - input_index; */
/*     /\* if (input_type == 1) { *\/ */
/*     /\*   printf("  length=%d  --  required_inputs_count=%d  --  remaining_inputs=%d\n", *\/ */
/*     /\*          curr_tuple->length, required_inputs_count, remaining_inputs); *\/ */
/*     /\*   printf("  Trying without this input as well.\n"); *\/ */
/*     /\* } *\/ */
/*     /\* if (required_inputs_count < remaining_inputs) { *\/ */
/*       secrets_step_v2(G, incompr, I, secrets, randoms, */
/*                       gauss_deps, gauss_rands, gauss_length, base_tuple_length, */
/*                       size_max, curr_tuple, input_type, */
/*                       secret_index, input_index+1, required_inputs_count, input_count); */
/*     /\* } *\/ */
/*     VarVector* dep_array = secrets[input_index]; */
/*     int tuple_idx = curr_tuple->length; */
/*     for (int i = 0; i < dep_array->length; i++) { */
/*       Var dep = dep_array->content[i]; */
/*       if (VarVector_contains(curr_tuple, dep)) { */
/*         secrets_step_v2(G, incompr, I, secrets, randoms, */
/*                         gauss_deps, gauss_rands, gauss_length, base_tuple_length, */
/*                         size_max, curr_tuple, input_type, */
/*                         secret_index, input_index+1, required_inputs_count-1, input_count); */
/*       } else { */
/*         curr_tuple->content[tuple_idx] = dep; */
/*         curr_tuple->length++; */
/*         secrets_step_v2(G, incompr, I, secrets, randoms, */
/*                         gauss_deps, gauss_rands, gauss_length, base_tuple_length, */
/*                         size_max, curr_tuple, input_type, */
/*                         secret_index, input_index+1, required_inputs_count-1, input_count); */
/*         curr_tuple->length--; */
/*       } */
/*     } */
/*   } */
/* } */


/* Trie* LinVerif(const Circuit* G, */
/*                VarVector **secrets, */
/*                VarVector **randoms, */
/*                int input_type, // 1 = standart linear gadget */
/*                                // 2 = multiplications are inputs */
/*                int secret_index, // only for input_type == 1 */
/*                int I,            // Number of input shares to leak */
/*                VarVector* J,      // Additional inputs that are available */
/*                int t,            // Size of the tuples to generate */
/*                Trie* return_trie // If not NULL, write incompressible tuples in this trie */
/*                                  // rather than creating a new one */
/*                ) { */
/*   Dependency** gauss_deps; */
/*   Dependency* gauss_rands; */
/*   int gauss_length = 0; */
/*   // Init tuple: J */
/*   Tuple* init; */
/*   if (input_type == 1 && J != &empty_array) { */
/*     init = extract_input(G, secret_index, J, t, */
/*                          &gauss_deps, &gauss_rands, &gauss_length); */
/*   } else { */
/*     init = VarVector_make_size(t); */
/*     gauss_deps = alloca(100 * sizeof(*gauss_deps));   // Not sure what size to use */
/*     for (int i = 0; i < 100; i++) gauss_deps[i] = alloca(100 * sizeof(**gauss_deps)); */
/*     gauss_rands = alloca(100 * sizeof(*gauss_rands)); // (same size as above) */
/*   } */

/*   /\* if (input_type == 1) { *\/ */
/*   /\*   printf("Initial gauss eliminated:\n"); *\/ */
/*   /\*   for (int i = 0; i < gauss_length; i++) { *\/ */
/*   /\*     printf("  [ "); *\/ */
/*   /\*     for (int j = 0; j < G->deps->deps_size; j++) printf("%d ", gauss_deps[i][j]); *\/ */
/*   /\*     printf("]  -- gauss_rand=%d\n", gauss_rands[i]); *\/ */
/*   /\*   } *\/ */
/*   /\* } *\/ */

/*   /\* printf("LinVerif: I=%d, t=%d.\n", I, t); *\/ */

/*   int total_input_count = input_type == 1 ? G->share_count : G->deps->mult_deps->length; */

/*   Trie* incompr = return_trie ? return_trie : make_trie(G->deps->length); */

/*   if (I > total_input_count) { */
/*     printf("Cannot reveal %d inputs: gadget has only %d inputs.\n", I, total_input_count); */
/*     return incompr; */
/*   } */

/*   secrets_step_v2(G, incompr, I, secrets, randoms, */
/*                   gauss_deps, gauss_rands, gauss_length, */
/*                   init->length, t, init, input_type, */
/*                   secret_index, 0, I, total_input_count); */

/*   /\* printf("\nTotal incompr: %d\n", trie_size(incompr)); *\/ */
/*   /\* for (int size = 2; size < 100; size++) { *\/ */
/*   /\*   int tot = trie_tuples_size(incompr, size); *\/ */
/*   /\*   printf("Incompr of size %d: %d\n", size, tot); *\/ */
/*   /\*   if (tot == 0) break; *\/ */
/*   /\* } *\/ */
/*   /\* printf("\n"); *\/ */

/*   if (input_type == 1 && J != &empty_array) { */
/*     for (int i = 0; i < 100; i++) free(gauss_deps[i]); */
/*     free(gauss_deps); */
/*     free(gauss_rands); */
/*   } */

/*   return incompr; */
/* } */


/* /\***************************************************************** */

/*   Generation of incompressible tuples of a multiplication gadget */

/*  *****************************************************************\/ */

/* Trie* MultVerif(const Circuit* G, */
/*                 int I, */
/*                 VarVector* L, */
/*                 int tmin, int tmax) { */
/*   // Split G into Ga, Gb and Gc */
/*   VarVector D_a, D_b, D_c; */
/*   split_circuit(G, &D_a, &D_b, &D_c); */

/* #define PRINT_DEPS(_name) {                                     \ */
/*     printf(#_name":\n");                                        \ */
/*     for (int i = 0; i < D_##_name.length; i++) {                \ */
/*       printf("  [ ");                                           \ */
/*       for (int j = 0; j < G->deps->deps_size; j++) {            \ */
/*         printf("%d ", G->deps->deps[D_##_name.content[i]][j]);  \ */
/*       }                                                         \ */
/*       printf("]\n");                                            \ */
/*     }                                                           \ */
/*     printf("\n\n");                                       \ */
/*   } */
/*   PRINT_DEPS(a); */
/*   PRINT_DEPS(b); */
/*   PRINT_DEPS(c); */


/*   VarVector **secrets_a, **randoms_a, **secrets_b, **randoms_b, **secrets_c, **randoms_c; */
/*   build_dependency_arrays_v2(G, D_a, &secrets_a, &randoms_a, false, SECRET_A_INDEX, 1); */
/*   build_dependency_arrays_v2(G, D_b, &secrets_b, &randoms_b, false, SECRET_B_INDEX, 1); */
/*   build_dependency_arrays_v2(G, D_c, &secrets_c, &randoms_c, true, -1, 1); */

/*   int gadget_c_inputs_count = G->deps->mult_deps->length; */

/*   /\* printf("\n############################ MultVerif step 1 ############################\n\n"); *\/ */

/*   Trie* incompr_C[gadget_c_inputs_count]; */
/*   for (int i = 0; i < gadget_c_inputs_count; i++) { */
/*     incompr_C[i] = make_trie(G->deps->length); */
/*   } */
/*   for (int t = tmin; t <= tmax; t++) { */
/*     /\* printf("\n\n********************************************\nMultVerif step 1: t=%d\n", t); *\/ */
/*     for (int I_c = 1; I_c < gadget_c_inputs_count; I_c++) { */
/*       //int old_count = trie_size(incompr_C[I_c]); */
/*       LinVerif(G, secrets_c, randoms_c, 2, -1, I_c, L, t, incompr_C[I_c]); */
/*       /\* printf("Incompressible tuples in C of size %d revealing %d inputs: %d.\n", *\/ */
/*       /\*        t, I_c, trie_size(incompr_C[I_c]) - old_count); *\/ */
/*     } */
/*   } */

/*   /\* printf("\n\n\n\n############################ MultVerif step 2 ############################\n\n"); *\/ */

/*   Trie* real_incompr = make_trie(G->deps->length); */
/*   for (int t = tmin; t <= tmax; t++) { */
/*     /\* printf("\n\n********************************************\nMultVerif step 2: t=%d\n", t); *\/ */

/*     int old_count = trie_size(real_incompr); */
/*     LinVerif(G, secrets_a, randoms_a, 1, SECRET_A_INDEX, I, &empty_array, t, real_incompr); */
/*     LinVerif(G, secrets_b, randoms_b, 1, SECRET_B_INDEX, I, &empty_array, t, real_incompr); */
/*     /\* printf("Real incompr of size %d without outputs: %d.\n", *\/ */
/*     /\*        t, trie_size(real_incompr) - old_count); *\/ */

/*     for (int t_c = tmin; t_c <= t; t_c++) { */
/*       int old_count = trie_size(real_incompr); */
/*       for (int I_c = 1; I_c < gadget_c_inputs_count; I_c++) { */
/*         ListComb* incomprs = list_from_trie(incompr_C[I_c], t_c); */
/*         ListCombElem* e = incomprs->head; */
/*         while (e) { */
/*           VarVector* J = VarVector_from_array(e->comb, t_c, t); */
/*           /\* if (t != 4 || t_c != 1 || J->content[0] != 43) { *\/ */
/*           /\*   e = e->next; *\/ */
/*           /\*   continue; *\/ */
/*           /\* } *\/ */
/*           /\* printf("Here we are.\n"); *\/ */
/*           LinVerif(G, secrets_a, randoms_a, 1, SECRET_A_INDEX, I, J, t, real_incompr); */
/*           LinVerif(G, secrets_b, randoms_b, 1, SECRET_B_INDEX, I, J, t, real_incompr); */
/*           e = e->next; */
/*         } */
/*         // TODO: free |incomprs| */
/*       } */
/*       /\* printf("Real incompr of size %d with %d outputs: %d.\n", *\/ */
/*       /\*        t, t_c, trie_size(real_incompr) - old_count); *\/ */
/*     } */
/*     /\* printf("Real incompr of size %d: %d.\n", t, trie_tuples_size(real_incompr, t)); *\/ */
/*   } */


/*   return real_incompr; */
/* } */


/* // Just for testing purposes */
/* void constructive_v2_for_testing(const Circuit* G) { */
/*   // Split G into Ga, Gb and Gc */
/*   VarVector D_a, D_b, D_c; */
/*   split_circuit(G, &D_a, &D_b, &D_c); */

/*   PRINT_DEPS(a); */

/*   VarVector **secrets_a, **randoms_a; */
/*   build_dependency_arrays_v2(G, D_a, &secrets_a, &randoms_a, false, SECRET_A_INDEX, 1); */

/*   Trie* incompr = make_trie(G->deps->length); */
/*   for (int t = 1; t < 5; t++) { */
/*     LinVerif(G, secrets_a, randoms_a, 1, SECRET_A_INDEX, 3, &empty_array, t, incompr); */
/*     printf("Incompr of size %d: %d.\n\n", t, trie_tuples_size(incompr, t)); */
/*     if (t == 4) print_all_tuples_size(incompr, 4); */
/*   } */

/*   //print_all_tuples_size(incompr, 4); */
/* } */


/* void constructive_v2(const Circuit* G, int coeff_max, int verbose) { */
/*   int I = G->share_count; */
/*   VarVector* L = &empty_array; */

/*   Trie* incompr = MultVerif(G, I, L, 1, 6); */

/*   printf("\nTotal incompr: %d\n", trie_size(incompr)); */

/*   for (int size = 1; size < 100; size++) { */
/*     int tot = trie_tuples_size(incompr, size); */
/*     printf("Incompr of size %d: %d\n", size, tot); */
/*     if (size > G->share_count && tot == 0) break; */
/*   } */

/*   /\* print_all_tuples_size(incompr, 3); *\/ */
/*   /\* print_all_tuples_size(incompr, 4); *\/ */

/* } */

#include <stdlib.h>
#include <stdio.h>
#include "circuit.h"
void constructive_v2(const Circuit* G, int coeff_max, int verbose) {
  (void)G;
  (void)coeff_max;
  (void)verbose;
  fprintf(stderr, "Compositional verification of multiplications not available. Exiting.\n");
  exit(EXIT_FAILURE);
}
