# IronMask: Versatile Verification of Masking Security

IronMask is a formal verification tool for the probaing and random probing security of masked implementations. The verified properties are:

* Probing security:
  + t-Non-Interference Security (NI)
  + t-Strong Non-Interference Security (SNI)
  + t-Probe Isolating Non-Interference (PINI)
  
* Random probing security:
  + (p, &epsilon;)-Random Probing (RP)
  + (t, p, &epsilon;)-Random Probing Composability (RPC)
  + (t, f)-Random Probing Expandability (RPE)


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
    ironmask [OPTIONS] [P|NI|SNI|PINI|RP|RPC|RPE] FILE
Computes the probing (P, NI, SNI, PINI) or random probing property (RP, RPC, RPE) for FILE

Options:
    -v[num], --verbose[num]             Sets verbosity level.
    -c[num], --coeff_max[num]           Sets the last precise coefficient to compute
                                        for RP-like properties.
    -t[num]                             Sets the t parameter for P/NI/SNI/PINI/RPC/RPE.
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

The parameter `t` is mandatory for all property except RP. When `t_output` is specified, the value of `t` is taken for input shares and the value of `t_output` for output shares. Otherwise, `t` is used for input shares and output shares.

The parameter `coeff_max` specifies the maximum size of tuples to test during the verification (which is also the maximum coefficient in the evaluation of &epsilon; which will be computed exactly). This parameter is not needed for probing verification.


#### Execution Examples

* The following command executes P verification on the gadget `gadget.sage`, checking if it is 2​-NI using 4 cores:

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

* The following command executes RPC verification on the gadget `gadget.sage` with a value of `t = 2` for input and output shares, and stops at the maximum coefficient of 5:

  ```
  ironmask gadget.sage RPC -c 5 -t 2 -v 2
  ```

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

Above is an example of the ISW​ multiplication gadget with 2 shares. 

* `#SHARES 2` is the number of shares used in the gadget
* `#IN a b` are the input variables of the gadget
* `#RANDOMS r0` are all of the random variables used in the gadget
* `#OUT d` is the output variable of the gadget

The next lines are the instructions (or gates) of the gadget. Allowed operations are `+` and `*`. The shares of input/output variables range from 0 to #shares - 1 . To specify the share for each variable, simply use the variable name suffixed by the share number `(eg. a0, b1, d0, ...)​`.  Input variables should be one letter variables in an alphabetical order starting from `a` (`a, b, c, ...`). Output variables should also be one letter variables.

In the robust probing model, you can used the notation `![ ... ]` around an expression to stop the propagation of glitches. For instance, `c1 = ![ a1 * b1 ]`.


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


## License

[GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html)
