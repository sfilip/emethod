#ifndef EREMEZ_REMEZ_H
#define EREMEZ_REMEZ_H

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <gmp.h>
#include <iostream>
#include <mpfr.h>
#include <mpreal.h>
#include <utility>
#include <vector>

void infnorm(std::pair<mpfr::mpreal, mpfr::mpreal> &norm,
             std::function<mpfr::mpreal(mpfr::mpreal)> &f,
             std::pair<mpfr::mpreal, mpfr::mpreal> const &dom);

void eremez(std::vector<mpfr::mpreal> &num, std::vector<mpfr::mpreal> &den,
            std::pair<mpfr::mpreal, mpfr::mpreal> const &dom,
            std::pair<int, int> const &type,
            std::function<mpfr::mpreal(mpfr::mpreal)> &f,
            std::function<mpfr::mpreal(mpfr::mpreal)> &w,
            mpfr::mpreal const &d1, mpfr::mpreal const &d2);

void remez(std::vector<mpfr::mpreal> &num, std::vector<mpfr::mpreal> &den,
            std::pair<mpfr::mpreal, mpfr::mpreal> const &dom,
            std::pair<int, int> const &type,
            std::function<mpfr::mpreal(mpfr::mpreal)> &f,
            std::function<mpfr::mpreal(mpfr::mpreal)> &w);

#endif