#ifndef FixFunctionByPiecewisePoly_HPP
#define FixFunctionByPiecewisePoly_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

#include "../Operator.hpp"
#include "GenericTable.hpp"
#include "FixFunction.hpp"
#include "PiecewisePolyApprox.hpp"

namespace flopoco{


	/** The FixFunctionByPiecewisePoly class */
	class FixFunctionByPiecewisePoly : public Operator
	{
	public:

		void buildCoeffTable();

		/**
		 * The FixFunctionByPiecewisePoly constructor
			 @param[string] func    a string representing the function, input range should be [0,1]
			 @param[int]    lsbIn   input LSB weight
			 @param[int]    msbOut  output MSB weight, used to determine wOut
			 @param[int]    lsbOut  output LSB weight
			 @param[int]    degree  degree of polynomial approximation used. Controls the trade-off between tables and multipliers.
			 @param[bool]   finalRounding: if false, the operator outputs its guard bits as well, saving the half-ulp rounding error. 
			                 This makes sense in situations that further process the result with further guard bits.
			 @param[bool]   plainStupidVHDL: if true, generate * and +; if false, use BitHeap-based FixMultAdd
			 
			 One could argue that MSB weight is redundant, as it can be deduced from an analysis of the function. 
			 This would require quite a lot of work for non-trivial functions (isolating roots of the derivative etc).
			 So this is currently left to the user.
		 */
		FixFunctionByPiecewisePoly(Target* target, string func, int lsbIn, int msbOut, int lsbOut, int degree, bool finalRounding = true,  double approxErrorBudget=0.25, map<string, double> inputDelays = emptyDelayMap);

		/**
		 * FixFunctionByPiecewisePoly destructor
		 */
		~FixFunctionByPiecewisePoly();
		
		void emulate(TestCase * tc);

		void buildStandardTestCases(TestCaseList* tcl);

		/** Factory method that parses arguments and calls the constructor */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);

		/** Factory register method */ 
		static void registerFactory();

	private:
		int degree;
		int lsbIn;
		int msbOut;
		int lsbOut;
		int alpha;
		PiecewisePolyApprox *polyApprox;
		int polyTableOutputSize;
		FixFunction *f;
		bool finalRounding;
		double approxErrorBudget;
		vector<mpz_class> coeffTableVector;
		vector <int> sigmaSign; /** +1 if sigma is always positive, -1 if sigma is always negative, O if sigma needs to be signed */
		vector<int> sigmaMSB;   /**< vector of MSB weights for each sigma term. Note that these MSB consider that sigma is signed: one may remove 1 if sigmaSign is +1 or -1  */

		/** Compute the MSBs of the intermediate terms sigma_i in an Horner evaluation scheme */
		void computeSigmaSignsAndMSBs();
	};

}

#endif
