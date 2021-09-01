
## Misc

### Ideas to improve generation of incompressible tuples

 - Generate tuples that leak a single share (faster than generating
   tuples that leak all shares), then combine them to leak all shares.
   
 - Build relations of variables that leak, and combine those together
   to build incompressible tuples. This would remove some recursion
   from constructive(-mult).c and improve its performance. The idea
   would be something like: "every time variable v1 is added to a
   tuple, variable v2, v3, or v4 are needed as well to unmask
   v1. Thus, never try to add v1 alone, but always with v2, v3 or v4".
   

## Profile and optimize


### Remove some pointers

Right now, we have _a lot_ of pointers everywhere. And pointers to
pointers to pointers... Like
`deps->mult_deps->deps[deps->deps[i][k]]->left_ptr`... I'm wondering
if some of those could be removed, in order to reduce the number of
indirections we do.. A bit of benchmarking with `perf` could be useful
there (in particular caches misses, IPC, and time waiting for memory).


### Multi-thread in the properties rather than in verification_rules.c

Right now, the multi-threading is implemented in
verification_rules.c. This is good for maintainability/readability,
but not so much for RP performance.


### Vectorize parts of the application

The vectorization is not straight-forward: both simplification rules
remove tuples from the overall list as they go; that's not easy to
vectorize. Furthermore, when simplifying tuples in the 2nd rule, the
simplification depends on wether a random is found to replace an
element in the tuple, which won't be the same on all elements...
