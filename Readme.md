# IronMask and IronMask+ and IronMaskArithmetic

IronMask is a formal verification tool for the probing and random probing security of masked implementations. Its design is described in ["IronMask: Versatile Verification of Masking Security"](https://eprint.iacr.org/2021/1671).
The verified properties are:

* Probing security:
  + Non-Interference Security (NI)
  + Strong Non-Interference Security (SNI)
  + Probe Isolating Non-Interference (PINI)
  + free-SNI (fSNI)
  + Input Output Separation (IOS)
  
* Random probing security:
  + Random Probing (RP)
  + Random Probing Composability (RPC)
  + Random Probing Expandability (RPE)

IronMask+ is an extension of IronMask able to analyze combined security against both faults and side-channel attcks. Its design is described in [Formal Definition and Verification for Combined Random Fault and Random Probing Security](https://eprint.iacr.org/2024/757).
The additional verified properties are:
* Combined fault and probing security
    + Combined Non-Interference (CNI)
        - t: maximum number of probes to consider in a tuple
        - k: maximum number of faults to consider
        - s: type of faults to consider (set:1, reset:0)
* Combined random security:
    + Known-Fault Random Combined Security (CRP)
        - k: maximum number of faults to consider
        - s: type of faults to consider (set:1, reset:0)
        - c: bound on number of coefficients to compute 
    + Known-Fault Random Combined Composability (CRPC)
        - t: number of output shares to consider
        - k: maximum number of faults to consider
        - s: type of faults to consider (set:1, reset:0)
        - c: bound on number of coefficients to compute

IronMaskArithmetic is another extension of IronMask able to verify probing and 
random probing security properties of masked implementations on prime Arithmetic 
field. The verified properties are :

* Probing security:
  + Non-Interference Security (NI)
  + Strong Non-Interference Security (SNI)

* Random probing security:
  + Random Probing (RP)
  + Random Probing Composability (RPC)
  + Random Probing Expandability (RPE)
  + Cardinal Random Probing Composability (cardRPC)

Note that the last security property (cardRPC) is for now only available for refresh gadgets.


## Installation

IronMask is written in C and uses the `pthreads` (POSIX threads) and
`GMP` (GNU Multiple Precision) libraries. Make sure those are
installed before compiling IronMask. 

By default, the C compiler used is Clang, but any C compiler should
work. You can change the C compiler in `src/Makefile`.

To compile IronMask, run

    make
    
This will produce the `ironmask` binary.


## Usage

```
Usage:
    ironmask [OPTIONS] [NI|SNI|freeSNI|uniformSNI|IOS|PINI|RP|RPC|RPE|cardRPC|CNI|CRP|CRPC|cardRPC] FILE
Computes the probing (NI, SNI, PINI) or random probing property (RP, RPC, RPE) or the combined fault property (CNI) for FILE
Computes the cardinal RPC enveloppes (cardRPC) for refresh gadgets in arithmetic field.

Options:
    -v[num], --verbose[num]             Sets verbosity level.
    -c[num], --coeff_max[num]           Sets the last precise coefficient to compute
                                        for RP-like properties.
    -t[num]                             Sets the t parameter for NI/SNI/PINI/RPC/RPE/CRPC.
    -k[num]                             Sets the k parameter for CNI/CRP/CRPC.
                                        This option is mandatory except when checking RP.
    -o[num], --t_output[num]            Sets the t_output parameter for RPC/RPE.
    -j[num], --jobs[num]                Sets the number of core to use.
                                        If [num] is -1, ironmask uses all cores.
    -i, --incompr-opt                   Enables incompressible tuples optimization.
                                        (this option may or may not work, you should probably
                                        not use it unless you know what you're doing)
    --glitch                            Takes glitches into account.
    --transition                        Takes transitions into account
    -h, --help                          Prints this help information.
```

The parameter `t` is mandatory for all property except RP and cardRPC. When `t_output` is specified, the value of `t` is taken for input shares and the value of `t_output` for output shares. Otherwise, `t` is used for input shares and output shares.

The parameter `coeff_max` specifies the maximum size of tuples to test during the verification (which is also the maximum coefficient in the evaluation of &epsilon; which will be computed exactly). This parameter is not needed for probing verification.


#### Execution Examples

* The following command executes P verification on the gadget `gadget.sage`, checking if it is 2-NI using 4 cores:

  ```
  ironmask gadget.sage NI -t 2 -j 4
  ```

* The following command executes RP verification on the gadget `gadget.sage`, and stops at the maximum coefficient of 5:

  ```
  ironmask gadget.sage RP -c 5
  ```

* The following command executes RPE verification on the gadget `gadget.sage` with a value of `t = 2` for input and output shares, and stops at the maximum coefficient of 5:

  ```
  ironmask gadget.sage RPE -c 5 -t 1 -v 0
  ```

  If t<sub>in</sub> = 2​ and t<sub>out</sub> = 1​ :

  ```
  ironmask gadget.sage RPE -c 5 -t 2 -t_output 1 -v 1
  ```

* The following command executes cardRP verification on the gadget `refresh.sage`, and stops at the maximum coefficient of 8:

  ```
  ironmask refresh.sage cardRPC -c 8
  ```

* The following command executes RPC verification on the gadget `gadget.sage` with a value of `t = 2` for input and output shares, and stops at the maximum coefficient of 5:

  ```
  ironmask gadget.sage RPC -c 5 -t 2 -v 2
  ```

* For CRP and CRPC properties, the verification must be run as follows:

1. Run the command 
    ```
    sage src/test_correction.py -p 'CRP/CRPC' -f gadget.sage -s 'fault_type (1 set, 0 reset)' -k 'nb_faults'
    ```
This command produces the faulty scenarios which cannot be corrected, necessary to compute the value of mu.

2. Run IronMask with the appropriate values for t, k, s and c. The values for k and s must be the same chosen for the first command, for instance
    ```
    ./ironmask gadget.sage -k 'nb_faults' -s 'fault_type' -c 'nb_coeffs' -t 1
    ```

3. Run again IronMask with the same arguments as Step 2, with additional arguments: 
    - -l to specify the desired leakage probability
    - -f to specify the desired fault probability
    For instance,
    ```
    ./ironmask gadget.sage -k 'nb_faults' -s 'fault_type' -c 'nb_coeffs' -t 1 -l 0,001 -f 0,001
    ``` 
    
    This will output the final values for \mu and \epsilon

## Input Format

Input gadget file have to be sage files in the following format :

```
#SHARES 2
#IN a b
#RANDOMS r0
#OUT d

c0 = a0 * b0    
d0 = c0 + r0

c1 = a1 * b1
c1 = c1 + r0
tmp = a0 * b1
c1 = c1 + tmp
tmp = a1 * b0

d1 = c1 + tmp
```

Above is an example of the ISW multiplication gadget with 2 shares. 

* `#SHARES 2` is the number of shares used in the gadget
* `#IN a b` are the input variables of the gadget
* `#RANDOMS r0` are all of the random variables used in the gadget
* `#OUT d` is the output variable of the gadget

The next lines are the instructions (or gates) of the gadget. Allowed operations are `+` and `*`. The shares of input/output variables range from 0 to #shares - 1 . To specify the share for each variable, simply use the variable name suffixed by the share number `(eg. a0, b1, d0, ...)`.  Input variables should be one letter variables in an alphabetical order starting from `a` (`a, b, c, ...`). Output variables should also be one letter variables.

In the robust probing model, you can used the notation `![ ... ]` around an expression to stop the propagation of glitches. For instance, `c1 = ![ a1 * b1 ]`.

For combined security, we add 
```
#DUPLICATIONS 3
```
in the header to indicate that all the input shares are duplicated three thimes. In this example, the shares `a0, a1, b0, b1` are then manipulated with their copies, namely we use `a0_0, a0_1, a0_2` instead of `a0`.

For verification of the gadget on arithmetic prime field (i.e. with IronMaskArithmetic), we add
```
#CAR q
```
in the header which specifies the characteristic of the field. 
Furthermore, we can add coefficients to the instructions of the gadget, an example will be 
```
c0 = 3 a0 * -1 b0
```
The coefficient is placed before the variable, and after the operator. In this way, we add the coefficient 3 at a0 and -1 at b0. 


## Output Format

The output is always a bit verbose, as the first thing being printed is the internal representation of the input gadgets, as well as some numbers on the gadgets (eg, number of inputs/outputs, randoms, intermediate variables....).

Then, depending on the property being verified, some prints will show you the progression of the verification.

Finally, the result is printed.


### Output of Probing Verification (NI, SNI, PINI)

For the verification of probing properties, the tool simply outputs if
for the considered value of `t`, whether the gadget is or not
t-NI/t-SNI/t-PINI.

If the gadget is not NI/SNI/PINI, then one or several failure tuples
are printed to illustrate that.


### Output of Random Probing Verification (RP, RPC, RPE)

When checking RP, RPC or RPE, the coefficients computed for the
function f(p) are shown, as well as the min/max failure probability
computed from f(p). The output is fairly self-explanatory.

### Output of Cardinal Random Probing Composition Verification (cardRPC)

When checking cardRPC security property with a maximum coefficients `c`, the 
output will be a list of coefficients for all $t_{in}$ and $t_{out}$. 
Each coefficient $c_i$ of the list represents the number of tuples of size $i$
which need $t_{in}$ inputs to simulate the leakage and the $t_{out}$ output 
shares leaked. Here is an exemple of output for a refresh gadget with $n = 2$ 
shares :

```
./ironmask gadgets/Arith/refresh/OPRefresh/gadget_OPRefresh_2_shares.sage cardRPC 

in with 1 elements: {a:0, }
randoms with 2 elements: {r0:0, r1:1, }
out with 1 elements: {c:0, }
Gadget with 1 input(s),  1 output(s),  2 share(s)
Total number of intermediate variables : 6
Total number of variables : 8
Total number of Wires : 10
Total number of duplications: 1

tin = 0, tout = 0
 f(p) = [ 1, 8, 28, 56, 70, 56, 28, 8, 1, 0, 0]

tin = 0, tout = 1
 f(p) = [ 1, 6, 6, 2, 0, 0, 0, 0, 0, 0, 0]

tin = 0, tout = 2
 f(p) = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

tin = 1, tout = 0
 f(p) = [ 0, 2, 16, 56, 112, 140, 112, 56, 16, 2, 0]

tin = 1, tout = 1
 f(p) = [ 0, 4, 36, 88, 128, 126, 84, 36, 9, 1, 0]

tin = 1, tout = 2
 f(p) = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

tin = 2, tout = 0
 f(p) = [ 0, 0, 1, 8, 28, 56, 70, 56, 28, 8, 1]

tin = 2, tout = 1
 f(p) = [ 0, 0, 3, 30, 82, 126, 126, 84, 36, 9, 1]

tin = 2, tout = 2
 f(p) = [ 1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1]


Verification completed in 0 min 0 sec.
```



## Internals

The file [`src/getting_started_dev.md`](src/getting_started_dev.md)
explains the structure of the project, the main task of each C file in
`src`, as well as how circuits and tuples are represented and
manipulated. The source code itself is fairly well commented, and
using `src/getting_started_dev.md` to navigate the source should get
you pretty far.

The file
[`src/incompressible-tuples-generation.md`](src/incompressible-tuples-generation.md)
presents our constructive algorithm to generate incompressible tuples,
as well as some ideas and suggestions to improve its performance.

## Tests

Tests are provided to evaluate the reliability of IronMaskArithmetic’s code, 
both with and without parallelization, and for all security properties verified 
by IronMaskArithmetic. To run them, simply execute the script 
`test/test_car_q.sh`.
## License

[GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html)
