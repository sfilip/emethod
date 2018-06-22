#include "efrac.h"


void applyRemez(std::vector<mpfr::mpreal> &num,
                std::vector<mpfr::mpreal> &den,
                std::function<mpfr::mpreal(mpfr::mpreal)> &f,
                std::function<mpfr::mpreal(mpfr::mpreal)> &w,
                std::pair<int, int> &type,
                std::pair<mpfr::mpreal, mpfr::mpreal> &dom)
{
    // diffcor + remez
    remez(num, den, dom, type, f, w);

    std::function<mpfr::mpreal(mpfr::mpreal)> q;
    std::function<mpfr::mpreal(mpfr::mpreal)> p;

    q = [den](mpfr::mpreal var) -> mpfr::mpreal {
        mpfr::mpreal res = 0;
        for (std::size_t i{0u}; i < den.size(); ++i)
            res += den[i] * mpfr::pow(var, i);
        return res;
    };
    p = [num](mpfr::mpreal var) -> mpfr::mpreal {
        mpfr::mpreal res = 0;
        for (std::size_t i{0u}; i < num.size(); ++i)
            res += num[i] * mpfr::pow(var, i);
        return res;
    };

    std::function<mpfr::mpreal(mpfr::mpreal)> r;
    r = [p, q](mpfr::mpreal var) -> mpfr::mpreal {
        return p(var) / q(var);
    };

    std::function<mpfr::mpreal(mpfr::mpreal)> err;
    err = [f, r, w](mpfr::mpreal var) -> mpfr::mpreal {
        return w(var) * (f(var) - r(var));
    };

    //std::string testFile = "outputError";
    //plotFunc(testFile, err, dom.first, dom.second);
}

bool efrac(mpfr::mpreal & error,
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
           mpfr::mpreal &scalingFactor)
{
    // diffcor + remez
    bool valid = true;
    eremez(num, den, dom, type, f, w, d1, d2);

    // determine the scaling factor for the numerator coefficients such
    // that the emethod condition is satisfied (i.e. |p_k| < xi)
    mpfr::mpreal maxNumVal = mpfr::abs(num[0]);
    for (std::size_t i{1u}; i < num.size(); ++i)
    {
        if (mpfr::abs(num[i]) > maxNumVal)
        {
            maxNumVal = mpfr::abs(num[i]);
        }
    }

    numScalingFactor = mpfr::pow(mpfr::mpreal(2),
                                 mpfr::floor(mpfr::log2(xi / maxNumVal)));
    // std::cout << "Num scale  = " << numScalingFactor << std::endl;
    // std::cout << xi / maxNumVal << std::endl;
    std::cout << "\nNumerator scaling factor = "
              << mpfr::floor(mpfr::log2(xi / maxNumVal))
              << std::endl;

    std::function<mpfr::mpreal(mpfr::mpreal)> q;
    std::function<mpfr::mpreal(mpfr::mpreal)> p;

    q = [den](mpfr::mpreal var) -> mpfr::mpreal {
        mpfr::mpreal res = 0;
        for (std::size_t i{0u}; i < den.size(); ++i)
            res += den[i] * mpfr::pow(var, i);
        return res;
    };
    p = [num](mpfr::mpreal var) -> mpfr::mpreal {
        mpfr::mpreal res = 0;
        for (std::size_t i{0u}; i < num.size(); ++i)
            res += num[i] * mpfr::pow(var, i);
        return res;
    };

    std::function<mpfr::mpreal(mpfr::mpreal)> r;
    r = [p, q, numScalingFactor](mpfr::mpreal var) -> mpfr::mpreal {
        return (p(var) / q(var)) * numScalingFactor;
    };

    std::function<mpfr::mpreal(mpfr::mpreal)> err;
    err = [f, r, w, numScalingFactor](mpfr::mpreal var) -> mpfr::mpreal {
        return w(var) * (f(var) - r(var) / numScalingFactor);
    };

    // code for plotting the approximation error function
    // (requires gnuplot!);
    // std::string testFile = "outputError";
    // plotFunc(testFile, err, dom.first, dom.second);

    // first do a naive rounding of the coefficients
    // and see what that gives as a result
    auto rdNum = num;
    auto rdDen = den;
    // std::cout << "Rounded coefficients\n";
    for (std::size_t i{0u}; i < rdNum.size(); ++i)
    {
        rdNum[i] *= numScalingFactor * scalingFactor;
        rdNum[i] = mpfr::round(rdNum[i]);
        rdNum[i] /= scalingFactor;
        // std::cout << rdNum[i] << std::endl;
    }
    for (std::size_t i{0u}; i < rdDen.size(); ++i)
    {
        rdDen[i] *= scalingFactor;
        rdDen[i] = mpfr::mpreal(rdDen[i]);
        rdDen[i] /= scalingFactor;
        // std::cout << rdDen[i] << std::endl;
    }

    std::function<mpfr::mpreal(mpfr::mpreal)> qRound;
    std::function<mpfr::mpreal(mpfr::mpreal)> pRound;

    qRound = [rdDen](mpfr::mpreal var) -> mpfr::mpreal {
        mpfr::mpreal res = 0;
        for (std::size_t i{0u}; i < rdDen.size(); ++i)
            res += rdDen[i] * mpfr::pow(var, i);
        return res;
    };
    pRound = [rdNum](mpfr::mpreal var) -> mpfr::mpreal {
        mpfr::mpreal res = 0;
        for (std::size_t i{0u}; i < rdNum.size(); ++i)
            res += rdNum[i] * mpfr::pow(var, i);
        return res;
    };

    std::function<mpfr::mpreal(mpfr::mpreal)> rRound;
    rRound = [pRound, qRound, numScalingFactor](mpfr::mpreal var) -> mpfr::mpreal {
        return (pRound(var) / qRound(var)) / numScalingFactor;
    };

    std::function<mpfr::mpreal(mpfr::mpreal)> errRound;
    errRound = [f, rRound, w, numScalingFactor](mpfr::mpreal var) -> mpfr::mpreal {
        return w(var) * (f(var) - rRound(var));
    };

    std::pair<mpfr::mpreal, mpfr::mpreal> errRoundNorm;
    infnorm(errRoundNorm, errRound, dom);
    std::cout << "\nNaive rounding error estimation = " << errRoundNorm.second << std::endl;

    // std::string testFileRound = "outputErrorRound";
    // plotFunc(testFileRound, errRound, dom.first, dom.second);

    // fpminimax

    // construct the basis functions
    // One strategy: for the denominator coefficients that saturate, we set their
    // value to the given bound
    std::vector<std::pair<std::size_t, mpfr::mpreal>> sat;
    std::vector<std::pair<std::size_t, mpfr::mpreal>> nsat;
    for (std::size_t i{1u}; i < den.size(); ++i)
    {
        if (den[i] == d1 || den[i] == d2)
        {
            sat.push_back(std::make_pair(i, den[i]));
        }
        else
        {
            nsat.push_back(std::make_pair(i, den[i]));
        }
    }

    std::vector<std::function<mpfr::mpreal(mpfr::mpreal)>> basis;
    for (std::size_t i{0u}; i < type.first + 1; ++i)
    {
        basis.push_back([scalingFactor, i, w](mpfr::mpreal x) -> mpfr::mpreal {
            return w(x) * mpfr::pow(x, i) / scalingFactor;
        });
    }

    for (std::size_t i{0u}; i < nsat.size(); ++i)
    {
        basis.push_back([r, scalingFactor, i, nsat, w](mpfr::mpreal x) -> mpfr::mpreal {
            return -r(x) * w(x) * mpfr::pow(x, nsat[i].first) / scalingFactor;
        });
    }

    std::function<mpfr::mpreal(mpfr::mpreal)> wr;
    wr = [r, w, sat](mpfr::mpreal x) -> mpfr::mpreal {
        mpfr::mpreal result = w(x) * r(x);
        for (auto &it : sat)
        {
            result += w(x) * r(x) * mpfr::pow(x, it.first) * it.second;
        }
        return result;
    };

    std::vector<mpfr::mpreal> lllCoeffs;
    std::vector<std::vector<mpfr::mpreal>> svpCoeffs;

    std::size_t discSize = type.first + type.second + 1u;
    std::vector<mpfr::mpreal> disc(discSize);
    generateChebyshevPoints(disc, discSize);
    changeOfVariable(disc, disc, dom);

    fpminimaxKernel(lllCoeffs, svpCoeffs, disc, wr, basis);
    // std::cout << "fpminimax coefficients:\n";
    // for (auto &it : lllCoeffs)
    //    std::cout << it.toLong() << std::endl;

    std::vector<mpfr::mpreal> finalCoeffs;
    // first add the numerator coefficients
    for (std::size_t i{0u}; i <= type.first; ++i)
    {
        finalCoeffs.push_back(lllCoeffs[i] / scalingFactor);
    }
    for (std::size_t i{0u}; i < lllCoeffs.size() - type.first - 1u; ++i)
    {
        nsat[i].second = lllCoeffs[type.first + 1u + i] / scalingFactor;
    }

    // now add the denominator coefficients
    std::size_t satIndex{0u};
    std::size_t nsatIndex{0u};
    for (std::size_t i{1u}; i <= type.second; ++i)
    {
        if (satIndex < sat.size() && sat[satIndex].first == i)
        {
            finalCoeffs.push_back(sat[satIndex].second);
            ++satIndex;
        }
        else
        {
            finalCoeffs.push_back(nsat[nsatIndex].second);
            ++nsatIndex;
        }
    }

    // check to see that the quantized coefficients in the denominator
    // adhere to the magnitude constraints
    
    //std::cout << "Denominator magnitude bounds and computed values:\n";
    for (int i{1}; i <= type.second; ++i)
    {
        //std::cout << d1 << " " << d2 << " -------- " << finalCoeffs[type.first + i]
        //          << " ------- " << den[i] << std::endl;
        if (finalCoeffs[type.first + i] < d1 || finalCoeffs[type.first + i] > d2)
            //std::cout << "Computed discretization is not good!\n";
            valid = false;
    }
    

    // std::string testFile2 = "outputErrorDisc";

    std::string outputCoeffs = "discCoeffsExample.txt";
    std::ofstream outputHandle;
    outputHandle.open(outputCoeffs.c_str());
    outputHandle << type.first << std::endl;
    outputHandle << type.second << std::endl;
    outputHandle << delta << std::endl;
    for (int i{0}; i <= type.first; ++i)
        outputHandle << finalCoeffs[i].toString("%.80RNf") << std::endl;

    outputHandle << 1 << std::endl;

    for (int i{1}; i <= type.second; ++i)
        outputHandle << finalCoeffs[type.first + i].toString("%.80RNf") << std::endl;

    std::function<mpfr::mpreal(mpfr::mpreal)> erH =
        [&](mpfr::mpreal x) -> mpfr::mpreal {
        mpfr::mpreal num = 0;
        for (int i{0}; i <= type.first; ++i)
        {
            num += finalCoeffs[i] * mpfr::pow(x, i);
        }
        mpfr::mpreal den = 1;
        for (int i = 1; i <= type.second; ++i)
        {
            den += finalCoeffs[type.first + i] * mpfr::pow(x, i);
        }

        return w(x) * (f(x) - (num / den) / numScalingFactor);
    };
    outputHandle.close();
    std::pair<mpfr::mpreal, mpfr::mpreal> erHNorm;
    infnorm(erHNorm, erH, dom);
    std::cout << "Lattice-based error estimation  = " << erHNorm.second << std::endl;
    error = erHNorm.second;

    // plotFunc(testFile2, erH, dom.first, dom.second);
    return valid;
}


