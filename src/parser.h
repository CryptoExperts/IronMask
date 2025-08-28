#pragma once

#include "circuit.h"
#include "utils.h"

ParsedFile * parse_file(char* filename);

void free_parsed_file(ParsedFile * parsed);

Circuit* gen_circuit(ParsedFile * pf, bool glitch, bool transition, Faults * fv);
Circuit* gen_circuit_arith(ParsedFile * pf, int characteristic);

