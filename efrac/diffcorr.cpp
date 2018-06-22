#include "diffcorr.h"

extern "C"
{
#include <qsopt_ex/QSopt_ex.h>
}

void mpreal_to_mpq(mpq_t &ratVal, mpfr::mpreal &mprealVal)
{

    mpf_t mpfVal;
    mpf_init(mpfVal);
    mpfr_get_f(mpfVal, mprealVal.mpfr_ptr(), GMP_RNDN);
    mpq_set_f(ratVal, mpfVal);
    mpf_clear(mpfVal);
}

void diff_corr_kernel(mpfr::mpreal &delta, std::vector<mpfr::mpreal> &num,
                      std::vector<mpfr::mpreal> &den,
                      std::pair<int, int> const &type,
                      std::vector<mpfr::mpreal> const &x,
                      std::function<mpfr::mpreal(mpfr::mpreal)> &qk,
                      std::function<mpfr::mpreal(mpfr::mpreal)> &f,
                      std::function<mpfr::mpreal(mpfr::mpreal)> &w,
                      mpfr::mpreal const &deltak, mpfr::mpreal const &d1,
                      mpfr::mpreal const &d2)
{

    int rval = 0;
    int status = 0;
    int ncols = type.first + type.second + 2;
    int nrows = 2 * (int)x.size();

    int *cmatcnt = new int[ncols];
    int *cmatbeg = new int[ncols];
    int *cmatind = new int[ncols * nrows];
    char *sense = new char[nrows];

    mpq_t *cmatval = new mpq_t[ncols * nrows];
    mpq_t *obj = new mpq_t[ncols];
    mpq_t *rhs = new mpq_t[nrows];
    mpq_t *lower = new mpq_t[ncols];
    mpq_t *upper = new mpq_t[ncols];

    mpq_QSprob p;

    // construct the objective function
    for (int i{0}; i < ncols; ++i)
    {
        mpq_init(obj[i]);
        if (i < ncols - 1)
            mpq_set_ui(obj[i], 0u, 1u);
        else
            mpq_set_ui(obj[i], 1u, 1u);
    }

    mpfr::mpreal coeff;
    // construct the constraint matrix
    for (int i{type.first + 1}; i < ncols - 1; ++i)
        for (int j{0}; j < nrows; j += 2)
        {
            mpq_init(cmatval[i * nrows + j]);
            mpq_init(cmatval[i * nrows + j + 1]);

            coeff = (f(x[j / 2]) * w(x[j / 2]) - deltak) *
                    mpfr::pow(x[j / 2], i - type.first);
            mpreal_to_mpq(cmatval[i * nrows + j], coeff);
            coeff = (-f(x[j / 2]) * w(x[j / 2]) - deltak) *
                    mpfr::pow(x[j / 2], i - type.first);
            mpreal_to_mpq(cmatval[i * nrows + j + 1], coeff);
        }

    for (int i{0}; i < type.first + 1; ++i)
        for (int j{0}; j < nrows; j += 2)
        {
            mpq_init(cmatval[i * nrows + j]);
            mpq_init(cmatval[i * nrows + j + 1]);

            coeff = -mpfr::pow(x[j / 2], i) * w(x[j / 2]);
            mpreal_to_mpq(cmatval[i * nrows + j], coeff);
            coeff = -coeff;
            mpreal_to_mpq(cmatval[i * nrows + j + 1], coeff);
        }

    for (int i{0}; i < nrows; i += 2)
    {
        mpq_init(cmatval[(ncols - 1) * nrows + i]);
        mpq_init(cmatval[(ncols - 1) * nrows + i + 1]);

        coeff = -qk(x[i / 2]);
        mpreal_to_mpq(cmatval[(ncols - 1) * nrows + i], coeff);
        mpq_set(cmatval[(ncols - 1) * nrows + i + 1],
                cmatval[(ncols - 1) * nrows + i]);

        // initialise the right hand side of the constraint matrix
        mpq_init(rhs[i]);
        mpq_init(rhs[i + 1]);
        coeff = deltak - f(x[i / 2]) * w(x[i / 2]);
        mpreal_to_mpq(rhs[i], coeff);
        coeff = deltak + f(x[i / 2]) * w(x[i / 2]);
        mpreal_to_mpq(rhs[i + 1], coeff);
    }

    // construct the variable constraints
    for (int i{0}; i < ncols; ++i)
    {
        mpq_init(lower[i]);
        mpq_init(upper[i]);
        if (i <= type.first)
        {
            mpq_set(lower[i], mpq_ILL_MINDOUBLE);
            mpq_set(upper[i], mpq_ILL_MAXDOUBLE);
        }
        else if (i < ncols - 1)
        {
            coeff = d1;
            mpreal_to_mpq(lower[i], coeff);
            coeff = d2;
            mpreal_to_mpq(upper[i], coeff);
        }
        else
        {
            mpq_set(lower[i], mpq_ILL_MINDOUBLE);
            mpq_set(upper[i], mpq_ILL_MAXDOUBLE);
        }
    }

    // set the sense of the inequalities: "<="
    for (int i{0}; i < nrows; ++i)
    {
        sense[i] = 'L';
    }

    for (int i{0}; i < ncols; ++i)
    {
        cmatcnt[i] = nrows;
        cmatbeg[i] = i * nrows;
        for (int j{0}; j < nrows; ++j)
            cmatind[i * nrows + j] = j;
    }

    p = mpq_QSload_prob("emethod", ncols, nrows, cmatcnt, cmatbeg, cmatind,
                        cmatval, QS_MIN, obj, rhs, sense, lower, upper, nullptr,
                        nullptr);

    if (p == nullptr)
    {
        std::cerr << "Unable to load the problem...\n";

        delete[] cmatcnt;
        delete[] cmatbeg;
        delete[] cmatind;
        for (int i{0}; i < ncols * nrows; ++i)
            mpq_clear(cmatval[i]);
        delete[] cmatval;
        delete[] sense;
        for (int i{0}; i < ncols; ++i)
            mpq_clear(obj[i]);
        delete[] obj;
        for (int i{0}; i < ncols; ++i)
        {
            mpq_clear(lower[i]);
            mpq_clear(upper[i]);
        }
        delete[] lower;
        delete[] upper;
        for (int i{0}; i < nrows; ++i)
            mpq_clear(rhs[i]);
        delete[] rhs;

        return;
    }

    rval = QSexact_solver(p, nullptr, nullptr, nullptr, DUAL_SIMPLEX, &status);

    if (rval)
        std::cerr << "QSexact_solver failed\n";
    if (status != QS_LP_OPTIMAL)
    {
        std::cerr << "Did not find optimal solution.\n";
        std::cerr << "Status code: " << status << std::endl;
    }

    mpq_t objval;
    mpq_init(objval);
    rval = mpq_QSget_objval(p, &objval);
    if (rval)
    {
        std::cerr << "Could not get objective value, error code: " << rval << "\n;";
    }
    else
    {
        delta = mpfr::mpreal(objval);
    }

    mpq_t *xs = new mpq_t[ncols];
    for (int i{0}; i < ncols; ++i)
        mpq_init(xs[i]);
    rval = mpq_QSget_x_array(p, xs);
    if (rval)
        std::cerr << "Could not get variable vector, error code " << rval << "\n";
    else
    {
        for (int i{0}; i < type.first + 1; ++i)
            num.emplace_back(xs[i]);
        den.emplace_back(1.0);
        for (int i{type.first + 1}; i < ncols - 1; ++i)
            den.emplace_back(xs[i]);
    }

    mpq_QSfree_prob(p);

    delete[] cmatcnt;
    delete[] cmatbeg;
    delete[] cmatind;
    for (int i{0}; i < ncols; ++i)
        mpq_clear(xs[i]);
    delete[] xs;
    for (int i{0}; i < ncols * nrows; ++i)
        mpq_clear(cmatval[i]);
    delete[] cmatval;
    delete[] sense;
    for (int i{0}; i < ncols; ++i)
        mpq_clear(obj[i]);
    delete[] obj;
    for (int i{0}; i < ncols; ++i)
    {
        mpq_clear(lower[i]);
        mpq_clear(upper[i]);
    }
    delete[] lower;
    delete[] upper;
    for (int i{0}; i < nrows; ++i)
        mpq_clear(rhs[i]);
    delete[] rhs;

    mpq_clear(objval);
}

void diff_corr_kernel(mpfr::mpreal &delta, std::vector<mpfr::mpreal> &num,
                      std::vector<mpfr::mpreal> &den,
                      std::pair<int, int> const &type,
                      std::vector<mpfr::mpreal> const &x,
                      std::function<mpfr::mpreal(mpfr::mpreal)> &qk,
                      std::function<mpfr::mpreal(mpfr::mpreal)> &f,
                      std::function<mpfr::mpreal(mpfr::mpreal)> &w,
                      mpfr::mpreal const &deltak)
{

    int rval = 0;
    int status = 0;
    int ncols = type.first + type.second + 3;
    int nrows = 2 * (int)x.size();

    int *cmatcnt = new int[ncols];
    int *cmatbeg = new int[ncols];
    int *cmatind = new int[ncols * nrows];
    char *sense = new char[nrows];

    mpq_t *cmatval = new mpq_t[ncols * nrows];
    mpq_t *obj = new mpq_t[ncols];
    mpq_t *rhs = new mpq_t[nrows];
    mpq_t *lower = new mpq_t[ncols];
    mpq_t *upper = new mpq_t[ncols];

    mpq_QSprob p;

    // construct the objective function
    for (int i{0}; i < ncols; ++i)
    {
        mpq_init(obj[i]);
        if (i < ncols - 1)
            mpq_set_ui(obj[i], 0u, 1u);
        else
            mpq_set_ui(obj[i], 1u, 1u);
    }

    mpfr::mpreal coeff;
    // construct the constraint matrix
    for (int i{type.first + 1}; i < ncols - 1; ++i)
        for (int j{0}; j < nrows; j += 2)
        {
            mpq_init(cmatval[i * nrows + j]);
            mpq_init(cmatval[i * nrows + j + 1]);

            coeff = (f(x[j / 2]) * w(x[j / 2]) - deltak) *
                    mpfr::pow(x[j / 2], i - type.first);
            mpreal_to_mpq(cmatval[i * nrows + j], coeff);
            coeff = (-f(x[j / 2]) * w(x[j / 2]) - deltak) *
                    mpfr::pow(x[j / 2], i - type.first);
            mpreal_to_mpq(cmatval[i * nrows + j + 1], coeff);
        }

    for (int i{0}; i < type.first + 1; ++i)
        for (int j{0}; j < nrows; j += 2)
        {
            mpq_init(cmatval[i * nrows + j]);
            mpq_init(cmatval[i * nrows + j + 1]);

            coeff = -mpfr::pow(x[j / 2], i) * w(x[j / 2]);
            mpreal_to_mpq(cmatval[i * nrows + j], coeff);
            coeff = -coeff;
            mpreal_to_mpq(cmatval[i * nrows + j + 1], coeff);
        }

    for (int i{0}; i < nrows; i += 2)
    {
        mpq_init(cmatval[(ncols - 1) * nrows + i]);
        mpq_init(cmatval[(ncols - 1) * nrows] + i + 1);

        coeff = -qk(x[i / 2]);
        mpreal_to_mpq(cmatval[(ncols - 1) * nrows + i], coeff);
        mpq_set(cmatval[(ncols - 1) * nrows + i + 1],
                cmatval[(ncols - 1) * nrows + i]);

        // initialise the right hand side of the constraint matrix
        mpq_init(rhs[i]);
        mpq_init(rhs[i + 1]);
        coeff = deltak - f(x[i / 2]) * w(x[i / 2]);
        mpreal_to_mpq(rhs[i], coeff);
        coeff = deltak + f(x[i / 2]) * w(x[i / 2]);
        mpreal_to_mpq(rhs[i + 1], coeff);
    }

    // construct the variable constraints
    for (int i{0}; i < ncols; ++i)
    {
        mpq_init(lower[i]);
        mpq_init(upper[i]);
        if (i <= type.first)
        {
            mpq_set(lower[i], mpq_ILL_MINDOUBLE);
            mpq_set(upper[i], mpq_ILL_MAXDOUBLE);
        }
        else if (i < ncols - 1)
        {
            coeff = 1;
            mpreal_to_mpq(lower[i], coeff);
            coeff = -1;
            mpreal_to_mpq(upper[i], coeff);
        }
        else
        {
            mpq_set(lower[i], mpq_ILL_MINDOUBLE);
            mpq_set(upper[i], mpq_ILL_MAXDOUBLE);
        }
    }

    // set the sense of the inequalities: "<="
    for (int i{0}; i < nrows; ++i)
    {
        sense[i] = 'L';
    }

    for (int i{0}; i < ncols; ++i)
    {
        cmatcnt[i] = nrows;
        cmatbeg[i] = i * nrows;
        for (int j{0}; j < nrows; ++j)
            cmatind[i * nrows + j] = j;
    }

    p = mpq_QSload_prob("emethod", ncols, nrows, cmatcnt, cmatbeg, cmatind,
                        cmatval, QS_MIN, obj, rhs, sense, lower, upper, nullptr,
                        nullptr);

    if (p == nullptr)
    {
        std::cerr << "Unable to load the problem...\n";

        delete[] cmatcnt;
        delete[] cmatbeg;
        delete[] cmatind;
        for (int i{0}; i < ncols * nrows; ++i)
            mpq_clear(cmatval[i]);
        delete[] cmatval;

        return;
    }

    rval = QSexact_solver(p, nullptr, nullptr, nullptr, DUAL_SIMPLEX, &status);

    if (rval)
        std::cerr << "QSexact_solver failed\n";
    if (status != QS_LP_OPTIMAL)
    {
        std::cerr << "Did not find optimal solution.\n";
        std::cerr << "Status code: " << status << std::endl;
    }

    mpq_t objval;
    mpq_init(objval);
    rval = mpq_QSget_objval(p, &objval);
    if (rval)
    {
        std::cerr << "Could not get objective value, error code: " << rval << "\n;";
    }
    else
    {
        delta = mpfr::mpreal(objval);
    }

    mpq_t *xs = new mpq_t[ncols];
    for (int i{0}; i < ncols; ++i)
        mpq_init(xs[i]);
    rval = mpq_QSget_x_array(p, xs);
    if (rval)
        std::cerr << "Could not get variable vector, error code " << rval << "\n";
    else
    {
        for (int i{0}; i < type.first + 1; ++i)
            num.emplace_back(xs[i]);
        den.emplace_back(1.0);
        for (int i{type.first + 1}; i < ncols - 1; ++i)
            den.emplace_back(xs[i]);
    }

    mpq_QSfree_prob(p);

    delete[] cmatcnt;
    delete[] cmatbeg;
    delete[] cmatind;
    for (int i{0}; i < ncols; ++i)
        mpq_clear(xs[i]);
    delete[] xs;
    for (int i{0}; i < ncols * nrows; ++i)
        mpq_clear(cmatval[i]);
    delete[] cmatval;
    mpq_clear(objval);
}

void diff_corr(std::vector<mpfr::mpreal> &num, std::vector<mpfr::mpreal> &den,
               mpfr::mpreal &errDC, std::pair<int, int> const &type,
               std::vector<mpfr::mpreal> const &x,
               std::function<mpfr::mpreal(mpfr::mpreal)> &f,
               std::function<mpfr::mpreal(mpfr::mpreal)> &w,
               mpfr::mpreal const &d1, mpfr::mpreal const &d2)
{
    if (num.empty() && den.empty())
    {
        for (int i{0}; i < type.first; ++i)
            num.emplace_back(0);
        den.emplace_back(1);
        for (int i{1}; i < type.second; ++i)
            num.emplace_back(0);
    }
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

    mpfr::mpreal dk = mpfr::abs(w(x[0]) * (f(x[0]) - pk(x[0]) / qk(x[0])));
    for (std::size_t i{0}; i < x.size(); ++i)
        if (mpfr::abs(w(x[i]) * (f(x[i]) - pk(x[i]) / qk(x[i]))) > dk)
            dk = mpfr::abs(w(x[i]) * (f(x[i]) - pk(x[i]) / qk(x[i])));

    mpfr::mpreal delta;
    num.clear();
    den.clear();
    diff_corr_kernel(delta, num, den, type, x, qk, f, w, dk, d1, d2);

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

    mpfr::mpreal ndk = mpfr::abs(w(x[0]) * (f(x[0]) - pk(x[0]) / qk(x[0])));
    for (std::size_t i{0}; i < x.size(); ++i)
        if (mpfr::abs(w(x[i]) * (f(x[i]) - pk(x[i]) / qk(x[i]))) > ndk)
            ndk = mpfr::abs(w(x[i]) * (f(x[i]) - pk(x[i]) / qk(x[i])));

    while (mpfr::abs(dk - ndk) / mpfr::abs(ndk) > 1e-5)
    {
        std::cout << "Differential correction delta = " << ndk << std::endl;
        dk = ndk;
        num.clear();
        den.clear();
        diff_corr_kernel(delta, num, den, type, x, qk, f, w, dk, d1, d2);

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

        ndk = mpfr::abs(w(x[0]) * (f(x[0]) - pk(x[0]) / qk(x[0])));
        for (std::size_t i{0}; i < x.size(); ++i)
            if (mpfr::abs(w(x[i]) * (f(x[i]) - pk(x[i]) / qk(x[i]))) > ndk)
                ndk = mpfr::abs(w(x[i]) * (f(x[i]) - pk(x[i]) / qk(x[i])));
    }
    errDC = ndk;
}


void diff_corr(std::vector<mpfr::mpreal> &num, std::vector<mpfr::mpreal> &den,
               mpfr::mpreal &errDC, std::pair<int, int> const &type,
               std::vector<mpfr::mpreal> const &x,
               std::function<mpfr::mpreal(mpfr::mpreal)> &f,
               std::function<mpfr::mpreal(mpfr::mpreal)> &w)
{
    if (num.empty() && den.empty())
    {
        for (int i{0}; i < type.first; ++i)
            num.emplace_back(0);
        den.emplace_back(1);
        for (int i{1}; i < type.second; ++i)
            num.emplace_back(0);
    }
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

    mpfr::mpreal dk = mpfr::abs(w(x[0]) * (f(x[0]) - pk(x[0]) / qk(x[0])));
    for (std::size_t i{0}; i < x.size(); ++i)
        if (mpfr::abs(w(x[i]) * (f(x[i]) - pk(x[i]) / qk(x[i]))) > dk)
            dk = mpfr::abs(w(x[i]) * (f(x[i]) - pk(x[i]) / qk(x[i])));

    mpfr::mpreal delta;
    num.clear();
    den.clear();
    diff_corr_kernel(delta, num, den, type, x, qk, f, w, dk);

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

    mpfr::mpreal ndk = mpfr::abs(w(x[0]) * (f(x[0]) - pk(x[0]) / qk(x[0])));
    for (std::size_t i{0}; i < x.size(); ++i)
        if (mpfr::abs(w(x[i]) * (f(x[i]) - pk(x[i]) / qk(x[i]))) > ndk)
            ndk = mpfr::abs(w(x[i]) * (f(x[i]) - pk(x[i]) / qk(x[i])));

    while (mpfr::abs(dk - ndk) / mpfr::abs(ndk) > 1e-5)
    {
        std::cout << "Differential correction delta = " << ndk << std::endl;
        dk = ndk;
        num.clear();
        den.clear();
        diff_corr_kernel(delta, num, den, type, x, qk, f, w, dk);

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

        ndk = mpfr::abs(w(x[0]) * (f(x[0]) - pk(x[0]) / qk(x[0])));
        for (std::size_t i{0}; i < x.size(); ++i)
            if (mpfr::abs(w(x[i]) * (f(x[i]) - pk(x[i]) / qk(x[i]))) > ndk)
                ndk = mpfr::abs(w(x[i]) * (f(x[i]) - pk(x[i]) / qk(x[i])));
    }
    errDC = ndk;
}
