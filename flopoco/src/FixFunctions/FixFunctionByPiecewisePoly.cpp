/*
  Polynomial Function Evaluator for FloPoCo
	This version uses piecewise polynomial approximation for a trade-off between tables and multiplier hardware.
	
  Authors: Florent de Dinechin (rewrite from scratch of original code by Mioara Joldes and Bogdan Pasca, see the attic directory)

  This file is part of the FloPoCo project
	launched by the Arénaire/AriC team of Ecole Normale Superieure de Lyon
  currently developed by the Socrate team at CITILab/INSA de Lyon
 
  Initial software.
  Copyright © ENS-Lyon, INSA-Lyon, INRIA, CNRS, UCBL,  
  2008-2014.
  All rights reserved.

  */

#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <limits.h>
#include <string.h>

#include <gmp.h>
#include <mpfr.h>

#include <gmpxx.h>
#include "../utils.hpp"


#include "FixFunctionByPiecewisePoly.hpp"
#include "FixFunctionByTable.hpp"
#include "FixHornerEvaluator.hpp"

using namespace std;

namespace flopoco{

	/* TODO for the brave
		 In many cases, we have some of the sigma_i who have a constant sign (see the reports).
		 This info is stored in sigmaSign, and currently unused.
		 In such cases, we could get back to reducing x to unsigned, and get back to fully unsigned arithmetic, at least in some of the steps.
		 There is probably a market for that.
		 This needs to be done cleanly (i.e. without bloating the code/making it unreadable)
		 Maybe we should wait for power-of-two segmentation

	 */

	
	/* Error analysis:
		 Target error is exp2(lsbOut).
		 Final rounding may entail up to exp2(lsbOut-1).
		 So approximation+rounding error budget is  exp2(lsbOut-1).
		 We call PiecewisePolyApprox with approximation target error bound exp2(lsbOut-2).
		 It reports polyApprox->LSB: may be lsbOut-2, may be up to lsbOut-2- intlog2(degree) 
		 It also reports polyApprox->approxErrorBound
		 so rounding error budget is  exp2(lsbOut-1) - polyApprox->approxErrorBound
		 Staying within this budget is delegated to FixHornerEvaluator: see there for the details
			 
	 */

	
#define DEBUGVHDL 0


	FixFunctionByPiecewisePoly::FixFunctionByPiecewisePoly(Target* target, string func, int lsbIn_, int msbOut_, int lsbOut_, int degree_, bool finalRounding_, double approxErrorBudget_, map<string, double> inputDelays):
		Operator(target, inputDelays), degree(degree_), lsbIn(lsbIn_), msbOut(msbOut_), lsbOut(lsbOut_), finalRounding(finalRounding_), approxErrorBudget(approxErrorBudget_){

		if(finalRounding==false){
			THROWERROR("FinalRounding=false not implemented yet" );
		}

		f=new FixFunction(func, false, lsbIn, msbOut, lsbOut); // this will provide emulate etc.
		
		srcFileName="FixFunctionByPiecewisePoly";
		
		ostringstream name;
		name<<"FixFunctionByPiecewisePoly"; 
		setNameWithFreqAndUID(name.str()); 

		setCriticalPath(getMaxInputDelays(inputDelays));

		setCopyrightString("Florent de Dinechin (2014)");
		addHeaderComment("-- Evaluator for " +  f-> getDescription() + "\n"); 
		REPORT(DETAILED, "Entering: FixFunctionByPiecewisePoly \"" << func << "\" " << lsbIn << " " << msbOut << " " << lsbOut << " " << degree);
 		int wX=-lsbIn;
		addInput("X", wX);
		int outputSize = msbOut-lsbOut+1; 
		addOutput("Y" ,outputSize , 2);
		useNumericStd();

		if(degree==0){ // This is a simple table
			REPORT(DETAILED, "Degree 0: building a simple table");
			FixFunctionByTable* table=new FixFunctionByTable(target, func, false, lsbIn, msbOut, lsbOut);
			addSubComponent(table);
			inPortMap(table, "X", "X");
			outPortMap(table, "Y", "YR");
			vhdl << instance(table, "simpleTable") << endl;
			syncCycleFromSignal("YR");
			vhdl << tab << "Y <= YR;" << endl; 
		}
		else{
			if(degree==1){ // For degree 1, MultiPartite could work better
			REPORT(INFO, "Degree 1: You should consider using FixFunctionByMultipartiteTable");
			}

			
			// Build the polynomial approximation
			double targetAcc= approxErrorBudget*pow(2, lsbOut);
			REPORT(INFO, "Computing polynomial approximation for target accuracy "<< targetAcc);
			polyApprox = new PiecewisePolyApprox(func, targetAcc, degree);
			alpha =  polyApprox-> alpha; // coeff table input size 
			
			// Resize its MSB to the one input by the user. 
			for (int i=0; i<(1<<alpha); i++) {
				polyApprox -> poly[i] -> coeff[0] -> changeMSB(msbOut);
			}
			polyApprox -> MSB[0] = msbOut;

			// Build the coefficient table out of the vector of polynomials. This is also where we add the final rounding bit
			buildCoeffTable();
			GenericTable* coeffTable = new GenericTable(target, alpha, polyTableOutputSize, coeffTableVector); 
			addSubComponent(coeffTable);

			// What remains of the error budget for the evaluation phase ?
			double roundingErrorBudget=exp2(lsbOut-1)-polyApprox->approxErrorBound;
			REPORT(INFO, "Overall error budget = " << exp2(lsbOut) << "  of which approximation error = " << polyApprox->approxErrorBound
						 << " hence rounding error budget = "<< roundingErrorBudget );

			// The VHDL that splits the input into A and Z
			vhdl << tab << declare("A", alpha)  << " <= X" << range(wX-1, wX-alpha) << ";" << endl;
			vhdl << tab << declare("Z", wX-alpha)  << " <= X" << range(wX-alpha-1, 0) << ";" << endl;
			vhdl << tab << declare("Zs", wX-alpha)  << " <= (not Z(" << wX-alpha-1 << ")) & Z" << range(wX-alpha-2, 0) << "; -- centering the interval" << endl;

			inPortMap(coeffTable, "X", "A");
			outPortMap(coeffTable, "Y", "Coeffs");
			vhdl << instance(coeffTable, "coeffTable") << endl;

			syncCycleFromSignal("Coeffs");
			// Split the table output into each coefficient
			int currentShift=0;
			for(int i=polyApprox->degree; i>=0; i--) {
				int actualSize = polyApprox->MSB[i] - polyApprox->LSB + (polyApprox->coeffSigns[i]==0 ? 1 : 0);
				vhdl << tab << declare(join("A",i), polyApprox->MSB[i] - polyApprox->LSB +1)
						 << " <= ";
				if (polyApprox->coeffSigns[i]!=0) // add constant sign back
					vhdl << (polyApprox->coeffSigns[i]==1? "\"0\"" :  "\"1\"") << " & " ;				
				vhdl << "Coeffs" << range(currentShift + actualSize-1, currentShift) << ";" << endl;
				currentShift += actualSize;
			}

			
			// What follows is related to Horner evaluator
			// Here I wish I could plug other (more parallel) evaluators. 

			//  compute the size of the intermediate terms sigma_i  (Horner-specific)  
			computeSigmaSignsAndMSBs(); // TODO make it a method of FixHornerEvaluator?

			// the previous has computed the min value of msbOut.
			if(msbOut<sigmaMSB[0])
				REPORT(0, "WARNING: msbOut is set to " << msbOut << " but I compute that it should be " << sigmaMSB[0]);


			REPORT(INFO, "Now building the Horner evaluator for rounding error budget "<< roundingErrorBudget);
			// This builds an architecture such as eps_finalround < 2^(lsbOut-1) and eps_round<2^(lsbOut-2)
#if 0 // This constructor computes sigma and msbs only out of the formats
			FixHornerEvaluator* horner = new FixHornerEvaluator(target, lsbIn+alpha+1, msbOut, lsbOut, degree, polyApprox->MSB, polyApprox->LSB, roundingErrorBudget);		
#else // This constructor uses the more accurate data computed out of the actual polynomials
			FixHornerEvaluator* horner = new FixHornerEvaluator(target, lsbIn+alpha+1, msbOut, lsbOut, degree, polyApprox->MSB, polyApprox->LSB, sigmaSign, sigmaMSB, roundingErrorBudget);		
#endif
			addSubComponent(horner);

			inPortMap(horner, "X", "Zs");
			outPortMap(horner, "R", "Ys");
			for(int i=0; i<=polyApprox->degree; i++) {
				inPortMap(horner,  join("A",i),  join("A",i));
			}
			vhdl << instance(horner, "horner") << endl;
			syncCycleFromSignal("Ys");
		
			vhdl << tab << "Y <= " << "std_logic_vector(Ys);" << endl;
		}
	}



	FixFunctionByPiecewisePoly::~FixFunctionByPiecewisePoly() {
		free(f);
	}





	
	void FixFunctionByPiecewisePoly::buildCoeffTable() {
		// First compute the table output size
		polyTableOutputSize=0;
		for (int i=0; i<=degree; i++) {
			polyTableOutputSize += polyApprox->MSB[i] - polyApprox->LSB + (polyApprox->coeffSigns[i]==0? 1 : 0);
		} 
		REPORT(DETAILED, "Poly table input size  = " << alpha);
		REPORT(DETAILED, "Poly table output size = " << polyTableOutputSize);

		int x;
		for(x=0; x<(1<<alpha); x++) {
			mpz_class z=0;
			int currentShift=0;
			for(int i=polyApprox->degree; i>=0; i--) {
				mpz_class coeff = polyApprox-> getCoeff(x, i); // coeff of degree i from poly number x
				if (polyApprox->coeffSigns[i] != 0) {// sign is constant among all the coefficients: remove it from here, it will be added back as a constant in the VHDL
					mpz_class mask = (mpz_class(1)<<(polyApprox->MSB[i] - polyApprox->LSB) ) - 1; // size is msb-lsb+1
					coeff = coeff & mask; 
				}
				z += coeff << currentShift; // coeff of degree i from poly number x
				// REPORT(DEBUG, "i=" << i << "   z=" << unsignedBinary(z, 64));
				if(i==0 && finalRounding){ // coeff of degree 0
					int finalRoundBitPos = lsbOut-1;
					z += mpz_class(1)<<(currentShift + finalRoundBitPos - polyApprox->LSB); // add the round bit
					//REPORT(DEBUG, "i=" << i << " + z=" << unsignedBinary(z, 64));
					// This may entail an overflow of z, in the case -tiny -> +tiny, e.g. first polynomial of atan
					// This is OK modulo 2^wOut (two's complement), but will break the vhdl output of Table: fix it here.
					z = z & ((mpz_class(1)<<polyTableOutputSize) -1);
					// REPORT(INFO, "Adding final round bit at position " << finalRoundBitPos-polyApprox->LSB);
				}
				currentShift +=  polyApprox->MSB[i] - polyApprox->LSB + (polyApprox->coeffSigns[i]==0? 1: 0);
			}
			coeffTableVector.push_back(z);
		}

	}



	
	

	void FixFunctionByPiecewisePoly::computeSigmaSignsAndMSBs(){
		mpfr_t res_left, res_right;
		sollya_obj_t rangeS;
		mpfr_init2(res_left, 10000); // should be enough for anybody
		mpfr_init2(res_right, 10000); // should be enough for anybody
		rangeS = sollya_lib_parse_string("[-1;1]");		

		REPORT(DEBUG, "Computing sigmas, signs and MSBs");

		// initialize the vector of MSB weights
		for (int j=0; j<=degree; j++) {
			sigmaMSB.push_back(INT_MIN);
			sigmaSign.push_back(17); // 17 meaning "not initialized yet"
		}
		
		for (int i=0; i<(1<<polyApprox -> alpha); i++){
			// initialize the vectors with sigma_d = a_d
			FixConstant* sigma = polyApprox -> poly[i] -> coeff[degree];
			sollya_obj_t sigmaS = sollya_lib_constant(sigma -> fpValue);
			int msb = sigma -> MSB;
			if (msb>sigmaMSB[degree])
				sigmaMSB[degree]=msb;
			sigmaSign[degree] = polyApprox -> coeffSigns[degree];
			
			for (int j=degree-1; j>=0; j--) {
				// interval eval of sigma_j
				// get the output range of sigma
				sollya_obj_t pi_jS = sollya_lib_mul(rangeS, sigmaS);
				sollya_lib_clear_obj(sigmaS);
				sollya_obj_t a_jS = sollya_lib_constant(polyApprox -> poly[i] -> coeff[j] -> fpValue);
				sigmaS = sollya_lib_add(a_jS, pi_jS);
				sollya_lib_clear_obj(pi_jS);
				sollya_lib_clear_obj(a_jS);
				// Get the endpoints
				int isrange = sollya_lib_get_bounds_from_range(res_left, res_right, sigmaS);
				if (isrange==false)
					THROWERROR("computeSigmaMSB: Not a range???");
				// First a tentative conversion to double to sort and get an estimate of the MSB and zeroness
				double l=mpfr_get_d(res_left, GMP_RNDN);
				double r=mpfr_get_d(res_right, GMP_RNDN);
				REPORT(DEBUG, "i=" << i << "  j=" << j << "  left=" << l << " right=" << r);
				// Now we want to know is if both have the same sign
				if (l>=0 && r>=0) { // sigma is positive
					if (sigmaSign[j] == 17 || sigmaSign[j] == +1)
						sigmaSign[j] = +1;
					else
						sigmaSign[j] = 0;
				}
				else if (l<0 && r<0) { // sigma is positive
					if (sigmaSign[j] == 17 || sigmaSign[j] == -1)
						sigmaSign[j] = -1;
					else
						sigmaSign[j] = 0;
				}
				else
					sigmaSign[j] = 0;
				

				// now finally the MSB computation
				int msb;
				mpfr_t mptmp;
				mpfr_init2(mptmp, 10000); // should be enough for anybody
				// we are interested in the max in magnitude
				if(fabs(l)>fabs(r)){
					mpfr_abs(mptmp, res_left, GMP_RNDN); // exact
				}
				else{
					mpfr_abs(mptmp, res_right, GMP_RNDN); // exact
				}
				mpfr_log2(mptmp, mptmp, GMP_RNDU);
				mpfr_floor(mptmp, mptmp);
				msb = mpfr_get_si(mptmp, GMP_RNDU);
				mpfr_clear(mptmp);
				msb++; // for the sign
				if (msb > sigmaMSB[j])
					sigmaMSB[j] = msb;
			} // for j
			
			
			sollya_lib_clear_obj(sigmaS);
		
		} // for i
		mpfr_clear(res_left);
		mpfr_clear(res_right);
		sollya_lib_clear_obj(rangeS);

		for (int j=degree; j>=0; j--) {
			REPORT(DETAILED, "Horner step " << j << ":   sigmaSign = " << sigmaSign[j] << " \t sigmaMSB = " << sigmaMSB[j]);
		}	

	}

	
	
#if 0
				sollya_lib_evaluate_function_over_interval(res, sigmaS, a);
				mpfi_get_left   (left, res);
				mpfi_get_right   (right, res);
				
				double l=mpfr_get_d(left, GMP_RNDN);
				double r=mpfr_get_d(right, GMP_RNDN);
				REPORT(INFO, "i=" << i << "  j=" << j << "  left=" << l << " right=" << r);
		mpfi_clear(a);
		mpfi_clear(res);
#endif



	void FixFunctionByPiecewisePoly::emulate(TestCase* tc){
		f->emulate(tc);
	}

	void FixFunctionByPiecewisePoly::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;

		tc = new TestCase(this); 
		tc->addInput("X", 0);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addInput("X", (mpz_class(1) << f->wIn) -1);
		emulate(tc);
		tcl->add(tc);

	}



	
	OperatorPtr FixFunctionByPiecewisePoly::parseArguments(Target *target, vector<string> &args) {
		int lsbIn, msbOut, lsbOut, d;
		string f;
		double approxErrorBudget;
		UserInterface::parseString(args, "f", &f); 
		UserInterface::parseInt(args, "lsbIn", &lsbIn);
		UserInterface::parseInt(args, "msbOut", &msbOut);
		UserInterface::parseInt(args, "lsbOut", &lsbOut);
		UserInterface::parsePositiveInt(args, "d", &d);
		UserInterface::parseFloat(args, "approxErrorBudget", &approxErrorBudget);
		return new FixFunctionByPiecewisePoly(target, f, lsbIn, msbOut, lsbOut, d, true, approxErrorBudget);
	}

	void FixFunctionByPiecewisePoly::registerFactory(){
		UserInterface::add("FixFunctionByPiecewisePoly", // name
											 "Evaluator of function f on [0,1), using a piecewise polynomial of degree d with Horner scheme.",
											 "FunctionApproximation",
											 "",
											 "f(string): function to be evaluated between double-quotes, for instance \"exp(x*x)\";\
                        lsbIn(int): weight of input LSB, for instance -8 for an 8-bit input;\
                        msbOut(int): weight of output MSB;\
                        lsbOut(int): weight of output LSB;\
                        d(int): degree of the polynomial;\
                        approxErrorBudget(real)=0.25: error budget in ulp for the approximation, between 0 and 0.5",                        
											 "This operator uses a table for coefficients, and Horner evaluation with truncated multipliers sized just right.<br>For more details, see <a href=\"bib/flopoco.html#DinJolPas2010-poly\">this article</a>.",
											 FixFunctionByPiecewisePoly::parseArguments
											 ) ;
		
	}

}
	

