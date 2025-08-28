Generation of incompressible tuples
===


## Algorithm

#### Initialization

Create map m from variables to sets of expressions:

```
    { var -> all expressions containing var }
```
    (see [google doc](https://docs.google.com/drawings/d/12BclEJr3eI1WhrILxAvCNTDHjbvQjNu_zq5Vg8IU77Y/edit) for example)

We now have a map:

```
   vars = { a0 => [expr1, expr2, expr3...],
            a1 => [expr4, expr5, expr6...],
            a2 => [expr7, expr8, expr9...],
            r0 => ...
            r1 => ... }
```

**Observation**: all incompressible tuples have at least one element
from `m[a0]`, one from `m[a1]` and one from `m[a2]` + optionally some
elements from `m[rx]` for `rx` \in `{ r0, r1, ... }`.


#### Main Algorithm

Assuming that there is a single secret for simplicity; but everything
works the same if we say "for each secret" around the whole thing.

**Data-structures and variables**:
 
 - expressions are represented using a vector where a bit set at index
   i means that this expression linearly depends on the i-th
   input/random. For instance, assuming a circuit with 1 input and 2
   randoms, `(0, 1, 1)` is a xor between the two randoms, and `(1, 0, 0)` 
   is simply the first share of the inputs. In IronMask and IronMAsk+, 
   shares of the inputs are represented using powers of 2: `(1, 0, 0)` 
   is the 1st share, `(4, 0, 0)` is the third. In IronMaskArithmetic, 
   expressions are reprensented using a vector where 0 is set at index i means 
   that this expression does not linearly depend on the i-th input/random. In 
   this case, `(0, 0, 0, 1, 1)` is a xor between the two randoms, 
   `(1, 0, 0, 0, 0)` is simply the first share and `(0, 0, 1, 0, 0)` is the 
   third share.

 - `vars` is the map defined above.
 
 - `t` is a tuple. It is represented as an array of expressions
   (meaning that it's an array of vectors since expressions are
   represented using vectors)
   
 - `gauss_deps` contains the value of the expressions inside `t`
   after gauss elimination.
   
 - `gauss_rands` contains the randoms used for the gauss
   elimination.
   
 - `revealed_secret` is an integer representing the part of the secret
   that is already revealed by the current tuple. 5 means the 1st and
   3rd share are revealed. 6 means that the 3rd and 2nd shares are
   revealed.
   
 - `unmask_idx`, used in `add_random` is the index of the element of
   the current tuple (`t`) that we currently want to "unmask" by
   adding a new expression/variable in the tuple.
   
   
**Pseudo-code**:

```
proc build_all_incompressible_failures(vars:map):
    for each combination t of elements from m[a0], m[a1], m[a2]:
        initialize_gauss(t, vars)


proc initialize_gauss(t:tuple, vars:map):
    gauss_deps = []
    gauss_rands = []
    revealed_secret = 0
    
    for i = 0 to t.size:
        gauss_deps[i] = t[i]
        for j = 0 to i-1:
            if gauss_deps[i] contains gauss_rands[j]:
                gauss_deps[i] ^= gauss_deps[j]  # Note: this is a vector addition
        gauss_rands[i] = "The first rand of gauss_deps[i]"
        if gauss_deps[i] does not contain any random:
            revealed_secret |= gauss_deps[0] # assuming the secret is the first dependency
            
    add_randoms(t, vars, gauss_deps, gauss_rands, revealed_secret, 0)
    
    
proc add_random(t:tuple, m:map, gauss_deps:dependency list, gauss_rands:int list,
                revealed_secret:int, unmask_idx:int):
    if t reveals a secret: (revealed_secret == all shares)
        check if it's incompressibles
        add t to the list of all failure tuples
        return
        
    if unmask_idx == t.size:
        return
        
    if revealed_secret already contains the secret shares of gauss_deps[unmask_idx]
        # TODO: maybe we should actually remove t[unmask_idx] in that case...
        add_random(t, m, gauss_deps, gauss_rands, revealed_secret, unmask_idx+1)
        return
    else:
        add_random(t, m, gauss_deps, gauss_rands, revealed_secret, unmask_idx+1)
        # Note: no return here!
    
    rand_idx = gauss_rands[unmask_idx]
    for each element e of vars[rand_idx] that is not already in t:
        t' = [t, e]
        i = t.size
        gauss_deps[unmask_idx+1] = e
        
        if e contains the same secret shares as gauss_deps[unmask_idx]: ### CORRECT?
            continue to next iteration
        
        for j = 0 to i-1:
            if gauss_deps[i] contains gauss_indicies[j]:
                gauss_deps[i] ^= gauss_deps[j]  # Note: this is a vector addition
                
        if gauss_deps[i] does not contain any random:
            add_random(t', m, gauss_deps, gauss_rands, 
                       revealed_secret | gauss_deps[0],
                       unmask_idx+1)
        else:
            add_random(t', m, gauss_deps, gauss_rands, 
                       revealed_secret,
                       unmask_idx+1)
```

The procedure `build_all_incompressible_failures` will build a list of
failures tuples. Some of them are not incompressibles though. To
filter out the incompressible tuples, we can simply do:

```
# Assuming the variable "all_failures" contains the failures generated by build_incompr_tuples
incompr_tuples = make_trie();
for i = 0 to <some_length>: 
    for each tuple t of size i in all_failures:
        if no subtuples of t are in incompr_tuples:
            add t to incompr_tuples
```

And now, `incompr_tuples` is a trie containing all incompressible
failure tuples.


**High-level**:

Let _n_ be the number of shares of the gadget.

```
proc gen_all_failures(n:number of shares, c:circuit):
    for each tuple t of n or less elements of c containing n shares:
        unmask_tuple(t, 0)
        
proc unmask_tuple(c:circuit, t:tuple, unmask_index:int):

    if t is a failure:
        check if it is incompressible and add it to the list of incompressible failures
        stop
        
    if unmask_index > length(t):
        stop
        
    unmask_tuple(c, t, unmask_index+1)
    
    t_gauss = result of gauss elimination on t
    
    r = random chosen for the gauss elimination on element unmask_index of t
    
    for all variables v of c containing r and not in t:
    
       unmask_tuple(c, [t, v], unmask_index+1)
```

Optimizations of this high-level description:

 - Do not redo the full Gauss elimination every time
 
 - Do not fully recheck "if t is a failure" every time
 

## Non-linear gadgets


We consider non-linear gadgets that are of the form "refresh inputs;
multiply; refresh outputs". 

**Overview.** The algorithm is fairly similar to the one for linear
gadgets, except that 2 recursion are needed: one on the output
randoms, and the other one on the input randoms. The idea is the
following:

 - Starting from a tuple `t`, perform a Gauss elimination on its
   output randoms to obtain `t'`.
   
 - Extract a tuple `t1` from `t'` corresponding to the input of interest
   (first or second).
   
 - Perform a Gauss elimination of `t1` (which will be on input
   randoms, by construction) to obtain `t1'`
   
 - Now, the 2 recursions:
    + Add a variable to unmask an element of `t'` and recurse
    + Or, add a variable to unmask an element of `t1'`, and recurse

**In practice**,

  - Both Gauss eliminations are performed along the way (ie, we don't
    recompute them entirely at every stage)
    
  - After adding an element to the tuple, we perform a Gauss
    elimination on that element, and extract a new tuple `t1` that we
    add to the previous `t1` (ie, we don't perform a full extraction
    followed by a full Gauss elimination after adding an element to
    the tuple).
    
  - Similarly to linear gadgets, we have an `unmask_idx_out` variable
    that indicates which element of `t` should be unmasked next, and a
    variable `unmask_idx_in` that indicates which element of `t1`
    should be unmasked next. This introduces two additional recursions:
      + One trying to skip `unmask_idx_out`
      + One trying to skip `unmask_idx_in`


### Optimzations

#### Preventing identical recursive calls (1)

The two recursions mentioned in the last bullet of the **In practice**
paragraph lead to an issue, as can be seen on the following toy
example (where `a` and `b` represent `unmask_idx_out` and
`unmask_idx_in`):

```
                       a,b
                    /      \
                  /          \
                /              \
           a+1,b                 a,b+1
          /    \                 /    \
         /      \               /      \
        /        \             /        \
     a+2,b    a+1,b+1        a+1,b+1    a,b+1
```

As you can see, we end up twice on `a+1,b+1`, and going further
would result in even more nodes being reached multiple times. To
prevent this from happening, we forbid left-then-right recursion,
ie, if we recursed on the left (ie, incremented `a`), then the next
recursion cannot be to the right (ie, it cannot increment `b`). In
our case, we take "left" recursion as being on `unmask_idx_out` and
"right" recursion as being on `unmask_idx_in`. 

We implement this by adding a parameter `out_rec` to our recursion. If
`out_rec` is true, then the previous recursion was on
`unmask_idx_out`, and the next one cannot be on `unmask_idx_in`. Note
that this only stands for recursive calls that skip indices, ie, not
for recursive calls that extend the main tuple (because it does not
work then, but I do not know why).
  

#### Delaying the start of recursion on `unmask_idx_in`

The recursion on `unmask_idx_in` only starts once the Gauss
elimination on `t` reveals all shares of the secret (possibly masked
by input randoms). This avoids some recursion.

This optimization makes sense: there is no need to try to unmask
secret shares if not all secret shares are even available.


#### Preventing identical recursive calls (2)

Recall that there are four recursive calls:
  1- Skipping to `unmask_idx_out+1`, `unmask_idx_in`
  2- Skipping to `unmask_idx_out`, `unmask_idx_in+1`
  3- Unmasking `unmask_idx_out` and moving on to `unmask_idx_out+1`, `unmask_idx_in`
  4- Unmasking `unmask_idx_in` and moving on to `unmask_idx_out`, `unmask_idx_in+1`
  
Despite the aforementioned optimization, some states are still reached
multiple times. 

The secondary recursion on `unmask_idx_in` (bullet 4) thus starts only
once the secondary recursion on `unmask_idx_out` (bullet 3) reaches
the end of the tuple. I'm not sure why this is correct, but this seems
correct...


## Broken Optimzations

### Skipping unmasking of elements that contain already-revealed secret shares (does not work)

During main recursion, we would like to skip indices that contain
already revealed secret. For instance, in the tuple:

    (a0, a0 ^ r0, a1 ^ r2)

We could skip `a0 ^ r0` since `a0` is already revealed. However,
this causes some issues in some cases. Consider for instance:

    (a0, a0 ^ r0 ^ r1, a1 ^ r0 ^ r2)

After Guass elimination, that gives us:

    (a0, r0, r1)

We would be tempted to skip the 2nd element (since a0 is already
revealed), and thus try to unmask only the last element. To do so,
we need to add a variable containing `r1`. However, by doing so, we
would miss the fact that adding `r0 ^ r2` also unmasks the 3rd
element: the tuple

    (a0, a0 ^ r0 ^ r1, a1 ^ r0 ^ r2, r0 ^ r2)

Becomes, after Gauss elimination:

    (a0, r0, r1, a1)

**TODO**: This optimization improves _greatly_ performance. Would be
nice to find a condition that allows to perform it.


### Conditionally doing the "skip" reccursive call

Recall that when reaching some `unmask_idx`, we always try to not
unmask the element at index `unmask_idx`, and skip to index
`unmask_idx+1` right away. This recursive call is actually quite
expensive.

The logic behind this recursive call is the following: it's possible
that the element at index `unmask_idx` does not need to be unmasked
because another element `e` of the tuple contains the same secret
shares, and unmasking `e` will have the same effect as unmasking the
element at index `unmask_idx`.

The idea of this optimization is thus to only do this recursive call
when another element of the tuple actually contains the same secret
shares.

In practice, it seems correct, and it is great for performance. I'm
having a hard time convincing myself that it is correct though.


### Associating specific secret shares to each element of the tuple


Initially, when building the tuple, the elements we start with are
selected so that each of them leaks a specific secret share (or
multiple secret shares in the case of multiplications). 

Then, when trying to unmask an element of the tuple, we do so in order
to reveal the secret share that this element was added in the tuple for.

For instance, consider

    (a0 ^ r0, a1 ^ r1, a2 ^ r2)
    
Here, each element clearly contains a secret share. When trying to
unmask the first element, we could add for instance `r0 ^ r1`, and end
up with:

    (a0 ^ r0, a1 ^ r1, a2 ^ r2, r0 ^ r1)
    
This tuple leaks `a0` and `a1`. Thus, when arriving to the second
element (`a1 ^ r1`), we notice that the secret share _it was added
for_ is already leaks, and we would thus like to skip this
element. (in this specific case, this is probably OK)

This idea of this optimization is thus to assign to each element of
the tuple one or more secret share, and before trying to unmask this
element, we first make sure that not all of the secrets shares
associated to it are revealed. For a more concrete example, consider:

    (a0 ^ r0, a1 ^ r1)
    
And say we add to this tuple `a1 ^ r0 ^ r2`, we thus get

    (a0 ^ r0, a1 ^ r1, a1 ^ r0 ^ r2)
    
The last element of this tuple was added to unmask `a0`, and
therefore, we associate to it the secret share `a0`, even though it
also contains `a1`: `a1` will be revealed through some other path
(namely, through `a1 ^ r1`).

However, this does not work. In the gadget
`GADGETS/Crypto2020_Gadgets/gadget_add_2_o2.sage`, the following tuple
is incompressible, and yet is not generated when this optimization is
used:

    [ 4 6 13 14 21 23 ]

Where the dependencies are

    14: { 1 1 1 0 0 0 1 0 }  [c0]
    4:  { 0 2 0 0 0 0 0 0 }  [b1]
    23: { 4 4 1 0 1 1 0 0 }  [c2]

    6:  { 0 0 1 0 0 0 0 0 }  [r0]
    21: { 4 0 0 0 1 1 0 0 }  [c2]
    13: { 1 0 1 0 0 0 1 0 }  [c0]

Or, using a more human-friendly notation:

    [ a0 ^ b0 ^ r0 ^ r4,
      b1,
      a2 ^ b2 ^ r0 ^ r2 ^ r3,
      r0,
      a2 ^ r2 ^ r3,
      a0 ^ r0 ^ r4 ]

Here is what happens when processing this tuple:

    base tuple:
    a2 ^ b2 ^ r0 ^ r2 ^ r3  // b2
    b1                      // b1
    a0 ^ b0 ^ r0 ^ r4,      // b0

    after gauss elim
    a2 ^ b2 ^ r0 ^ r2 ^ r3               // b2
    b1                                   // b1
    a0 ^ b0 ^ r4 ^ a2 ^ b2 ^ r2 ^ r3     // b0

    + r0 (& gauss elim)
    a2 ^ b2 ^ r0 ^ r2 ^ r3               // b2
    b1                                   // b1
    a0 ^ b0 ^ r4 ^ a2 ^ b2 ^ r2 ^ r3     // b0
    a0 ^ b0 ^ r4                         // b0 (because it was added to unmask the 1st elem)

    + a2 ^ r2 ^ r3 (& gauss elim)
    a2 ^ b2 ^ r0 ^ r2 ^ r3               // b2
    b1                                   // b1
    a0 ^ b0 ^ r4 ^ a2 ^ b2 ^ r2 ^ r3     // b0
    a0 ^ b0 ^ r4                         // b2 (because it was added to unmask the 1st elem)
    b2                                   // b0 (because it was added to unmask the 3rd elem)

    ~ Stopping because the 4th elem (a0 ^ b0 ^ r4) relates to b2, and b2 is already revealed.

Note that his would not have been an issue is the "base tuple" had
been in another order. For instance:

    base tuple:
    a0 ^ b0 ^ r0 ^ r4,      // b0
    b1                      // b1
    a2 ^ b2 ^ r0 ^ r2 ^ r3  // b2

    after gauss elim
    a0 ^ b0 ^ r0 ^ r4,                   // b0
    b1                                   // b1
    a2 ^ b2 ^ r2 ^ r3 ^ a0 ^ b0 ^ r4     // b2

    + r0 (& gauss elim)
    a0 ^ b0 ^ r0 ^ r4,                   // b0
    b1                                   // b1
    a2 ^ b2 ^ r2 ^ r3 ^ a0 ^ b0 ^ r4     // b2
    a0 ^ b0 ^ r4                         // b0 (because it was added to unmask the 1st elem)

    + a2 ^ r2 ^ r3 (& gauss elim)
    a0 ^ b0 ^ r0 ^ r4,                   // b0
    b1                                   // b1
    a2 ^ b2 ^ r2 ^ r3 ^ a0 ^ b0 ^ r4     // b2
    a0 ^ b0 ^ r4                         // b0 (because it was added to unmask the 1st elem)
    b2                                   // b2 (because it was added to unmask the 3rd elem)


    + a0 ^ r0 ^ r4 (& gauss elim)
    a0 ^ b0 ^ r0 ^ r4,                   // b0
    b1                                   // b1
    a2 ^ b2 ^ r2 ^ r3 ^ a0 ^ b0 ^ r4     // b2
    a0 ^ b0 ^ r4                         // b0 (because it was added to unmask the 1st elem)
    b2                                   // b2 (because it was added to unmask the 3rd elem)
    b0                                   // b0 (because it was added to unmask the 4th elem)

