#include <stdio.h>  // For fprintf, stderr...
#include <stdlib.h> // For malloc, exit...
#include <stdint.h> // For uint64_t
#include <getopt.h> // For getopt_long
#include <unistd.h> // For access
#include <string.h> // For strcmp
#include <time.h>   // For clock
#include <locale.h> // For setlocale
#include <inttypes.h>

#include "circuit.h"
#include "list_tuples.h"
#include "combinations.h"
#include "verification_rules.h"
#include "parser.h"
#include "coeffs.h"
#include "cardRPC.h" 
#include "NI.h"
#include "SNI.h"
#include "freeSNI.h"
#include "IOS.h"
#include "PINI.h"
#include "RP.h"
#include "RPC.h"
#include "RPE.h"
#include "config.h"
#include "constructive.h"
#include "utils.h"
#include "CNI.h"
#include "CRP.h"
#include "CRPC.h"

#define GLITCH_OPT 1000
#define TRANSITION_OPT 1001

/***********************************************************
                            Main
 ***********************************************************/

int is_int(char* s) {
  if (!s || !*s) return 0;
  while (*s) {
    if (*s < '0' || *s > '9') return 0;
    s++;
  }
  return 1;
}

int is_double(char* s) {
  if (!s || !*s) return 0;
  int cpt=0;
  while (*s) {
    if(*s == ','){ cpt++; }
    else if (*s < '0' || *s > '9'){ return 0; }
    s++;
  }
  if(cpt > 1) return 0;
  return 1;
}

void usage() {
  printf("Usage:\n"
         "    ironmask [OPTIONS] [NI|SNI|freeSNI|uniformSNI|IOS|PINI|RP|RPC|RPE|cardRPC|CNI|CRP|CRPC|cardRPC] FILE\n"
         "Computes the probing (NI, SNI, PINI) or random probing property (RP, RPC, RPE) or the combined fault property (CNI) for FILE\n"
         "Computes the cardinal RPC enveloppes (cardRPC) for refresh gadgets in arithmetic field.\n\n"

         "Options:\n"
         "    -v[num], --verbose[num]             Sets verbosity level.\n"
         "    -c[num], --coeff_max[num]           Sets the last precise coefficient to compute\n"
         "                                        for RP-like properties.\n"
         "    -t[num]                             Sets the t parameter for NI/SNI/PINI/RPC/RPE/CRPC.\n"
         "    -k[num]                             Sets the k parameter for CNI/CRP/CRPC.\n"
         "                                        This option is mandatory except when checking RP.\n"
         "    -o[num], --t_output[num]            Sets the t_output parameter for RPC/RPE.\n"
         "    -j[num], --jobs[num]                Sets the number of core to use.\n"
         "                                        If [num] is -1, ironmask uses all cores.\n"
         "    -i, --incompr-opt                   Enables incompressible tuples optimization.\n"
         "                                        (this option may or may not work, you should probably\n"
         "                                        not use it unless you know what you're doing)\n"
         "    --glitch                            Takes glitches into account.\n"
         "    --transition                        Takes transitions into account\n"
         "    -h, --help                          Prints this help information.\n\n");

  exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  setlocale(LC_NUMERIC, "");

  int verbose = 0, coeff_max = -1, t = -1, t_output = -1, opt_incompr = 0, cores = 1, k = -1;
  double pleak = -1, pfault = -1;
  bool glitch = false, transition = false;
  bool set = true;
  char* property = NULL;
  char* filename = NULL;

  while (1) {
    static struct option long_options[] = {
      { "help",        no_argument,       0, 'h'            },
      { "verbose",     required_argument, 0, 'v'            },
      { "coeff_max",   required_argument, 0, 'c'            },
      { "t",           required_argument, 0, 't'            },
      { "k",           required_argument, 0, 'k'            },
      { "l",           required_argument, 0, 'l'            },
      { "f",           required_argument, 0, 'f'            },
      { "s",           required_argument, 0, 's'            },
      { "t_output",    required_argument, 0, 'o'            },
      { "jobs",        required_argument, 0, 'j'            },
      { "incompr-opt", no_argument,       0, 'i'            },
      { "glitch",      no_argument,       0, GLITCH_OPT     },
      { "transition",  no_argument,       0, TRANSITION_OPT },
      { 0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "hc:v:t:k:l:f:s:o:j:i",
                        long_options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 'h':
        usage();
        break;
      case 'i':
        opt_incompr = 1;
        break;
      case 'v':
        if (!is_int(optarg)) {
          fprintf(stderr, "Option --verbose/-v expects an integer. Provided: '%s'. Exiting.\n",
                  optarg);
          exit(EXIT_FAILURE);
        } else {
          verbose = atoi(optarg);
        }
        break;
      case 'c':
        if (!is_int(optarg)) {
          fprintf(stderr, "Option --coeff_max/-c expects an integer. Provided: '%s'. Exiting.\n",
                  optarg);
          exit(EXIT_FAILURE);
        } else {
          coeff_max = atoi(optarg);
        }
        break;
      case 't':
        if (!is_int(optarg)) {
          fprintf(stderr, "Option -t expects an integer. Provided: '%s'. Exiting.\n",
                  optarg);
          exit(EXIT_FAILURE);
        } else {
          t = atoi(optarg);
        }
        break;
      case 'k':
        if (!is_int(optarg)) {
          fprintf(stderr, "Option -k expects an integer. Provided: '%s'. Exiting.\n",
                  optarg);
          exit(EXIT_FAILURE);
        } else {
          k = atoi(optarg);
        }
        break;
      case 'l':
        if (!is_double(optarg)) {
          fprintf(stderr, "Option -l expects a float. Provided: '%s'. Exiting.\n",
                  optarg);
          exit(EXIT_FAILURE);
        } else {
          sscanf(optarg, "%lf", &pleak);
        }
        break;
      case 'f':
        if (!is_double(optarg)) {
          fprintf(stderr, "Option -f expects a float. Provided: '%s'. Exiting.\n",
                  optarg);
          exit(EXIT_FAILURE);
        } else {
          sscanf(optarg, "%lf", &pfault);
        }
        break;
      case 's':
        if (!is_int(optarg)) {
          fprintf(stderr, "Option -s expects an integer 0 or 1. Provided: '%s'. Exiting.\n",
                  optarg);
          exit(EXIT_FAILURE);
        } else {
          set = atoi(optarg) == 0 ? false : true;
        }
        break;
      case 'o':
        if (!is_int(optarg)) {
          fprintf(stderr, "Option --t_output expects an integer. Provided: '%s'. Exiting.\n",
                  optarg);
          exit(EXIT_FAILURE);
        } else {
          t_output = atoi(optarg);
        }
        break;
      case 'j':
        if (!is_int(optarg)) {
          fprintf(stderr, "Option -j expects an integer. Provided: '%s'. Exiting.\n", optarg);
          exit(EXIT_FAILURE);
        } else {
          cores = atoi(optarg);
        }
        break;
      case GLITCH_OPT:
        glitch = true;
        break;
      case TRANSITION_OPT:
        transition = true;
        break;
      default:
        usage();
    }
  }

  while (optind < argc) {
    if ((strcmp(argv[optind], "constr")   == 0) ||
        (strcmp(argv[optind], "NI")   == 0) ||
        (strcmp(argv[optind], "SNI")  == 0) ||
        (strcmp(argv[optind], "freeSNI")  == 0) ||
        (strcmp(argv[optind], "IOS")  == 0) ||
        (strcmp(argv[optind], "PINI") == 0) ||
        (strcmp(argv[optind], "RP")   == 0) ||
        (strcmp(argv[optind], "RPC")  == 0) ||
        (strcmp(argv[optind], "RPE")  == 0) ||
        (strcmp(argv[optind], "CNI") == 0)  ||
        (strcmp(argv[optind], "CRP") == 0)  ||
        (strcmp(argv[optind], "CRPC") == 0) ||
        (strcmp(argv[optind], "cardRPC") == 0)) {
      property = argv[optind];
    } else {
      if (filename) {
        fprintf(stderr, "I don't know what to do with extra argument '%s'.\n\n",
                argv[optind]);
        usage();
      }
      if (access(argv[optind], R_OK) == 0) {
        filename = argv[optind];
      } else {
        fprintf(stderr, "I don't know what to do with argument '%s'. It does not correspond to an existing filename, nor a property RP/RPC/RPE.\n\n",
                argv[optind]);
        usage();
      }
    }
    optind++;
  }

  if (!property) {
    fprintf(stderr, "Mandatory property argument missing. What do you expect me to compute? :'(\n\n");
    usage();
  }

  if (!filename) {
    fprintf(stderr, "Mandatory argument <filename> missing.\n\n");
    usage();
  }

  if (((strcmp(property, "NI")   == 0) ||
       (strcmp(property, "SNI")  == 0) ||
       (strcmp(property, "freeSNI")  == 0) ||
       (strcmp(property, "IOS")  == 0) ||
       (strcmp(property, "PINI") == 0) ||
       (strcmp(property, "RPC")  == 0) ||
       (strcmp(property, "RPE")  == 0)) &&
      (t == -1)) {
    fprintf(stderr, "When computing property %s, argument -t T is mandatory. \n\n",
            property);
    usage();
  }

  if (((strcmp(property, "CNI")   == 0) ||
       (strcmp(property, "CRPC") == 0)) &&
      ((t == -1) || (k==-1))) {
    fprintf(stderr, "When computing property %s, arguments -t T and -k K are mandatory. \n\n",
            property);
    usage();
  }

  if ((strcmp(property, "CRP")   == 0) &&
      (k==-1)) {
    fprintf(stderr, "When computing property %s, argument -k K are mandatory. \n\n",
            property);
    usage();
  }

  if (t != -1 && t_output == -1) {
    t_output = t;
  }

  ParsedFile * pf = parse_file(filename);
  pf->glitch = glitch;
  pf->transition = transition;

  int characteristic = pf->characteristic;
  Circuit* circuit;
  
  if (characteristic == 2){
    circuit = gen_circuit(pf, glitch, transition, NULL);
    //print_circuit(circuit);
  }
  else{ 
    circuit = gen_circuit_arith(pf, characteristic);
    //print_circuit_arith(circuit);  
  }

  printf("Gadget with %d input(s),  %d output(s),  %d share(s)\n"
         "Total number of intermediate variables : %d\n"
         "Total number of variables : %d\n"
         "Total number of Wires : %d\n",
         circuit->secret_count, circuit->output_count, circuit->share_count,
         circuit->length,
         circuit->deps->length,
         circuit->total_wires);

  if(pf->nb_duplications){
    printf("Total number of duplications: %d\n\n", circuit->nb_duplications);
  }
  else{
    printf("\n");
  }

  if (circuit->length + circuit->output_count * circuit->share_count * circuit->nb_duplications > 255) {
    if (sizeof(Var) < 2) {
      fprintf(stderr, "This circuit contains more than 255 variables, and cannot be processed by this version of IronMask as it was compiled. Change Comb to uint16_t instead of uint8_t, and recompile. Exiting.\n");
      exit(EXIT_FAILURE);
    }
  }

  initialize_table_coeffs();
  time_t start, end;
  time(&start);
  if (strcmp(property, "constr") == 0) {
    compute_RP_coeffs_incompr(circuit, coeff_max, verbose);
  } else if (strcmp(property, "NI") == 0) {
    compute_NI(circuit, cores, t);
  } else if (strcmp(property, "SNI") == 0) {
    compute_SNI(circuit, cores, t);
  } else if (strcmp(property, "PINI") == 0) {
    compute_PINI(circuit, cores, t);
  } else if (strcmp(property, "freeSNI") == 0) {
    compute_freeSNI(circuit, cores, t);
  } else if (strcmp(property, "IOS") == 0) {
    compute_IOS(circuit, cores, t);
  } else if (strcmp(property, "RP") == 0) {
    compute_RP_coeffs(circuit, cores, coeff_max, opt_incompr);
  } else if (strcmp(property, "RPC") == 0) {
    compute_RPC_coeffs(circuit, cores, coeff_max, opt_incompr, t, t_output);
  } else if (strcmp(property, "RPE") == 0) {
    compute_RPE_coeffs(circuit, cores, coeff_max, t, t_output);
  } else if (strcmp(property, "cardRPC") == 0) {
    env_cRPC(circuit, coeff_max, cores);
  } else if (strcmp(property, "CNI") == 0) {
    compute_CNI(pf, cores, t, k, set);
  } else if (strcmp(property, "CRP") == 0) {
    if(pleak != -1 && pfault != -1){
      compute_CRP_val(pf, coeff_max, k, pleak, pfault, set);
    } else{
      compute_CRP_coeffs(pf, cores, coeff_max, k, set);
    }
  } else if (strcmp(property, "CRPC") == 0) {
    if(pleak != -1 && pfault != -1){
      compute_CRPC_val(pf, coeff_max, k, t, pleak, pfault, set);
    }
    else{
      compute_CRPC_coeffs(pf, cores, coeff_max, k, t, set);
    }
  } else {
    fprintf(stderr, "Property %s not implemented. Exiting.\n", property);
    exit(EXIT_FAILURE);
  }
  time(&end);
  uint64_t diff_time = (uint64_t)difftime(end, start);

  printf("\nVerification completed in %" PRIu64 " min %" PRIu64 " sec.\n",
         diff_time / 60, diff_time % 60);

  free_parsed_file(pf);
  free_circuit(circuit);
  return EXIT_SUCCESS;
}
