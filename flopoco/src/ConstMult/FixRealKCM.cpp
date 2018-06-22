// TODO: repair FixSOPC, FixIIR, FixComplexKCM
/*
 * A faithful multiplier by a real constant, using a variation of the KCM method
 *
 * This file is part of the FloPoCo project developed by the Arenaire
 * team at Ecole Normale Superieure de Lyon
 *
 * Authors : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr
 * 			 3IF-Dev-Team-2015
 *
 * Initial software.
 * Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
 * 2008-2011.
 * All rights reserved.
 */
/*

Remaining 1-ulp bug:
flopoco verbose=3 FixRealKCM lsbIn=-8 msbIn=0 lsbOut=-7 constant="0.16" signedInput=true TestBench
It is the limit case of removing one table altogether because it contributes nothng.
I don't really understand

*/



#include "../Operator.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <sollya.h>
#include "../utils.hpp"
#include "FixRealKCM.hpp"
#include "../IntAddSubCmp/IntAdder.hpp"

using namespace std;

namespace flopoco{





	//standalone operator
	FixRealKCM::FixRealKCM(Target* target, bool signedInput_, int msbIn_, int lsbIn_, int lsbOut_, string constant_, double targetUlpError_):
							Operator(target),
							parentOp(this),
							signedInput(signedInput_),
							msbIn(msbIn_),
							lsbIn(lsbIn_),
							lsbOut(lsbOut_),
							constant(constant_),
							targetUlpError(targetUlpError_),
							addRoundBit(true)
	{
		init();		 // check special cases, computes number of tables and errorInUlps.

		// Now we have everything to compute g
		computeGuardBits();

		// To help debug KCM called from other operators, report in FloPoCo CLI syntax
		REPORT(DETAILED, "FixRealKCM  signedInput=" << signedInput << " msbIn=" << msbIn << " lsbIn=" << lsbIn << " lsbOut=" << lsbOut << " constant=\"" << constant << "\"  targetUlpError="<< targetUlpError);

		addInput("X",  msbIn-lsbIn+1);
		//		addFixInput("X", signedInput,  msbIn, lsbIn); // The world is not ready yet
		inputSignalName = "X"; // for buildForBitHeap
		addOutput("R", msbOut-lsbOut+1);

		// Special cases
		if(constantRoundsToZero)	{
			vhdl << tab << "R" << " <= " << zg(msbOut-lsbOut+1) << ";" << endl;
			return;
		}

		if(constantIsPowerOfTwo)	{
			// The code here is different that the one for the bit heap constructor:
			// In the stand alone case we must compute full negation.
			string rTempName = createShiftedPowerOfTwo(inputSignalName);
			int rTempSize = parentOp->getSignalByName(rTempName)->width();

			if(negativeConstant) { // In this case msbOut was incremented in init()
				vhdl << tab << "R" << " <= " << zg(msbOut-lsbOut+1) << " - ";
				if(signedInput) {
					vhdl << "("
							 <<  rTempName << of(rTempSize-1) << " & " // sign extension
							 <<  rTempName << range(rTempSize-1, g)
							 << ");" << endl;
				}
				else{ // unsigned input
					vhdl <<  rTempName << range(rTempSize-1, g) << ";" << endl;
				}
			}
			else{
				vhdl << tab << "R <= "<< rTempName << range(msbOut-lsbOut+g, g) << ";" << endl;
			}
			return;
		}


		// From now we have stuff to do.
		//create the bitheap
		//		int bitheaplsb = lsbOut - g;
		REPORT(DEBUG, "Creating bit heap for msbOut=" << msbOut <<" lsbOut=" << lsbOut <<" g=" << g);
		bitHeap = new BitHeap(this, msbOut-lsbOut+1+g); // hopefully some day we get a fixed-point bit heap

		buildTablesForBitHeap(); // does everything up to bit heap compression

		manageCriticalPath(target->localWireDelay() + target->lutDelay());

		//compress the bitheap and produce the result
		bitHeap->generateCompressorVHDL();

		//manage the critical path
		syncCycleFromSignal(bitHeap->getSumName());

		// Retrieve the bits we want from the bit heap
		vhdl << declare("OutRes",msbOut-lsbOut+1+g) << " <= " <<
			bitHeap->getSumName() << range(msbOut-lsbOut+g, 0) << ";" << endl; // This range is useful in case there was an overflow?

		vhdl << tab << "R <= OutRes" << range(msbOut-lsbOut+g, g) << ";" << endl;

		outDelayMap["R"] = getCriticalPath();
	}





	FixRealKCM::FixRealKCM(
												 Operator* parentOp_,
												 string multiplicandX,
												 bool signedInput_,
												 int msbIn_,
												 int lsbIn_,
												 int lsbOut_,
												 string constant_,
												 bool addRoundBit_,
												 double targetUlpError_
												 ):
		Operator(parentOp_->getTarget()),
		parentOp(parentOp_),
		signedInput(signedInput_),
		msbIn(msbIn_),
		lsbIn(lsbIn_),
		lsbOut(lsbOut_),
		constant(constant_),
		targetUlpError(targetUlpError_),
		addRoundBit(addRoundBit_), // will be set by buildForBitHeap()
		bitHeap(NULL),
		inputSignalName(multiplicandX)
	{

		init(); // check special cases, computes number of tables, but do not compute g: just take lsbOut as it is.

	}








	/**
	* @brief init : all operator initialization stuff goes here
	*/
	// Init computes the table splitting, because it is independent of g.
	// It may take suboptimal decisions if lsbOut is later enlarged with guard bits.
	void FixRealKCM::init()
	{
		//useNumericStd();

		srcFileName="FixRealKCM";

		setCopyrightString("Florent de Dinechin (2007-2016)");

		if(lsbIn>msbIn)
			throw string("FixRealKCM: Error, lsbIn>msbIn");

		if(targetUlpError > 1.0)
			THROWERROR("FixRealKCM: Error, targetUlpError="<<
					targetUlpError<<">1.0. Should be in ]0.5 ; 1].");
		//Error on final rounding is er <= 2^{lsbout - 1} = 1/2 ulp so
		//it's impossible to reach 0.5 ulp of precision if cste is real
		if(targetUlpError <= 0.5)
			THROWERROR("FixRealKCM: Error, targetUlpError="<<
					targetUlpError<<"<0.5. Should be in ]0.5 ; 1]");

		// Convert the input string into a sollya evaluation tree
		sollya_obj_t node;
		node = sollya_lib_parse_string(constant.c_str());
		/* If  parse error throw an exception */
		if (sollya_lib_obj_is_error(node))
		{
				ostringstream error;
				error << srcFileName << ": Unable to parse string "<<
					constant << " as a numeric constant" <<endl;
				throw error.str();
		}

		mpfr_init2(mpC, 10000);
		mpfr_init2(absC, 10000);
		sollya_lib_get_constant(mpC, node);

		//if negative constant, then set negativeConstant
		negativeConstant = ( mpfr_cmp_si(mpC, 0) < 0 ? true: false );

		signedOutput = negativeConstant || signedInput;
		mpfr_abs(absC, mpC, GMP_RNDN);

		REPORT(DEBUG, "Constant evaluates to " << mpfr_get_d(mpC, GMP_RNDN));

		// build the name
		ostringstream name;
		name <<"FixRealKCM_"  << vhdlize(msbIn)
				 << "_"  << vhdlize(lsbIn)
				 <<	"_" << vhdlize(lsbOut)
				 << "_" << vhdlize(constant, 10)
				 << (signedInput  ?"_signed" : "_unsigned");
		setNameWithFreqAndUID(name.str());

		//compute the logarithm only of the constants
		mpfr_t log2C;
		mpfr_init2(log2C, 100); // should be enough for anybody
		mpfr_log2(log2C, absC, GMP_RNDN);
		msbC = mpfr_get_si(log2C, GMP_RNDU);
		mpfr_clears(log2C, NULL);

		// Now we can check when this is a multiplier by 0: either because the it is zero, or because it is close enough
		constantRoundsToZero = false;
		if(mpfr_zero_p(mpC) != 0){
			constantRoundsToZero = true;
			msbOut=lsbOut; // let us return a result on one bit, why not.
			errorInUlps=0;
			REPORT(INFO, "It seems somebody asked for a multiplication by 0. We can do that.");
			return;
		}

		// A few sanity checks related to the magnitude of the constant

		// A bit of weight l is sent to position l+msbC+1 at most.
		// msbIn is sent to msbIn+ msbC +1 at most
		msbOut =   msbIn + msbC;
		if(msbOut<lsbOut){
			constantRoundsToZero = true;
			msbOut=lsbOut; // let us return a result on one bit, why not.
			errorInUlps=0.5;// TODO this is an overestimation
			REPORT(INFO, "Multiplying the input by such a small constant always returns 0. This simplifies the architecture.");
			return;
		}



		// Now even if the constant doesn't round completely to zero, it could be small enough that some of the inputs bits will have little impact on the output
		// However this depends on how many guard bits we add...
		// So the relevant test has been pushed to the table generation

		// Finally, check if the constant is a power of two -- obvious optimization there
		constantIsPowerOfTwo = (mpfr_cmp_ui_2exp(absC, 1, msbC) == 0);
		if(constantIsPowerOfTwo) {
			if(negativeConstant) {
				msbOut++; // To cater for the asymmetry of fixed-point : -2^(msbIn-1) has no representable opposite otherwise
				// We still have the original shift information in msbC
			}


			if(lsbIn+msbC<lsbOut) {   // The truncation case
				REPORT(DETAILED, "Constant is a power of two. Simple shift will be used instead of tables, but still there will be a truncation");
				errorInUlps=1;
			}


			else { // The padding case
				// The stand alone constructor computes a full subtraction. The Bitheap one adds negated bits, and a constant one that completes the subtraction.
				REPORT(DETAILED, "Constant is a power of two. Simple shift will be used instead of tables, and this KCM will be exact");
				errorInUlps=0;
			}
			return; // init stops here.
		}

		REPORT(DETAILED, "msbConstant=" << msbC  << "   (msbIn,lsbIn)=("<<
					 msbIn << "," << lsbIn << ")    lsbIn=" << lsbIn <<
				"   (msbOut,lsbOut)=(" << msbOut << "," << lsbOut <<
					 ")  signedOutput=" << signedOutput
		);

		// Compute the splitting of the input bits.
		// Ordering: Table 0 is the one that inputs the MSBs of the input
		// Let us call l the native LUT size (discounting the MUXes internal to logic clusters that enable to assemble them as larger LUTs).
		// Asymptotically (for large wIn), it is better to split the input in l-bit slices than (l+1)-bit slices, even if we have these muxes for free:
		// costs (in LUTs) are roundUp(wIn/l) versus 2*roundUp(wIn/(l+1))
		// Exception: when one of the tables is a 1-bit one, having it as a separate table will cost one LUT and one addition more,
		// whereas integrating it in the previous table costs one more LUT and one more MUX (the latter being for free)
		// Fact: the tables which inputs MSBs have larger outputs, hence cost more.
		// Therefore, the table that will have this double LUT cost should be the rightmost one.


		int optimalTableInputWidth = getTarget()->lutInputs();
		//		int wIn = msbIn-lsbIn+1;

		// first do something naive, then we'll optimize it.
		numberOfTables=0;
		int tableInMSB= msbIn;
		int tableInLSB = msbIn-(optimalTableInputWidth-1);
		while (tableInLSB>lsbIn) {
			m.push_back(tableInMSB);
			l.push_back(tableInLSB);
			tableInMSB -= optimalTableInputWidth ;
			tableInLSB -= optimalTableInputWidth ;
			numberOfTables++;
		}
		tableInLSB=lsbIn;

		// For the last table we just check if is not of size 1
		if(tableInLSB==tableInMSB && numberOfTables>0) {
			// better enlarge the previous table
			l[numberOfTables-1] --;
		}
		else { // last table is normal
			m.push_back(tableInMSB);
			l.push_back(tableInLSB);
			numberOfTables++;
		}

		// Are the table outputs signed? We compute this information here in order to avoid having constant bits added to the bit heap.
		// The chunk input to tables of index i>0 is always an unsigned number, so their output will be constant, fixed by the sign of
		for (int i=0; i<numberOfTables; i++) {
			int s;
			if (i==0 && signedInput) {
				// This is the only case when we can have a variable sign
					s=0;
			}
			else { // chunk input is positive
					if(negativeConstant)
						s=-1;
					else
						s=1;
			}
			tableOutputSign.push_back(s);
		}
		/* How to use this information?
			 The case +1 is simple: don't tabulate the constant 0 sign, don't add it to the bit heap
			 The case 0 is usual: we have a two's complement number to add to the bit heap,
          there are methods for that.
       The case -1 presents an additional problem: the sign bit is not always 1,
			    indeed there is one entry were the sign bit is 0: when input is 0.
					As usual there is a bit heap trick (found by Luc Forget):
          negate all the bits and add one ulp to the constant vector of the bit heap.

					For simplicity, in the current code we compute xi*absC (the abs. value of the constant)
          then subtract this from the bitHeap.
					The effect is exactly the same (hoping that the not gets merged in the table)
					and the C++ code is simpler.
*/
		for (int i=0; i<numberOfTables; i++) {
			REPORT(DETAILED, "Table " << i << "   inMSB=" << m[i] << "   inLSB=" << l[i] << "   tableOutputSign=" << tableOutputSign[i]  );
		}

		// Finally computing the error due to this setup. We express it as ulps at position lsbOut-g, whatever g will be
		errorInUlps=0.5*numberOfTables;
		REPORT(DETAILED,"errorInUlps=" << errorInUlps);
	}






	double FixRealKCM::getErrorInUlps() {
		return errorInUlps;
	}



	void FixRealKCM::computeGuardBits(){
		if(numberOfTables==2 && targetUlpError==1.0)
			g=0; // specific case: two CR tables make up a faithful sum
		else{
			// Was:			g = ceil(log2(numberOfTables/((targetUlpError-0.5)*exp2(-lsbOut)))) -1 -lsbOut;
			g=0;
			double maxErrorWithGuardBits=errorInUlps;
			double tableErrorBudget = targetUlpError-0.5 ; // 0.5 is for the final rounding
			while (maxErrorWithGuardBits > tableErrorBudget) {
				g++;
				maxErrorWithGuardBits /= 2.0;
			}
		}
		REPORT(DETAILED, "For errorInUlps=" << errorInUlps << " and targetUlpError=" << targetUlpError << "  we compute g=" << g);
	}


	void FixRealKCM::addToBitHeap(BitHeap* bitHeap_, int g_) {
		bitHeap=bitHeap_;
		g=g_;

		bool special=specialCasesForBitHeap();
		if(!special)
			buildTablesForBitHeap();
	}


	bool FixRealKCM::specialCasesForBitHeap() {
	// Special cases
		if(constantRoundsToZero)		{
			// do nothing
			return true;
		}

		// In all the following it should be clear that lsbOut is the initial lsb asked by the larger operator.
		// Some g was computed and we actually compute/tabulate to lsbOut -g
		// We keep these two variables separated because it makes this two-step process more explicit than changing lsbOut.

		if(constantIsPowerOfTwo)	{
			// Create a signal that shifts it or truncates it into place
			string rTempName = createShiftedPowerOfTwo(inputSignalName);
			int rTempSize = parentOp->getSignalByName(rTempName)->width();

			if(negativeConstant) {
				bitHeap -> subtractSignedBitVector(0, rTempName, rTempSize);
			}
			else{
				if(signedInput)
					bitHeap -> addSignedBitVector(0, rTempName, rTempSize);
				else // !negativeConstant and !signedInput
					bitHeap -> addUnsignedBitVector(0, rTempName, rTempSize);
			}
			return true; // and that 's all folks.
			}
		// From now on we have a "normal" constant and we instantiate tables.
		return false;
	}


	// Just to factor out code.
	/* This builds the input shifted WRT lsb-g
	 but doesn't worry about adding or subtracting it,
	 which depends wether we do a standalone or bit-heap
	*/
	string FixRealKCM::createShiftedPowerOfTwo(string resultSignalName){
		string rTempName = getName() + "_Rtemp"; // Should be unique in a bit heap if each KCM got a UID.
		// Compute shift that must be applied to x to align it to lsbout-g.
		// This is a shift left: negative means shift right.
		int shift= lsbIn -(lsbOut-g)  + msbC ;
		int rTempSize = msbC+msbIn -(lsbOut -g) +1; // initial msbOut is msbC+msbIn
		REPORT(0,"msbC=" << msbC << "     Shift left of " << shift << " bits");
		// compute the product by the abs constant
		parentOp->vhdl << tab << parentOp->declare(rTempName, rTempSize) << " <= ";
		// Still there are two cases:
		if(shift>=0) {  // Shift left; pad   THIS SEEMS TO WORK
			parentOp->vhdl << inputSignalName << " & " << zg(shift);
			// rtempsize= msbIn-lsbin+1   + shift  =   -lsbIn   + msbIn   +1   - (lsbOut-g -lsbIn +msbC)
		}
		else { // shift right; truncate
			parentOp->vhdl << inputSignalName << range(msbIn-lsbIn, -shift);
		}
		parentOp->vhdl <<  "; -- constant is a power of two, shift left of " << shift << " bits" << endl;
		return rTempName;
	}


	void FixRealKCM::buildTablesForBitHeap() {

		for(int i=0; i<numberOfTables; i++) {
			string sliceInName = join(getName() + "_A", i); // Should be unique in a bit heap if each KCM got a UID.
			string sliceOutName = join(getName() + "_T", i); // Should be unique in a bit heap if each KCM got a UID.
			string instanceName = join(getName() + "_Table", i); // Should be unique in a bit heap if each KCM got a UID.

			// Now that we have g we may compute if it has useful output bits
			int tableOutSize = m[i] + msbC  - lsbOut + g +1;
			if(tableOutSize<=0) {
				REPORT(DETAILED, "Warning: Table " << i << " was contributing nothing to the bit heap and has been discarded")
			}
			else { // Build it and add its output to the bit heap
				REPORT(DEBUG, "lsbIn=" << lsbIn);
				parentOp->vhdl << tab << parentOp->declare(sliceInName, m[i]- l[i] +1 ) << " <= "
											 << inputSignalName << range(m[i]-lsbIn, l[i]-lsbIn) << "; -- input address  m=" << m[i] << "  l=" << l[i]  << endl;
				FixRealKCMTable* t = new FixRealKCMTable(parentOp->getTarget(), this, i);
				parentOp->addSubComponent(t);
				parentOp->inPortMap (t , "X", sliceInName);
				parentOp->outPortMap(t , "Y", sliceOutName);
				parentOp->vhdl << parentOp->instance(t , instanceName);

				int sliceOutWidth = parentOp->getSignalByName(sliceOutName)->width();

				// Add these bits to the bit heap
				switch(tableOutputSign[i]) {
				case 0:
					bitHeap -> addSignedBitVector(0, // weight
																				sliceOutName, // name
																				sliceOutWidth // size
																				);
					break;
				case 1:
					bitHeap -> addUnsignedBitVector(0, // weight
																					sliceOutName, // name
																					sliceOutWidth // size
																					);
					break;
				case -1: // In this case the table simply stores x* absC
					bitHeap -> subtractUnsignedBitVector(0, // weight
																							 sliceOutName, // name
																							 sliceOutWidth // size
																							 );

					break;
				default: THROWERROR("unexpected value in tableOutputSign");
				}
			}
		}
	}




	// TODO manage correctly rounded cases, at least the powers of two
	// To have MPFR work in fix point, we perform the multiplication in very
	// large precision using RN, and the RU and RD are done only when converting
	// to an int at the end.
	void FixRealKCM::emulate(TestCase* tc)
	{
		// Get I/O values
		mpz_class svX = tc->getInputValue("X");
		bool negativeInput = false;
		int wIn=msbIn-lsbIn+1;
		int wOut=msbOut-lsbOut+1;

		// get rid of two's complement
		if(signedInput)	{
			if ( svX > ( (mpz_class(1)<<(wIn-1))-1) )	 {
				svX -= (mpz_class(1)<<wIn);
				negativeInput = true;
			}
		}

		// Cast it to mpfr
		mpfr_t mpX;
		mpfr_init2(mpX, msbIn-lsbIn+2);
		mpfr_set_z(mpX, svX.get_mpz_t(), GMP_RNDN); // should be exact

		// scale appropriately: multiply by 2^lsbIn
		mpfr_mul_2si(mpX, mpX, lsbIn, GMP_RNDN); //Exact

		// prepare the result
		mpfr_t mpR;
		mpfr_init2(mpR, 10*wOut);

		// do the multiplication
		mpfr_mul(mpR, mpX, mpC, GMP_RNDN);

		// scale back to an integer
		mpfr_mul_2si(mpR, mpR, -lsbOut, GMP_RNDN); //Exact
		mpz_class svRu, svRd;

		mpfr_get_z(svRd.get_mpz_t(), mpR, GMP_RNDD);
		mpfr_get_z(svRu.get_mpz_t(), mpR, GMP_RNDU);

		//		cout << " emulate x="<< svX <<"  before=" << svRd;
 		if(negativeInput != negativeConstant)		{
			svRd += (mpz_class(1) << wOut);
			svRu += (mpz_class(1) << wOut);
		}
		//		cout << " emulate after=" << svRd << endl;

		//Border cases
		if(svRd > (mpz_class(1) << wOut) - 1 )		{
			svRd = 0;
		}

		if(svRu > (mpz_class(1) << wOut) - 1 )		{
			svRu = 0;
		}

		tc->addExpectedOutput("R", svRd);
		tc->addExpectedOutput("R", svRu);

		// clean up
		mpfr_clears(mpX, mpR, NULL);
	}


	TestList FixRealKCM::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;

		if(index==-1)
		{ // The unit tests

			vector<string> constantList; // The list of constants we want to test
			constantList.push_back("\"0\"");
			constantList.push_back("\"0.125\"");
			constantList.push_back("\"-0.125\"");
			constantList.push_back("\"4\"");
			constantList.push_back("\"-4\"");
			constantList.push_back("\"log(2)\"");
			constantList.push_back("-\"log(2)\"");
			constantList.push_back("\"0.00001\"");
			constantList.push_back("\"-0.00001\"");
			constantList.push_back("\"0.0000001\"");
			constantList.push_back("\"-0.0000001\"");
			constantList.push_back("\"123456\"");
			constantList.push_back("\"-123456\"");

			for(int wIn=3; wIn<16; wIn+=4) { // test various input widths
				for(int lsbIn=-1; lsbIn<2; lsbIn++) { // test various lsbIns
					string lsbInStr = to_string(lsbIn);
					string msbInStr = to_string(lsbIn+wIn);
					for(int lsbOut=-1; lsbOut<2; lsbOut++) { // test various lsbIns
						string lsbOutStr = to_string(lsbOut);
						for(int signedInput=0; signedInput<2; signedInput++) {
							string signedInputStr = to_string(signedInput);
							for(auto c:constantList) { // test various constants
								paramList.push_back(make_pair("lsbIn",  lsbInStr));
								paramList.push_back(make_pair("lsbOut", lsbOutStr));
								paramList.push_back(make_pair("msbIn",  msbInStr));
								paramList.push_back(make_pair("signedInput", signedInputStr));
								paramList.push_back(make_pair("constant", c));
								testStateList.push_back(paramList);
								paramList.clear();
							}
						}
					}
				}
			}
		}
		else
		{
				// finite number of random test computed out of index
		}

		return testStateList;
	}

	OperatorPtr FixRealKCM::parseArguments(Target* target, std::vector<std::string> &args)
	{
		int lsbIn, lsbOut, msbIn;
		bool signedInput;
		double targetUlpError;
		string constant;
		UserInterface::parseInt(args, "lsbIn", &lsbIn);
		UserInterface::parseString(args, "constant", &constant);
		UserInterface::parseInt(args, "lsbOut", &lsbOut);
		UserInterface::parseInt(args, "msbIn", &msbIn);
		UserInterface::parseBoolean(args, "signedInput", &signedInput);
		UserInterface::parseFloat(args, "targetUlpError", &targetUlpError);
		return new FixRealKCM(
				target,
				signedInput,
				msbIn,
				lsbIn,
				lsbOut,
				constant,
				targetUlpError
			);
	}

	void FixRealKCM::registerFactory()
	{
		UserInterface::add(
				"FixRealKCM",
				"Table based real multiplier. Output size is computed",
				"ConstMultDiv",
				"",
				"signedInput(bool): 0=unsigned, 1=signed; \
				msbIn(int): weight associated to most significant bit (including sign bit);\
				lsbIn(int): weight associated to least significant bit;\
				lsbOut(int): weight associated to output least significant bit; \
				constant(string): constant given in arbitrary-precision decimal, or as a Sollya expression, e.g \"log(2)\"; \
				targetUlpError(real)=1.0: required precision on last bit. Should be strictly greater than 0.5 and lesser than 1;",
				"This variant of Ken Chapman's Multiplier is briefly described in <a href=\"bib/flopoco.html#DinIstoMas2014-SOPCJR\">this article</a>.<br> Special constants, such as 0 or powers of two, are handled efficiently.",
				FixRealKCM::parseArguments,
				FixRealKCM::unitTest
		);
	}


	/************************** The FixRealKCMTable class ********************/


	FixRealKCMTable::FixRealKCMTable(Target* target, FixRealKCM* mother, int i):
			Table(target,
						mother->m[i] - mother->l[i]+1, // wIn
						mother->m[i] + mother->msbC  - mother->lsbOut + mother->g +1, //wOut TODO: the +1 could sometimes be removed
						0, // minIn
						-1, // maxIn
						1), // logicTable
			mother(mother),
			index(i),
			lsbInWeight(mother->l[i])
	{
		ostringstream name;
		srcFileName="FixRealKCM";
		name << mother->getName() << "_Table_" << index;
		setName(name.str()); // This one not a setNameWithFreqAndUID
		setCopyrightString("Florent de Dinechin (2007-2011-?), 3IF Dev Team");

	}




	mpz_class FixRealKCMTable::function(int x0)
	{
		// This function returns a value that is signed.
		int x;

		// get rid of two's complement
		x = x0;
		//Only the MSB "digit" has a negative weight
		if(mother->tableOutputSign[index]==0)	{ // only in this case interpret input as two's complement
			if ( x0 > ((1<<(wIn-1))-1) )	{
				x -= (1<<wIn);
			}
		} // Now x is a signed number only if it was chunk 0 and its sign bit was set

		//cout << "index=" << index << " x0=" << x0 << "  sx=" << x <<"  wIn="<<wIn<< "   "  <<"  wout="<<wOut<< "   " ;

		mpz_class result;
		mpfr_t mpR, mpX;

		mpfr_init2(mpR, 10*wOut);
		mpfr_init2(mpX, 2*wIn); //To avoid mpfr bug if wIn = 1

		mpfr_set_si(mpX, x, GMP_RNDN); // should be exact
		// Scaling so that the input has its actual weight
		mpfr_mul_2si(mpX, mpX, lsbInWeight, GMP_RNDN); //Exact

		//						double dx = mpfr_get_d(mpX, GMP_RNDN);
		//			cout << "input as double=" <<dx << "  lsbInWeight="  << lsbInWeight << "    ";

		// do the mult in large precision
		if(mother->tableOutputSign[index]==0)
			mpfr_mul(mpR, mpX, mother->mpC, GMP_RNDN);
		else // multiply by the absolute value of the constant, the bitheap logic does the rest
			mpfr_mul(mpR, mpX, mother->absC, GMP_RNDN);

		// Result is integer*mpC, which is more or less what we need: just scale it to an integer.
		mpfr_mul_2si( mpR,
									mpR,
									-mother->lsbOut + mother->g,
									GMP_RNDN	); //Exact

		//			double dr=mpfr_get_d(mpR, GMP_RNDN);
		//			cout << "  dr=" << dr << "  ";

		// Here is when we do the rounding
		mpfr_get_z(result.get_mpz_t(), mpR, GMP_RNDN); // Should be exact

		//cout << mother->tableOutputSign[index] << "  result0=" << result << "  ";


		//cout  << result << "  " <<endl;

		// Add the rounding bit to the table 0 if there are guard bits,
		if(mother->addRoundBit && (index==0) && (mother->g>0)) {
			int roundBit=1<<(mother->g-1);
			// but beware: the table output may be subtracted (see buildTablesForBitHeap() )
			if(mother-> tableOutputSign[0] >= 0) // This table will be added
				result += roundBit;
			else // This table will be subtracted
				result -= roundBit;
		}
		if(result<0) {
			if(mother-> tableOutputSign[index]!=1) {
				// this is a two's complement number with a non-constant sign bit
				// so we encode it as two's complement
				result +=(mpz_class(1) << wOut);
			}
			else{
				THROWERROR("kcmTableContent: For table " << index << " at index " << x0 << ", the computed table entry is " << result <<". This table should contain positive values");
			}
		}

		return result;
	}
}





