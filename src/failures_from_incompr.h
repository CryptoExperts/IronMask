#pragma once

#include "circuit.h"
#include "trie.h"

void compute_failures_from_incompressibles(const Circuit* c, Trie* incompr,
                                           int coeff_max, int verbose);
