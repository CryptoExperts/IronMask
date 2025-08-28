This folder contains gadgets implementations for [MaskVerif](https://gitlab.com/benjgregoire/maskverif), [matverif](https://github.com/NicsTr/binary_masking), and [SILVER](https://github.com/Chair-for-Security-Engineering/SILVER).


## How the gadgets were generated

- **MaskVerif**: gadgets were manually generated from VRAPS
  gadgets. Both formats are pretty similar, and this requires adding
  `;` at the end of each line, changing the headers, and using array
  notation for shares (ie, `a0` in VRAPS is `a[0]` in MaskVerif).
  
- **matverif**: gadgets were automatically generated from VRAPS using
  the script [`vraps_to_matverif.pl`](/vraps_to_matverif.pl). This
  script will not work in all situations, but is enough for the ISW
  multiplication and nlogn refresh.
  
- **SILVER**: gadgets were automatically generated from VRAPS using
  the script [`vraps_to_silver.pl`](/vraps_to_silver.pl). Once again,
  this script will probably not work for all VRAPS gadgets.
  
  
## Were to find additional gadgets

To benchmark IronMask, we used some other gadgets of other tools, that
can be found:

- **MaskVerif**: in the [bench
  folder](https://gitlab.com/benjgregoire/maskverif/-/tree/master/benchs)
  of its gitlab repository. (commit
  10f1591ec38e9b24ac6ce2b271026f73698ea0ad)  
  Additionally, the MaskVerif implementations of the Bordes-Karpman
  multiplication are in the [schemes/maskverif
  folder](https://github.com/NicsTr/binary_masking/tree/master/schemes/maskverif)
  of the matverif github (commit 5cfe43f2330f6aaf6282f216572f7bfee13607fc)
  
- **matverif**: in the [schemes
  folder](https://github.com/NicsTr/binary_masking/tree/master/schemes)
  of its github repository. (commit 5cfe43f2330f6aaf6282f216572f7bfee13607fc)
