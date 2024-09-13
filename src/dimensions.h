#pragma once

// This file offers functions to "reduce the dimensions" of a
// circuit. This means removing variables from a circuit to reduce its
// size and thus reduce the complexity of the enumerative search of
// failure tuples.
//
//  - remove_elementary_wires removes input variables and multiplications
//    of input wires.
//
//  - remove_randoms removes random variables
//
//  - advanced_dimension_reduction uses reduced sets to remove "less
//    powerful" wires. dimensions.c contains more explanations on how
//    this works.
//
// The structure DimRedData is used when calling
// remove_elementary_wires: it contains the wires that were removed,
// which enables this optimization to be used in RP-like properties,
// since it enables failures containing removed wires to be built.
//
// Note that remove_randoms and advanced_dimension_reduction should
// not be used in the random probing model.


#include "circuit.h"
#include "vectors.h"

typedef struct _dim_red_data {
  int* new_to_old_mapping;
  VarVector* removed_wires;
  Circuit* old_circuit;
} DimRedData;


void advanced_dimension_reduction(Circuit* circuit);
DimRedData* remove_elementary_wires(Circuit* circuit, bool print);
void remove_randoms(Circuit* circuit);
void free_dim_red_data(DimRedData* dim_red_data);
