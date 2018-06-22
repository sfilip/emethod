#ifndef _PIECEWISEPOLYAPPROX_HPP_
#define _PIECEWISEPOLYAPPROX_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <limits.h>
#include <float.h>

#include <sollya.h>
#include <gmpxx.h>

#include "Operator.hpp" // mostly for reporting
#include "FixFunction.hpp"
#include "FixConstant.hpp"
#include "BasicPolyApprox.hpp"

using namespace std;

/* Stylistic convention here: all the sollya_obj_t have names that end with a capital S */
namespace flopoco{

	/** The PiecewisePolyApprox object builds and maintains a piecewise machine-efficient polynomial approximation to a fixed-point function over [0,1]
	*/

	class PiecewisePolyApprox {
	public:

		/** A minimal constructor
		 */
		PiecewisePolyApprox(FixFunction* f, double targetAccuracy, int degree);

		/** A minimal constructor that parses a sollya string		 */
		PiecewisePolyApprox(string sollyaString, double targetAccuracy, int degree);

		virtual ~PiecewisePolyApprox();

		static OperatorPtr parseArguments(Target *target, vector<string> &args);

		static void registerFactory();

		/** get the bits of coeff of degree d of polynomial number i */
		mpz_class getCoeff(int i, int d);

		void build();

		int degree;                       /**< degree of the polynomial approximations */
		int alpha;                        /**< the input domain [0,1] will be split in 2^alpha subdomains */
		vector<BasicPolyApprox*> poly;    /**< The vector of polynomials, eventually should all be on the same format */
		int LSB;                          /**< common weight of the LSBs of the polynomial approximations */
		vector<int> MSB;                   /**< vector of MSB weights for each coefficient */
		double approxErrorBound;           /**< guaranteed upper bound on the approx error of each approximation provided. Should be smaller than targetAccuracy */
		vector<int> coeffSigns;            /**< If all the coeffs of a given degree i are strictly positive (resp. strictly negative), then coeffSigns[i]=+1 (resp. -1). Otherwise 0 */
	private:
		FixFunction *f;                   /**< The function to be approximated */
		double targetAccuracy;            /**< please build an approximation at least as accurate as that */

		string srcFileName; /**< useful only to enable same kind of reporting as for FloPoCo operators. */
		string uniqueName_; /**< useful only to enable same kind of reporting as for FloPoCo operators. */
		bool needToFreeF;   /**< in an ideal world, this should not be needed */

	};

}
#endif // _POLYAPPROX_HH_


// Garbage below
