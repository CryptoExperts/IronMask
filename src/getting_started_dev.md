Getting started
===

This file describes how the code of IronMask is organized, as
well as what datastructures are used and how.


## Project structure

The main structure is as follows:

```
main.c: entry point.
|
|--- parser.c: parses the input file into a Circuit data structure (cf circuit.h)
|
|
|--- NI.c, SNI.c, RP.c, RPC.c, RPE.c, cardRPC.c, CNI.c, CRP.c, CRPC.c, freeSNI.c, IOS.c : verifies the corresponding property
|     |
|     |--- dimensions.c: reduces the size of the circuit to consider, so that the
|     |                  enumerative search is less expensive. This optimization is only possible for verification of circuits on the binary field.
|     |
|     |--- verification_rules.c: applies simplification rules on tuples to determine
|     |                          whether they are failures or not. This is the core file of IronMask and IronMask+
|     |
|     |--- coeffs.c: computes coefficients from the failures, 
|                    mainly in its compute_tree function
|                    It also computes the failure probability for RP-like properties.
|
|
|--- constructive.c, constructive-mult.c: generates incompressible failure tuples, constructively, for binary field (used for the NI security properties).
     |
     |
     |--- failures_from_incompr.c: generates failure tuples from incompressible failures.
     |
     |--- constructive_arith.c, constructive-mult_arith.c : generates incompressible failure tuples, constructively, for arithmetic field (i.e characteristic $\neq 2$)
                                                            These are the core files of IronMaskArithmetic.  
```

Some additional files are of interest:

 - `config.h` contains some configuration variables, which can be used
   to tweak IronMask a bit. It's fairly well commented; open it to see
   what this is about.

 - `combinations.c` generates combinations of integers (_i.e._, tuples)
 
 - `vectors.c`/`vectors.h` defines vectors structures for different
   data types but with a common API. (would have been much cleaner in
   C++ with templates or standard library, but here we are, with C,
   writting dirty macros...)
    
 - `trie.c` defines a [trie](https://en.wikipedia.org/wiki/Trie) of
   tuples, along with utilities to add/extract tuples to the trie, as
   well as check the presence/absence of a tuple in the trie.
    
 - `hash_tuples.c` defines a hashmap of tuples. It is currently
   unused, and left around "just in case".
   
 - `list_tuples.c` defines a doubly-linked-list of tuples, and some
    utilities to add/remove elements. Currently not used either.
   


## Data structures


### Circuit and dependencies

Once an input file is parsed (by `parser.c`), it is represented as a
`Circuit` data structure (defined in `circuit.h`). The instructions of
the input program are stored as a list of dependencies (structure
`DependencyList` in `circuit.h`). For instance, the program

```
    t1 = a0 ^ r0
    t2 = a1 ^ r1
```

Would be (more or less; see below) represented by the list 

```
    [ [a0, r0], [a1, r1] ]
```

Except that we do not keep variable names around: we associate an
index to each variable of the program, and the dependency list above
thus becomes (assuming that `a0` and `a1` have index `0` and `1` and
`r0` and `r1` have index `2` and `3`):

```
    [ [0, 2], [1, 3] ]
```

To be more precise, the first elements of the dependency list are the
inputs of the program themselves (secret shares and randoms), which
mean that our dependency list is rather:

```
    [ [0], [1], [2], [3], [0, 2], [1, 3] ]
```

Finally, instead of representing those dependency by a list of
indices, we represent them using an array of 0 and 1: 0 at index i
means "no dependency on the i-th variable", while 1 indicates a
dependency on the i-th variable. In IronMask and IronMAsk+, there is one 
exception to this rule: a single index is used for each input, where the bits 
set indicate on which share of the corresponding input the element depends. 
The dependency list above thus is (that's the real one this time):

```
    [ [1, 0, 0],
      [2, 0, 0],
      [1, 1, 0],
      [2, 0, 1] ]
```

Each item of this dependency list is a `Dependency*` (defined in
`circuit.h` as well). Currently, this is implemented as an array of
`int` (32 bits), thus allowing to accept gadgets with up to 32 shares.

This dependency list only represents _linear_ dependencies (ie,
assignments or xors). For each multiplication of the program, we
create a `MultDependency`, which stores the dependencies of its left
and right operands. The main dependency list can then have
dependencies on those MultDependencies. For instance, consider the
program

```
    t0 = a0 ^ r0
    t1 = a0 ^ r1
    t2 = b0 ^ r2
    t3 = b1 ^ r3
    t4 = t0 & t2
    t5 = t1 & r3
    t6 = t4 ^ t5 ^ r4
```

The following `MultDependency` will be created:

```
   MultDependency 1:
     left:  [1, 0, 1, 0, 0, 0, 0]   # corresponds to a0 ^ r0
     right: [0, 1, 0, 0, 1, 0, 0]   # corresponds to b0 ^ r2
     
   MultDependency 2:
     left:  [2, 0, 0, 1, 0, 0, 0]   # corresponds to a1 ^ r1
     right: [0, 2, 0, 0, 0, 1, 0]   # corresponds to b1 ^ r3
     
```

And the main dependency list will be:

```
     inputs |  randoms     | mults
     -------+--------------+------
    [ [1, 0, 0, 0, 0, 0, 0, 0, 0],     # first input
      [2, 0, 0, 0, 0, 0, 0, 0, 0],
      [0, 1, 0, 0, 0, 0, 0, 0, 0],     # second input
      [0, 2, 0, 0, 0, 0, 0, 0, 0],
      [0, 0, 1, 0, 0, 0, 0, 0, 0],     # randoms
      [0, 0, 0, 1, 0, 0, 0, 0, 0],
      [0, 0, 0, 0, 1, 0, 0, 0, 0],
      [0, 0, 0, 0, 0, 1, 0, 0, 0],
      [0, 0, 0, 0, 0, 0, 1, 0, 0],
      [1, 0, 1, 0, 0, 0, 0, 0, 0],     # t0 = a0 ^ r0
      [2, 0, 0, 1, 0, 0, 0, 0, 0],     # t1 = a1 ^ r1
      [0, 1, 0, 0, 1, 0, 0, 0, 0],     # t2 = b0 ^ r2
      [0, 2, 0, 0, 0, 1, 0, 0, 0],     # t3 = b1 ^ r3
      [0, 0, 0, 0, 0, 0, 0, 1, 0],     # t4 = t0 & t2
      [0, 0, 0, 0, 0, 0, 0, 0, 1],     # t5 = t1 & t3
      [0, 0, 0, 0, 0, 0, 1, 1, 1],     # t6 = t4 ^ t5 ^ r4
```


Each element of the dependency list thus correspond to an intermediate
variable or an input (secret share or random). Therefore, a tuple of
size _n_ is a combination of _n_ elements of the dependency list.

#### Optimized bit-representation

This optimization is only available for the verification of gadgets on binary 
fields.

When checking if a tuple is a failure, we use a Gaussian
elimination. One of the operations that is used the most thus consists
in xoring variables. To do this efficiently, we use the `BitDep`
structure (defined in `circuit.h`), which uses an array of 64-bit
integers as a bitvector to represent randoms, and another one to
represent multiplications. 

Consider a 8-share multiplication gadget. It contains 64
multiplications. With this representation, the multiplication part of
two variables can be xored with a single xor instead of 64.

In practice, using this optimization provides a speedup of ~x2
compared to using larger arrays of `Dependency` to store the
variables.


### Tuples and combinations

As mentionned above, a tuple of size _n_ is a combination of _n_
elements of the dependency list. To represent a tuple, we thus use an
array of `Comb` (defined in `combinations.h`). `Comb` is currently a
`uint8_t`, which means that IronMask does not work with circuit
containing more than 255 variables and inputs (to support larger
circuits, simply changing `Comb` to `int` would be enough, although
the memory consumption of the tool would increase).

Notably, we don't have a `Tuple` structure, which would store an array
of `Comb` as well as the size of the array: almost everytime we deal
with tuples, we can split the computation by tuple size, and thus only
have to deal with tuples of the same size. Therefore, we usually use a
variable (that is typically called `comb_len`) to store the size of
the tuples/combinations we are dealing with.

To store lists of combinations, `list_tuples.h` defines a type
`ListComb` (a linked-list of `Comb`), although it's not being use
currently (which makes me wonder if this last paragraph was really
needed...)
