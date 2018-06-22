#ifndef PolyCoeffTable_HPP
#define PolyCoeffTable_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>
#include <sollya.h>

#include "../Operator.hpp"
#include "../Table.hpp"
#include "PolynomialEvaluator.hpp"

#include "Function.hpp"
#include "PiecewiseFunction.hpp"
#include "HOTBM/MPPolynomial.hh"

namespace flopoco{
  // The following was cut from UtilSollya, to be removed some day
  //  sollya_obj_list_t makeIntPtrChainToFromBy(int m, int n, int k) ;
  // sollya_obj_list_t makeIntPtrChainFromArray(int m, int *a);
  char *sPrintBinary(mpfr_t x);
  char *sPrintBinaryZ(mpfr_t x);


  /** @brief The PolyCoeffTable class. */
  class PolyCoeffTable : public Table {

  public:


	PolyCoeffTable(Target* target, PiecewiseFunction* pf,  int targetAccuracy, int n);
	PolyCoeffTable(Target* target, string func,  int targetAccuracy, int n);
	/* TODO: Doxygen parameters*/

	/**
	 * @brief PolyCoeffTable destructor
	 */
	~PolyCoeffTable();

	//MPPolynomial* getMPPolynomial(sollya_obj_t t);
	vector<FixedPointCoefficient*> getPolynomialCoefficients(sollya_obj_t poly);
	vector<vector<FixedPointCoefficient*> > getPolynomialCoefficientsVector();
	void printPolynomialCoefficientsVector();
	void updateMinWeightParam(int i, FixedPointCoefficient* zz);
	vector<FixedPointCoefficient*> getCoeffParamVector(); // This is the useful one
	void printCoeffParamVector();
	double getMaxApproxError();
	//void generateDebug();
	// void generateDebugPwf();
	// sollya_obj_t makePrecList(int m, int n, int precshift);
	// vector<int> getNrIntArray();

	/************************************************/
	/********Virtual methoods from class Table*******/
	mpz_class function(int x);

	int    double2input(double x);
	double input2double(int x);
	mpz_class double2output(double x);
	double output2double(mpz_class x);
	/************************************************/
  protected:
	void buildActualTable();
	Function *f;
	int degree;
	sollya_obj_t degreeS;
	int targetAccuracy;  /**< Output precision required from this polynomial. The output interval is assumed to be [0,1], so targetAccuracy will actually determine all the coefficient sizes */
	vector<int> sizeList; /* Sizes, in bits, of each coefficient */
	vector< vector<FixedPointCoefficient*> > polyCoeffVector;
	vector<FixedPointCoefficient*> coeffParamVector; /**< This is a vector of coefficient parameters: for each degree, the size and weight of the corresponding coeff */
	double maxError;
	PiecewiseFunction *pwf;
	vector <int> nrIntervalsArray; /**< A vector containing as many entries as functions (size is usually 1, but 2 for the polynomials used in FPSqrtPoly). Each entry is an integer that gives the number of subintervals in which the corresponding function has been split. */
	vector <mpz_class> actualTable; /**< The final compact coefficient table: one entry per polynomial/interval, each entry is the concatenation of the bit vectors of all the coefficients.*/
  };
}
#endif
