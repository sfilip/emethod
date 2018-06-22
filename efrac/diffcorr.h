#ifndef EREMEZ_DIFFCORR_H
#define EREMEZ_DIFFCORR_H

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <gmp.h>
#include <iostream>
#include <mpfr.h>
#include <mpreal.h>
#include <utility>
#include <vector>

void diff_corr(std::vector<mpfr::mpreal> &num, std::vector<mpfr::mpreal> &den,
               mpfr::mpreal &errDC, std::pair<int, int> const &type,
               std::vector<mpfr::mpreal> const &x,
               std::function<mpfr::mpreal(mpfr::mpreal)> &f,
               std::function<mpfr::mpreal(mpfr::mpreal)> &w,
               mpfr::mpreal const &d1, mpfr::mpreal const &d2);

void diff_corr(std::vector<mpfr::mpreal> &num, std::vector<mpfr::mpreal> &den,
               mpfr::mpreal &errDC, std::pair<int, int> const &type,
               std::vector<mpfr::mpreal> const &x,
               std::function<mpfr::mpreal(mpfr::mpreal)> &f,
               std::function<mpfr::mpreal(mpfr::mpreal)> &w);

#endif