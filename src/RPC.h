#pragma once

#include "circuit.h"

void compute_RPC_coeffs(Circuit* circuit, int cores, int coeff_max,
                        int opt_incompr, int t, int t_output);
