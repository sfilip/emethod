#ifndef EREMEZ_CHEBY_H
#define EREMEZ_CHEBY_H

#include <mpreal.h>
#include <vector>
#include <utility>

void changeOfVariable(std::vector<mpfr::mpreal> &out,
                      std::vector<mpfr::mpreal> const &in,
                      std::pair<mpfr::mpreal, mpfr::mpreal> const &dom);

void evaluateClenshaw(mpfr::mpreal &result,
                      std::vector<mpfr::mpreal> &p,
                      mpfr::mpreal &x,
                      std::pair<mpfr::mpreal, mpfr::mpreal> const &dom);

void evaluateClenshaw(mpfr::mpreal &result, std::vector<mpfr::mpreal> &p,
                      mpfr::mpreal &x);

void evaluateClenshaw2ndKind(mpfr::mpreal &result, std::vector<mpfr::mpreal> &p,
                             mpfr::mpreal &x);

void generateChebyshevPoints(std::vector<mpfr::mpreal> &v, std::size_t n);

void generateChebyshevCoefficients(std::vector<mpfr::mpreal> &c,
                                   std::vector<mpfr::mpreal> &fv, std::size_t n);

void derivativeCoefficients1stKind(std::vector<mpfr::mpreal> &derivC,
                                   std::vector<mpfr::mpreal> &c);

void derivativeCoefficients2ndKind(std::vector<mpfr::mpreal> &derivC,
                                   std::vector<mpfr::mpreal> &c);

#endif //EREMEZ_CHEBY_H
