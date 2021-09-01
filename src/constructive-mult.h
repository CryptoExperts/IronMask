#pragma once

#include "circuit.h"
#include "trie.h"

Trie* compute_incompr_tuples_mult(const Circuit* c, int coeff_max, int verbose);
