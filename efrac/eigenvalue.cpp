//
// Created by silvi on 11/19/2017.
//

#include "eigenvalue.h"
#include <algorithm>

void balance(MatrixXq &A)
{
    long n = A.rows();

    mpfr::mpreal rNorm; // row norm
    mpfr::mpreal cNorm; // column norm
    bool converged = false;
    mpfr::mpreal one = mpfr::mpreal(1.0);

    mpfr::mpreal g, f, s;
    while (!converged)
    {
        converged = true;
        for (std::size_t i = 0u; i < n; ++i)
        {
            rNorm = cNorm = 0.0;
            for (std::size_t j = 0u; j < n; ++j)
            {
                if (i == j)
                    continue;
                cNorm += mpfr::fabs(A(j, i));
                rNorm += mpfr::fabs(A(i, j));
            }
            if ((cNorm == 0.0) || (rNorm == 0))
                continue;

            g = rNorm >> 1u;
            f = 1.0;
            s = cNorm + rNorm;

            while (mpfr::isfinite(cNorm) && cNorm < g)
            {
                f <<= 1u;
                cNorm <<= 2u;
            }

            g = rNorm << 1u;

            while (mpfr::isfinite(cNorm) && cNorm > g)
            {
                f >>= 1u;
                cNorm >>= 2u;
            }

            if ((rNorm + cNorm) < s * f * 0.95)
            {
                converged = false;
                g = one / f;
                // multiply by D^{-1} on the left
                A.row(i) *= g;
                // multiply by D on the right
                A.col(i) *= f;
            }
        }
    }
}

void generateColleagueMatrix1stKind(MatrixXq &C, std::vector<mpfr::mpreal> &a,
                                    bool withBalancing)
{
    using mpfr::mpreal;
    std::vector<mpfr::mpreal> c = a;

    std::size_t n = a.size() - 1;
    // construct the initial matrix

    for (std::size_t i = 0u; i < n; ++i)
        for (std::size_t j = 0u; j < n; ++j)
            C(i, j) = 0;

    mpreal denom = -1;
    denom /= c[n];
    denom >>= 1;
    for (std::size_t i = 0u; i < a.size() - 1; ++i)
        c[i] *= denom;
    c[n - 2] += 0.5;

    for (std::size_t i = 0u; i < n - 1; ++i)
        C(i, i + 1) = C(i + 1, i) = 0.5;
    C(n - 2, n - 1) = 1;

    for (std::size_t i = 0u; i < n; ++i)
    {
        C(i, 0) = c[n - i - 1];
    }

    if (withBalancing)
        balance(C);
}

void determineEigenvalues(VectorXcq &eigenvalues, MatrixXq &C)
{
    Eigen::EigenSolver<MatrixXq> es(C);
    eigenvalues = es.eigenvalues();
}

void getRealValues(std::vector<mpfr::mpreal> &roots, VectorXcq &eigenValues,
                   mpfr::mpreal &a, mpfr::mpreal &b)
{
    using mpfr::mpreal;
    mpreal threshold = 10;
    mpfr_pow_si(threshold.mpfr_ptr(), threshold.mpfr_srcptr(), -20, GMP_RNDN);
    for (int i = 0; i < eigenValues.size(); ++i)
    {
        mpreal imagValue = mpfr::abs(eigenValues(i).imag());
        if (mpfr::abs(eigenValues(i).imag()) < threshold)
        {
            if (a <= eigenValues(i).real() && b >= eigenValues(i).real())
            {
                roots.push_back(eigenValues(i).real());
            }
        }
    }
    std::sort(roots.begin(), roots.end());
}

void generateColleagueMatrix2ndKind(MatrixXq &C, std::vector<mpfr::mpreal> &a,
                                    bool withBalancing)
{
    using mpfr::mpreal;
    std::vector<mpfr::mpreal> c = a;

    std::size_t n = a.size() - 1;
    // construct the initial matrix

    for (std::size_t i = 0u; i < n; ++i)
        for (std::size_t j = 0u; j < n; ++j)
            C(i, j) = 0;

    mpreal denom = -1;
    denom /= c[n];
    denom >>= 1;
    for (std::size_t i = 0u; i < a.size() - 1; ++i)
        c[i] *= denom;
    c[n - 2] += 0.5;

    for (std::size_t i = 0u; i < n - 1; ++i)
        C(i, i + 1) = C(i + 1, i) = 0.5;
    C(n - 2, n - 1) = 0.5;

    for (std::size_t i = 0u; i < n; ++i)
    {
        C(i, 0) = c[n - i - 1];
    }

    if (withBalancing)
        balance(C);
}

void balance(MatrixXd &A)
{
    long n = A.rows();

    double rNorm; // row norm
    double cNorm; // column norm
    bool converged = false;

    double g, f, s;
    while (!converged)
    {
        converged = true;
        for (std::size_t i = 0u; i < n; ++i)
        {
            rNorm = cNorm = 0.0;
            for (std::size_t j = 0u; j < n; ++j)
            {
                if (i == j)
                    continue;
                cNorm += fabs(A(j, i));
                rNorm += fabs(A(i, j));
            }
            if ((cNorm == 0.0) || (rNorm == 0))
                continue;

            g = rNorm / 2.0;
            f = 1.0;
            s = cNorm + rNorm;

            while (std::isfinite(cNorm) && cNorm < g)
            {
                f *= 2.0;
                cNorm *= 4.0;
            }

            g = rNorm * 2.0;

            while (std::isfinite(cNorm) && cNorm > g)
            {
                f /= 2.0;
                cNorm /= 4.0;
            }

            if ((rNorm + cNorm) < s * f * 0.95)
            {
                converged = false;
                g = 1.0 / f;
                // multiply by D^{-1} on the left
                A.row(i) *= g;
                // multiply by D on the right
                A.col(i) *= f;
            }
        }
    }
}

void generateColleagueMatrix1stKind(MatrixXd &C,
                                    std::vector<double> &a, bool withBalancing)
{
    std::vector<double> c = a;

    std::size_t n = a.size() - 1;
    // construct the initial matrix

    for (std::size_t i = 0u; i < n; ++i)
        for (std::size_t j = 0u; j < n; ++j)
            C(i, j) = 0;

    double denom = -1;
    denom /= c[n];
    denom /= 2;
    for (std::size_t i = 0u; i < a.size() - 1; ++i)
        c[i] *= denom;
    c[n - 2] += 0.5;

    for (std::size_t i = 0u; i < n - 1; ++i)
        C(i, i + 1) = C(i + 1, i) = 0.5;
    C(n - 2, n - 1) = 1;

    for (std::size_t i = 0u; i < n; ++i)
    {
        C(i, 0) = c[n - i - 1];
    }

    if (withBalancing)
        balance(C);
}

void generateColleagueMatrix2ndKind(MatrixXd &C,
                                    std::vector<double> &a, bool withBalancing)
{
    std::vector<double> c = a;

    std::size_t n = a.size() - 1;
    // construct the initial matrix

    for (std::size_t i = 0u; i < n; ++i)
        for (std::size_t j = 0u; j < n; ++j)
            C(i, j) = 0;

    double denom = -1;
    denom /= c[n];
    denom /= 2;
    for (std::size_t i = 0u; i < a.size() - 1; ++i)
        c[i] *= denom;
    c[n - 2] += 0.5;

    for (std::size_t i = 0u; i < n - 1; ++i)
        C(i, i + 1) = C(i + 1, i) = 0.5;
    C(n - 2, n - 1) = 0.5;

    for (std::size_t i = 0u; i < n; ++i)
    {
        C(i, 0) = c[n - i - 1];
    }

    if (withBalancing)
        balance(C);
}

void determineEigenvalues(VectorXcd &eigenvalues,
                          MatrixXd &C)
{
    Eigen::EigenSolver<MatrixXd> es(C);
    eigenvalues = es.eigenvalues();
}

void getRealValues(std::vector<double> &realValues,
                   VectorXcd &complexValues,
                   double &a, double &b)
{
    double threshold = 10;
    threshold = pow(10, -20);
    for (int i = 0; i < complexValues.size(); ++i)
    {
        double imagValue = fabs(complexValues(i).imag());
        if (imagValue < threshold)
        {
            if (a <= complexValues(i).real() && b >= complexValues(i).real())
            {
                realValues.push_back(complexValues(i).real());
            }
        }
    }
    std::sort(realValues.begin(), realValues.end());
}
