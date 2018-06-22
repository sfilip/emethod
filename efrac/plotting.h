#ifndef PLOTTING_H
#define PLOTTING_H

#include <mpfr.h>
#include <gmp.h>
#include <string>
#include <mpreal.h>
#include <vector>
#include <functional>

void plotFunc(std::string &filename,
              std::function<mpfr::mpreal(mpfr::mpreal)> &f, mpfr::mpreal &a,
              mpfr::mpreal &b, mp_prec_t prec = 165ul);

void plotFuncs(std::string &filename,
               std::vector<std::function<mpfr::mpreal(mpfr::mpreal)>> &fs,
               mpfr::mpreal &a, mpfr::mpreal &b, mp_prec_t prec = 165ul);

void plotVals(std::string &filename,
              std::vector<std::pair<mpfr::mpreal, mpfr::mpreal>> &p,
              mp_prec_t prec = 165ul);

void plotFuncEtVals(std::string &filename,
                    std::function<mpfr::mpreal(mpfr::mpreal)> &f,
                    std::vector<std::pair<mpfr::mpreal, mpfr::mpreal>> &p,
                    mpfr::mpreal &a, mpfr::mpreal &b, mp_prec_t prec = 165ul);

#endif
