// Checks if the tuple |local_deps| is a failure when adding randoms
// (that were removed from the circuit by the function
// |remove_randoms| of dimensions.c).
int is_failure_with_randoms(const Circuit* circuit,
                            BitDep** local_deps, GaussRand* gauss_rands, BitDep** local_deps_copy, GaussRand* gauss_rands_copy,
                            int local_deps_len, int t_in,
                            int comb_free_space, Dependency shares_to_ignore, bool PINI) {
                    
  if (comb_free_space == 0) return 0;
  /* printf("Trying to expand with randoms\n"); */
  /* printf("Tuple: {\n"); */
  /* for (int i = 0; i < local_deps_len; i++) { */
  /*   printf("  [ %d %d | %016lx | ", local_deps[i]->secrets[0], local_deps[i]->secrets[1], */
  /*          local_deps[i]->randoms); */
  /*   for (int j = 0; j < 1+circuit->deps->mult_deps->length/64; j++) { */
  /*     printf("%016lx ", local_deps[i]->mults[j]); */
  /*   } */
  /*   printf("]\n"); */
  /* } */
  /* printf("}\n"); */


  DependencyList* deps = circuit->deps;
  int secret_count = circuit->secret_count;
  int random_count = circuit->random_count + secret_count;
  int deps_size = deps->deps_size;
  int mult_count = deps->mult_deps->length;
  int non_mult_deps_count = deps_size - mult_count;
  int bit_rand_len = 1 + random_count / 64;
  int bit_mult_len = 1 + mult_count / 64;

  // Precomputing secrets of each element of |local_deps|
  Dependency secrets[local_deps_len][2];
  for (int i = 0; i < local_deps_len; i++) {
    secrets[i][0] = local_deps[i]->secrets[0];
    secrets[i][1] = local_deps[i]->secrets[1];
    for (int j = 0; j < bit_mult_len; j++) {
      uint64_t mult_elem = local_deps[i]->mults[j];
      while (mult_elem != 0) {
        int mult_idx_in_elem = __builtin_ia32_lzcnt_u64(mult_elem);
        mult_elem &= ~(1ULL << (63-mult_idx_in_elem));
        int mult_idx = j * 64 + (63-mult_idx_in_elem);
        MultDependency* mult_dep = deps->mult_deps->deps[mult_idx];
        secrets[i][0] |= mult_dep->contained_secrets[0];
        secrets[i][1] |= mult_dep->contained_secrets[1];
      }
    }
  }

  // Collecting all randoms of |local_deps| in the binary array |randoms|.
  uint64_t randoms[bit_rand_len];
  memset(randoms, 0, bit_rand_len * sizeof(*randoms));
  for (int i = 0; i < local_deps_len; i++) {
    for (int j = 0; j < bit_rand_len; j++) {
      randoms[j] |= local_deps[i]->randoms[j];
    }
  }

  // Building the array |randoms_arr| that contains the index of the
  // randoms in |local_deps|.
  Var randoms_arr[non_mult_deps_count];
  int randoms_arr_len = 0;
  for (int i = 0; i < bit_rand_len; i++) {
    uint64_t rand_elem = randoms[i];
    while (rand_elem != 0) {
      int rand_idx_in_elem = 63-__builtin_ia32_lzcnt_u64(rand_elem);
      rand_elem &= ~(1ULL << rand_idx_in_elem);
      randoms_arr[randoms_arr_len++] = rand_idx_in_elem + i*64;
    }
  }

  // Generating all combinations of randoms of |randoms_arr| and
  // checking if they make the tuple become a failure.
  Comb randoms_comb[randoms_arr_len];
  for (int size = 1; size <= comb_free_space && size <= randoms_arr_len; size++) {
    for (int i = 0; i < size; i++) randoms_comb[i] = i;
    do {
      Comb real_comb[size];
      for (int i = 0; i < size; i++) real_comb[i] = randoms_arr[randoms_comb[i]];
      uint64_t selected_randoms[bit_rand_len];
      memset(selected_randoms, 0, bit_rand_len * sizeof(*selected_randoms));
      for (int i = 0; i < size; i++) {
        int idx    = real_comb[i] / 64;
        int offset = 63-__builtin_ia32_lzcnt_u64(real_comb[i]);
        selected_randoms[idx] |= 1ULL << offset;
      }

      Dependency secret_deps[secret_count];
      secret_deps[0] = secret_deps[1] = 0;

      // Computing which secret shares are leaked
      for (int i = 0; i < local_deps_len; i++) {
        if (!gauss_rands[i].is_set) {
          secret_deps[0] |= secrets[i][0];
          if (secret_count == 2) secret_deps[1] |= secrets[i][1];
        } else {
          int all_rands_selected = 1;
          for (int j = 0; j < bit_rand_len; j++) {
            if ((local_deps[i]->randoms[j] & selected_randoms[j]) !=
                local_deps[i]->randoms[j]) {
              all_rands_selected = 0;
              break;
            }
          }
          if (all_rands_selected) {
            // |selected_randoms| fully unmasks |local_deps[i]|
            secret_deps[0] |= secrets[i][0];
            secret_deps[1] |= secrets[i][1];
          }
        }
      }

      if (PINI) {
        secret_deps[0] |= secret_deps[1];
      }

      // Checking if enough secret shares are leaking for the tuple to
      // be a failure.
      for (int k = 0; k < secret_count; k++) {
        secret_deps[k] &= ~shares_to_ignore;
        if (Is_leaky(secret_deps[k], t_in, comb_free_space-size)) {
          return 1;
        }
      }

    } while (incr_comb_in_place(randoms_comb, size, randoms_arr_len));
  }

  /* printf("Didn't manage to get a failure...\n"); */

  return 0;
}






// Checks if the tuple |local_deps| is a failure when adding randoms
// (that were removed from the circuit by the function
// |remove_randoms| of dimensions.c).
static int is_failure_with_randoms(const Circuit* circuit,
                            BitDep** local_deps, GaussRand* gauss_rands, BitDep** local_deps_copy, GaussRand* gauss_rands_copy,
                            int local_deps_len, int t_in,
                            int comb_free_space, Dependency shares_to_ignore, bool PINI) {
  if (comb_free_space == 0) return 0;
  /* printf("Trying to expand with randoms\n"); */
  /* printf("Tuple: {\n"); */
  /* for (int i = 0; i < local_deps_len; i++) { */
  /*   printf("  [ %d %d | %016lx | ", local_deps[i]->secrets[0], local_deps[i]->secrets[1], */
  /*          local_deps[i]->randoms); */
  /*   for (int j = 0; j < 1+circuit->deps->mult_deps->length/64; j++) { */
  /*     printf("%016lx ", local_deps[i]->mults[j]); */
  /*   } */
  /*   printf("]\n"); */
  /* } */
  /* printf("}\n"); */

  DependencyList* deps = circuit->deps;
  int secret_count = circuit->secret_count;
  int random_count = circuit->random_count + secret_count;
  int deps_size = deps->deps_size;
  int mult_count = deps->mult_deps->length;
  int non_mult_deps_count = deps_size - mult_count;
  int bit_rand_len = 1 + random_count / 64;
  int bit_mult_len = 1 + mult_count / 64;

  // Collecting all randoms of |local_deps| in the binary array |randoms|.
  uint64_t randoms[bit_rand_len];
  memset(randoms, 0, bit_rand_len * sizeof(*randoms));
  for (int i = 0; i < local_deps_len; i++) {
    for (int j = 0; j < bit_rand_len; j++) {
      randoms[j] |= local_deps[i]->randoms[j];
    }
  }

  // if(pr){
  //   for (int j = 0; j < bit_rand_len; j++) {
  //     printf("%llu\n",randoms[j]);
  //     printf("rand_count = %d\n", circuit->random_count);
  //   }
  // }

  // Building the array |randoms_arr| that contains the index of the
  // randoms in |local_deps|.
  Var randoms_arr[non_mult_deps_count];
  int randoms_arr_len = 0;
  for (int i = 0; i < bit_rand_len; i++) {
    uint64_t rand_elem = randoms[i];
    while (rand_elem != 0) {
      int rand_idx_in_elem = 63-__builtin_ia32_lzcnt_u64(rand_elem);
      rand_elem &= ~(1ULL << rand_idx_in_elem);
      randoms_arr[randoms_arr_len++] = rand_idx_in_elem + i*64;
    }
  }

  // if(pr){
  //   printf("Free Space = %d\n", comb_free_space);
  //   printf("Elems : ");
  //   for(int i=0; i<randoms_arr_len; i++){
  //     printf("%hu ", randoms_arr[i]);
  //   }
  //   printf("\n");
  // }

  int copy_size = 0;

  // Generating all combinations of randoms of |randoms_arr| and
  // checking if they make the tuple become a failure.
  Comb randoms_comb[randoms_arr_len];
  int size = min(comb_free_space, randoms_arr_len);
  for (int i = 0; i < size; i++) randoms_comb[i] = i;
  do {

    Comb real_comb[size];
    for (int i = 0; i < size; i++) real_comb[i] = randoms_arr[randoms_comb[i]];
    uint64_t selected_randoms[bit_rand_len];
    memset(selected_randoms, 0, bit_rand_len * sizeof(*selected_randoms));

    // if(pr){
    //   printf("randoms_comb comb = ");
    //   for (int i = 0; i < comb_free_space; i++){ printf("%u ", randoms_comb[i]); }
    //   printf("\n");
    // }

    for (int i = 0; i < size; i++) {
      // if(pr) printf("real comb = %d ", real_comb[i]);
      int idx    = real_comb[i] / 64;
      // if(pr) printf(", idx = %d ", idx);
      // int offset = 63-__builtin_ia32_lzcnt_u64(real_comb[i]);
      // if(pr) printf(", offset = %d\n", offset);
      selected_randoms[idx] |= 1ULL << real_comb[i];
    }

    // if(pr){
    //   //selected_randoms[0] = 1 << 6;
    //   for (int j = 0; j < bit_rand_len; j++) {
    //     printf("selected = %llu\n",selected_randoms[j]);
    //   }
      
    //   //printf("selected new = %lu\n",selected_randoms[0] & randoms[0]);
    // }

    Dependency secret_deps[secret_count];
    secret_deps[0] = secret_deps[1] = 0;
    copy_size = 0;

    // Computing which secret shares are leaked
    for (int i = 0; i < local_deps_len; i++) {
      if (!gauss_rands[i].is_set) {
        secret_deps[0] |= local_deps[i]->secrets[0];
        if (secret_count == 2) secret_deps[1] |= local_deps[i]->secrets[1];
      } else {

        memcpy(local_deps_copy[copy_size], local_deps[i], sizeof(*local_deps[i]));
        for (int j = 0; j < bit_rand_len; j++) {
          local_deps_copy[copy_size]->randoms[j] = (local_deps[i]->randoms[j] & selected_randoms[j]) ^ local_deps[i]->randoms[j];
        }
        copy_size ++;
      }
    }

    for (int i = 0; i < copy_size; i++) {
        gauss_step(local_deps_copy[i], local_deps_copy, gauss_rands_copy,
                  bit_rand_len, bit_mult_len, i);
        set_gauss_rand(local_deps_copy, gauss_rands_copy, i, bit_rand_len);
    }

    for (int i = 0; i < copy_size; i++) {
      if (!gauss_rands_copy[i].is_set) {
        secret_deps[0] |= local_deps_copy[i]->secrets[0];
        if (secret_count == 2) secret_deps[1] |= local_deps_copy[i]->secrets[1];
      }
    }

    if (PINI) {
      secret_deps[0] |= secret_deps[1];
    }

    // Checking if enough secret shares are leaking for the tuple to
    // be a failure.
    for (int k = 0; k < secret_count; k++) {
      secret_deps[k] &= ~shares_to_ignore;
      if (Is_leaky(secret_deps[k], t_in, comb_free_space-size)) {
        return 1;
      }
    }

  } while (incr_comb_in_place(randoms_comb, size, randoms_arr_len));

  /* printf("Didn't manage to get a failure...\n"); */

  return 0;
}






int _verify_tuples_freeSNI_IOS(const Circuit* circuit, // The circuit
                   int comb_len, // The length of the tuples (includes prefix->length)
                   const DimRedData* dim_red_data, // Data to generate the actual tuples
                                                   // after the dimension reduction
                   bool stop_at_first_failure, // If true, stops after the first failure
                   BitDep** output_deps,
                   GaussRand * output_gauss_rands,
                   void (failure_callback)(const Circuit*,Comb*, int, SecretDep*, void*),
                   //    ^^^^^^^^^^^^^^^^
                   // The function to call when a failure is found
                   void* data, // additional data to pass to |failure_callback|
                   bool freesni,
                   bool ios
                   ) {

  DependencyList* deps    = circuit->deps;
  // Retrieving bitvector-based structures
  BitDepVector** bit_deps  = deps->bit_deps;

  int secret_count        = circuit->secret_count;
  int random_count        = circuit->random_count + secret_count;
  int mult_count          = deps->mult_deps->length;
  int bit_mult_len = 1 + (mult_count / 64);
  int bit_rand_len = 1 + (random_count / 64);
  int last_var = circuit->length;

  //VarVector* prefix = &empty_VarVector;


  // Local dependencies
  int local_deps_max_size = deps->length * 10; // sounds reasonable?

  // to test independence of output shares
  GaussRand gauss_rands[local_deps_max_size];  
  BitDep* local_deps[local_deps_max_size];

  // to determine the sets of input shares to simulate the t_1 + t_2 probes
  GaussRand gauss_rands_without_outs[local_deps_max_size];
  BitDep* local_deps_without_outs[local_deps_max_size];

  Dependency choices[local_deps_max_size];
  Dependency secrets[local_deps_max_size][2];
  Dependency secrets_xor[local_deps_max_size][2];
  choices[0] = -1;
  for(int i=0; i<local_deps_max_size; i++){
    local_deps[i] = alloca(sizeof(**local_deps));
    local_deps_without_outs[i] = alloca(sizeof(**local_deps_without_outs));
    secrets[i][0] = 0;
    secrets[i][1] = 0;
    secrets_xor[i][0] = 0;
    secrets_xor[i][1] = 0;
  }

  int failure_count = 0;
  uint64_t tuples_checked = 0;

  int local_deps_without_outs_len = 0;
  int local_deps_without_len_tmp  = 0;
  
  // if((!freesni) && (!ios)){
  //   ////////////////////////////////////////////////////////////////////
  //   // First perform reduction on the prefix of the t_2 output shares
  //   // since they will be the same for all the sets of t_1 probes 
  //   // this is used with |local_deps_without_outs|
  //   ////////////////////////////////////////////////////////////////////
  //   for(int i = 0; i< prefix->length; i++){
  //     BitDepVector* bit_dep_arr = bit_deps[prefix->content[i]];
  //     for (int dep_idx = 0; dep_idx < bit_dep_arr->length; dep_idx++) {
  //       //without outs
  //       gauss_step(bit_dep_arr->content[dep_idx], local_deps_without_outs, gauss_rands_without_outs,
  //                   bit_rand_len, bit_mult_len, local_deps_without_outs_len);
  //       set_gauss_rand(local_deps_without_outs, gauss_rands_without_outs, local_deps_without_outs_len, bit_rand_len);
  //       local_deps_without_outs_len++;
  //     }
  //   }
  //   local_deps_without_len_tmp = local_deps_without_outs_len;
  // }

  ////////////////////////////////////////////////////////////////////
  // Copy the already reduced n-1 output shares passed to the function
  // to avoid doing row reduction on them at each iteration
  // this is used with |local_deps|
  ////////////////////////////////////////////////////////////////////
  int local_deps_len = 0;
  int local_deps_len_tmp;

  for(int i=0; i< (circuit->share_count)-1; i++){
    memcpy(local_deps[i], output_deps[i], sizeof(*output_deps[i]));
    memcpy(gauss_rands+i, output_gauss_rands+i, sizeof(gauss_rands[i]));

    local_deps_len++;
  }
  local_deps_len_tmp = local_deps_len;

  // Start iterating on the sets of probes
  Comb* curr_comb = init_comb(NULL, comb_len, &empty_VarVector, comb_len);
  
  int total_iterations = 0;
  int total_tuples = 0;
  Dependency final_inputs[2];
  Dependency final_output[1] = {0}; // only for IOS
  int first_invalid_local_deps_index = 0;
  uint64_t t = (1 << circuit->share_count)-1;

  do {
    tuples_checked++;

    // printf("Tuple: [ ");
    // for (int i = 0; i < comb_len; i++) printf("%d ", curr_comb[i]);
    // printf("]   ");
    // printf("Tuple: [ ");
    // for (int i = 0; i < comb_len; i++) printf("%s ", deps->names[curr_comb[i]]);
    // printf("]\n");
    
    local_deps_len = local_deps_len_tmp + first_invalid_local_deps_index;
    local_deps_without_outs_len = local_deps_without_len_tmp + first_invalid_local_deps_index;

    ////////////////////////////////////////////////////////////////////
    // Updating |local_deps| and |local_deps_without_outs|
    // with INTERNAL DEPS
    ////////////////////////////////////////////////////////////////////
    for (int i = first_invalid_local_deps_index; i < comb_len; i++) {
      BitDepVector* bit_dep_arr = bit_deps[curr_comb[i]];

      for (int dep_idx = 0; dep_idx < bit_dep_arr->length; dep_idx++) {

        gauss_step(bit_dep_arr->content[dep_idx], local_deps, gauss_rands,
                   bit_rand_len, bit_mult_len, local_deps_len);
        set_gauss_rand(local_deps, gauss_rands, local_deps_len, bit_rand_len);

        //without outs
        gauss_step(bit_dep_arr->content[dep_idx], local_deps_without_outs, gauss_rands_without_outs,
                   bit_rand_len, bit_mult_len, local_deps_without_outs_len);
        set_gauss_rand(local_deps_without_outs, gauss_rands_without_outs, local_deps_without_outs_len, bit_rand_len);


        local_deps_len++;
        local_deps_without_outs_len++;

      }

    }

    // if((comb_len == 2) & (curr_comb[0] == 37-15) & (curr_comb[1] == 43-15)){
    //   printf("Start without outs = %d\n", start_without_outs);
    //   printf("After: final_inputs = %lu, %lu\n", final_inputs[0], final_inputs[1]);
    //   printf("After Gauss: {\n");
    //   for (int i = 0; i < local_deps_len+start_without_outs; i++) {
    //     printf("  [ %lu %lu | ",
    //               local_deps[i]->secrets[0], local_deps[i]->secrets[1]);
    //     for (int k = 0; k < bit_rand_len; k++){
    //       printf("%lu ", local_deps[i]->randoms[k]);
    //     }
    //     printf(" | "); 
    //     if(circuit->contains_mults){
    //       for (int k = 0; k < bit_mult_len; k++){
    //         printf("%lu ", local_deps[i]->mults[k]);
    //       }
    //     } 
    //     printf("| %lu ", local_deps[i]->out); 
    //     printf("] -- gauss_rand = %lu %d -- secrets = %lu %lu\n", gauss_rands[i].mask, gauss_rands[i].is_set, secrets[0], secrets[1]);
      
    //   }
    //   printf("\n");
    // }

    ////////////////////////////////////////////////////////////////////
    // determining the sets of input shares to simulate the
    // probes -> SNI like verification
    ////////////////////////////////////////////////////////////////////
    final_inputs[0] = 0; final_inputs[1] = 0; final_output[0] = 0;
    for(int i = 0; i < local_deps_without_outs_len; i++){
      if(gauss_rands_without_outs[i].is_set) continue;

      compute_input_secrets(circuit, local_deps_without_outs[i], secrets[i], mult_count, false);
      final_inputs[0] = final_inputs[0] | secrets[i][0];
      final_inputs[1] = final_inputs[1] | secrets[i][1];
    }

    if((hamming_weight(final_inputs[0]) > comb_len) ||
       (((secret_count==2) && (hamming_weight(final_inputs[1]) > comb_len)))){
        
        printf("SNI-like Failure\n");
        goto process_failure;
    }

    ////////////////////////////////////////////////////////////////////
    // Next, try to construct the set O of output shares independent
    // and uniform conditioned on the probes simulated by I,J , and 
    // c_{|I \inter J}
    ////////////////////////////////////////////////////////////////////
    index_choices = 0;
    for(int i = local_deps_len_tmp; i< local_deps_len; i++){
      if(gauss_rands[i].is_set) continue;

      if(local_deps[i]->out == 0){
        continue;
      }
      else{
        // free SNI
        if(freesni){
          compute_input_secrets(circuit, local_deps[i], secrets[i], mult_count, false);
          compute_input_secrets(circuit, local_deps[i], secrets_xor[i], mult_count, true);
          if((hamming_weight(secrets[i][0] | local_deps[i]->out) > comb_len) ||
             (hamming_weight(secrets[i][1] | local_deps[i]->out) > comb_len)){
        
            final_inputs[0] = final_inputs[0] | (secrets_xor[i][0]) | (local_deps[i]->out ^ t);
            final_inputs[1] = final_inputs[1] | (secrets_xor[i][1]) | (local_deps[i]->out ^ t);
          }
          else if((hamming_weight((secrets_xor[i][0]) | (local_deps[i]->out ^ t)) <= comb_len) &&
                  (hamming_weight((secrets_xor[i][1]) | (local_deps[i]->out ^ t)) <= comb_len)){
            choices[index_choices] = i;
            index_choices++;
          } 
          else{
            final_inputs[0] = final_inputs[0] | secrets[i][0] | local_deps[i]->out;
            final_inputs[1] = final_inputs[1] | secrets[i][1] | local_deps[i]->out;
          }

        }

        // IOS
        else if(ios){
          compute_input_secrets(circuit, local_deps[i], secrets[i], mult_count, false);
          compute_input_secrets(circuit, local_deps[i], secrets_xor[i], mult_count, true);
          
          if((hamming_weight(secrets[i][0]) > comb_len) || (hamming_weight(local_deps[i]->out) > comb_len) ||
             (hamming_weight(secrets[i][1]) > comb_len)){
        
            final_inputs[0] = final_inputs[0] | (secrets_xor[i][0]);
            final_inputs[1] = final_inputs[1] | (secrets_xor[i][1]);
            final_output[0] = final_output[0] | (local_deps[i]->out ^ t);

          }
          else if((hamming_weight(secrets_xor[i][0]) <= comb_len) && (hamming_weight(local_deps[i]->out ^ t) <= comb_len) &&
                  (hamming_weight(secrets_xor[i][1]) <= comb_len)){
            choices[index_choices] = i;
            index_choices++;
          } 
          else{
            final_inputs[0] = final_inputs[0] | secrets[i][0];
            final_inputs[1] = final_inputs[1] | secrets[i][1];
            final_output[0] = final_output[0] | local_deps[i]->out;
          }
        }

        // uniform SNI
        // else{
        //   if((hamming_weight(local_deps[i]->out) > comb_len)){
        //     final_inputs[0] = final_inputs[0] | (local_deps[i]->out ^ t);
        //     final_inputs[1] = final_inputs[1] | (local_deps[i]->out ^ t);
        //   }
        //   else if((hamming_weight((local_deps[i]->out ^ t)) <= comb_len)){
        //       choices[index_choices] = i;
        //       index_choices++;
        //   } 
        //   else{
        //     final_inputs[0] = final_inputs[0] | local_deps[i]->out;
        //     final_inputs[1] = final_inputs[1] | local_deps[i]->out;
        //   }
        // }
        
      } 
    }

    // if(curr_comb[0] == 1){
    //   printf("After: final_inputs = %lu, %lu\n", final_inputs[0], final_inputs[1]);
    //   printf("After Gauss: {\n");
    //   for (int i = 0; i < local_deps_len; i++) {
    //     printf("  [ %lu %lu | ",
    //               local_deps[i]->secrets[0], local_deps[i]->secrets[1]);
    //     for (int k = 0; k < bit_rand_len; k++){
    //       printf("%lu ", local_deps[i]->randoms[k]);
    //     }
    //     printf(" | "); 
    //     if(circuit->contains_mults){
    //       for (int k = 0; k < bit_mult_len; k++){
    //         printf("%lu ", local_deps[i]->mults[k]);
    //       }
    //     } 
    //     printf("| %lu ", local_deps[i]->out); 
    //     printf("] -- gauss_rand = %lu -- secrets = %lu %lu\n", gauss_rands[i].mask, secrets[0], secrets[1]);
      
    //   }
    //   printf("\n");
    //   printf("%d %d %d\n", final_inputs[0], final_inputs[1], final_output[0]);
    // }

    ////////////////////////////////////////////////////////////////////
    // Try to complete sets of input shares that will satisfy
    // the property
    ////////////////////////////////////////////////////////////////////
    if(index_choices > 0){
      //printf("After: final_inputs = %lu, %lu\n", final_inputs[0], final_inputs[1]);
      total_tuples++;
      bool fail = true;
      Dependency final_inputs_tmp[2] = {final_inputs[0], final_inputs[1]};
      Dependency final_output_tmp = final_output[0];

      for(uint64_t i = 0; i < (1ULL << index_choices); i++){
        total_iterations ++;
        final_inputs_tmp[0] = final_inputs[0];
        final_inputs_tmp[1] = final_inputs[1];
        final_output_tmp = final_output[0];

        for(int j = 0; j< index_choices; j++){
          if(freesni){
            if(((1ULL << j) & i)){
              final_inputs_tmp[0] = final_inputs_tmp[0] | (secrets_xor[choices[j]][0]) | (local_deps[choices[j]]->out ^t);
              final_inputs_tmp[1] = final_inputs_tmp[1] | (secrets_xor[choices[j]][1]) | (local_deps[choices[j]]->out ^t);
            }
            else{
              final_inputs_tmp[0] = final_inputs_tmp[0] | secrets[choices[j]][0] | local_deps[choices[j]]->out;
              final_inputs_tmp[1] = final_inputs_tmp[1] | secrets[choices[j]][1] | local_deps[choices[j]]->out;
            }
          }
          else if(ios){
            if(((1ULL << j) & i)){
              final_inputs_tmp[0] = final_inputs_tmp[0] | (secrets_xor[choices[j]][0]);
              final_inputs_tmp[1] = final_inputs_tmp[1] | (secrets_xor[choices[j]][1]);
              final_output_tmp = final_output_tmp | (local_deps[choices[j]]->out ^t);
            }
            else{
              final_inputs_tmp[0] = final_inputs_tmp[0] | secrets[choices[j]][0];
              final_inputs_tmp[1] = final_inputs_tmp[1] | secrets[choices[j]][1];
              final_output_tmp = final_output_tmp | local_deps[choices[j]]->out;
            }
          }
          // else{
          //   if(((1ULL << j) & i)){
          //     final_inputs_tmp[0] = final_inputs_tmp[0] | (local_deps[choices[j]]->out ^t);
          //     final_inputs_tmp[1] = final_inputs_tmp[1] | (local_deps[choices[j]]->out ^t);
          //   }
          //   else{
          //     final_inputs_tmp[0] = final_inputs_tmp[0] | local_deps[choices[j]]->out;
          //     final_inputs_tmp[1] = final_inputs_tmp[1] | local_deps[choices[j]]->out;
          //   }
          // }
        }
        if((hamming_weight(final_inputs_tmp[0]) <= comb_len) && 
           (hamming_weight(final_inputs_tmp[1]) <= comb_len) &&
           (hamming_weight(final_output_tmp) <= comb_len)){
          fail = false;
          break;
        }
      }
      if(fail){
        final_inputs[0] = final_inputs_tmp[0];
        final_inputs[1] = final_inputs_tmp[1];
        goto process_failure;
      }
      else{
        goto process_success;
      }
    }
    else{
      if((hamming_weight(final_inputs[0]) > comb_len) ||
         (hamming_weight(final_inputs[1]) > comb_len) ||
         (hamming_weight(final_output[0]) > comb_len)){
        goto process_failure;
      }
      else{
        goto process_success;
      }
    }

    ////////////////////////////////////////////////////////////////////
    // Tuple is a failure
    ////////////////////////////////////////////////////////////////////
    process_failure:
    if (failure_callback) {
      SecretDep leaky_inputs[2] = { 0 };
      Dependency secret_deps[2] = { 0 };
      if (dim_red_data) {
        expand_tuple_to_failure(circuit, -1, 0,
                                curr_comb, comb_len, leaky_inputs, secret_deps,
                                comb_len, dim_red_data, failure_callback, data);
        // if((!freesni) && (!ios)){
        //   expand_tuple_to_failure(circuit, -1, 0,
        //                           prefix->content, prefix->length, leaky_inputs, secret_deps,
        //                           prefix->length, dim_red_data, failure_callback, data);
        // }
      } else {
        failure_callback(circuit, curr_comb, comb_len, leaky_inputs, data);
      }
    }
    failure_count++;
    if (stop_at_first_failure) {
      break;
    }

    ////////////////////////////////////////////////////////////////////
    // Tuple is not a failure, move on to the next tuple to check
    ////////////////////////////////////////////////////////////////////
    process_success:;
  } while ((first_invalid_local_deps_index = next_comb(curr_comb, comb_len, last_var, NULL)) >= 0);

  // if(failure_count == 0){
  //    printf("Total tuples with additional iterations = %d\n", total_tuples);
  //    printf("Total additional iterations = %d\n", total_iterations);
  //    printf("Iterations per tuple =  %lf\n\n", (total_iterations * 1.0)/total_tuples);
  // }

  return failure_count;
}
