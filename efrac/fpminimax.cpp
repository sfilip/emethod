#include "fpminimax.h"
#include <gmpxx.h>
#include <fstream>
#include <sstream>
#include <cstdlib>

std::pair<mpz_class, mp_exp_t> mpfrDecomp(mpfr::mpreal const &val)
{
    if (val == 0)
        return std::make_pair(mpz_class(0l), 0);
    mpz_t buffer;
    mpz_init(buffer);
    mp_exp_t exp;
    // compute the normalized decomposition of the data
    // (in regard to the precision of the internal data representation)
    exp = mpfr_get_z_2exp(buffer, val.mpfr_srcptr());
    while (mpz_divisible_ui_p(buffer, 2ul))
    {
        mpz_divexact_ui(buffer, buffer, 2ul);
        ++exp;
    }
    mpz_class signif(buffer);
    mpz_clear(buffer);
    return std::make_pair(signif, exp);
}

std::pair<mpz_class, mp_exp_t> mpfrDecomp(double val)
{
    if (val == 0.0)
        return std::make_pair(mpz_class(0l), 0);
    mpz_t buffer;
    mpfr_t mpfr_value;
    mpfr_init2(mpfr_value, 54);
    mpfr_set_d(mpfr_value, val, GMP_RNDN);
    mpz_init(buffer);
    mp_exp_t exp;
    // compute the normalized decomposition of the data
    // (in regard to the precision of the internal data representation)
    exp = mpfr_get_z_exp(buffer, mpfr_value);
    while (mpz_divisible_ui_p(buffer, 2ul))
    {
        mpz_divexact_ui(buffer, buffer, 2ul);
        ++exp;
    };
    mpz_class signif(buffer);
    mpz_clear(buffer);
    mpfr_clear(mpfr_value);
    return std::make_pair(signif, exp);
}

void maxVectorNorm(mpz_t *maxNorm, fplll::ZZ_mat<mpz_t> &basis)
{

    mpz_t maxValue;
    mpz_t iter;
    mpz_init(maxValue);
    mpz_init(iter);
    mpz_set_ui(maxValue, 0);

    for (int i = 0; i < basis.get_rows(); ++i)
    {
        mpz_set_ui(iter, 0);
        for (int j = 0; j < basis.get_cols(); ++j)
        {
            mpz_addmul(iter, basis(i, j).get_data(), basis(i, j).get_data());
        }
        if (mpz_cmp(iter, maxValue) > 0)
            mpz_set(maxValue, iter);
    }
    mpfr_t fMaxValue;
    mpfr_init_set_z(fMaxValue, maxValue, GMP_RNDN);
    mpfr_sqrt(fMaxValue, fMaxValue, GMP_RNDN);
    mpfr_get_z(*maxNorm, fMaxValue, GMP_RNDD);

    mpz_clear(iter);
    mpz_clear(maxValue);
    mpfr_clear(fMaxValue);
}

void extractBasisVectors(std::vector<std::vector<mpfr::mpreal>> &vBasis,
                         fplll::ZZ_mat<mpz_t> &basis, std::size_t dimRows, std::size_t dimCols)
{
    vBasis.resize(dimRows);
    for (std::size_t i = 0u; i < dimRows; ++i)
    {
        vBasis[i].resize(dimCols);
        for (std::size_t j = 0u; j < dimCols; ++j)
        {
            vBasis[i][j] = basis(i, j).get_data();
        }
    }
}

void generateCVPInstance(fplll::ZZ_mat<mpz_t> &B,
                         std::vector<mpz_class> &t,
                         std::vector<mpfr::mpreal> const &x,
                         std::vector<mpfr::mpreal> const &fx,
                         std::vector<std::function<mpfr::mpreal(mpfr::mpreal)>> &basisFunc,
                         mp_prec_t prec)
{
    using mpfr::mpreal;
    mp_prec_t prevPrec = mpreal::get_default_prec();
    mpreal::set_default_prec(prec);

    // store the min value for an exponent used to represent
    // in mantissa-exponent form the lattice basis and the
    // vector T to be approximated using the LLL approach
    mp_exp_t minExp = 0;
    // determine the scaling factor for the basis vectors and
    // the unknown vector to be approximated by T
    std::pair<mpz_class, mp_exp_t> decomp;

    for (std::size_t i{0u}; i < x.size(); ++i)
    {
        decomp = mpfrDecomp(fx[i]);
        if (decomp.second < minExp)
            minExp = decomp.second;
        for (std::size_t j{0u}; j < basisFunc.size(); ++j)
        {
            decomp = mpfrDecomp(basisFunc[j](x[i]));
            if (decomp.second < minExp)
                minExp = decomp.second;
        }
    }

    // scale the basis and vector t
    B.resize(basisFunc.size(), x.size());
    mpz_t intBuffer;
    mpz_init(intBuffer);

    for (std::size_t i{0u}; i < basisFunc.size(); ++i)
        for (std::size_t j{0u}; j < x.size(); ++j)
        {
            decomp = mpfrDecomp(basisFunc[i](x[j]));
            decomp.second -= minExp;
            mpz_ui_pow_ui(intBuffer, 2u, (unsigned int)decomp.second);
            mpz_mul(intBuffer, intBuffer, decomp.first.get_mpz_t());
            mpz_set(B(i, j).get_data(), intBuffer);
        }
    t.clear();
    for (std::size_t i{0u}; i < fx.size(); ++i)
    {
        decomp = mpfrDecomp(fx[i]);
        decomp.second -= minExp;
        mpz_ui_pow_ui(intBuffer, 2u, (unsigned int)decomp.second);
        mpz_mul(intBuffer, intBuffer, decomp.first.get_mpz_t());
        t.push_back(mpz_class(intBuffer));
    }

    // clean-up
    mpz_clear(intBuffer);

    mpreal::set_default_prec(prevPrec);
}

void applyKannanEmbedding(fplll::ZZ_mat<mpz_t> &B,
                          std::vector<mpz_class> &t)
{

    int outCols = B.get_cols() + 1;
    int outRows = B.get_rows() + 1;

    mpz_t w; // the CVP t vector weight
    mpz_init(w);
    maxVectorNorm(&w, B);
    B.resize(outRows, outCols);

    for (int i = 0; i < outRows - 1; ++i)
        mpz_set_ui(B(i, outCols - 1).get_data(), 0);
    for (int j = 0; j < outCols - 1; ++j)
        mpz_set(B(outRows - 1, j).get_data(), t[j].get_mpz_t());
    mpz_set(B(outRows - 1, outCols - 1).get_data(), w);
    mpz_clear(w);
}

void fpminimaxKernel(std::vector<mpfr::mpreal> &lllCoeffs,
                     std::vector<std::vector<mpfr::mpreal>> &svpCoeffs,
                     std::vector<mpfr::mpreal> const &x,
                     std::function<mpfr::mpreal(mpfr::mpreal)> &targetFunc,
                     std::vector<std::function<mpfr::mpreal(mpfr::mpreal)>> &basisFuncs,
                     mp_prec_t prec)
{
    using mpfr::mpreal;
    mp_prec_t prevPrec = mpreal::get_default_prec();
    mpreal::set_default_prec(prec);
    std::size_t n = basisFuncs.size();

    std::vector<mpfr::mpreal> fx;
    fx.resize(x.size());

    for (std::size_t i{0u}; i < x.size(); ++i)
    {
        fx[i] = targetFunc(x[i]);
    }

    fplll::ZZ_mat<mpz_t> B;
    std::vector<mpz_class> t;
    generateCVPInstance(B, t, x, fx, basisFuncs, prec);
    applyKannanEmbedding(B, t);

    fplll::ZZ_mat<mpz_t> U(B.get_rows(), B.get_cols());
    fplll::lll_reduction(B, U, 0.99, 0.51);
    //fplll::bkzReduction(B, U, 8);
    int xdp1 = (int)mpz_get_si(U(U.get_rows() - 1,
                                 U.get_cols() - 1)
                                   .get_data());

    std::vector<mpz_class> intLLLCoeffs;
    std::vector<std::vector<mpz_class>> intSVPCoeffs(n);
    xdp1 = (int)mpz_get_si(U(U.get_rows() - 1,
                             U.get_cols() - 1)
                               .get_data());
    mpz_t coeffAux;
    mpz_init(coeffAux);
    switch (xdp1)
    {
    case 1:
        for (int i{0}; i < U.get_cols() - 1; ++i)
        {
            mpz_neg(coeffAux, U(U.get_rows() - 1, i).get_data());
            intLLLCoeffs.push_back(mpz_class(coeffAux));
            for (int j{0}; j < (int)n; ++j)
            {
                mpz_set(coeffAux, U(j, i).get_data());
                intSVPCoeffs[j].push_back(mpz_class(coeffAux));
            }
        }
        break;
    case -1:
        for (int i = 0; i < U.get_cols() - 1; ++i)
        {
            intLLLCoeffs.push_back(mpz_class(U(U.get_rows() - 1, i).get_data()));
            for (int j = 0; j < (int)n; ++j)
            {
                mpz_set(coeffAux, U(j, i).get_data());
                intSVPCoeffs[j].push_back(mpz_class(coeffAux));
            }
        }
        break;
    default:
        std::cout << "Failed to generate the approximation\n";
        exit(EXIT_FAILURE);
    }
    mpz_clear(coeffAux);

    svpCoeffs.resize(intSVPCoeffs.size());
    lllCoeffs.resize(intLLLCoeffs.size());

    for (std::size_t i{0u}; i < intLLLCoeffs.size(); ++i)
    {

        mpreal newCoeff;
        lllCoeffs[i] = intLLLCoeffs[i].get_str();
        for (std::size_t j{0u}; j < svpCoeffs.size(); ++j)
        {
            mpfr::mpreal buffer = intSVPCoeffs[j][i].get_str();
            svpCoeffs[j].push_back(buffer);
        }
    }

    mpreal::set_default_prec(prevPrec);
}
