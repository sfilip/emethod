#ifndef _FIXFUNCTION_HPP_
#define _FIXFUNCTION_HPP_

#include <string>
#include <iostream>

#include <sollya.h>
#include <gmpxx.h>
#include "../TestBenches/TestCase.hpp"

using namespace std;

/* Stylistic convention here: all the sollya_obj_t have names that end with a capital S */
namespace flopoco{
	
	/** The FixFunction objects holds a fixed-point function. 
			It provides an interface to Sollya services such as 
			parsing it,
			evaluating it at arbitrary precision,
			providing a polynomial approximation on an interval
	*/

	class FixFunction {
	public:


		/**
			 The FixFunctionByTable constructor
			 @param[string] func    a string representing the function
			 @param[int]    lsbX    input LSB weight (-lsbX is the input size)
			 @param[int]    msbOut  output MSB weight, used to determine wOut
			 @param[int]    lsbOut  output LSB weight
			 @param[bool]   signedInput: if true, input range is [0,1], else input range is [0,1]

			 One could argue that MSB weight is redundant, as it can be deduced from an analysis of the function. 
			 This would require quite a lot of work for non-trivial functions (isolating roots of the derivative etc).
			 So this is currently left to the user.
			 There are defaults for lsbOut and msbOut for situations when they are computed afterwards.
		 */
		FixFunction(string sollyaString, bool signedIn, int lsbIn=0, int msbOut=0, int lsbOut=0);
		FixFunction(sollya_obj_t fS, bool signedIn);

		virtual ~FixFunction();

		string getDescription() const;

		/** helper method: computes the value of the function on a double; no range check */
		double eval(double x) const;

		/** helper method: computes the value of the function on an MPFR; no range check */
		void eval(mpfr_t r, mpfr_t x) const;

		/** if correctlyRounded is true, compute the CR result in rNorD; otherwise computes the RD in rNorD and the RU in ru */
		void eval(mpz_class x, mpz_class &rNorD, mpz_class &ru, bool correctlyRounded=false) const;

		void emulate(TestCase * tc,	bool correctlyRounded=false /**< if true, correctly rounded RN; if false, faithful function */);

		// All the following public, not good practice I know, but life is complicated enough
		string sollyaString;
		int lsbIn;   
		int wIn;   
		int msbOut;
		int lsbOut;
		int wOut;
		bool signedIn;
		string description;
		sollya_obj_t fS;
		sollya_obj_t rangeS;
	};

}
#endif // _FIXFUNCTION_HH_
