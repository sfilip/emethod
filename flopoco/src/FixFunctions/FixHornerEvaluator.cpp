/*

  This file is part of the FloPoCo project
  initiated by the Aric team at Ecole Normale Superieure de Lyon
  and developed by the Socrate team at Institut National des Sciences Appliquées de Lyon

  Author : Florent de Dinechin, Florent.de-Dinechin@insa-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL, INSA,
  2008-2014.
  All rights reserved.

*/
#include <iostream>

#include "FixHornerEvaluator.hpp"

using namespace std;


	/*
		 This is a simplified version of the computation in the ASAP 2010 paper, simplified because x is in [-1,1)

		 WRT the ASAP paper,
		 1/we want to use FixMultAdd operations:
		 \sigma_d  =  a_d
		 \sigma_i  =  a_i + x^T_i * \sigma_{i+1}     \forall i \in \{ 0...d-1\}
		 p(y)      =  \sigma_0


		 2/  each Horner step may entail up to two errors: one when truncating X to Xtrunc, one when rounding/truncating the product.
		 step i rounds to lsbMult[i], such that lsbMult[i] <=  polyApprox->LSB
		 Heuristic to determine lsbMult[i] is as follows:
		 - set them all to polyApprox->LSB.
		 - Compute the cumulated rounding error (as per the procedure below)
		 - while it exceeds the rounding error budget, decrease lsbMult[i], starting with the higher degrees (which will have the lowest area/perf impact)
		 - when we arrive to degree 1, increase again.

		 The error of one step is the sum of two terms: the truncation of X, and the truncation of the product.
		 The first is sometimes only present in lower-degree steps (for later steps, X may be used untruncated
		 For the second, two cases:
		 * If plainVHDL is true, we
		   - get the full product,
			 - truncate it to lsbMult[i]-1,
			 - add it to the coefficient, appended with a rounding bit in position lsbMult[i]-1
			 - truncate the result to lsbMult[i], so we have effectively performed a rounding to nearest to lsbMult[i]:
			 epsilonMult = exp2(lsbMult[i]-1)
		 * If plainVHDL is false, we use a FixMultAdd faithful to lsbMult[i], so the mult rounding error is twice as high as in the plainVHDL case:
			 epsilonMult = exp2(lsbMult[i])


		 We still should consider the DSP granularity to truncate x to the smallest DSP-friendly size larger than |sigma_{j-1}|
		 TODO

		 So all that remains is to compute the parameters of the FixMultAdd.
		 We need
		 * size of \sigma_d: MSB is that of  a_{d-j}, plus 1 for overflows (sign extended).
							 LSB is lsbMult[i]

		 * size of x truncated x^T_i : since it is multiplied by \sigma_{j-1}, it should have the same size
		 * weight of the MSB of the product: since y \in [-1,1), this is the MSB of \sigma_{j-1}, i.e. the MSB of a_{d-j+1}
		 ** in case of signed coeffs, the max abs value is 2^(MSB-1).
		 **   however, since we have zero-extended x in this case there is some thinking TODO
		 * size of the truncated result:
		 * weight of the LSB of a_{d-j}
		 * weight of the MSB of a_{d-j}
	*/


#define LARGE_PREC 1000 // 1000 bits should be enough for everybody


namespace flopoco{


	void FixHornerEvaluator::computeLSBs(){

		lsbSigma[degree] = lsbCoeff; // This one is not variable
		for(int i=degree-1; i>=0; i--) {
			lsbSigma[i] = lsbCoeff; // these ones will decrease if we need more accuracy
		}
		double error;
		bool done=false;
		// Now compute the rounding error entailed by this setup, and loop if it overshoots the budget
		while (!done){
			error=0.0;
			for(int i=degree-1; i>=0; i--) {
				// Truncation of x:
				// One input to the mult will be sigma[i+1], of size msbSigma[i+1]-lsbSigma[i+1]+1
				// The other will be X, of size 0-lsbIn+1;  truncate it to the same size:
				lsbXTrunc[i] = max(lsbIn, lsbSigma[i+1]-msbSigma[i+1]);
				//REPORT(DETAILED, "... lsbXTrunc[" << i<< "]=" << lsbXTrunc[i]);
				if(lsbXTrunc[i]!=lsbIn) // there has been some truncation
					error += exp2(lsbSigma[i]);
				// otherwise no truncation, no error...

				// P is the full product of sigma_i+1 by xtrunc: sum the MSBs+1, and sum the LSBs
				// Again TODO this +1 is most of the time overkill (only for the case -1*-1. Can sigma reach -1?)
				lsbP[i] = lsbSigma[i+1] + lsbXTrunc[i];
				if(getTarget()->plainVHDL()) 	// we will be able to round the product to lsbSigma[i]
					error += exp2(lsbSigma[i]-1);
				else // we will truncate the product to lsbSigma[i]
					error += exp2(lsbSigma[i]);
				REPORT(DETAILED, "i="<< i  << " lsbXtrunc=" << 	lsbXTrunc[i] << " msbP=" << msbP[i]<< " lsbP=" << lsbP[i]<< " msbSigma=" << 	msbSigma[i]<< " lsbSigma=" << 	lsbSigma[i]);
			}
			if(error < roundingErrorBudget){
				REPORT(INFO, "Rounding error bounded by "<< error  << ": Success!");
				done=true;
			}
			else {// increase all the LSBs and start over. TODO refine
				REPORT(DETAILED, "Rounding error bounded by "<< error  << ", error budget was " << roundingErrorBudget << ": decreasing LSBs...");
				for(int i=degree-1; i>=0; i--) {
					lsbSigma[i] --; // these ones will decrease if we need more accuracy
				}
			}
		} // while


		// A bit of reporting
		for(int i=degree-1; i>=0; i--) {
			REPORT(INFO, "  level " << i << " requires a signed " << 0-lsbXTrunc[i]+1 << "x" <<  msbSigma[i+1] - lsbSigma[i+1] +1 << " multiplier");
		}
	}




	void FixHornerEvaluator::initialize(){
		setNameWithFreqAndUID("FixHornerEvaluator");
		setCopyrightString("F. de Dinechin (2014)");
		srcFileName="FixHornerEvaluator";

		if(!signedXandCoeffs)
			REPORT(0,"signedXandCoeffs=false, this code has probably never been tested in this case. If it works, please remove this warning. If it doesn't, we deeply apologize and invite you to fix it.");

		if(roundingErrorBudget==-1)
			roundingErrorBudget = exp2(-lsbOut);
			// computing the coeff sizes
		for (int i=0; i<=degree; i++)
			coeffSize.push_back(msbCoeff[i]-lsbCoeff+1); // see FixConstant.hpp for the constant format

		// declaring inputs
		if(signedXandCoeffs){
			addInput("X"  , 0 - lsbIn+1);
			vhdl << tab << declareFixPoint("Xs", true, 0, lsbIn) << " <= signed(X);" << endl;
		}
		else{
			addInput("X"  , -lsbIn);
			vhdl << tab << declareFixPoint("Xs", false, -1, lsbIn) << " <= unsigned(X);" << endl;
		}
		for (int i=0; i<=degree; i++)
			addInput(join("A",i), coeffSize[i]);

		// declaring outputs
		addOutput("R", msbOut-lsbOut+1);
		//		setCriticalPath( getMaxInputDelays(inputDelays) + target->localWireDelay() );

}





	void FixHornerEvaluator::generateVHDL(){

		// Split the coeff table output into various coefficients
		for(int i=0; i<=degree; i++) {
			vhdl << tab << declareFixPoint(join("As", i), signedXandCoeffs, msbCoeff[i], lsbCoeff)
					 << " <= " << (signedXandCoeffs?"signed":"unsigned") << "(" << join("A",i) << ");" <<endl;
		}

		// Initialize the Horner recurrence
		vhdl << tab << declareFixPoint(join("Sigma", degree), true, msbSigma[degree], lsbSigma[degree])
				 << " <= " << join("As", degree)  << ";" << endl;

		for(int i=degree-1; i>=0; i--) {
			resizeFixPoint(join("XsTrunc", i), "Xs", 0, lsbXTrunc[i]);

			//  assemble faithful operators (either FixMultAdd, or truncated mult)

			if(getTarget()->plainVHDL()) {	// stupid pipelining here
				vhdl << tab << declareFixPoint(join("P", i), true, msbP[i],  lsbP[i])
						 <<  " <= "<< join("XsTrunc", i) <<" * Sigma" << i+1 << ";" << endl;

				// Align before addition
				resizeFixPoint(join("Ptrunc", i), join("P", i), msbSigma[i], lsbSigma[i]-1);
				resizeFixPoint(join("Aext", i), join("As", i), msbSigma[i], lsbSigma[i]-1);
				setCycle(getCurrentCycle() + getTarget()->plainMultDepth(1-lsbXTrunc[i], msbSigma[i+1]-lsbSigma[i+1]+1) );

				vhdl << tab << declareFixPoint(join("SigmaBeforeRound", i), true, msbSigma[i], lsbSigma[i]-1)
						 << " <= " << join("Aext", i) << " + " << join("Ptrunc", i) << "+'1';" << endl;
				resizeFixPoint(join("Sigma", i), join("SigmaBeforeRound", i), msbSigma[i], lsbSigma[i]);
				nextCycle();
			}

			else { // using FixMultAdd
				//REPORT(DEBUG, "*** iteration " << i << );
				FixMultAdd::newComponentAndInstance(this,
																						join("Step",i),     // instance name
																						join("XsTrunc",i),  // x
																						join("Sigma", i+1), // y
																						join("As", i),       // a
																						join("Sigma", i),   // result
																						msbSigma[i], lsbSigma[i]
																						);
			}
			syncCycleFromSignal(join("Sigma", i));
			nextCycle();
		}
		if(finalRounding)
			resizeFixPoint("Ys", "Sigma0",  msbOut, lsbOut);

		vhdl << tab << "R <= " << "std_logic_vector(Ys);" << endl;

	}


	// A naive constructor that does a worst case analysis of datapath looing onnly at the coeff sizes
	FixHornerEvaluator::FixHornerEvaluator(Target* target,
																				 int lsbIn_, int msbOut_, int lsbOut_,
																				 int degree_, vector<int> msbCoeff_, int lsbCoeff_,
																				 double roundingErrorBudget_,
																				 bool signedXandCoeffs_,
																				 bool finalRounding_, map<string, double> inputDelays)
	: Operator(target), degree(degree_), lsbIn(lsbIn_), msbOut(msbOut_), lsbOut(lsbOut_),
		msbCoeff(msbCoeff_), lsbCoeff(lsbCoeff_),
		roundingErrorBudget(roundingErrorBudget_) ,signedXandCoeffs(signedXandCoeffs_),
		finalRounding(finalRounding_)
  {
		initialize();

		// initialize the vectors to the proper size so we can use them as arrays. I know.
		for (int i=0; i<degree; i++) {
			msbSigma.push_back(0);
			signSigma.push_back(0); // For signSigma this happens to be the default
			msbP.push_back(0);
			lsbP.push_back(0);
			lsbSigma.push_back(0);
			lsbXTrunc.push_back(0);
		}

		// Initialize the MSBs with a very rough analysis
		msbSigma[degree] = msbCoeff[degree];
		for(int i=degree-1; i>=0; i--) {
			msbSigma[i] = msbCoeff[i] + 1; // TODO this +1 is there for addition overflow, and probably overkill most of the times.
		}
		for(int i=degree-1; i>=0; i--) {
			msbP[i] = msbSigma[i+1] + 0 + 1;
		}

		// optimizing the lsbMults
		computeLSBs();

		generateVHDL();
  }


	// An optimized constructor if the caller has been able to compute the signs and MSBs of the sigma terms
	FixHornerEvaluator::FixHornerEvaluator(Target* target,
																				 int lsbIn_, int msbOut_, int lsbOut_,
																				 int degree_, vector<int> msbCoeff_, int lsbCoeff_,
																				 vector<int> sigmaSign_, vector<int> sigmaMSB_,
																				 double roundingErrorBudget_,
																				 bool signedXandCoeffs_,
																				 bool finalRounding_, map<string, double> inputDelays)
	: Operator(target), degree(degree_), lsbIn(lsbIn_), msbOut(msbOut_), lsbOut(lsbOut_),
		msbCoeff(msbCoeff_), lsbCoeff(lsbCoeff_),
		roundingErrorBudget(roundingErrorBudget_) ,
		signedXandCoeffs(signedXandCoeffs_),
		finalRounding(finalRounding_),
		signSigma(sigmaSign_),  msbSigma(sigmaMSB_)
  {
		initialize();
		// initialize the vectors to the proper size so we can use them as arrays. I know.
		for (int i=0; i<degree; i++) {
			msbP.push_back(0);
			lsbP.push_back(0);
			lsbSigma.push_back(0);
			lsbXTrunc.push_back(0);
		}

		for(int i=degree-1; i>=0; i--) {
			msbP[i] = msbSigma[i+1] + 0 + 1;
		}


		// optimizing the lsbMults
		computeLSBs();

		generateVHDL();
  }


	FixHornerEvaluator::~FixHornerEvaluator(){}


}
