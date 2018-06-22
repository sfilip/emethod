/*
  FixFunction object for FloPoCo

  Authors: Florent de Dinechin

  This file is part of the FloPoCo project
  developed by the Aric team at Ecole Normale Superieure de Lyon
	then by the Socrate team at INSA de Lyon

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,

  All rights reserved.

*/

#include "FixFunction.hpp"
#include <sstream>

namespace flopoco{


	FixFunction::FixFunction(string sollyaString_, bool signedIn_, int lsbIn_, int msbOut_, int lsbOut_):
		sollyaString(sollyaString_), lsbIn(lsbIn_), msbOut(msbOut_), lsbOut(lsbOut_), wOut(msbOut_-lsbOut+1), signedIn(signedIn_)
	{
		if(signedIn)
			wIn=-lsbIn+1; // add the sign bit at position 0
		else
			wIn=-lsbIn;
		if(signedIn)
			rangeS = sollya_lib_parse_string("[-1;1]");
		else
			rangeS = sollya_lib_parse_string("[0;1]");
		ostringstream completeDescription;
		completeDescription << sollyaString_;
		if(signedIn)
			completeDescription << " on [-1,1)";
		else
			completeDescription << " on [0,1)";

		if(lsbIn!=0) // we have an IO specification
			completeDescription << " for lsbIn=" << lsbIn << " (wIn=" << wIn << "), msbout=" << msbOut << ", lsbOut=" << lsbOut ;
		description = completeDescription.str();

		// Now do the parsing in Sollya
		fS= sollya_lib_parse_string(sollyaString_.c_str());

		/* If  parse error throw an exception */
		if (sollya_lib_obj_is_error(fS))
			throw("FixFunction: Unable to parse input function.");
	}




	FixFunction::FixFunction(sollya_obj_t fS_, bool signedIn_):
		signedIn(signedIn_), fS(fS_)
	{
		if(signedIn)
			rangeS = sollya_lib_parse_string("[-1;1]");
		else
			rangeS = sollya_lib_parse_string("[0;1]");
	}




	FixFunction::~FixFunction()
	{
	  sollya_lib_clear_obj(fS);
	  sollya_lib_clear_obj(rangeS);
	}

	string FixFunction::getDescription() const
	{
		return description;
	}

	void FixFunction::eval(mpfr_t r, mpfr_t x) const
	{
		sollya_lib_evaluate_function_at_point(r, fS, x, NULL);
	}


	double FixFunction::eval(double x) const
	{
		mpfr_t mpX, mpR;
		double r;

		mpfr_inits(mpX, mpR, NULL);
		mpfr_set_d(mpX, x, GMP_RNDN);
		sollya_lib_evaluate_function_at_point(mpR, fS, mpX, NULL);
		r = mpfr_get_d(mpR, GMP_RNDN);

		mpfr_clears(mpX, mpR, NULL);
		return r;
	}


	void FixFunction::eval(mpz_class x, mpz_class &rNorD, mpz_class &ru, bool correctlyRounded) const
	{
		int precision=10*(wIn+wOut);
		sollya_lib_set_prec(sollya_lib_constant_from_int(precision));

		mpfr_t mpX, mpR;
		mpfr_init2(mpX,wIn+2);
		mpfr_init2(mpR,precision);

		if(signedIn) {
			mpz_class negateBit = mpz_class(1) << (wIn);
			if ((x >> (-lsbIn)) !=0)
				x -= negateBit;
		}
		/* Convert x to an mpfr_t in [0,1[ */
		mpfr_set_z(mpX, x.get_mpz_t(), GMP_RNDN);
		mpfr_div_2si(mpX, mpX, -lsbIn, GMP_RNDN);

		/* Compute the function */
		eval(mpR, mpX);
		//		REPORT(FULL,"function() input is:"<<sPrintBinary(mpX));
		//		REPORT(FULL,"function() output before rounding is:"<<sPrintBinary(mpR));
		/* Compute the signal value */
		mpfr_mul_2si(mpR, mpR, -lsbOut, GMP_RNDN);

		/* So far we have a highly accurate evaluation. Rounding to target size happens only now
		 */
		if(correctlyRounded){
			mpfr_get_z(rNorD.get_mpz_t(), mpR, GMP_RNDN);
		}
		else{
			mpfr_get_z(rNorD.get_mpz_t(), mpR, GMP_RNDD);
			mpfr_get_z(ru.get_mpz_t(), mpR, GMP_RNDU);
		}

		//		REPORT(FULL,"function() output r = ["<<rd<<", " << ru << "]");
		mpfr_clear(mpX);
		mpfr_clear(mpR);
	}





	void FixFunction::emulate(TestCase * tc, bool correctlyRounded){
			mpz_class x = tc->getInputValue("X");
			mpz_class rNorD,ru;
			eval(x,rNorD,ru,correctlyRounded);
			// cerr << "x=" << x << " -> " << rNorD << " " << ru << endl; // for debugging
			tc->addExpectedOutput("Y", rNorD);
			if(!correctlyRounded)
				tc->addExpectedOutput("Y", ru);
	}
} //namespace
