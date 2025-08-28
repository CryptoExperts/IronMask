#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "circuit.h"
#include "coeffs.h"
#include "combinations.h"
#include "constructive_arith.h"
#include "vectors.h"

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) > (b) ? (b) : (a))

/*                             Parallelisation Stuff                          */

struct env_tout_cRPC_args {
  Circuit* c;
  int share_count; 
  uint64_t ***final_env; 
  Tuple *output_tuple;  
  int coeff_max; 
  int tout;
  int gauss_length;
  int new_revealed_secret;
  pthread_mutex_t *mutex;
};

void *start_thread_env_tout_cRPC(void *void_args);



void gaussian_transformation(Circuit *c, int index, Dependency **gauss_deps, 
                             Dependency *gauss_rands, int gauss_length, bool debug){
      /*
      if(debug){
        printf("Before : \n");
        for(int i = 0; i < gauss_length; i++){
          printf("[");
          for (int j = 0; j < c->deps->deps_size; j++){
            printf("%d ", gauss_deps[i][j]);
          }
          printf("]\n");
        }
        printf("Gauss Rands : \n");
        for (int i = 0; i < gauss_length; i++){
          printf("[%d]\n", gauss_rands[i]);
        }
        printf("\n");
      }*/


      int new_gauss_length = gauss_length;
      const DepArrVector* dep_arr = c->deps->deps[index];
      for (int dep_idx = 0; dep_idx < dep_arr->length; dep_idx++) {
        apply_gauss_arith(c->deps->deps_size, dep_arr->content[dep_idx],
                          gauss_deps, gauss_rands, new_gauss_length++,
                          c->characteristic);
      }
      gauss_rands[gauss_length] = get_first_rand_arith(gauss_deps[gauss_length], 
                                                       c->deps->deps_size, 
                                                       c->deps->first_rand_idx);
      /*
      if(debug){
        printf("After : \n");                  
        for(int i = 0; i < new_gauss_length; i++){
          printf("[");
          for (int j = 0; j < c->deps->deps_size; j++){
            printf("%d ", gauss_deps[i][j]);
          }
          printf("]\n");
        }
        printf("Gauss Rands : \n");
        for (int i = 0; i < new_gauss_length; i++){
          printf("[%d]\n", gauss_rands[i]);
        }
        
        printf("\n");
      }*/
  
}


int compute_revealed_secret(Circuit *c, Tuple *curr_tuple, 
                            Dependency** gauss_deps, int revealed_secret, 
                            int output_len, int debug){
  int index = curr_tuple->length - 1 + output_len;
  int first_rand = get_first_rand_arith(gauss_deps[index], c->deps->deps_size,
                                        c->deps->first_rand_idx);
  
  //if(debug){
  //  printf("index = %d\n", index);
  //  printf("first_rand = %d\n", first_rand);
  //}                                      
   
  if (!first_rand){
    for (int i = 0; i < c->share_count; i++){
      if (gauss_deps[index][i])
        revealed_secret |= 1 << i;
    }
  }
  
  return revealed_secret;  
}

int output_count_tuple(Tuple *curr_tuple, int first_index_output){
  int output_count = 0;
  for (int i = 0; i < curr_tuple->length; i++){
    if (curr_tuple->content[i] >= first_index_output)
      output_count += 1;
  }
  return output_count;
}

void compute_coeffs_tuple(Circuit *c, Tuple *curr_tuple, uint64_t *coeffs){
  Comb comb[curr_tuple->length];
  memcpy(comb, curr_tuple->content, curr_tuple->length * sizeof(*comb));
  update_coeff_c_single(c, coeffs, comb, curr_tuple->length);
}


void update_env_cRPC (Circuit *c, Tuple *curr_tuple, int start_index, 
                      Dependency** gauss_deps, Dependency* gauss_rands,
                      int gauss_length, int share_count, 
                      uint64_t *env[][share_count + 1], int revealed_secret, 
                      int coeff_max, int tout){
  
  if (curr_tuple->length == coeff_max)
    return;
  
  bool debug = false;  
  for (int i = start_index; i < c->deps->length - c->share_count; i++){
    Tuple_push(curr_tuple, i);
    
    if (revealed_secret == ((1 << c->share_count) - 1)){
      int tin = c->share_count;
      compute_coeffs_tuple(c, curr_tuple, env[tin][tout]);
      update_env_cRPC (c, curr_tuple, start_index + 1, gauss_deps, gauss_rands, 
                       gauss_length, share_count, env, revealed_secret, 
                       coeff_max, tout);


    }
    else {
      gaussian_transformation(c, i, gauss_deps, gauss_rands, gauss_length, 
                              debug);
    
      int new_gauss_length = gauss_length + 1;
      int new_revealed_secret = compute_revealed_secret(c, curr_tuple, 
                                gauss_deps, revealed_secret, tout, debug);
    
      int tin = __builtin_popcount(new_revealed_secret);
    
      //if (tout == 1 && tin == 1){
      //  printf("[");
      //  for (int l = 0; l < curr_tuple->length; l++){
      //    printf(" %d ", curr_tuple->content[l]);
      //  }
      //  printf("]\n");
      //}  

      compute_coeffs_tuple(c, curr_tuple, env[tin][tout]);
      
      //if(tout == 1 && tin == 1){
      //  printf("env[tin][tout][2] = %d\n", env[tin][tout][2]);
      //}

      update_env_cRPC (c, curr_tuple, start_index + 1, gauss_deps, gauss_rands, 
                       new_gauss_length, share_count, env, new_revealed_secret, 
                       coeff_max, tout);
    }

    /*gaussian_transformation(c, i, gauss_deps, gauss_rands, gauss_length, 
                              debug);
    
    int new_gauss_length = gauss_length + 1;
    int new_revealed_secret = compute_revealed_secret(c, curr_tuple, 
                                gauss_deps, revealed_secret, tout, debug);
    
    int tin = __builtin_popcount(new_revealed_secret);
    
    compute_coeffs_tuple(c, curr_tuple, env[tin][tout]);
    update_env_cRPC (c, curr_tuple, start_index + 1, gauss_deps, gauss_rands, 
                       new_gauss_length, share_count, env, new_revealed_secret, 
                       coeff_max, tout);
    */ 


    start_index += 1;
    Tuple_pop(curr_tuple);
  }
}

void print_coeffs_env (Circuit * c, uint64_t ***env){  
  for (int tin = 0; tin < c->share_count + 1; tin++){
    for(int tout = 0; tout < c->share_count + 1; tout++){
      printf("tin = %d, tout = %d\n f(p) = [", tin, tout);
      for (int i = 0; i < c->total_wires; i++){
        printf(" %lu,", env[tin][tout][i]);
      }
      printf(" %lu]\n\n", env[tin][tout][c->total_wires]);
    }
  }
}
  
bool update_output_tuple(Tuple *output_tuple, int *start_output_index, 
                         int *end_output_index, int tout){  
 if(*start_output_index > *end_output_index && output_tuple->length > 0){
    Tuple_pop(output_tuple);
    *start_output_index = output_tuple->content[output_tuple->length] + 1;
    return update_output_tuple(output_tuple, start_output_index, 
                               end_output_index, tout);
 }
 
 if(*start_output_index > *end_output_index){
   return false;
 }
 
 Tuple_push(output_tuple, *start_output_index);
 *start_output_index += 1;
 if (output_tuple->length == tout)
   return true;
 else 
   return update_output_tuple(output_tuple, start_output_index, 
                              end_output_index, tout);
}


void max_coeffs(int tin, int tout, int share_count, uint64_t ***final_env, 
                uint64_t *env[][share_count + 1], int max_coeff){
  
  //Debug
  //if(tout == 1 && tin == 1) {
  //  printf("tin = %d\n", tin);
  //  printf("tout = %d\nf(p) = [", tout);
  //  for (int i = 0; i < max_coeff; i++){
  //    printf( "%d, ", env[tin][tout][i]);
  //  }
  //  printf("]\n\n");
  //}

  //for (int i = 0; i <= max_coeff; i++)    
    //final_env[tin][tout][i] = max(final_env[tin][tout][i], env[tin][tout][i]);
  bool to_change = false;
  for (int i = 0; i <= max_coeff; i++){
    if (final_env[tin][tout][i] == env[tin][tout][i])
      continue;
    if (!final_env[tin][tout][i] && env[tin][tout][i]){
      to_change = true;
      break;
    }
    if (final_env[tin][tout][i] && !env[tin][tout][i])
      break;
    
    to_change = final_env[tin][tout][i]  < env[tin][tout][i];
    break;  
  }
  
  if (to_change){
    for (int i = 0; i <= max_coeff; i++){
      final_env[tin][tout][i] = env[tin][tout][i];
    }
  }  
}

void env_tout_cRPC(Circuit *c, int share_count, 
                  uint64_t ***final_env, 
                  uint64_t *env[][share_count + 1], Dependency **gauss_deps, 
                  Dependency *gauss_rands, Tuple *curr_tuple, int coeff_max, 
                   int tout, int cores){
  
  if (tout == 0){    
    //First Step - Cardinal RPC enveloppes with no outputs leaked.
    //Adding the empty tuples to the enveloppes that leaks 0 input shares.
    int revealed_secret = 0;
    compute_coeffs_tuple(c, curr_tuple, env[0][0]);  

    //Update the cardinal RPC enveloppes for tout = 0.
    update_env_cRPC (c, curr_tuple, 0, gauss_deps, gauss_rands, 0, 
                     share_count, env, revealed_secret, coeff_max, 0);

    // Adding it in the final enveloppes.                 
    for (int tin = 0; tin < share_count + 1; tin++)        
      max_coeffs(tin, 0, share_count, final_env, env, coeff_max);
    }
  else {
    //Compute all the output set of size |tout|.
    int max_deps_length = c->deps->length + 1;
    int revealed_secret = 0;
    int output_combinations = n_choose_k(tout, share_count);
    Tuple *output_set[output_combinations];
    
    Tuple* output_tuple = Tuple_make_size(max_deps_length);
    int start_output_index = c->deps->length - share_count;
    int end_output_index = c->deps->length - 1;
    for (int i = 0; i < output_combinations; i++){
      output_set[i] = Tuple_make_size(tout);

      update_output_tuple(output_tuple, &start_output_index, &end_output_index, 
                          tout);
      
      for(int j = 0; j < output_tuple->length; j++)
        Tuple_push(output_set[i], output_tuple->content[j]);    
    }
    Tuple_free(output_tuple);
    
    //For each output set of size |tout|, compute the dependency associated with
    //tuples of probes of intermediate variables.
    for (int i = 0; i < output_combinations; i++){
      int gauss_length = 0;
      int new_revealed_secret = revealed_secret;
      
      //Compute the dependency of the Output Set, that we add at the top of 
      //|gauss_deps| and |gauss_rands|. 
      for (int j = 0; j < tout; j++){
        gaussian_transformation(c, output_set[i]->content[j], gauss_deps, 
                                gauss_rands, gauss_length, false);
        new_revealed_secret = compute_revealed_secret(c, output_set[i], 
                              gauss_deps, new_revealed_secret, j - tout + 1, 
                              false);
        gauss_length++;
      }
      env[__builtin_popcount(new_revealed_secret)][tout][0] = 1;
      update_env_cRPC (c, curr_tuple, 0, gauss_deps, gauss_rands, gauss_length, 
                       share_count, env, new_revealed_secret, coeff_max, tout);
      
      for (int tin = 0; tin < share_count + 1; tin++){        
        max_coeffs(tin, tout, share_count, final_env, env, coeff_max);
        
        //Redefine to 0 to give good results for the next output set.
        for(int l = 0; l < c->total_wires + 1; l++)
          env[tin][tout][l] = 0;
      }
      
      //Free Output Set
      Tuple_free(output_set[i]);
    }
  }
}

void env_tout_cRPC_parallel(Circuit *c, int share_count, uint64_t ***final_env, 
                            uint64_t *env[][share_count + 1], 
                            Dependency **gauss_deps, Dependency *gauss_rands, 
                            Tuple *curr_tuple, int coeff_max, int tout, 
                            int cores, int *threads_used, pthread_t *threads, 
                            pthread_mutex_t *mutex){
  
  if (tout == 0){    

          struct env_tout_cRPC_args *args = malloc(sizeof(*args));
          args->c = c;
          args->coeff_max = coeff_max;
          args->final_env = final_env;
          args->gauss_length = 0;
          args->mutex = mutex;
          args->new_revealed_secret = 0;
          args->output_tuple = NULL;
          args->share_count = share_count;
          args->tout = tout;

          pthread_mutex_lock(mutex);
          int ret = pthread_create(&threads[*threads_used], NULL, 
                                 start_thread_env_tout_cRPC, args);
          pthread_mutex_unlock(mutex);
          *threads_used += 1;
          if (ret)
            free(args);
  }
  else {
    //Compute all the output set of size |tout|.
    int max_deps_length = c->deps->length + 1;
    int revealed_secret = 0;
    int output_combinations = n_choose_k(tout, share_count);
    Tuple *output_set[output_combinations];
    
    Tuple* output_tuple = Tuple_make_size(max_deps_length);
    int start_output_index = c->deps->length - share_count;
    int end_output_index = c->deps->length - 1;
    for (int i = 0; i < output_combinations; i++){
      output_set[i] = Tuple_make_size(tout);

      update_output_tuple(output_tuple, &start_output_index, &end_output_index, 
                          tout);
      
      for(int j = 0; j < output_tuple->length; j++)
        Tuple_push(output_set[i], output_tuple->content[j]);    
    }
    Tuple_free(output_tuple);
    
    /*printf("tout = %d\n\n", tout);
    for (int i = 0; i < output_combinations; i++){
      printf("Output Tuple [%d] = [", i);
      for (int j = 0; j < output_set[i]->length; j++){
        printf(" %d ", output_set[i]->content[j]);
      }
      printf("]\n");
    }*/

    for (int i = 0; i < output_combinations; i++){

      struct env_tout_cRPC_args *args = malloc(sizeof(*args));
      args->c = c;
      args->coeff_max = coeff_max;
      args->final_env = final_env;
      args->gauss_length = 0;
      args->mutex = mutex;
      args->new_revealed_secret = revealed_secret;
      args->output_tuple = output_set[i];
      args->share_count = share_count;
      args->tout = tout;

      pthread_mutex_lock(mutex);
      int ret = pthread_create(&threads[*threads_used], NULL, 
                                 start_thread_env_tout_cRPC, args);
      pthread_mutex_unlock(mutex);
      *threads_used += 1;
      if (ret)
        free(args);

      if (*threads_used == cores){
        for (int j = 0; j < cores; j++){
          pthread_join(threads[j], NULL);
        }
        *threads_used = 0;
      }
    }
  }
}


//For now, only operationnal for a refresh gadget.
void env_cRPC(Circuit *c, int coeff_max, int cores){

  if (coeff_max == -1)
    coeff_max = c->total_wires;
  coeff_max = min(coeff_max, c->total_wires);
  
  int max_deps_length = c->deps->length + 1;
  
  //Initialisation Stuff
  Tuple* curr_tuple = Tuple_make_size(max_deps_length);
  int share_count = c->share_count;
  
  uint64_t *env[share_count + 1][share_count + 1];
  for (int tin = 0; tin < share_count + 1; tin++){
    for (int tout = 0; tout < share_count + 1; tout++){
      env[tin][tout] = calloc((c->total_wires + 1), sizeof(*env[tin][tout]));
    }
  }
  
  uint64_t ***final_env = malloc((share_count + 1) * sizeof(*final_env));
  for (int tin = 0; tin < share_count + 1; tin++){
    final_env[tin] = malloc((share_count + 1) * sizeof(**final_env));
    for (int tout = 0; tout < share_count + 1; tout++){
      final_env[tin][tout] = calloc((c->total_wires + 1), 
                                     sizeof(*final_env[tin][tout]));
    }
  }
  
  Dependency** gauss_deps = malloc(max_deps_length * sizeof(*gauss_deps));
  for (int i = 0; i < max_deps_length; i++) {
    gauss_deps[i] = malloc(c->deps->deps_size * sizeof(*gauss_deps[i]));
  }
  Dependency* gauss_rands = malloc(max_deps_length * sizeof(*gauss_rands));

  if (cores <= 1){
    env_tout_cRPC(c, share_count, final_env, env, gauss_deps, gauss_rands, 
                  curr_tuple, coeff_max, 0, cores);
  
  
    for (int tout = 1; tout < share_count + 1; tout++){
      env_tout_cRPC(c, share_count, final_env, env, gauss_deps, gauss_rands, 
                    curr_tuple, coeff_max, tout, cores);
    }
  }
  else {
    int threads_used = 0;
    pthread_t *threads = malloc(cores * sizeof(*threads));
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    env_tout_cRPC_parallel(c, share_count, final_env, env, gauss_deps, 
                           gauss_rands, curr_tuple, coeff_max, 0, cores, 
                           &threads_used, threads, &mutex);
  
  
    for (int tout = 1; tout < share_count + 1; tout++){
      env_tout_cRPC_parallel(c, share_count, final_env, env, gauss_deps, 
                             gauss_rands, curr_tuple, coeff_max, tout, cores, 
                             &threads_used, threads, &mutex);
    }
    for (int j = 0; j < threads_used; j++)
      pthread_join(threads[j], NULL);
    free(threads);
  }
  print_coeffs_env(c, final_env);

  //Freeing Stuff
  {
  for (int tin = 0; tin < share_count + 1; tin++){
    for (int tout = 0; tout < share_count + 1; tout++){
      free(env[tin][tout]);
      free(final_env[tin][tout]);
    }
    free(final_env[tin]);
  }
  free(final_env);
  for (int i = 0; i < max_deps_length; i++)
    free(gauss_deps[i]);
  free(gauss_deps);
  free(gauss_rands);
  Tuple_free(curr_tuple);
  }
}

void *start_thread_env_tout_cRPC(void *void_args){
  struct env_tout_cRPC_args* args = (struct env_tout_cPRC_args *) void_args;

  int max_deps_length = args->c->deps->length + 1;
          
  //Build the array of gaussian dependencies and the linked array of gaussian
  //randoms. It is empty now, but when we will add a probes on |curr_tuples|, 
  //we will write the dependency on the probes after gaussian elimination with
  // the other probes existing in |curr_tuples|. And we will write the index of 
  //the randoms that are the pivot for this probes in |gauss_rands|.  
  Tuple * curr_tuple = Tuple_make_size(max_deps_length);
  Dependency** gauss_deps = malloc(max_deps_length * sizeof(*gauss_deps));
  for (int i = 0; i < max_deps_length; i++) {
    gauss_deps[i] = malloc(args->c->deps->deps_size * sizeof(*gauss_deps[i]));
  }
  Dependency* gauss_rands = malloc(max_deps_length * sizeof(*gauss_rands));

  uint64_t *env[args->share_count + 1][args->share_count + 1];
  for (int tin = 0; tin < args->share_count + 1; tin++){
    for (int tout = 0; tout < args->share_count + 1; tout++){
      env[tin][tout] = calloc((args->c->total_wires + 1), 
                               sizeof(*env[tin][tout]));
    }
  }

  if (!args->tout){
    //First Step - Cardinal RPC enveloppes with no outputs leaked.
    //Adding the empty tuples to the enveloppes that leaks 0 input shares.
    int revealed_secret = 0;
    compute_coeffs_tuple(args->c, curr_tuple, env[0][0]);  

    //Update the cardinal RPC enveloppes for tout = 0.
    update_env_cRPC (args->c, curr_tuple, 0, gauss_deps, gauss_rands, 0, 
                     args->share_count, env, revealed_secret, args->coeff_max, 
                     0);
  
    // Adding it in the final enveloppes.                 
    for (int tin = 0; tin < args->share_count + 1; tin++){        
      pthread_mutex_lock(args->mutex);
      max_coeffs(tin, 0, args->share_count, args->final_env, env, 
        args->coeff_max);
      pthread_mutex_unlock(args->mutex);
    }
  }

  else{
    //Compute the dependency of the Output Set, that we add at the top of 
    //|gauss_deps| and |gauss_rands|. 
    for (int j = 0; j < args->tout; j++){
      gaussian_transformation(args->c, args->output_tuple->content[j], 
                              gauss_deps, gauss_rands, args->gauss_length, 
                              false);
      args->new_revealed_secret = compute_revealed_secret(args->c, 
                                  args->output_tuple, gauss_deps, 
                                  args->new_revealed_secret, j - args->tout + 1, 
                                  false);
      args->gauss_length++;
    }
  
    env[__builtin_popcount(args->new_revealed_secret)][args->tout][0] = 1;
    update_env_cRPC (args->c, curr_tuple, 0, gauss_deps, gauss_rands, 
                     args->gauss_length, args->share_count, env, 
                     args->new_revealed_secret, args->coeff_max, args->tout);
      
    for (int tin = 0; tin < args->share_count + 1; tin++){        
      pthread_mutex_lock(args->mutex);
      max_coeffs(tin, args->tout, args->share_count, args->final_env, env, 
                args->coeff_max);
      pthread_mutex_unlock(args->mutex);
        
      //Redefine to 0 to give good results for the next output set.
      for(int l = 0; l < args->c->total_wires + 1; l++)
        env[tin][args->tout][l] = 0;
    }
    Tuple_free(args->output_tuple);
  }    

  
  // Freeing stuffs 
  Tuple_free(curr_tuple);
  for (int tin = 0; tin < args->share_count + 1; tin++){
    for (int tout = 0; tout < args->share_count + 1; tout++)
      free(env[tin][tout]);
  }
  for (int i = 0; i < max_deps_length; i++)
    free(gauss_deps[i]);
  free(gauss_deps);
  free(gauss_rands);  
  free(args);
  return NULL; 
}



