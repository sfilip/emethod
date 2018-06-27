# E-methodHW – Polynomial and Rational Function Approximations for FPGAs

***
***

## Project Overview

This project proposes an automatic method for the evaluation of functions via polynomial or rational approximations and its hardware implementation, on FPGAs.

These approximations are evaluated using Ercegovac's iterative E-method <sup>[1](#ref1)</sup>  <sup>[2](#ref2)</sup>  adapted for FPGA implementation.

The polynomial and rational function coefficients are optimized such that they satisfy the constraints of the E-method.

If you would like to find more technical details, as well as comparisons with other tools that offer hardware function approximations, have a look in the [research report]. You can also have a look at the [slides] of the presentation that Silviu-Ioan Filip gave at ARITH25, presenting the project.

---

<a name="ref1">1</a>: M. D. Ercegovac, “A general method for evaluation of functions and computation in a digital computer,” Ph.D. dissertation, Dept. of Computer Science, University of Illinois, Urbana-Champaign, IL, 1975.

<a name="ref2">2</a>: M. D. Ercegovac, “A general hardware-oriented method for evaluation of functions and computations in a digital computer,” IEEE Trans. Comput., vol. C-26, no. 7, pp. 667–680, 1977.

[research report]: ./doc/e-metod_research_report.pdf
[slides]: ./doc/slides_arith2018_Silviu_Filip.pdf

***
***

## Installation Instructions

**E-methodHW** depends on the *efrac* library and on *FloPoCo*.
In turn, *efrac* depends on: [gmp], [mpfr], [mpreal], [fplll], [eigen] and [qsopt_ex].
You can follow instructions on the respective websites in order to install them on your system.
*[FloPoCo]* has a pretty extensive list of dependencies, that you can check on the project's page. Alternatively, you can use their [one-line install] to get everything ready (you can leave out the download and install of the actual library, as everything required is provided in this repository).
Alternatively (and hoping you have one of the supported distributions) here is the required shell command:
```sh
sudo apt-get install g++ libgmp3-dev libmpfr-dev libfplll-dev libxml2-dev bison libmpfi-dev flex cmake libboost-all-dev libgsl0-dev && wget https://gforge.inria.fr/frs/download.php/33151/sollya-4.1.tar.gz && tar xzf sollya-4.1.tar.gz && cd sollya-4.1/ && ./configure && make -j4 && sudo make install
```

[gmp]: https://gmplib.org/
[mpfr]: https://www.mpfr.org/
[mpreal]: www.holoborodko.com/pavel/mpfr/
[fplll]: https://github.com/fplll/fplll
[eigen]: https://eigen.tuxfamily.org/
[qsopt_ex]: https://github.com/jonls/qsopt-ex

[FloPoCo]: http://flopoco.gforge.inria.fr/
[one-line install]: http://flopoco.gforge.inria.fr/flopoco_installation.html

In order to use **E-MethodHW** you need to start by either `cloning` (follow the instructions at the top of this page) or `downloading` and then unpacking the archive.
Asuming that the extracted project is in the `emethod` directory,  building the project can be done using the following commands:

```sh
cd emethod
cd main
cmake .
make all
```
There are several build options, that can be passed directly to cmake.
```sh
-DCFG_TYPE=Release|Debug
```
set by default to `Release`, will generate debug information and show debug messages during compilation if set to `Debug`.
```sh
-DCFG_FLOPOCO=Lib|LibExec
```
set by default to `Lib`, allows building FloPoCo as either library only, or library and executable (useful for comparing the performance of E-methodHW, as described in the research report).

If you would like to remove the files generated during build or subsequent runs of `emethodHW`, type:
```sh
make clean-all
```
If you would like to remove the files generated during the build of `emethodHW` type:
```sh
make clean-cmake-files
```
If you would like to remove only the files generated during the execution of `emethodHW`, type:
```sh
make clean-run-files
```

***
***

## Usage

***
***