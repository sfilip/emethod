#include <cstdio>
#include <cstdlib>
#include <functional>
#include <gmp.h>
#include <iostream>
#include <mpfr.h>
#include <mpreal.h>
#include <utility>
#include <vector>

extern "C"
{
#include <qsopt_ex/QSopt_ex.h>
}

#include "cheby.h"
#include "eigenvalue.h"
#include "fpminimax.h"
#include "plotting.h"
#include "diffcorr.h"
#include "remez.h"


void applyRemez(std::vector<mpfr::mpreal> &num,
                std::vector<mpfr::mpreal> &den,
                std::function<mpfr::mpreal(mpfr::mpreal)> &f,
                std::function<mpfr::mpreal(mpfr::mpreal)> &w,
                std::pair<int, int> &type,
                std::pair<mpfr::mpreal, mpfr::mpreal> &dom);

bool efrac(mpfr::mpreal &error,
            std::vector<mpfr::mpreal> &num,
            std::vector<mpfr::mpreal> &den,
            mpfr::mpreal &numScalingFactor,
            std::function<mpfr::mpreal(mpfr::mpreal)> &f,
            std::function<mpfr::mpreal(mpfr::mpreal)> &w,
            mpfr::mpreal &delta,
            mpfr::mpreal &xi, mpfr::mpreal &d1,
            mpfr::mpreal &d2,
            std::pair<int, int> &type,
            std::pair<mpfr::mpreal, mpfr::mpreal> &dom,
            mpfr::mpreal &scalingFactor);
