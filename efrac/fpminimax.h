#ifndef FPMINIMAX_H
#define FPMINIMAX_H

#include <gmp.h>
#include <mpfr.h>
#include <mpreal.h>
#include <fplll.h>
#include <vector>
#include <functional>

void fpminimaxKernel(std::vector<mpfr::mpreal> &lllCoeffs,
                     std::vector<std::vector<mpfr::mpreal>> &svpCoeffs,
                     std::vector<mpfr::mpreal> const &x,
                     std::function<mpfr::mpreal(mpfr::mpreal)> &targetFunc,
                     std::vector<std::function<mpfr::mpreal(mpfr::mpreal)>> &basisFuncs,
                     mp_prec_t prec = 165u);

#endif
