#include "remez.h"
#include "diffcorr.h"
#include "cheby.h"
#include "eigenvalue.h"

void domSplit(std::vector<std::pair<mpfr::mpreal, mpfr::mpreal>> &doms,
              std::pair<mpfr::mpreal, mpfr::mpreal> const &dom, std::size_t N)
{
    std::vector<mpfr::mpreal> splitPoints;
    for (std::size_t i{0u}; i <= N; ++i)
        splitPoints.emplace_back(dom.first +
                                 (dom.second - dom.first) * (mpfr::mpreal(i)) / N);

    for (std::size_t i{0}; i < N; ++i)
        doms.emplace_back(std::make_pair(splitPoints[i], splitPoints[i + 1]));
}

void infnorm(std::pair<mpfr::mpreal, mpfr::mpreal> &norm,
             std::function<mpfr::mpreal(mpfr::mpreal)> &f,
             std::pair<mpfr::mpreal, mpfr::mpreal> const &dom)
{
    mpfr::mpreal ia = -1;
    mpfr::mpreal ib = 1;
    std::size_t maxDegree = 8u;
    std::size_t N = 256;
    std::vector<std::pair<mpfr::mpreal, mpfr::mpreal>> doms;
    domSplit(doms, dom, N);
    norm.first = dom.first;
    norm.second = mpfr::abs(f(dom.first));

    std::vector<mpfr::mpreal> x;
    generateChebyshevPoints(x, maxDegree + 1u);

    for (std::size_t i = 0u; i < doms.size(); ++i)
    {
        std::vector<mpfr::mpreal> nx;
        changeOfVariable(nx, x, doms[i]);
        std::vector<mpfr::mpreal> fx(maxDegree + 1u);
        for (std::size_t j{0u}; j < fx.size(); ++j)
            fx[j] = f(nx[j]);
        std::vector<mpfr::mpreal> chebyCoeffs(maxDegree + 1);
        generateChebyshevCoefficients(chebyCoeffs, fx, maxDegree);
        std::vector<mpfr::mpreal> derivCoeffs(maxDegree);
        derivativeCoefficients1stKind(derivCoeffs, chebyCoeffs);

        MatrixXq Cm(maxDegree - 1, maxDegree - 1);
        generateColleagueMatrix1stKind(Cm, derivCoeffs, true);
        std::vector<mpfr::mpreal> eigenRoots;
        VectorXcq roots;
        determineEigenvalues(roots, Cm);
        getRealValues(eigenRoots, roots, ia, ib);
        changeOfVariable(eigenRoots, eigenRoots, doms[i]);

        mpfr::mpreal candMax;
        for (const auto &eigenRoot : eigenRoots)
        {
            candMax = mpfr::abs(f(eigenRoot));
            if (candMax > norm.second)
            {
                norm.first = eigenRoot;
                norm.second = candMax;
            }
        }
    }
}

void eremez(std::vector<mpfr::mpreal> &num, std::vector<mpfr::mpreal> &den,
            std::pair<mpfr::mpreal, mpfr::mpreal> const &dom,
            std::pair<int, int> const &type,
            std::function<mpfr::mpreal(mpfr::mpreal)> &f,
            std::function<mpfr::mpreal(mpfr::mpreal)> &w,
            mpfr::mpreal const &d1, mpfr::mpreal const &d2)
{
    std::vector<mpfr::mpreal> x;
    generateChebyshevPoints(x, type.first + type.second + 2);
    changeOfVariable(x, x, dom);
    mpfr::mpreal errDC;

    diff_corr(num, den, errDC, type, x, f, w, d1, d2);

    std::pair<mpfr::mpreal, mpfr::mpreal> cnorm;
    std::function<mpfr::mpreal(mpfr::mpreal)> qk;
    std::function<mpfr::mpreal(mpfr::mpreal)> pk;

    qk = [den](mpfr::mpreal var) -> mpfr::mpreal {
        mpfr::mpreal res = 0;
        for (std::size_t i{0u}; i < den.size(); ++i)
            res += den[i] * mpfr::pow(var, i);
        return res;
    };
    pk = [num](mpfr::mpreal var) -> mpfr::mpreal {
        mpfr::mpreal res = 0;
        for (std::size_t i{0u}; i < num.size(); ++i)
            res += num[i] * mpfr::pow(var, i);
        return res;
    };

    std::function<mpfr::mpreal(mpfr::mpreal)> err;
    err = [pk, qk, f, w](mpfr::mpreal var) -> mpfr::mpreal {
        return w(var) * (f(var) - pk(var) / qk(var));
    };
    infnorm(cnorm, err, dom);
    std::cout << "Outer iteration 0:\n";
    std::cout << "Location\tMax error\n";
    std::cout << cnorm.first << "\t" << cnorm.second << std::endl;

    mpfr::mpreal prevErr = cnorm.second * 2;
    std::size_t itCount{1u};
    while (mpfr::abs(cnorm.second - errDC) / cnorm.second > 1e-5)
    {
        prevErr = cnorm.second;
        x.emplace_back(cnorm.first);
        diff_corr(num, den, errDC, type, x, f, w, d1, d2);

        qk = [den](mpfr::mpreal var) -> mpfr::mpreal {
            mpfr::mpreal res = 0;
            for (std::size_t i{0u}; i < den.size(); ++i)
                res += den[i] * mpfr::pow(var, i);
            return res;
        };
        pk = [num](mpfr::mpreal var) -> mpfr::mpreal {
            mpfr::mpreal res = 0;
            for (std::size_t i{0u}; i < num.size(); ++i)
                res += num[i] * mpfr::pow(var, i);
            return res;
        };

        err = [pk, qk, f, w](mpfr::mpreal var) -> mpfr::mpreal {
            return w(var) * (f(var) - pk(var) / qk(var));
        };
        infnorm(cnorm, err, dom);
        std::cout << "Outer iteration " << itCount << ":\n";
        ++itCount;
        std::cout << "Location\tMax error\n";
        std::cout << cnorm.first << "\t" << cnorm.second << std::endl;
    }
}

void remez(std::vector<mpfr::mpreal> &num, std::vector<mpfr::mpreal> &den,
            std::pair<mpfr::mpreal, mpfr::mpreal> const &dom,
            std::pair<int, int> const &type,
            std::function<mpfr::mpreal(mpfr::mpreal)> &f,
            std::function<mpfr::mpreal(mpfr::mpreal)> &w)
{
    std::vector<mpfr::mpreal> x;
    generateChebyshevPoints(x, type.first + type.second + 2);
    changeOfVariable(x, x, dom);
    mpfr::mpreal errDC;

    diff_corr(num, den, errDC, type, x, f, w);

    std::pair<mpfr::mpreal, mpfr::mpreal> cnorm;
    std::function<mpfr::mpreal(mpfr::mpreal)> qk;
    std::function<mpfr::mpreal(mpfr::mpreal)> pk;

    qk = [den](mpfr::mpreal var) -> mpfr::mpreal {
        mpfr::mpreal res = 0;
        for (std::size_t i{0u}; i < den.size(); ++i)
            res += den[i] * mpfr::pow(var, i);
        return res;
    };
    pk = [num](mpfr::mpreal var) -> mpfr::mpreal {
        mpfr::mpreal res = 0;
        for (std::size_t i{0u}; i < num.size(); ++i)
            res += num[i] * mpfr::pow(var, i);
        return res;
    };

    std::function<mpfr::mpreal(mpfr::mpreal)> err;
    err = [pk, qk, f, w](mpfr::mpreal var) -> mpfr::mpreal {
        return w(var) * (f(var) - pk(var) / qk(var));
    };
    infnorm(cnorm, err, dom);
    std::cout << "Outer iteration 0:\n";
    std::cout << "Location\tMax error\n";
    std::cout << cnorm.first << "\t" << cnorm.second << std::endl;

    mpfr::mpreal prevErr = cnorm.second * 2;
    std::size_t itCount{1u};
    while (mpfr::abs(cnorm.second - errDC) / cnorm.second > 1e-5)
    {
        prevErr = cnorm.second;
        x.emplace_back(cnorm.first);
        diff_corr(num, den, errDC, type, x, f, w);

        qk = [den](mpfr::mpreal var) -> mpfr::mpreal {
            mpfr::mpreal res = 0;
            for (std::size_t i{0u}; i < den.size(); ++i)
                res += den[i] * mpfr::pow(var, i);
            return res;
        };
        pk = [num](mpfr::mpreal var) -> mpfr::mpreal {
            mpfr::mpreal res = 0;
            for (std::size_t i{0u}; i < num.size(); ++i)
                res += num[i] * mpfr::pow(var, i);
            return res;
        };

        err = [pk, qk, f, w](mpfr::mpreal var) -> mpfr::mpreal {
            return w(var) * (f(var) - pk(var) / qk(var));
        };
        infnorm(cnorm, err, dom);
        std::cout << "Outer iteration " << itCount << ":\n";
        ++itCount;
        std::cout << "Location\tMax error\n";
        std::cout << cnorm.first << "\t" << cnorm.second << std::endl;
    }
}
