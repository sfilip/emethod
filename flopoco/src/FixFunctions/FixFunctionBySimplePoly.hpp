#ifndef FixFunctionBySimplePoly_HPP
#define FixFunctionBySimplePoly_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

#include "../Operator.hpp"
#include "FixFunction.hpp"
#include "BasicPolyApprox.hpp"

namespace flopoco{


	/** The FixFunctionBySimplePoly class */
	class FixFunctionBySimplePoly : public Operator
	{
	public:
		/**
		 * The FixFunctionBySimplePoly constructor
			 @param[string] func    a string representing the function, input range should be [0,1]
			 @param[int]    lsbIn   input LSB weight
			 @param[int]    msbOut  output MSB weight, used to determine wOut
			 @param[int]    lsbOut  output LSB weight
			 @param[int]    lsbOut  output LSB weight
			 @param[bool]   finalRounding: if false, the operator outputs its guard bits as well, saving the half-ulp rounding error.
							 This makes sense in situations that further process the result with further guard bits.

			 One could argue that MSB weight is redundant, as it can be deduced from an analysis of the function.
			 This would require quite a lot of work for non-trivial functions (isolating roots of the derivative etc).
			 So this is currently left to the user.
		 */
		FixFunctionBySimplePoly(Target* target, string func, bool signedIn, int lsbIn, int msbOut, int lsbOut, bool finalRounding = true,  map<string, double> inputDelays = emptyDelayMap);

		/**
		 * FixFunctionBySimplePoly destructor
		 */
		~FixFunctionBySimplePoly();

		void emulate(TestCase * tc);

		void buildStandardTestCases(TestCaseList* tcl);

		static OperatorPtr parseArguments(Target *target, vector<string> &args);

		static void registerFactory();

	private:
		FixFunction *f;
		BasicPolyApprox *poly;
		bool finalRounding;

		vector<int> coeffMSB;
		vector<int> coeffLSB;
		vector<int> coeffSize;
	};

}

#endif
