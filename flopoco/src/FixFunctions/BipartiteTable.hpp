#ifndef BIPARTITETABLE_HPP_
#define BIPARTITETABLE_HPP_

#include <vector>

#include <gmp.h>
#include <gmpxx.h>
#include <mpfr.h>

#include "../utils.hpp"

#include "../Operator.hpp"
#include "FixFunction.hpp"
#include "GenericTable.hpp"

using namespace std;

namespace flopoco{

	class BipartiteTable : public Operator
	{

	public:

		/**
			 The BipartiteTable constructor
			 @param[string] func    a string representing the function, input range should be [0,1)
			 @param[int]    lsbIn   input LSB weight
			 @param[int]    msbOut  output MSB weight, used to determine wOut
			 @param[int]    lsbOut  output LSB weight
		 */
		BipartiteTable(Target* target, string functionName,
				int lsbIn=0, int msbOut=0, int lsbOut=0,  map<string, double> inputDelays = emptyDelayMap);

		virtual ~BipartiteTable();

		void emulate(TestCase * tc);
		void buildStandardTestCases(TestCaseList* tcl);

		void computeTIVvalues(vector<mpz_class> &values);
		void computeTOvalues(vector<mpz_class> &values);

		string functionName;
		int lsbIn;
		int wIn;
		int msbOut;
		int lsbOut;
		int wOut;

	private:

		FixFunction *f;

		int n0, n1, n2;		/**< the size of the parts into which the input X is split */
		int g;				/**< the number of extra guard bits to be used for the computations */

	};

}
#endif // BIPARTITETABLE_HH_
