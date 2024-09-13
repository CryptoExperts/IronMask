#pragma once

#include "circuit.h"
#include "dimensions.h"
#include "utils.h"

void compute_CRPC_coeffs(ParsedFile * pf, int cores, int coeff_max, int k, int t, bool set);

void compute_CRPC_val(ParsedFile * pf, int coeff_max, int k, int t, double pleak, double pfault, bool set);