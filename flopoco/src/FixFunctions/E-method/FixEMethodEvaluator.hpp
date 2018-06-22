/*

  A hardware implementation of the E-method for the evaluation of polynomials and rational polynomials.

  Author : Matei Istoan

*/

#ifndef FIXEMETHODEVALUATOR_HPP_
#define FIXEMETHODEVALUATOR_HPP_

#include <string>
#include <vector>

#include <iostream>
#include <sstream>
#include <iterator>

#include <sollya.h>
#include <gmpxx.h>

#include "Operator.hpp"
#include "Signal.hpp"

#include "ConstMult/IntConstMult.hpp"
#include "BitHeap/BitHeap.hpp"

#include "FixFunctions/FixFunction.hpp"
#include "FixConstant.hpp"
#include "FixFunctions/E-method/GenericSimpleSelectionFunction.hpp"
#include "FixFunctions/E-method/GenericComputationUnit.hpp"

#include "utils.hpp"

#define LARGEPREC 10000

#define RADIX8plusSUPPORT 1

namespace flopoco {

	/**
	 * A hardware implementation of the E-method for the evaluation of polynomials and rational polynomials.
	 * Computing:
	 * 		y_0 = R(x) = P(x)/Q(x)
	 * , where:
	 * 		P(x) = p_n*x^n + p_{n-1}*x^{n-1} + ... + p_1*x^1 + p_0
	 * 		Q(x) = q_m*x^m + q_{m-1}*x^{m-1} + ... + q_1*x^1 + 1
	 * 		so q_0 = 1
	 * Currently only supporting radix 2.
	 * 		Planned support for higher radixes (4, 8 etc.).
	 * Format of the input X is given as a parameter.
	 * Format of the coefficients is given as a parameter.
	 * Checks can, and should, be made to insure that all parameters are in ranges where the method converges.
	 *
	 * The main iteration is:
	 *		w[j] = r * [w[j-1] - A*d[j-1]]
	 * , where:
	 * 		the x[j] notation represents the value of x at computation step j
	 * 		r is the radix
	 * 		w is the remainder vector
	 * 		d is the digit vector
	 * 		A is the parameter matrix
	 *
	 * 	For the case of rational polynomials, matrix A is of the form:
	 * 		(1    -x    0    0    ...    0)
	 * 		(q_1   1    -x   0    ...    0)
	 * 		(q_2   0    1    -x   ...    0)
	 * 		...
	 * 		(q_N   0    0    0    ...    1)
	 * 	and w is initialized to:
	 * 		(p_0 p_1 ... p_N)
	 * 	and d is initialized to:
	 * 		(0 0 ... 0)
	 * 	, where N is the maximum between n and m. If P and Q are of different lengths,
	 * 	then the missing coefficients are set to zero.
	 *
	 * 	The specific iteration step for evaluating rational polynomials is:
	 *		w_i[j] = r * (w_i[j-1] - d_0[j-1]*q_i - d_i[j-1] + d_{i+1}[j-1]*x)
	 *	The iterations steps are simpler for i=0 and i=n.
	 *	For i=0, the iteration is:
	 *		w_0[j] = r * (w_0[j-1] - d_0[j-1] + d_1[j-1]*x)
	 *	For i=n, the iteration step is:
	 *		w_n[j] = r * (w_n[j-1] - d_0[j-1]*q_n - d_n[j-1])
	*/

  class FixEMethodEvaluator : public Operator
  {
  public:
    /**
     * A constructor that exposes all options.
     * @param   radix                the radix used for the implementation
     * @param   maxDigit             the maximum digit in the used digit set
     * @param   msbInOut             MSB of the input/output
     * @param   lsbInOut             LSB of the input/output
     * @param   coeffsP              vector holding the coefficients of polynomial P
     * @param   coeffsQ              vector holding the coefficients of polynomial Q
     * @param   delta                the parameter delta in the E-Method algorithm,
     *                               showing the overlap between two redundant digits
     *                               set to 0.5 by default
     * @param   scaleInput           flag showing if the input is to be scaled, or not
     *                               set to false by default
     * @param   inputScaleFactor     factor by which to scale the input
     *                               set by default to -1, meaning that the input is scaled by 1/2*alpha
     */
	FixEMethodEvaluator(Target* target,
			size_t radix,
			size_t maxDigit,
			int msbInOut,
			int lsbInOut,
			vector<string> coeffsP,
			vector<string> coeffsQ,
			double delta = 0.5,
			bool scaleInput = false,
			double inputScaleFactor = -1,
			map<string, double> inputDelays = emptyDelayMap);

	/**
	 * Class destructor
	 */
    ~FixEMethodEvaluator();

    /**
     * Test case generator
     */
    void emulate(TestCase * tc);

    // User-interface stuff
    /**
     * Factory method
     */
    static OperatorPtr parseArguments(Target *target, std::vector<std::string> &args);

    static void registerFactory();

    static TestList unitTest(int index);

  private:
    /**
     * Create a copy of a vector containing constants (given as strings)
     */
    void copyVectors();

    /**
     * Set the parameters of the algorithm that depend on delta and Q's coefficients
     */
    void setAlgorithmParameters();

    /**
     * Ensure that the coefficients of P all satisfy:
     * 		|p_i| <= xi
     */
    void checkPCoeffs();

    /**
     * Ensure that the coefficients of Q all satisfy:
     * 		|q_i| <= 1/2 * alpha
     */
    void checkQCoeffs();

    /**
     * Ensure that the input X satisfies:
     * 		|x| <= 1/2 * alpha
     */
    void checkX();


  private:
    size_t radix;                     /**< the radix used for the implementation */
    size_t maxDigit;                  /**< the radix used for the implementation */
    size_t n;                         /**< degree of the polynomial P */
    size_t m;                         /**< degree of the polynomial Q */
    int msbInOut;                     /**< MSB of the input/output */
    int lsbInOut;                     /**< LSB of the input/output */
    vector<string> coeffsP;           /**< vector of the coefficients of P */
    vector<string> coeffsQ;           /**< vector of the coefficients of Q */
    mpfr_t mpCoeffsP[10000];          /**< vector of the coefficients of P */
    mpfr_t mpCoeffsQ[10000];          /**< vector of the coefficients of Q */

    double delta;                     /**< the parameter delta in the E-Method algorithm */
    double alpha;                     /**< the parameter alpha in the E-Method algorithm */
    double xi;                        /**< the parameter xi in the E-Method algorithm */

    bool scaleInput;                  /**< flag showing whether the input X to the circuit will be scaled, or not */
    double inputScaleFactor;          /**< the factor by which the input is scaled */

    size_t maxDegree;                 /**< the maximum between the degrees of the polynomials P and Q */
    size_t nbIter;                    /**< the number of iterations */
    int g;                            /**< number of guard bits */

    size_t wHatSize;                  /**< size of the W^Hat signal */
    int msbWHat;                      /**< the MSB of the W^Hat signal */
    int lsbWHat;                      /**< the LSB of the W^Hat signal */
    Signal *dWHat;                    /**< dummy signal for W^Hat */
    int msbW;                         /**< the MSB of the W signal */
    int lsbW;                         /**< the LSB of the W signal */
    Signal *dW;                       /**< dummy signal for W */
    int msbD;                         /**< the MSB of the D signals */
    int lsbD;                         /**< the LSB of the D signals */
    Signal *dD;                       /**< dummy signal for D */
    int msbX;                         /**< the MSB of the X signal */
    int lsbX;                         /**< the LSB of the X signal */
    Signal *dX;                       /**< dummy signal for X */
    int msbDiMX;                      /**< the MSB of the DiMultX signals */
    int lsbDiMX;                      /**< the LSB of the DiMultX signals */
    Signal *dDiMX;                    /**< dummy signal for DiMultX */

    int currentCycle;                 /**< used for pipelining to store the current cycle */
    double currentCriticalPath;       /**< used for pipelining to store the current critical path */
  };

} /* namespace flopoco */

#endif /* _FIXEMETHODEVALUATOR_HPP_ */
