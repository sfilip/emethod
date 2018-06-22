/*
  An integer multiplier mess for FloPoCo

  TODO in the virtual multiplier case, manage properly the case when the initial instant is not  0

  Authors:  Bogdan Pasca, and then F de Dinechin, Matei Istoan, Kinga Illyes and Bogdan Popa spent 12 cumulated months getting rid of the last bits of his code

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2012.
  All rights reserved.
*/



// TODO
// Again needs a complete overhaul I'm afraid
// - Improve the case when a multiplier fits one DSP (very common)
//     In this case even for truncated mults we should have simple truncation, g=0 (also in neededGuardBits),
//     and a special management of negate and signedIO


/*
   Who calls whom
   the constructor calls buildLogicOnly or buildTiling
   (maybe these should be unified some day)
   They call buildXilinxTiling or buildAlteraTiling or buildHeapLogicOnly

*/



/* VHDL variable names:
   X, Y: inputs
   XX,YY: after swap

*/






/* For two's complement arithmetic on n bits, the representable interval is [ -2^{n-1}, 2^{n-1}-1 ]
   so the product lives in the interval [-2^{n-1}*2^{n-1}-1,  2^n]
   The value 2^n can only be obtained as the product of the two minimal negative input values
   (the weird ones, which have no opposite)
   Example on 3 bits: input interval [-4, 3], output interval [-12, 16] and 16 can only be obtained by -4*-4.
   So the output would be representable on 2n-1 bits in two's complement, if it werent for this weird*weird case.

   So even for signed multipliers, we just keep the 2n bits, including one bit used for only for this weird*weird case.
   Current situation is: if you don't like this you manage it from outside:
   An application that knows that it will not use the full range (e.g. signal processing, poly evaluation) can ignore the MSB bit,
   but we produce it.

*/




#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "IntMultiplier.hpp"
#include "IntAddSubCmp/IntAdder.hpp"
#include "Targets/StratixII.hpp"
#include "Targets/StratixIII.hpp"
#include "Targets/StratixIV.hpp"
#include "../BitHeap/Plotter.hpp"

   using namespace std;

   namespace flopoco {

#define vhdl parentOp->vhdl
#define declare parentOp->declare
#define inPortMap parentOp->inPortMap
#define outPortMap parentOp->outPortMap
#define instance parentOp->instance
#define useSoftRAM parentOp->useSoftRAM
#define manageCriticalPath parentOp->manageCriticalPath
#define getCriticalPath parentOp->getCriticalPath
#define setCycle parentOp->setCycle
#define oplist parentOp->getOpListR()
#define addSubComponent parentOp->addSubComponent
#define	addToGlobalOpList parentOp->addToGlobalOpList


   	bool IntMultiplier::tabulatedMultiplierP(Target* target, int wX, int wY){
   		return (wX+wY <=  target->lutInputs()+2);
   	}

	//TODO: function must add better handling of corner cases
   	int IntMultiplier::neededGuardBits(Target* target, int wX, int wY, int wOut)
   	{
   		int g;
		if( (wOut >= wX+wY) // no rounding
			 || tabulatedMultiplierP(target, wX, wY) ) // multiplier will be tabulated
		{
			g=0;
		}
		else {
			unsigned i=0;
			mpz_class ulperror=1;
			while(wX+wY - wOut  > intlog2(ulperror)) {
				// REPORT(DEBUG,"i = "<<i<<"  ulperror "<<ulperror<<"  log:"<< intlog2(ulperror) << "  wOut= "<<wOut<< "  wFullP= "<< wX+wY);
				i++;
				ulperror += (i+1)*(mpz_class(1)<<i);
			}
			g=wX+wY-i-wOut;
			// REPORT(DEBUG, "ulp truncation error=" << ulperror << "    g=" << g);
		}
		return g;
	}


	void IntMultiplier::initialize()
	{
		if(wXdecl<0 || wYdecl<0){
			THROWERROR("Error in IntMultiplier::initialize: negative input size");
		}
		wFullP = wXdecl+wYdecl;

		// Halve number of cases by making sure wY<=wX:
		// interchange x and y in case wY>wX
		// After which we negate y (the smaller) by 1/ complementing it and 2/  adding it back to the bit heap

		string newxname, newyname;
		if(wYdecl > wXdecl){
			REPORT(DEBUG, "initialize(): Swapping X and Y")
			wX=wYdecl;
			wY=wXdecl;
			newxname=yname;
			newyname=xname;
		}else
		{
			wX=wXdecl;
			wY=wYdecl;
			newxname=xname;
			newyname=yname;
		}

		// The larger of the two inputs
		vhdl << tab << declare(addUID("XX"), wX, true) << " <= " << newxname << " ;" << endl;

		// possibly negate the smaller of the two inputs
		// 	there is no need for initialize() to do the negation for the corner cases, as they handle the negation themselves
		if(!negate || tabulatedMultiplierP(parentOp->getTarget(), wX, wY) || wY<3)
		{
			vhdl << tab << declare(addUID("YY"), wY, true) << " <= " << newyname << " ;" << endl;
		}else
		{
			vhdl << tab << "-- we compute -xy as x(not(y)+1)" << endl;
			vhdl << tab << declare(addUID("YY"), wY, true) << " <= not " << newyname << " ;" << endl;
		}
	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// The virtual constructor
	IntMultiplier::IntMultiplier (Operator* parentOp_, BitHeap* bitHeap_,
		Signal* x, Signal* y,
		int lsbWeightInBitHeap_,
		bool negate_, bool signedIO_,
		int lsbFullMultWeightInBitheap_):
	Operator (parentOp_->getTarget()),
	lsbWeightInBitHeap(lsbWeightInBitHeap_),
	lsbFullMultWeightInBitheap(lsbFullMultWeightInBitheap_),
	parentOp(parentOp_),
	bitHeap(bitHeap_),
	negate(negate_),
	signedIO(signedIO_)
	{

		multiplierUid=parentOp->getNewUId();
		srcFileName="IntMultiplier";

		xname = x->getName();
		yname = y->getName();
		wXdecl = x->width();
		wYdecl = y->width();

		ostringstream name;
		name <<"VirtualIntMultiplier";

		if(parentOp->getTarget()->useHardMultipliers())
			name << "UsingDSP_";
		else
			name << "LogicOnly_";
		name << wXdecl << "_" << wYdecl <<"_" << (lsbWeightInBitHeap<0?"m":"") << abs(lsbWeightInBitHeap) << "_" << (signedIO?"signed":"unsigned");
		setNameWithFreqAndUID ( name.str() );

		REPORT(DEBUG, "Building " << name.str() );

		// This is for legacy use of wOut -- in principle it could be removed
		wOut=wX+wY;
		if(lsbWeightInBitHeap<0) // truncation
			wOut += lsbWeightInBitHeap;

		initialize();
		fillBitHeap();
		// leave the compression to the parent op
	}



	// The constructor for a stand-alone operator
	IntMultiplier::IntMultiplier (Target* target_, int wX_, int wY_, int wOut_, bool signedIO_, map<string, double> inputDelays_, bool enableSuperTiles_):
	Operator ( target_, inputDelays_ ),
	wXdecl(wX_), wYdecl(wY_), wOut(wOut_),
	negate(false), signedIO(signedIO_),enableSuperTiles(enableSuperTiles_)
	{
		srcFileName="IntMultiplier";
		setCopyrightString ( "Florent de Dinechin, Kinga Illyes, Bogdan Popa, Bogdan Pasca, 2012" );

		// the addition operators need the ieee_std_signed/unsigned libraries
		useNumericStd();

		REPORT(DEBUG, "Entering IntMultiplier standalone constructor for wX_=" <<  wX_  << ", wY_=" << wY_ << ", wOut_=" << wOut_);
		REPORT(DEBUG, "   gettarget()->useHardMultipliers() =" << getTarget()->useHardMultipliers() );

		if(wOut<0)
		{
			THROWERROR("IntMultiplier: in stand-alone constructor: ERROR: negative wOut");
		}
		if(wOut==0) {
			wOut=wXdecl+wYdecl;
			REPORT(DETAILED, "wOut set to " << wOut);
		}

		parentOp=this;

		// set the name of the multiplier operator
		{
			ostringstream name;
			name <<"IntMultiplier";
			if(parentOp->getTarget()->useHardMultipliers())
				name << "_UsingDSP_";
			else
				name << "_LogicOnly_";
			name << wXdecl << "_" << wYdecl <<"_" << wOut << "_" << (signedIO?"signed":"unsigned");
			setNameWithFreqAndUID ( name.str() );
			REPORT(DEBUG, "Building " << name.str() );
		}


		multiplierUid=parentOp->getNewUId();
		xname="X";
		yname="Y";

		initialize();
		int g = neededGuardBits(parentOp->getTarget(), wXdecl, wYdecl, wOut);
		int possibleOutputs=1;
		if(g>0)
		{
			lsbWeightInBitHeap=wOut+g-wFullP; // Should be negative; # truncated bits is the opposite of this number in this case
			possibleOutputs=2;
		}
		else
		{
			lsbWeightInBitHeap=0;
		}
		lsbFullMultWeightInBitheap = lsbWeightInBitHeap;
		REPORT(DEBUG,"" << tab << "Stand-alone constructor: g=" << g << (g==0 ? " (full multiplier)" : " (truncated multiplier)")
			<< "  lsbWeightInBitHeap=" << lsbWeightInBitHeap << "   lsbFullMultWeightInBitheap=" << lsbFullMultWeightInBitheap
			<< "   possibleOutputs=" << possibleOutputs);

		// Set up the IO signals
		addInput ( xname  , wXdecl, true );
		addInput ( yname  , wYdecl, true );
		addOutput ( "R"  , wOut, 2 , true );

		if(target_->plainVHDL()) {
			vhdl << tab << declareFixPoint("XX",signedIO,-1, -wXdecl) << " <= " << (signedIO?"signed":"unsigned") << "(" << xname <<");" << endl;
			vhdl << tab << declareFixPoint("YY",signedIO,-1, -wYdecl) << " <= " << (signedIO?"signed":"unsigned") << "(" << yname <<");" << endl;
			vhdl << tab << declareFixPoint("RR",signedIO,-1, -wXdecl-wYdecl) << " <= XX*YY;" << endl;
			vhdl << tab << "R <= std_logic_vector(RR" << range(wXdecl+wYdecl-1, wXdecl+wYdecl-wOut) << ");" << endl;
		}
		else{

			// The bit heap
			bitHeap = new BitHeap(this, wOut+g, enableSuperTiles);

			//FIXME: a bitheap doesn't have a sign, does it?
			bitHeap->setSignedIO(signedIO);


			// initialize the critical path
			setCriticalPath(getMaxInputDelays ( inputDelays_ ));


			fillBitHeap();

			// For a stand-alone operator, we add the rounding-by-truncation bit,
			// The following turns truncation into rounding, except that the overhead is large for small multipliers.
			// No rounding needed for a tabulated multiplier.
			if(lsbWeightInBitHeap<0 && !tabulatedMultiplierP(target_, wX, wY))
			{
					//int weight = -lsbWeightInBitHeap-1;
				int weight = g-1;

				bitHeap->addConstantOneBit(weight);
			}

			bitHeap -> generateCompressorVHDL();

			vhdl << tab << "R" << " <= " << bitHeap-> getSumName() << range(wOut+g-1, g) << ";" << endl;
		}
	}





	IntMultiplier* IntMultiplier::newComponentAndInstance(
		Operator* parentOp,
															// use this name because of the #defines on top of this file
		string instanceName,
		string xSignalName,
		string ySignalName,
		string rSignalName,
		int rMSB,
		int rLSB
		)
	{
		Signal* x = parentOp->getSignalByName(xSignalName);
		Signal* y = parentOp->getSignalByName(ySignalName);
		bool signedIO = (x->isFixSigned() && y->isFixSigned());
		int wX = x->MSB() - x->LSB() +1;
		int wY = y->MSB() - y->LSB() +1;
		int wOut = rMSB - rLSB +1;
		// TODO Following is a ugly hack to work around bad signed/unsigned logic somewhere else
		// I observe in gtkwave that the result in the unsigned case is two bits to the left of what it should be
#define UGLYHACK 1
#if UGLYHACK
		if(!signedIO)
			wOut+=2;
#endif

		IntMultiplier* f = new IntMultiplier(parentOp->getTarget(),
			wX, wY, wOut, signedIO
			);
		addSubComponent(f);

		inPortMap(f, "X", xSignalName); // without op-> because of the #defines. This is dirty code
		inPortMap(f, "Y", ySignalName);

#if UGLYHACK
		outPortMap(f, "R", join(rSignalName,"i_slv"));  // here rSignalName_slv is std_logic_vector
		vhdl << instance(f, instanceName);

		vhdl << tab << declare(join(rSignalName,"_slv"), wOut-2)
		<< " <= " << (join(rSignalName, "i_slv")) << range(wOut-3, 0) << ";" << endl;
		vhdl << tab << parentOp->declareFixPoint(rSignalName, f->signedIO, rMSB, rLSB)
		<< " <= " <<  (f->signedIO ? "signed(" : "unsigned(") << (join(rSignalName, "_slv")) << range(wOut-3, 0) << ");" << endl;
#else
 // this is how it should be
		outPortMap(f, "R", join(rSignalName,"_slv"));  // here rSignalName_slv is std_logic_vector
		vhdl << instance(f, instanceName);

		vhdl << tab << parentOp->declareFixPoint(rSignalName, f->signedIO, rMSB, rLSB)
		<< " <= " <<  (f->signedIO ? "signed(" : "unsigned(") << (join(rSignalName, "_slv")) << ");" << endl;
#endif
		return f;
	}




	void  IntMultiplier::fillBitHeap()
	{
		Plotter* plotter= bitHeap->getPlotter();
		///////////////////////////////////////
		//  architectures for corner cases   //
		///////////////////////////////////////

		// TODO Support negate in all the corner cases
		// To manage stand-alone cases, we just build a bit-heap of max height one, so the compression will do nothing


		// The really small ones fit in one or two LUTs and that's as small as it gets
		if(tabulatedMultiplierP(parentOp->getTarget(), wX, wY))
		{
			REPORT(DEBUG, " in fillBitHeap(): small multiplication, will use a SmallMultTable");
			vhdl << tab << "-- Ne pouvant me fier a mon raisonnement, j'ai appris par coeur le résultat de toutes les multiplications possibles" << endl;

			SmallMultTable *t = new SmallMultTable(parentOp->getTarget(), wX, wY, wOut, negate, signedIO, signedIO);
			addToGlobalOpList(t);

			//This table is either exact, or correctly rounded if wOut<wX+wY
			// FIXME the offset is probably wrong -- possible fix for now
			vhdl << tab << declare(addUID("XY"), wX+wY)
			<< " <= " << addUID("YY") << " & " << addUID("XX") << ";" << endl;
			inPortMap(t, "X", addUID("XY"));
			outPortMap(t, "Y", addUID("RR"));
			vhdl << instance(t, "multTable");
			useSoftRAM(t);

#if 1
			// commented by Florent who didn't know what to do with the g
			// uncommented by Matei, who thinks he knows how to get around this problem (for now)
			plotter->addSmallMult(0, 0, wX, wY);
			//plotter->plotMultiplierConfiguration(getName(), localSplitVector, wX, wY, wOut, g);
			plotter->plotMultiplierConfiguration(getName(), localSplitVector, wX, wY, wOut,
				(lsbWeightInBitHeap>0 ? lsbWeightInBitHeap-lsbFullMultWeightInBitheap : wX+wY+lsbWeightInBitHeap-wOut));
#endif
			// Copy all the output bits of the multiplier to the bit heap if they are positive
			for (int w=0; w<wFullP; w++)
			{
				int wBH = w+lsbWeightInBitHeap;

				if(wBH >= 0){
					bitHeap->addBit(wBH, join(addUID("RR"), of(wBH)));
				}
			}

			return;
		}

		// Multiplication by 1-bit integer is simple
		if(wY == 1)
		{
			REPORT(DEBUG, " in fillBitHeap(): multiplication by a 1-bit integer");

			ostringstream signExtensionX;
			int xSize = wX;

			if(negate){
				vhdl << tab << "-- we compute -(x*y) as (not(x)+1)*y" << endl;

				if(signedIO)
					signExtensionX << "not(" << addUID("XX") << of(wX-1) << ") & ";
				else
					signExtensionX << "\'1\' & ";
				for(int i=wX+1; i<(int)bitHeap->getMaxWeight(); i++)
					if(signedIO)
						signExtensionX << "not(" << addUID("XX") << of(wX-1) << ") & ";
					else
						signExtensionX << "\'1\' & ";

					vhdl << tab << declare(addUID("XX")+"_neg", wX+(wX+1>=(int)bitHeap->getMaxWeight() ? 1 : 1+bitHeap->getMaxWeight()-wX))
					<< " <= " << signExtensionX.str() << "not(" << addUID("XX") << ");" << endl;
					xSize += (wX+1>(int)bitHeap->getMaxWeight() ? 1 : 1+bitHeap->getMaxWeight()-wX);
				}else{
					if(signedIO)
						signExtensionX << addUID("XX") << of(wX-1) << " & ";
					else
						signExtensionX << "\'0\' & ";
					for(int i=wX+1; i<(int)bitHeap->getMaxWeight(); i++)
						if(signedIO)
							signExtensionX << addUID("XX") << of(wX-1) << " & ";
						else
							signExtensionX << "\'0\' & ";
					}

					vhdl << tab << "-- How to obfuscate multiplication by 1 bit: first generate a trivial bit vector" << endl;

					if (signedIO){
						manageCriticalPath(  parentOp->getTarget()->localWireDelay(wX) +  parentOp->getTarget()->adderDelay(wX+1) );

						vhdl << tab << declare(addUID("RR"), bitHeap->getMaxWeight())  << " <= (" << zg(bitHeap->getMaxWeight())
						<< " - (" << (negate ? addUID("XX")+"_neg" : signExtensionX.str()+addUID("XX"))+((wX>wOut || lsbWeightInBitHeap<0) ? range(xSize-1, xSize-bitHeap->getMaxWeight()) : "") << "))"
						<< " when " << addUID("YY") << "(0)='1' else " << zg(bitHeap->getMaxWeight(), 0) << ";" << endl;
					}
					else{
						manageCriticalPath(  parentOp->getTarget()->localWireDelay(wX) +  parentOp->getTarget()->lutDelay() );

						vhdl << tab  << declare(addUID("RR"), bitHeap->getMaxWeight()) << " <= ("
						<< (negate ? addUID("XX")+"_neg" : signExtensionX.str()+addUID("XX"))+((wX>wOut || lsbWeightInBitHeap<0) ? range(xSize-1, xSize-bitHeap->getMaxWeight()) : "") << ")"
						<< " when " << addUID("YY") << "(0)='1' else " << zg(bitHeap->getMaxWeight(), 0) << ";" << endl;
					}

					vhdl << tab << "-- then send its relevant bits to a useless bit heap " << endl;

					for (int w=0; w<(int)bitHeap->getMaxWeight(); w++){
						int wBH = w+(lsbWeightInBitHeap>0 ? lsbWeightInBitHeap : 0);
						if(wBH >= 0)
							bitHeap->addBit(wBH, join(addUID("RR"), of(wBH)));
					}
					if((negate) && (lsbWeightInBitHeap >= 0)){
				//bitHeap->addBit(lsbWeightInBitHeap, addUID("YY")+of(0));
						if(signedIO)
							bitHeap->subtractUnsignedBitVector(lsbWeightInBitHeap, addUID("YY"), 1);
						else
							bitHeap->addUnsignedBitVector(lsbWeightInBitHeap, addUID("YY"), 1);
					}

					vhdl << tab << "-- then compress this height-1 bit heap by doing nothing" << endl;

					outDelayMap[addUID("R")] = getCriticalPath();
					return;
				}

		// Multiplication by 2-bit integer is one addition, which is delegated to BitHeap compression anyway
		// TODO this code mostly works but it is large and sub-optimal (adding 0s to bit heap)
				if(wY == 2)
				{
					REPORT(DEBUG, "in fillBitHeap(): multiplication by a 2-bit integer");

					string x 	= addUID("XX");
					string y 	= addUID("YY");
					string xneg = x + "_neg";
					ostringstream signExtensionR0, signExtensionR1_pre, signExtensionR1_post;

					if(negate){
						vhdl << tab << "-- we compute -(x*y) as (not(x)+1)*y" << endl;
						vhdl << tab << declare(xneg, wX) << " <= not(" << x << ");" << endl;
					}

			//sign extensions
					if(signedIO){
						signExtensionR0 << (negate ? "not(" : "") << addUID("XX") << of(wX-1) << (negate ? ")" : "") << " & "
						<< (negate ? "not(" : "") << addUID("XX") << of(wX-1) << (negate ? ")" : "") << " & ";
						signExtensionR1_pre		<< (negate ? "not(" : "") << addUID("XX") << of(wX-1) << (negate ? ")" : "") << " & ";
						signExtensionR1_post	<< " & \'0\'";
					}else{
						signExtensionR0 << (negate ? "\"11\"" : "\"00\"") << " & ";
						signExtensionR1_pre << (negate ? "\'1\'" : "\'0\'") << " & ";
						signExtensionR1_post << " & \'0\'";
					}

			//create the intermediary signals for the multiplication
					vhdl << tab << declare(addUID("R0"),wX+2) << " <= ("
					<< signExtensionR0.str() << (negate ? xneg : x) << ") when " << y << "(0)='1' else " << zg(wX+2,0) << ";" << endl;

					vhdl << tab << declare(addUID("R1i"),wX+2) << " <= ("
					<< signExtensionR1_pre.str() << (negate ? xneg : x) << signExtensionR1_post.str() << ") when " << y << "(1)='1' else " << zg(wX+2,0) << ";" << endl;

					vhdl << tab << declare(addUID("R1"),wX+2) << " <= "
					<< (signedIO ? "not " : "") << addUID("R1i") << ";" << endl;

			//add the bits to the bitheap
					for(int w=0; w<wX+1; w++){
						int wBH = w+lsbWeightInBitHeap;

						if(wBH >= 0){
							bitHeap->addBit(wBH, join(addUID("R0"), of(w)));
							bitHeap->addBit(wBH, join(addUID("R1"), of(w)));
						}
					}
			//sign extension, if needed (no point in sign extending for just one bit)
					if(wX+2<(int)bitHeap->getMaxWeight()){
						bitHeap->addBit(wX+1+lsbWeightInBitHeap, "not("+join(addUID("R0")+")", of(wX+1)));
						bitHeap->addBit(wX+1+lsbWeightInBitHeap, "not("+join(addUID("R1")+")", of(wX+1)));

						for(int w=wX+1+lsbWeightInBitHeap; w<(int)bitHeap->getMaxWeight(); w++){
							bitHeap->addConstantOneBit(w);
							bitHeap->addConstantOneBit(w);
						}
					}else{
						bitHeap->addBit(wX+1+lsbWeightInBitHeap, join(addUID("R0"), of(wX+1)));
						bitHeap->addBit(wX+1+lsbWeightInBitHeap, join(addUID("R1"), of(wX+1)));
					}

			//add y to the bit-heap, if needed
					if((negate)){
						if(signedIO){
							if(lsbWeightInBitHeap >= 0)
								bitHeap->addSignedBitVector(lsbWeightInBitHeap, addUID("YY"), 2);
							else if(lsbWeightInBitHeap >= -1)
								bitHeap->addSignedBitVector(lsbWeightInBitHeap+1, addUID("YY")+of(1), 1);
						}else{
							if(lsbWeightInBitHeap >= 0)
								bitHeap->addUnsignedBitVector(lsbWeightInBitHeap, addUID("YY"), 2);
							else if(lsbWeightInBitHeap >= -1)
								bitHeap->addUnsignedBitVector(lsbWeightInBitHeap+1, addUID("YY")+of(1), 1);
						}
					}

			//add one bit to complete the negation of R1
					if(signedIO && lsbWeightInBitHeap>=0){
						bitHeap->addConstantOneBit(lsbWeightInBitHeap);
					}

					return;
				}

		// Multiplications that fit directly into a DSP
				int dspXSize, dspYSize;

				parentOp->getTarget()->getDSPWidths(dspXSize, dspYSize, signedIO);

		//correct the DSP sizes for Altera targets
				if(parentOp->getTarget()->getVendor() == "Altera")
				{
					if(dspXSize >= 18){
						dspXSize = 18;
					}
					if(dspYSize >= 18){
						dspYSize = 18;
					}
					if(!signedIO){
						dspXSize--;
						dspYSize--;
					}
				}

		//TODO: write a multiplication that fits into a DSP in a synthetsizable way,
		//		so that both an addition and a multiplication can fit in a DSP
		//		and that the multiplier can be pipelined using the internal registers

		//If the DSP utilization ratio is satisfied, then just implement
		//	the multiplication in a DSP, without passing through a bitheap
		//	the last three conditions ensure that the multiplier can actually fit in a DSP
				REPORT(DEBUG, "in fillBitHeap(): testing for multiplications that fit directly into a DSP multiplier block");
				if((wX <= dspXSize) && (wY <= dspYSize) && worthUsingOneDSP( 0, 0, wX, wY, dspXSize, dspYSize))
				{
					REPORT(DEBUG, "in fillBitHeap(): multiplication that fits directly into a DSP multiplier block");

					ostringstream s, zerosXString, zerosYString, zerosYNegString;
					int zerosX = dspXSize - wX + (signedIO ? 0 : 1);
					int zerosY = dspYSize - wY + (signedIO ? 0 : 1);
			//int startingIndex, endingIndex;

					if(zerosX<0)
						zerosX=0;
					if(zerosY<0)
						zerosY=0;

			//sign extension of the inputs (or zero-extension, if working with unsigned numbers)
					if(signedIO){
				//sign extension
				//	for X
						zerosXString << (zerosX>0 ? "(" : "");
						for(int i=0; i<zerosX; i++)
							zerosXString << addUID("XX") << of(wX-1) << (i!=(zerosX-1) ? " & " : "");
						zerosXString << (zerosX>0 ? ")" : "");
				//	for Y and Y negated
						zerosYString << (zerosY>0 ? "(" : "");
						zerosYNegString << (zerosY>0 ? "(" : "");
						for(int i=0; i<zerosY; i++){
							zerosYString 	<< addUID("YY") << of(wY-1) << (i!=(zerosY-1) ? " & " : "");
							zerosYNegString << "(not " << addUID("YY") << of(wY-1) << ")" << (i!=(zerosY-1) ? " & " : "");
						}
						zerosYString << (zerosY>0 ? ")" : "");
						zerosYNegString << (zerosY>0 ? ")" : "");
					}
					else{
				//zero extension
						zerosXString 	<< (zerosX>0 ? zg(zerosX) : "");
						zerosYString 	<< (zerosY>0 ? zg(zerosY) : "");
						zerosYNegString << (zerosY>0 ? og(zerosY) : "");
					}

			//if negated, the product becomes -xy = not(y)*x + x
			//	if not negated, the product remains xy=x*y
			//TODO: this should be more efficient than negating the product at
			//		the end, as it should be implemented in a single DSP, both the multiplication and the addition
					if(negate)
						vhdl << tab << declare(addUID("YY")+"_neg", wY+zerosY)
					<< " <= " << (zerosY>0 ? join(zerosYNegString.str(), " & ") : "") << " not(" << addUID("YY") << ");" << endl;

			//manage the pipeline
					manageCriticalPath(parentOp->getTarget()->DSPMultiplierDelay());
					s << "DSP_Res_" <<  getuid();
					string sou=(negate || signedIO ? "signed": "unsigned");
					vhdl << tab << declare(s.str(), dspXSize+dspYSize+(signedIO ? 0 : 2))
					<< " <=  std_logic_vector(" << sou << "'(";
					vhdl << sou <<  "(" << (zerosX>0 ? join(zerosXString.str(), " & ") : "") << addUID("XX") << ")"
					<< " * ";
					vhdl	 << sou ;
					if(negate)
						vhdl <<  "(" << addUID("YY") << "_neg)";
					else
						vhdl	 << "(" << (zerosY>0 ? zerosYString.str()+" & " : "") << addUID("YY") << ")";
					vhdl << "));" << endl;

			//manage the pipeline: TODO
			//syncCycleFromSignal(s.str());

			//add the bits of x*(not y) (respectively x*y, when not negated)
					for (int w=0; w<(wFullP-1); w++)
					{
						int wBH = w+lsbWeightInBitHeap;
						if(wBH >= 0)
						{
							bitHeap->addBit(wBH, join(s.str(), of(w)));
						}
					}
			//treat the msb bit of x*y differently, as sign extension might be needed
					if(signedIO && ((int)bitHeap->getMaxWeight()>(wFullP+lsbWeightInBitHeap)))
						bitHeap->addBit(wFullP-1+lsbWeightInBitHeap, "not("+s.str()+of(wFullP-1)+")");
					else
				//no sign extension needed
						bitHeap->addBit(wFullP-1+lsbWeightInBitHeap, s.str()+of(wFullP-1));

			//keep sign-extending the product, if necessary
					if(signedIO && ((int)bitHeap->getMaxWeight()>(wFullP+lsbWeightInBitHeap)))
						for(int w=wFullP-1+lsbWeightInBitHeap; w<(int)bitHeap->getMaxWeight(); w++)
							if(w >= 0)
								bitHeap->addConstantOneBit(w);

			//if the product is negated, then add the bits of x, (not x)<<2^wY, 2^wY
							if(negate)
							{
				//add x
								for(int w=0; w<(wX-1); w++)
									if(w+lsbWeightInBitHeap >= 0)
										bitHeap->addBit(w+lsbWeightInBitHeap, join(addUID("XX"), of(w)));
				//treat the msb bit of x differently, as sign extension might be needed
									if(signedIO && ((int)bitHeap->getMaxWeight()>(wX+lsbWeightInBitHeap)))
										bitHeap->addBit(wX-1+lsbWeightInBitHeap, "not("+addUID("XX")+of(wX-1)+")");
									else
					//no sign extension needed
										bitHeap->addBit(wX-1+lsbWeightInBitHeap, addUID("XX")+of(wX-1));

				//x, when added and using signed operators, should be sign-extended
									if(signedIO && ((int)bitHeap->getMaxWeight()>(wX+lsbWeightInBitHeap)))
										for(int w=wX-1+lsbWeightInBitHeap; w<(int)bitHeap->getMaxWeight(); w++)
											if(w >= 0)
												bitHeap->addConstantOneBit(w);
										}

			// this should be it
										return;
									}

		// Now getting more and more generic

		// Finish the negation of the smaller input by adding X (since -yx=not(y)x + x)

		//		setCycle(0); // TODO F2D FIXME for the virtual multiplier case where inputs can arrive later
		// setCriticalPath(initialCP);
#if 0 // TODO re-plug it
		if(negate){
			vhdl << tab << "-- Finish the negation of the product (-xy as x((111..111-y)+111..111)) by adding XX + 2^wY.not XX +  2^wY" << endl;

			// Adding XX
			for(int i=0; i<wX; i++)
			{
				int w = lsbWeightInBitHeap + i-truncatedBits;
				if(w>=0)
				{
					ostringstream rhs;
					if(signedIO && i==wX-1){
						rhs << addUID("not XX") << of(i);
						// sign extension
						for(unsigned int j=i; j<bitHeap->getMaxWeight(); j++)
							bitHeap->addConstantOneBit(j);
					}
					else
						rhs << addUID("XX") << of(i);
					bitHeap->addBit(w, rhs.str());
				}
			}

			// Adding 2^(wY+1) not XX
			for(int i=0; i<wX; i++)
			{
				int w = wY + lsbWeightInBitHeap + i-truncatedBits;
				if(w>=0 && w<(int)bitHeap->getMaxWeight())
				{
					ostringstream rhs;
					rhs << "not " << addUID("XX") << of(i);
					bitHeap->addBit(w, rhs.str());
				}
			}
			int w = wY + lsbWeightInBitHeap -truncatedBits;
			if(w>=0 && w<(int)bitHeap->getMaxWeight())
				bitHeap->addConstantOneBit(wY + lsbWeightInBitHeap -truncatedBits);

		}
#endif

		if(parentOp->getTarget()->useHardMultipliers()){
			REPORT(DETAILED,"in fillBitHeap(): using DSP blocks for multiplier implementation");
			parentOp->getTarget()->getDSPWidths(wxDSP, wyDSP, signedIO);
			REPORT(DETAILED,"in fillBitHeap(): starting tiling with DSPs");
			buildTiling();
		}
		else{
			// This target has no DSP, going for a logic-only implementation
			REPORT(DETAILED,"in fillBitHeap(): logic-only multiplier implementation");
			REPORT(DETAILED,"in fillBitHeap(): starting logic only tiling");
			buildLogicOnly();
		}


		if(parentOp->getTarget()->generateFigures()) {
			// before update plotter->plotMultiplierConfiguration(getName(), localSplitVector, wX, wY, wOut, g);
			int guardBits = (lsbWeightInBitHeap>0) ? lsbWeightInBitHeap-lsbFullMultWeightInBitheap : wX+wY+lsbWeightInBitHeap-wOut;

			plotter->plotMultiplierConfiguration(getName(), localSplitVector,
					wX, wY, /*wOut*/ wX+wY+lsbWeightInBitHeap-guardBits, /*g*/ guardBits);
		}
	}




	/**************************************************************************/
	void IntMultiplier::buildLogicOnly()
	{
		buildHeapLogicOnly(0,0,wX,wY);
	}


	/**************************************************************************/
	void IntMultiplier::buildTiling()
	{
#if 1
		int* multiplierWidth;
		int size;

		if( parentOp->getTarget()->getVendor() == "Altera")
		{
			REPORT(DEBUG, "in buildTiling(): will construct tiling for Altera targets");
			if ( parentOp->getTarget()->getID()=="StratixII")
			{
				StratixII* t = (StratixII*) parentOp->getTarget();
				multiplierWidth = t->getDSPMultiplierWidths();
				size = t->getNrDSPMultiplier();

			}else if( parentOp->getTarget()->getID()=="StratixIII")
			{
				StratixIII* t = (StratixIII*) parentOp->getTarget();
				multiplierWidth = t->getDSPMultiplierWidths();
				size = t->getNrDSPMultiplier();

			}else if( parentOp->getTarget()->getID()=="StratixIV")
			{
				StratixIV* t = (StratixIV*) parentOp->getTarget();
				multiplierWidth = t->getDSPMultiplierWidths();
				size = t->getNrDSPMultiplier();

			}else //add Altera chips here
			{
				StratixII* t = (StratixII*) parentOp->getTarget();
				multiplierWidth = t->getDSPMultiplierWidths();
				size = t->getNrDSPMultiplier();
			}

			for(int i=0; i<size; i++)
				multWidths.push_back(multiplierWidth[i]);

			buildAlteraTiling(0, 0, wX, wY, 0, signedIO, signedIO);
		}else
		{
			// Xilinx here
			REPORT(DEBUG, "in buildTiling(): will construct tiling for Xilinx targets");
			if((!signedIO) && ((wX==41) && (wY==41)))
				buildFancy41x41Tiling();
			else
				buildXilinxTiling();
		}
#endif
	}


	//the fancy tiling is used only for a hardwired case 41 41 82
	/***********************************************************************/
	void IntMultiplier::buildFancy41x41Tiling()
	{
		//THROWERROR("fancy tiling not implemented yet");

		stringstream inx,iny;
		inx<<addUID("XX");
		iny<<addUID("YY");


		int widthX, widthY,topx,topy;

		//topright dsp;
		widthX=wxDSP;
		widthY=wyDSP;
		topx=0;
		topy=0;
		MultiplierBlock* m = new MultiplierBlock(widthX,widthY,topx,topy,inx.str(),iny.str(), lsbWeightInBitHeap);
		m->setNext(NULL);
		m->setPrevious(NULL);
		localSplitVector.push_back(m);
		bitHeap->addMultiplierBlock(m);


		//topleft dsp
		widthX=wyDSP;
		widthY=wxDSP;
		topx=wxDSP;
		topy=0;
		m = new MultiplierBlock(widthX,widthY,topx,topy,inx.str(),iny.str(), lsbWeightInBitHeap);
		m->setNext(NULL);
		m->setPrevious(NULL);
		localSplitVector.push_back(m);
		bitHeap->addMultiplierBlock(m);

		//bottomleft dsp
		widthX=wxDSP;
		widthY=wyDSP;
		topx=wyDSP;
		topy=wxDSP;
		m = new MultiplierBlock(widthX,widthY,topx,topy,inx.str(),iny.str(), lsbWeightInBitHeap);
		m->setNext(NULL);
		m->setPrevious(NULL);
		localSplitVector.push_back(m);
		bitHeap->addMultiplierBlock(m);

		//bottomright dsp
		widthX=wyDSP;
		widthY=wxDSP;
		topx=0;
		topy=wyDSP;
		m = new MultiplierBlock(widthX,widthY,topx,topy,inx.str(),iny.str(), lsbWeightInBitHeap);
		m->setNext(NULL);
		m->setPrevious(NULL);
		localSplitVector.push_back(m);
		bitHeap->addMultiplierBlock(m);

		//logic

		buildHeapLogicOnly(wyDSP,wyDSP,wxDSP,wxDSP,parentOp->getNewUId());

	}


	/**************************************************************************/
	void IntMultiplier::buildHeapLogicOnly(int lsbX, int lsbY, int msbX, int msbY,int blockUid)
	{
		REPORT(FULL,"buildHeapLogicOnly called for " << lsbX << " " << lsbY << " " << msbX << " " << msbY);
		Target *target= parentOp->getTarget();

		if(blockUid == -1)
			blockUid++;    /// ???????????????

		vhdl << tab << "-- code generated by IntMultiplier::buildHeapLogicOnly()"<< endl;
		vhdl << tab << "-- buildheaplogiconly called for lsbX=" << lsbX << " lsbY=" << lsbY << " msbX="<< msbX << " msbY="<< msbY << endl;

		int dx, dy;				// Here we need to split in small sub-multiplications
		int li = target->lutInputs();

		dx = li >> 1;
		dy = li - dx;
		REPORT(DEBUG, "in buildHeapLogicOnly(): small multiplier sizes: dx=" << dx << "  dy=" << dy );

		int wXX=wX;
		int wYY=wY;

		int wX = msbX-lsbX;
		int wY = msbY-lsbY;
		int chunksX =  int(ceil( ( double(wX) / (double) dx) ));
		int chunksY =  int(ceil( ( 1+double(wY-dy) / (double) dy) ));
		int sizeXPadded = dx*chunksX;
		int sizeYPadded = dy*chunksY;

		int padX = sizeXPadded-wX;
		int padY = sizeYPadded-wY;

		REPORT(DEBUG, "in buildHeapLogicOnly(): X split in "<< chunksX << " chunks and Y in " << chunksY << " chunks; ");
		REPORT(DEBUG, "in buildHeapLogicOnly():   sizeXPadded=" << sizeXPadded << "  sizeYPadded=" << sizeYPadded);

		//we do more than 1 sub-product
		// FIXME where is the else?
		//		 possible answer: the case handled by the else should never happen
		//						  -> it is a corner case and should have already been handled at this point
		//						  actually, there shouldn't be a need for the if, in the first case
		if (chunksX + chunksY >= 2)
		{
			// Padding X to the right
			vhdl << tab << declare(addUID("Xp", blockUid), sizeXPadded) << " <= "
			<< addUID("XX") << range(msbX-1,lsbX) << " & " <<zg(padX) << ";" << endl;
			REPORT(DEBUG, "in buildHeapLogicOnly(): " << addUID("XX") << range(msbX-1,lsbX) << " & " << zg(padX)<<";");

			// Padding Y to the right
			vhdl << tab << declare(addUID("Yp",blockUid), sizeYPadded)<<" <= "
			<< addUID("YY") << range(msbY-1,lsbY) << " & "<< zg(padY) << ";" << endl;
			REPORT(DEBUG, "in buildHeapLogicOnly(): " << addUID("YY") << range(msbY-1,lsbY) << " & " << zg(padY)<<";");

			//SPLITTING the inputs
			for (int k=0; k<chunksX ; k++)
			{
				vhdl << tab << declare(join(addUID("x",blockUid),"_",k),dx) << " <= "<< addUID("Xp",blockUid) << range((k+1)*dx-1,k*dx)<<";"<<endl;
				REPORT(DEBUG, "in buildHeapLogicOnly(): " << join(addUID("x",blockUid),"_",k) << " <= " << addUID("Xp",blockUid) << range((k+1)*dx-1,k*dx) << ";");
			}
			for (int k=0; k<chunksY ; k++)
			{
				vhdl << tab << declare(join(addUID("y",blockUid),"_",k),dy) << " <= " << addUID("Yp",blockUid) << range((k+1)*dy-1, k*dy)<<";"<<endl;
				REPORT(DEBUG, "in buildHeapLogicOnly(): " << join(addUID("y",blockUid),"_",k)<<" <= " << addUID("Yp",blockUid) << range((k+1)*dy-1,k*dy) << ";");
			}

			SmallMultTable *tUU, *tSU, *tUS, *tSS;

			// In the negate case we will negate the bits coming out of this table
			tUU = new SmallMultTable( target, dx, dy, dx+dy, false /*negate*/, false /*signx*/, false/*signy*/);
			addToGlobalOpList(tUU);

			if(signedIO)
			{ // need for 4 different tables

				tSU = new SmallMultTable( target, dx, dy, dx+dy, false, true, false );
				addToGlobalOpList(tSU);

				tUS = new SmallMultTable( target, dx, dy, dx+dy, false, false, true );
				addToGlobalOpList(tUS);


				tSS = new SmallMultTable( target, dx, dy, dx+dy, false, true, true );
				addToGlobalOpList(tSS);
			}
			setCycle(0); // TODO FIXME for the virtual multiplier case where inputs can arrive later

			// SmallMultTable is built to cost this:
			double delay=  parentOp->getTarget()->localWireDelay(chunksX) + parentOp->getTarget()->lutDelay();
			manageCriticalPath( delay) ;
			for (int iy=0; iy<chunksY; iy++)
			{
				vhdl << tab << "-- Partial product row number " << iy << endl;
				for(int ix=0; ix<chunksX; ix++)
				{
					SmallMultTable *t;

					if(!signedIO)
						t=tUU;
					else
					{
						// 4  cases
						if( ((ix==chunksX-1)&&(msbX==wXX)) && ((iy==chunksY-1)&&(msbY==wYY) ))
							t=tSS;
						else if ((ix==chunksX-1)&&(msbX==wXX))
							t=tSU;
						else if ((iy==chunksY-1)&&(msbY==wYY))
							t=tUS;
						else
							t=tUU;
					}
					REPORT(FULL, "ix=" << ix << "  iy=" << iy);
					//smallMultTable needed only if it is on the left of the truncation line
					// was if(dx*(ix+1)+dy*(iy+1)+lsbX+lsbY-padX-padY > wFullP-wOut-g)
					if(dx*(ix+1)+dy*(iy+1)+lsbX+lsbY-padX-padY + lsbWeightInBitHeap > 0)
					{
						bitHeap->getPlotter()->addSmallMult(dx*(ix)+lsbX-padX, dy*(iy)+lsbY-padY,dx,dy);
						REPORT(FULL, "in buildHeapLogicOnly(): adding a small multiplier");
						REPORT(FULL, "in buildHeapLogicOnly(): " << XY(ix,iy,blockUid)
							<< " <= " << addUID("y",blockUid) << "_" << iy << " & " << addUID("x",blockUid) <<"_" << ix << ";");

						vhdl << tab << declare(XY(ix,iy,blockUid), dx+dy)
						<< " <= " << addUID("y",blockUid) << "_" << iy << " & " << addUID("x",blockUid) << "_" << ix << ";" << endl;

						inPortMap(t, "X", XY(ix,iy,blockUid));
						outPortMap(t, "Y", PP(ix,iy,blockUid));
						vhdl << instance(t, PPTbl(ix,iy,blockUid));
						useSoftRAM(t);

						vhdl << tab << "-- Adding the relevant bits to the heap of bits" << endl;
						REPORT(DEBUG, "in buildHeapLogicOnly(): " << "  Adding the relevant bits to the heap of bits");

						// Two's complement management
						// There are really 2 cases:
						// If the result is known positive, i.e. if tUU and !negate, nothing to do
						// If the result is in two's complement
						//    sign extend by adding ones on weights >= the MSB of the table, so its sign is propagated.
						//    Also need to complement the sign bit


						// The following comments are obsolete since we negate X at the beginning of the operator:
						// Note that even when negate and tUU, the result is known negative, but it may be zero, so its actual sign is not known statically
						// Note also that in this case (negate and tUU), the result overflows the dx+dy two's complement format.
						// This is why tUU is never negated above, and we negate it below. A bit messy, but probably the most resource-efficient


						bool resultSigned = false;

						if((t==tSS) || (t==tUS) || (t==tSU))
							resultSigned = true ;

						int maxK=t->wOut; // or, dx+dy + (t==tUU && negate?1:0);
						int minK=0;
						//if(ix == chunksX-1)
						if ((ix == 0))
							minK+=padX;
						if((iy == 0))
							//maxK-=padY;
							minK+=padY;

						REPORT(FULL, "in buildHeapLogicOnly(): " << "The bits will be added from mink=" << minK << " to maxk=" << maxK);
						REPORT(FULL,  "in buildHeapLogicOnly(): " << "ix=" << ix << " iy=" << iy << "  maxK=" << maxK
							<< "  negate=" << negate <<  "  resultSigned="  << resultSigned );

						for (int k=minK; k<maxK; k++) {
							ostringstream s, nots;
							s << PP(ix,iy,blockUid) << of(k); // right hand side
							nots << "not " << s.str();

							int weight =  ix*dx+iy*dy+k  +lsbX+lsbY-padX-padY + lsbWeightInBitHeap;

							if(weight>=0)
							{
								// otherwise these bits are just truncated
								if(resultSigned && (k==maxK-1))
								{
									// This is a sign bit: sign-extend it by 1/ complementing and 2/ adding constant 1s all the way up to maxweight
									REPORT(FULL, "in buildHeapLogicOnly(): " << "adding neg bit " << nots.str() << " at weight " << weight);
									bitHeap->addBit(weight, nots.str());
									REPORT(FULL,  "in buildHeapLogicOnly(): " << "  adding constant ones from weight "<< weight << " to "<< bitHeap->getMaxWeight());
									for (unsigned w=weight; w<bitHeap->getMaxWeight(); w++) {
										// REPORT(DEBUG, "w=" << w);
										bitHeap->addConstantOneBit(w);
									}
								}
								else
								{
									// just add the bit
									bitHeap->addBit(weight, s.str());
								}
							}
						}

						vhdl << endl;
					}
				}
				REPORT(FULL, "in buildHeapLogicOnly(): " << "Exiting buildHeapLogicOnly()");
			}
		}

	}

	/** checks how many DSPs will be used in case of a tiling **/
	int IntMultiplier::checkTiling(int wxDSP, int wyDSP, int& horDSP, int& verDSP)
	{
		int widthOnX=wX;
		int widthOnY=wY;
		int horDS=0;
		int verDS=0;

		//**** how many dsps will be vertical*******************************/
		int hor=0;

		//if the multiplication is signed, the first DSP will have a different size, will be bigger
		if( widthOnX>=wxDSP)
		{
			hor++;
			widthOnX-=wxDSP;
		}

		if(signedIO)
			wxDSP--;

		//how many DSPs fits on the remaining part, without the first one
		horDS=int(ceil ( (double(widthOnX) / (double) wxDSP)))+hor;
		/***********************************************************************/


		//*** how many dsps will be horizontal**********************************/
		int ver=0;

		if( widthOnY>=wyDSP)
		{
			ver++;
			widthOnY-=wyDSP;
		}


		if(signedIO)
			wyDSP--;

		verDS=int(ceil ( (double(widthOnY) / (double) wyDSP)))+ver;
		//***********************************************************************/

		horDSP=horDS;
		verDSP=verDS;

		return verDS*horDS;
	}


	void IntMultiplier::addExtraDSPs(int lsbX, int lsbY, int botx, int boty, int wxDSP, int wyDSP, bool isDSPImplementable)
	{
#if 1
		REPORT(DEBUG, "in addExtraDSPs(): at DSPs of size sizeX=" << wxDSP << " and sizeY=" << wyDSP
			<< ": lsbX=" << lsbX << " lsbY=" << lsbY << " msbX=" << botx << " msbY=" << boty);
		int topx=lsbX, topy=lsbY;

		//if the block is on the margins of the multipliers, then the coordinates have to be reduced.
		if(lsbX<0)
			topx=0;
		if(lsbY<0)
			topy=0;

		//if the truncation line splits the block, then the used block is smaller, the coordinates need to be updated
		// was		if((botx+boty > wFullP-wOut-g) && (topx+topy < wFullP-wOut-g))
		// before update if((botx+boty > wFullP-wOut-g) && (topx+topy < wFullP-wOut-g))
		if((botx+boty + lsbWeightInBitHeap > 0) && (topx+topy + lsbWeightInBitHeap < 0))
		{
			int x=topx;
			int y=boty;
			// before update while((x+y<wFullP-wOut-g)&&(x<botx))
			while((x+y + lsbWeightInBitHeap < 0) && (x < botx))
			{
				x++;
				topx=x;
			}

			x=botx;
			y=topy;
			//before update while((x+y<wFullP-wOut-g)&&(y<boty))
			while((x+y + lsbWeightInBitHeap < 0) && (y < boty))
			{
				y++;
				topy=y;
			}
		}

		//now check against the DSPThreshold
		if(isDSPImplementable || worthUsingOneDSP(topx, topy, botx, boty, wxDSP, wyDSP))
		{
			//worth using DSP
			topx = botx-wxDSP;
			topy = boty-wyDSP;

			int weightShiftDSP = lsbWeightInBitHeap;

			MultiplierBlock* m;
			m = new MultiplierBlock(wxDSP, wyDSP, topx, topy, addUID("XX"), addUID("YY"), weightShiftDSP);
			m->setNext(NULL);
			m->setPrevious(NULL);
			localSplitVector.push_back(m);
			bitHeap->addMultiplierBlock(m);
			REPORT(DEBUG, "in addExtraDSPs(): adding a multiplier block of size wxDSP=" << wxDSP << " and wyDSP=" << wyDSP
				<< ": lsbX=" << topx << " lsbY=" << topy << " weightShiftDSP=" << weightShiftDSP << " weight=" << m->getWeight());

		}
		else
		{
			//build logic
			if((topx < botx) && (topy < boty))
			{
				REPORT(DEBUG, "in addExtraDSPs(): adding a multiplier as logic-only at coordinates topX=" << topx << " topY=" << topy
					<< " botX=" << botx << " botY=" << boty);
				buildHeapLogicOnly(topx, topy, botx, boty, parentOp->getNewUId());
			}
		}

		REPORT(DEBUG, "in addExtraDSPs(): Exiting addExtraDSPs");
#endif
	}


	/**
	 * Checks the area usage of a DSP, according to a given block and DSPThreshold(threshold)
	 * 		- DSPThreshold(threshold) = says what percentage of 1 DSP area is allowed to be lost
	 * Algorithm: compute the area which is used out of a DSP, and compute
	 * the unused ratio from this. The used area can be a triangle, a trapeze or
	 * a pentagon, in the truncated case, or a rectangle in the case of a full
	 * multiplier.
	 */
	/**
	 * Definition of the DSP use threshold t:
	 * Consider a submultiplier block, by definition smaller than (or equal to) a DSP in both dimensions
	 * let r=(submultiplier area)/(DSP area); r is between 0 and 1
	 * if r >= 1-t   then use a DSP for this block
	 * So: t=0 means: any submultiplier that does not fill a DSP goes to logic
	 *   t=1 means: any submultiplier, even very small ones, go to DSP
	*/

	 bool IntMultiplier::worthUsingOneDSP(int topX, int topY, int botX, int botY, int wxDSP, int wyDSP)
	 {
#if 1
	 	Target* target = parentOp->getTarget();
	 	REPORT(DEBUG, "in worthUsingOneDSP at coordinates: topX=" << topX << " topY=" << topY << " botX=" << botX << " botY" << botY
	 		<< " with DSP size wxDSP=" << wxDSP << " wyDSP=" << wyDSP);

	 	double intersectRightX, intersectRightY, intersectTopX, intersectTopY, intersectBottomX, intersectBottomY, intersectLeftX, intersectLeftY;
	 	double intersectX1, intersectY1, intersectX2, intersectY2;
	 	int aTopEdge, bTopEdge, cTopEdge, aBottomEdge, bBottomEdge, cBottomEdge, aRightEdge, bRightEdge, cRightEdge, aLeftEdge, bLeftEdge, cLeftEdge;
	 	int aTruncLine, bTruncLine, cTruncLine;
	 	double usefulDSPArea, totalDSPArea;

		//if the truncation line passes under the DSP block
		// before update if((wFullP == wOut) || (lsbX+lsbY > wFullP-wOut-g))
	 	if(((lsbWeightInBitHeap >= 0) && (lsbWeightInBitHeap == lsbFullMultWeightInBitheap)) || (topX+topY+lsbWeightInBitHeap >= 0))
	 	{
	 		REPORT(DEBUG, "in worthUsingOneDSP:   full multiplication case, truncation line passes under this block");
	 		int x = (topX > (botX-wxDSP)) ? topX :botX-wxDSP;
	 		int y = (topY > (botY-wyDSP)) ? topY : botY-wyDSP;
			// REPORT(DEBUG, "*********** x=" << x << "  y=" << y);

	 		usefulDSPArea = (botX-x)*(botY-y);
	 		totalDSPArea = wxDSP*wyDSP;

	 		REPORT(DEBUG, "in worthUsingOneDSP:   usable blockArea=" << usefulDSPArea << "   dspArea=" << totalDSPArea);

			//checking according to ratio/area
	 		if(usefulDSPArea >= (1.0-target->unusedHardMultThreshold())*totalDSPArea)
	 		{
	 			REPORT(DEBUG, "in worthUsingOneDSP: "
	 				<< usefulDSPArea << ">= (1.0-" << target->unusedHardMultThreshold() << ")*" << totalDSPArea << " -> worth using a DSP block");
	 			return true;
	 		}
	 		else
	 		{
	 			REPORT(DEBUG, "in worthUsingOneDSP: "
	 				<< usefulDSPArea << "< (1.0-" << target->unusedHardMultThreshold() << ")*" << totalDSPArea << " -> not worth using a DSP block");
	 			return false;
	 		}
	 	}

		// equations of the lines which bound the area to be paved
		// line equation: Ax + By + C = 0
		// A=y1-y2, B=x2-x1, C=x1y2-y1x2
		aTopEdge = 0;		//top edge (points with bigger y coordinates)
		bTopEdge = 1;
		cTopEdge = -botY;

		aBottomEdge = 0;		//bottom edge
		bBottomEdge = 1;
		cBottomEdge = -(topY>botY-wyDSP ? topY : botY-wyDSP);

		aLeftEdge = 1;		//left edge (points with smaller x coordinates)
		bLeftEdge = 0;
		cLeftEdge = -(topX>botX-wxDSP ? topX : botX-wxDSP);

		aRightEdge = 1;		//right edge
		bRightEdge = 0;
		cRightEdge = -botX;

		//FIXME: the coordinates of the truncation line need redoing to cover all cases
		//			and to fit in the mentality of not using g as a global parameter
		/*
		 * before update
		//equation of truncation line - given by the 2 points (wX-g, 0) and (0, wY-g)
		aTruncLine = g-wY;
		bTruncLine = g-wX;
		cTruncLine = (wX-g)*(wY-g);
		*/

		//equation of the truncation line
		int truncationLine;

		if(lsbWeightInBitHeap < 0)
			truncationLine = -lsbWeightInBitHeap;
		else
			truncationLine = lsbWeightInBitHeap - lsbFullMultWeightInBitheap;

		//equation of truncation line - given by the 2 points (0, abs(truncationLine)) and (abs(truncationLine), 0)
		aTruncLine = truncationLine;
		bTruncLine = truncationLine;
		cTruncLine = -(truncationLine*truncationLine);

		//first, assume that the truncated part is a triangle
		//	if so, then the two intersections are with the left and bottom edge

		//the left edge intersected with the truncation line
		intersectLeftX = (1.0 * (bLeftEdge*cTruncLine - bTruncLine*cLeftEdge)) / (aLeftEdge*bTruncLine - aTruncLine*bLeftEdge);
		intersectLeftY = (1.0 * (aTruncLine*cLeftEdge - aLeftEdge*cTruncLine)) / (aLeftEdge*bTruncLine - aTruncLine*bLeftEdge);

		//the bottom edge intersected with the truncation line
		intersectBottomX = (1.0 * (bBottomEdge*cTruncLine - bTruncLine*cBottomEdge)) / (aBottomEdge*bTruncLine - aTruncLine*bBottomEdge);
		intersectBottomY = (1.0 * (aTruncLine*cBottomEdge - aBottomEdge*cTruncLine)) / (aBottomEdge*bTruncLine - aTruncLine*bBottomEdge);

		//check to see if the intersection points are inside the target square
		//	intersectLeft should be above the top edge
		//	intersectBottom should be to the right of right edge

		//check intersectLeft - intersect1 is the intersection between the truncation line and
		//						either the left edge, or the top edge
		if(intersectLeftX*aTopEdge + intersectLeftY*bTopEdge + cTopEdge > 0)
		{
			//then the intersection is the one between the truncation line and
			//	the top edge
			intersectTopX = 1.0 * (bTopEdge*cTruncLine - bTruncLine*cTopEdge) / (aTopEdge*bTruncLine - aTruncLine*bTopEdge);
			intersectTopY = 1.0 * (aTruncLine*cTopEdge - aTopEdge*cTruncLine) / (aTopEdge*bTruncLine - aTruncLine*bTopEdge);

			intersectX1 = intersectTopX;
			intersectY1 = intersectTopY;
		}else
		{
			intersectX1 = intersectLeftX;
			intersectY1 = intersectLeftY;
		}

		//check intersectBottom - intersect1 is the intersection between the truncation line and
		//						  either the bottom or the right edge
		if(intersectBottomX*aRightEdge + intersectBottomY*bRightEdge + cRightEdge > 0)
		{
			//then the intersection is the one between the truncation line and
			//	the right edge
			intersectRightX = 1.0 * (bRightEdge*cTruncLine - bTruncLine*cRightEdge) / (aRightEdge*bTruncLine - aTruncLine*bRightEdge);
			intersectRightY = 1.0 * (aTruncLine*cRightEdge - aRightEdge*cTruncLine) / (aRightEdge*bTruncLine - aTruncLine*bRightEdge);

			intersectX2 = intersectRightX;
			intersectY2 = intersectRightY;
		}else
		{
			intersectX2 = intersectBottomX;
			intersectY2 = intersectBottomY;
		}

		/*
		//renormalize the intersection points' coordinates
		intersectX1 = (intersectX1 < topX) ? topX : intersectX1;
		intersectY2 = (intersectY2 < topY) ? topY : intersectY2;
		*/

		//compute the useful DSP area

		//determine the shape of the useful area
		if((intersectY1 < botY) && (intersectX2 < botX))
		{
			//this is a pentagon
			usefulDSPArea = ((1.0*botX-intersectX1 + 1.0*botX-intersectX2)*(intersectY1-intersectY2)/2.0) + ((1.0*botY-intersectY1)*(1.0*botX-intersectX1));
		}else if((intersectY1 == botY) && (intersectX2 < botX))
		{
			//this is a trapeze
			usefulDSPArea = (1.0*botX-intersectX1 + 1.0*botX-intersectX2)*(intersectY1-intersectY2)/2.0;
		}else if((intersectY1 == botY) && (intersectX2 == botX))
		{
			//this is a triangle
			usefulDSPArea = (1.0*botX-intersectX1)*(1.0*botY-intersectY2)/2.0;
		}

		totalDSPArea = wxDSP*wyDSP;

		REPORT(DEBUG, "in worthUsingOneDSP:   truncated multiplication case");
		REPORT(DEBUG, "in worthUsingOneDSP:   usable blockArea= " << usefulDSPArea);
		REPORT(DEBUG, "in worthUsingOneDSP:   dspArea= " << totalDSPArea);
		REPORT(DEBUG, "in worthUsingOneDSP:   intersectX1=" << intersectX1 << " intersectY1=" << intersectY1 << " intersectX2=" << intersectX2 << " intersectY2=" << intersectY2);

		//test if the DSP DSPThreshold is satisfied
		if(usefulDSPArea >= (1.0-target->unusedHardMultThreshold())*totalDSPArea)
		{
			REPORT(DEBUG, "in worthUsingOneDSP:   result of the test: " << usefulDSPArea << ">=(1.0-" << target->unusedHardMultThreshold() << ")*" << totalDSPArea << " -> WORTH using a DSP block");
			return true;
		}else
		{
			REPORT(DEBUG, "in worthUsingOneDSP:   result of the test: " << usefulDSPArea << "<(1.0-" << target->unusedHardMultThreshold() << ")*" << totalDSPArea << " -> NOT worth using a DSP block");
			return false;
		}
#endif
	}


	void IntMultiplier::buildAlteraTiling(int blockTopX, int blockTopY, int blockBottomX, int blockBottomY, int dspIndex, bool signedX, bool signedY)
	{
#if 1
		int dspSizeX,dspSizeY;
		int widthX, widthY;
		int lsbX, lsbY, msbX, msbY;
		int multWidthsSize = multWidths.size();
		int newMultIndex = dspIndex;
		bool dspSizeNotFound = true;
		bool originalSignedX = signedX;
		bool originalSignedY = signedY;

		//set the size of the DSP widths, preliminary
		dspSizeX = multWidths[multWidthsSize-newMultIndex-1];
		if(signedX == false)
			dspSizeX--;
		dspSizeY = multWidths[multWidthsSize-newMultIndex-1];
		if(signedY == false)
			dspSizeY--;

		//normalize block coordinates
		if(blockTopX < 0)
			blockTopX = 0;
		if(blockTopY < 0)
			blockTopY = 0;
		if(blockBottomX < 0)
			blockBottomX = 0;
		if(blockBottomY < 0)
			blockBottomY = 0;

		REPORT(DEBUG, "in buildAlteraTiling(): call to buildAlteraTiling() at DSP size dspSizeX=" << dspSizeX << " and dspSizeY=" << dspSizeY
			<< " with parameters: blockTopX=" << blockTopX << " blockTopY=" << blockTopY << " blockBottomX=" << blockBottomX << " blockBottomY=" << blockBottomY
			<< (signedX ? " signed" : " unsigned") << " by " << (signedY ? "signed" : "unsigned"));

		//if the whole DSP is outside the range of interest, skip over it altogether
		//before update if((blockTopX+blockTopY<wFullP-wOut-g) && (blockBottomX+blockBottomY<wFullP-wOut-g))
		if((blockTopX+blockTopY+lsbWeightInBitHeap < 0) && (blockBottomX+blockBottomY+lsbWeightInBitHeap < 0))
		{
			REPORT(DEBUG, "in buildAlteraTiling(): " << tab << "addition of DSP skipped: out of range of interest at coordinates blockTopX="
				<< blockTopX << " blockTopY=" << blockTopY << " blockBottomX=" << blockBottomX
				<< " blockBottomY=" << blockBottomY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
			return;
		}

		//if the block is of width/height zero, then end this function call, as there is nothing to do
		widthX = (blockBottomX-blockTopX+1)/dspSizeX;
		widthY = (blockBottomY-blockTopY+1)/dspSizeY;
		if(((widthX==0) && (blockTopX==blockBottomX)) || ((widthY==0) && (blockTopY==blockBottomY)))
			return;

		REPORT(DEBUG, "in buildAlteraTiling(): Testing for the best DSP size to use");
		//search for the largest DSP size that corresponds to the ratio
		while(dspSizeNotFound)
		{
			widthX = (blockBottomX-blockTopX+1)/dspSizeX;
			widthY = (blockBottomY-blockTopY+1)/dspSizeY;

			REPORT(DEBUG, "in buildAlteraTiling(): " << tab << "testing for DSP size at dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY
				<< " and widthX=" << widthX << " widthY=" << widthY);

			if((widthX==0) && (widthY==0))
			{
				//if both DSP dimensions are large enough, the DSP block might still fit the DSPThreshold
				if(worthUsingOneDSP(blockTopX, blockTopY, blockBottomX, blockBottomY, dspSizeX, dspSizeY))
				{
					//target->unusedHardMultThreshold() fulfilled; search is over
					dspSizeNotFound = false;
				}else
				{
					//target->unusedHardMultThreshold() not fulfilled; pass on to the next smaller DSP size
					if(newMultIndex == multWidthsSize-1)
					{
						dspSizeNotFound = false;
					}else
					{
						newMultIndex++;
					}
					dspSizeX = multWidths[multWidthsSize-newMultIndex-1];
					if(signedX == false)
						dspSizeX--;
					dspSizeY = multWidths[multWidthsSize-newMultIndex-1];
					if(signedY == false)
						dspSizeY--;
				}
			}
			else
			{
				//there is only one dimension on which the DSP can fit
				if((widthX<=1) || (widthY<=1))
				{
					int tx = ((widthX==0 || widthY==0) ? blockBottomX-dspSizeX : blockTopX), ty = ((widthX==0 || widthY==0) ? blockBottomY-dspSizeY : blockTopY);
					int bx = blockBottomX, by = blockBottomY;

					if(worthUsingOneDSP((tx<blockTopX ? blockTopX : tx), (ty<blockTopY ? blockTopY : ty), bx, by, dspSizeX, dspSizeY))
					{
						//target->unusedHardMultThreshold() fulfilled; search is over
						dspSizeNotFound = false;
					}else
					{
						//target->unusedHardMultThreshold() not fulfilled; pass on to the next smaller DSP size
						if(newMultIndex == multWidthsSize-1)
						{
							dspSizeNotFound = false;
						}else
						{
							newMultIndex++;
						}
						dspSizeX = multWidths[multWidthsSize-newMultIndex-1];
						if(signedX == false)
							dspSizeX--;
						dspSizeY = multWidths[multWidthsSize-newMultIndex-1];
						if(signedY == false)
							dspSizeY--;
					}
				}else
				{
					dspSizeNotFound = false;
				}
			}
		}

		REPORT(DEBUG, "in buildAlteraTiling(): " << tab << "DSP block size set to dspSizeX=" << dspSizeX << " and dspSizeY=" << dspSizeY);

		if(signedX && signedY)
		{
			REPORT(DEBUG, "in buildAlteraTiling(): Initial call to buildAlteraTiling(), with both parameters signed");

			//SxS multiplication
			//	just for the top-index (for both x and y)
			//the starting point
			//	what remains to be done: one SxS multiplication in the top corner
			//							 recursive call on the top line (UxS)
			//							 recursive call on the right column (SxU)
			//							 recursive call for the rest of the multiplication (UxU)


			/*
			 * First version: sign is handled in the DSPs
			 */

			//top corner, SxS multiplication
			 REPORT(DEBUG, "in buildAlteraTiling(): adding DSP (signed by signed) at coordinates lsbX=" << blockBottomX-dspSizeX << " lsbY=" << blockBottomY-dspSizeY
			 	<< " msbX=" << blockBottomX << " msbY=" << blockBottomY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
			 addExtraDSPs(blockBottomX-dspSizeX, blockBottomY-dspSizeY, blockBottomX, blockBottomY, dspSizeX, dspSizeY);

			//top line, UxS multiplications where needed, UxU for the rest
			 buildAlteraTiling(blockTopX, blockTopY, blockBottomX-dspSizeX, blockBottomY, newMultIndex, false, true);

			//right column, SxU multiplication where needed, UxU for the rest
			//FIXME: conditions for sign are not true -> needs to be signed on the bottom part
			 buildAlteraTiling(blockBottomX-dspSizeX, blockTopY, blockBottomX, blockBottomY-dspSizeY, newMultIndex, true, false);



			/*
			 * Second version: sign is handled separately
			 */
			/*
			//handle top line
			//		for now just call the buildHeapLogicOnly function which should handle the rest of the work
			buildHeapLogicOnly(blockTopX, blockBottomY-1, blockBottomX, blockBottomY, parentOp->getNewUId());

			//handle right column
			//		for now just call the buildHeapLogicOnly function which should handle the rest of the work
			buildHeapLogicOnly(blockBottomX-1, blockTopY, blockBottomX, blockBottomY-1, parentOp->getNewUId());

			//handle the rest of the multiplication
			//	the remaining part is UxU
			buildAlteraTiling(blockTopX, blockTopY, blockBottomX-1, blockBottomY-1, newMultIndex, false, false);
			*/
		}else
		{
			//SxU, UxS, UxU multiplications - they all share the same structure,
			//	the only thing that changes being the size of the operands

			//start tiling
			lsbX = (blockBottomX-blockTopX < dspSizeX) ? blockTopX : blockBottomX-dspSizeX;
			lsbY = (blockBottomY-blockTopY < dspSizeY) ? blockTopY : blockBottomY-dspSizeY;
			msbX = blockBottomX;
			msbY = blockBottomY;

			REPORT(DEBUG, "in buildAlteraTiling(): Original multiplier block separated in widthX=" << widthX << " by widthY=" << widthY << " blocks");

			//handle the bits that can be processed at the current DSP size
			for(int i=0; i<(widthY>0 ? widthY : 1); i++)
			{
				for(int j=0; j<(widthX>0 ? widthX : 1); j++)
				{
					// read the sign-ness and the DSP sizes
					//	only the blocks on the border need to have signed components
					if((j!=0) && (signedX))
					{
						signedX = false;
						dspSizeX--;

						lsbX++;
					}
					if((widthX>1) && (j==0) && (originalSignedX) && (i!=0))
					{
						signedX = originalSignedX;
						dspSizeX++;

						lsbX--;
					}

					if((i!=0) && (signedY))
					{
						signedY = false;
						dspSizeY--;

						lsbY++;
					}

					//if the whole DSP is outside the range of interest
					//before update if((lsbX+lsbY<wFullP-wOut-g) && (msbX+msbY<wFullP-wOut-g))
					if((lsbX+lsbY+lsbWeightInBitHeap < 0) && (msbX+msbY+lsbWeightInBitHeap < 0))
					{
						if((widthX != 0) && (j != (widthX-1)))
						{
							msbX = lsbX;
							lsbX = lsbX-dspSizeX;
						}
						REPORT(DEBUG, "in buildAlteraTiling(): adding DSP skipped (out of range of interest) at coordinates lsbX="
							<< lsbX << " lsbY=" << lsbY << " msbX=" << msbX << " msbY=" << msbY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
						continue;
					}

					if(((widthX<=1) && (widthY<=1)) && ((blockBottomX-blockTopX <= dspSizeX) && (blockBottomY-blockTopY <= dspSizeY)))
					{
						//special case; the remaining block is less than or equal to a DSP

						/* FIXME: in this version, once the block is smaller than the current size DSP, it goes to
						 * 			addExtraDSPs, which either implements it as a DSP (when it meets the dspThreshold), or as logic
						 * 		  What should actually be done: try to see if there is a smaller DSP size, and try to tile the remaining
						 * 		    space using the smaller size DSPs
						lsbX = blockTopX;
						lsbY = blockTopY;
						msbX = blockBottomX;
						msbY = blockBottomY;

						REPORT(DEBUG, "" << tab << tab << "adding DSP (to cover the whole block) at coordinates lsbX="
						<< blockTopX << " lsbY=" << blockTopY << " msbX=" << blockBottomX << " msbY=" << blockBottomY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
						addExtraDSPs(blockTopX, blockTopY, blockBottomX, blockBottomY, dspSizeX, dspSizeY);
						*/

						//try to implement it at this DSP size, if possible
						if(worthUsingOneDSP(lsbX, lsbY, msbX, msbY, dspSizeX, dspSizeY))
						{
							//this multiplier block can be implemented at this DSP size, satisfying the DSPRatio
							REPORT(DEBUG, "in buildAlteraTiling(): adding DSP (to cover the whole block) at coordinates lsbX="
								<< lsbX << " lsbY=" << lsbY << " msbX=" << msbX << " msbY=" << msbY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
							addExtraDSPs(lsbX, lsbY, msbX, msbY, dspSizeX, dspSizeY, true);
						}else
						{
							//check to see if there are other DSP size available
							if(newMultIndex >= multWidthsSize-1)
							{
								//this is the smallest DSP size, so just implement the block
								REPORT(DEBUG, "in buildAlteraTiling(): adding DSP (to cover the whole block) at coordinates lsbX="
									<< lsbX << " lsbY=" << lsbY << " msbX=" << msbX << " msbY=" << msbY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
								addExtraDSPs(lsbX, lsbY, msbX, msbY, dspSizeX, dspSizeY);
							}else
							{
								//the DSP size can still be decreased, so continue tiling with smaller DSPs
								REPORT(DEBUG, "in buildAlteraTiling(): multiplier block does not fit into a DSP of this size; the size can still be decreased");
								buildAlteraTiling(lsbX, lsbY, msbX, msbY, newMultIndex+1, signedX, signedY);
							}
						}
					}else
					{
						//the regular case; just add a new DSP
						if(worthUsingOneDSP((lsbX<0 ? blockTopX : lsbX), (lsbY<0 ? blockTopY : lsbY), (msbX<0 ? blockTopX : msbX), (msbY<0 ? blockTopY : msbY), dspSizeX, dspSizeY))
						{
							REPORT(DEBUG, "in buildAlteraTiling(): DSPThreshold satisfied - adding DSP at coordinates lsbX="
								<< lsbX << " lsbY=" << lsbY << " msbX=" << msbX << " msbY=" << msbY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
							addExtraDSPs(lsbX, lsbY, msbX, msbY, dspSizeX, dspSizeY, true);
						}
						else
						{
							REPORT(DEBUG, "in buildAlteraTiling(): DSPThreshold NOT satisfied - recursive call at coordinates lsbX="
								<< lsbX << " lsbY=" << lsbY << " msbX=" << msbX << " msbY=" << msbY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY
								<< (signedX ? " signed" : " unsigned") << " by " << (signedY ? "signed" : "unsigned"));
							if(newMultIndex == multWidthsSize-1)
							{
								REPORT(DEBUG, "in buildAlteraTiling(): DSP size cannot be decreased anymore; will add DSP at this size");
								// before update if((lsbX+lsbY<wFullP-wOut-g) && (msbX+msbY<wFullP-wOut-g))
								if((lsbX+lsbY+lsbWeightInBitHeap < 0) && (msbX+msbY+lsbWeightInBitHeap < 0))
								{
									if((widthX!=0) && (j!=widthX-1))
									{
										msbX = lsbX;
										lsbX = lsbX-dspSizeX;
									}
									REPORT(DEBUG, "in buildAlteraTiling(): adding DSP skipped: out of range of interest at coordinates lsbX="
										<< blockTopX << " lsbY=" << blockTopY << " msbX=" << blockBottomX << " msbY=" << blockBottomY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
									continue;
								}else
								{
									addExtraDSPs(lsbX, lsbY, msbX, msbY, dspSizeX, dspSizeY);
								}
							}else
							{
								REPORT(DEBUG, "in buildAlteraTiling(): DSP size can still be decreased");
								buildAlteraTiling(lsbX, lsbY, msbX, msbY, newMultIndex+1, signedX, signedY);
							}
						}

						if((widthX!=0) && (j!=widthX-1))
						{
							msbX = lsbX;
							lsbX = lsbX-dspSizeX;
						}
					}
				}

				//pass on to the next column
				if((widthY!=0) && (i!=widthY-1))
				{
					msbX = blockBottomX;
					msbY = lsbY;
					lsbX = (blockBottomX-dspSizeX < blockTopX) ? blockTopX : blockBottomX-dspSizeX;
					lsbY = (lsbY-dspSizeY < blockTopY) ? blockTopY : lsbY-dspSizeY;
				}
			}

			REPORT(DEBUG, "in buildAlteraTiling(): last DSP added at lsbX=" << lsbX << " lsbY=" << lsbY << " msbX=" << msbX << " msbY=" << msbY);

			//handle the bottom leftover bits, if necessary
			if((lsbY>0) && (lsbY != blockTopY))
			{
				REPORT(DEBUG, "in buildAlteraTiling(): handling the bottom leftover bits at coordinates lsbX="
					<< lsbX << " lsbY=" << blockTopY << " msbX=" << blockBottomX << " msbY=" << lsbY
					<< (signedX ? " signed" : " unsigned") << " by " << (signedY ? "signed" : "unsigned"));
				// before update if((lsbX+blockTopY<wFullP-wOut-g) && (blockBottomX+lsbY<wFullP-wOut-g))
				if((lsbX+blockTopY+lsbWeightInBitHeap < 0) && (blockBottomX+lsbY+lsbWeightInBitHeap < 0))
				{
					REPORT(DEBUG, "in buildAlteraTiling(): " << tab << " adding DSP skipped (out of range of interest) at coordinates lsbX="
						<< lsbX << " lsbY=" << blockTopY << " msbX=" << blockBottomX << " msbY=" << lsbY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
				}else
				{
					buildAlteraTiling(lsbX, blockTopY, blockBottomX, lsbY, newMultIndex, originalSignedX, false);
				}
			}

			//handle the left-side leftover bits, if necessary
			if((lsbX>0) && (lsbX != blockTopX))
			{
				REPORT(DEBUG, "in buildAlteraTiling(): handling the left-side leftover bits at coordinates lsbX="
					<< blockTopX << " lsbY=" << blockTopY << " msbX=" << lsbX << " msbY=" << blockBottomY
					<< (signedX ? " signed" : " unsigned") << " by " << (signedY ? "signed" : "unsigned"));
				// before update if((blockTopX+blockTopY<wFullP-wOut-g) && (lsbX+blockBottomY<wFullP-wOut-g))
				if((blockTopX+blockTopY+lsbWeightInBitHeap < 0) && (lsbX+blockBottomY+lsbWeightInBitHeap < 0))
				{
					REPORT(DEBUG, "in buildAlteraTiling(): " << tab << "adding DSP skipped (out of range of interest) at coordinates lsbX="
						<< blockTopX << " lsbY=" << blockTopY << " msbX=" << lsbX << " msbY=" << blockBottomY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
				}else
				{
					buildAlteraTiling(blockTopX, blockTopY, lsbX, blockBottomY, newMultIndex, false, originalSignedY);
				}
			}

			REPORT(DEBUG, "in buildAlteraTiling(): End of call to buildAlteraTiling, at dspSizeX=" << dspSizeX << " and dspSizeY=" << dspSizeY
				<< " with parameters  - blockTopX=" << blockTopX << " blockTopY=" << blockTopY << " blockBottomX=" << blockBottomX << " blockBottomY=" << blockBottomY
				<< (originalSignedX ? " signed" : " unsigned") << " by " << (originalSignedY ? "signed" : "unsigned"));
		}
#endif
	}






	// FIXME: better tiling is needed: better recursive decomposition
	void IntMultiplier::buildXilinxTiling()
	{
#if 1
		int widthXX, widthX;											//local wxDSP
		int widthYY, widthY;											//local wyDSP
		int hor1, hor2, ver1, ver2;
		int horizontalDSP, verticalDSP;
		int nrDSPvertical 	= checkTiling(wyDSP, wxDSP, hor1, ver1);	//number of DSPs used in the case of vertical tiling
		int nrDSPhorizontal = checkTiling(wxDSP, wyDSP, hor2, ver2);	//number of DSPs used in case of horizontal tiling
		int botx = wX;
		int boty = wY;
		int topx, topy;

		//decides if a horizontal tiling will be used or a vertical one
		if(nrDSPvertical < nrDSPhorizontal)
		{
			widthXX 		= wyDSP;
			widthYY 		= wxDSP;
			horizontalDSP 	= hor1;
			verticalDSP 	= ver1;
			REPORT(DEBUG, "in buildXilinxTiling(): tiling using DSP blocks placed vertically (wxDSP=" << wyDSP << ", wyDSP=" << wxDSP << ")");
			REPORT(DEBUG, "in buildXilinxTiling():   using " << nrDSPvertical << " DSPs, "
				<< horizontalDSP << " (horizontally) by " << verticalDSP << " (vertically)");
		}else
		{
			widthXX			= wxDSP;
			widthYY			= wyDSP;
			horizontalDSP	= hor2;
			verticalDSP		= ver2;
			REPORT(DEBUG, "in buildXilinxTiling(): tiling using DSP blocks placed horizontally (wxDSP=" << wxDSP << ", wyDSP=" << wyDSP << ")");
			REPORT(DEBUG, "in buildXilinxTiling():   using " << nrDSPvertical << " DSPs, "
				<< horizontalDSP << " (horizontally) by " << verticalDSP << " (vertically)");
		}


		//applying the tiles
		for(int i=0; i<verticalDSP; i++)
		{
			//managing the size of a DSP according to its position if signed
			widthY = widthYY;
			if(signedIO && (i != 0))
				widthY--;

			topy = boty-widthY;
			botx = wX;

			for(int j=0; j<horizontalDSP; j++)
			{
				//managing the size of a DSP according to its position if signed
				widthX = widthXX;
				if((signedIO)&&(j!=0))
					widthX--;

				topx = botx-widthX;

				//add the block only if at least a part of it is above the truncation line (bit range of interest in the bit heap)
				// before update if(botx+boty>wFullP-wOut-g)
				if(botx+boty+lsbWeightInBitHeap >= 0)
				{
					addExtraDSPs(topx, topy, botx, boty, widthX, widthY);
				}
				botx=botx-widthX;
			}

			boty=boty-widthY;
		}
		REPORT(FULL, "in buildXilinxTiling(): Exiting buildXilinxTiling()");
#endif
	}


	// Destructor
	IntMultiplier::~IntMultiplier() {
	}


	//signal name construction

	string IntMultiplier::addUID(string name, int blockUID)
	{
		ostringstream s;

		s << name << "_m" << multiplierUid;
		if(blockUID != -1)
			s << "b" << blockUID;
		return s.str() ;
	};

	string IntMultiplier::PP(int i, int j, int uid )
	{
		std::ostringstream p;

		if(uid == -1)
			p << "PP" <<  "_X" << i << "Y" << j;
		else
			p << "PP" << uid << "X" << i << "Y" << j;
		return  addUID(p.str());
	};

	string IntMultiplier::PPTbl(int i, int j, int uid )
	{
		std::ostringstream p;

		if(uid==-1)
			p << addUID("PP") <<  "_X" << i << "Y" << j << "_Tbl";
		else
			p << addUID("PP") << "_" << uid << "X" << i << "Y" << j << "_Tbl";
		return p.str();
	};


	string IntMultiplier::XY( int i, int j,int uid)
	{
		std::ostringstream p;

		if(uid==-1)
			p  << "Y" << j<< "X" << i;
		else
			p  << "Y" << j<< "X" << i<<"_"<<uid;
		return  addUID(p.str());
	};






	IntMultiplier::SmallMultTable::SmallMultTable(Target* target, int dx, int dy, int wO, bool negate, bool  signedX, bool  signedY ) :
		Table(target, dx+dy, wO, 0, -1, true), // logic table
		dx(dx), dy(dy), wO(wO), negate(negate), signedX(signedX), signedY(signedY) {
			ostringstream name;
			srcFileName="LogicIntMultiplier::SmallMultTable";
		// No getUid() in the name: this kind of table should be added to the globalOpList
			name <<"SmallMultTable"<< (negate?"M":"P") << dy << "x" << dx << "r" << wO << (signedX?"Xs":"Xu") << (signedY?"Ys":"Yu");
			setNameWithFreqAndUID(name.str());
		};


		mpz_class IntMultiplier::SmallMultTable::function(int yx){
			mpz_class r;
			int y = yx>>dx;
			int x = yx -(y<<dx);
			int wF=dx+dy;

			if(signedX){
				if ( x >= (1 << (dx-1)))
					x -= (1 << dx);
			}
			if(signedY){
				if ( y >= (1 << (dy-1)))
					y -= (1 << dy);
			}
		//if(!negate && signedX && signedY) cerr << "  y=" << y << "  x=" << x;
			r = x * y;
		//if(!negate && signedX && signedY) cerr << "  r=" << r;
			if(negate)
				r=-r;
		//if(negate && signedX && signedY) cerr << "  -r=" << r;
			if ( r < 0)
				r += mpz_class(1) << wF;
		//if(!negate && signedX && signedY) cerr << "  r2C=" << r;

		if(wOut<wF){ // wOut is that of Table
			// round to nearest, but not to nearest even
			int tr=wF-wOut; // number of truncated bits
			// adding the round bit at half-ulp position
			r += (mpz_class(1) << (tr-1));
			r = r >> tr;
		}

		//if(!negate && signedX && signedY) cerr << "  rfinal=" << r << endl;

		return r;

	};


	// Interestingly, this function is called only for a standalone constructor.
	void IntMultiplier::emulate (TestCase* tc)
	{
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");
		mpz_class svR;

		if(!signedIO)
		{
			svR = svX * svY;
		}
		else
		{
			// Manage signed digits
			mpz_class big1 = (mpz_class(1) << (wXdecl));
			mpz_class big1P = (mpz_class(1) << (wXdecl-1));
			mpz_class big2 = (mpz_class(1) << (wYdecl));
			mpz_class big2P = (mpz_class(1) << (wYdecl-1));

			if(svX >= big1P)
				svX -= big1;

			if(svY >= big2P)
				svY -= big2;

			svR = svX * svY;
		}

		if(negate)
			svR = -svR;

		// manage two's complement at output
		if(svR < 0)
			svR += (mpz_class(1) << wFullP);

		if(wOut>=wFullP)
			tc->addExpectedOutput("R", svR);
		else	{
			// there is truncation, so this mult should be faithful
			svR = svR >> (wFullP-wOut);
			tc->addExpectedOutput("R", svR);
			svR++;
			svR &= (mpz_class(1) << (wOut)) -1;
			tc->addExpectedOutput("R", svR);
		}
	}



	void IntMultiplier::buildStandardTestCases(TestCaseList* tcl)
	{
		TestCase *tc;

		mpz_class x, y;

		// 1*1
		x = mpz_class(1);
		y = mpz_class(1);
		tc = new TestCase(this);
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);

		// -1 * -1
		x = (mpz_class(1) << wXdecl) -1;
		y = (mpz_class(1) << wYdecl) -1;
		tc = new TestCase(this);
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);

		// The product of the two max negative values overflows the signed multiplier
		x = mpz_class(1) << (wXdecl -1);
		y = mpz_class(1) << (wYdecl -1);
		tc = new TestCase(this);
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);
	}

	OperatorPtr IntMultiplier::parseArguments(Target *target, std::vector<std::string> &args) {
		int wX,wY, wOut ;
		bool signedIO,superTile;
		UserInterface::parseStrictlyPositiveInt(args, "wX", &wX);
		UserInterface::parseStrictlyPositiveInt(args, "wY", &wY);
		UserInterface::parsePositiveInt(args, "wOut", &wOut);
		UserInterface::parseBoolean(args, "signedIO", &signedIO);
		UserInterface::parseBoolean(args, "superTile", &superTile);
		return new IntMultiplier(target, wX, wY, wOut, signedIO, emptyDelayMap, superTile);
	}



	void IntMultiplier::registerFactory(){
		UserInterface::add("IntMultiplier", // name
			"A pipelined integer multiplier.",
											 "BasicInteger", // category
											 "", // see also
											 "wX(int): size of input X; wY(int): size of input Y;\
											 wOut(int)=0: size of the output if you want a truncated multiplier. 0 for full multiplier;\
											 signedIO(bool)=false: inputs and outputs can be signed or unsigned;\
                        superTile(bool)=false: if true, attempts to use the DSP adders to chain sub-multipliers. This may entail lower logic consumption, but higher latency.", // This string will be parsed
											 "", // no particular extra doc needed
											 IntMultiplier::parseArguments,
											 IntMultiplier::unitTest
											 ) ;
	}

	TestList IntMultiplier::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;

		if(index==-1)
		{ // The unit tests

			for(int target = 0; target < 2; target ++)
			{
				for(int pipeline = 0; pipeline < 2; pipeline++)
				{
					for(float threshold = 0; threshold <= 1.4; threshold= threshold + 0.7)
					{
						for(int wXwYwOut = 9; wXwYwOut < 58; wXwYwOut= wXwYwOut + 2)
						{
							if(target == 0)
							{
								paramList.push_back(make_pair("target","Stratix2"));
							}
							else
							{
								paramList.push_back(make_pair("target","Vixtex6"));
							}

							if(pipeline == 0)
							{
								paramList.push_back(make_pair("pipeline","false"));
							}
							else
							{
								paramList.push_back(make_pair("pipeline","true"));
							}

							if(threshold >= 1)
							{
								paramList.push_back(make_pair("hardMultThreshold","1"));
							}
							else
							{
								paramList.push_back(make_pair("hardMultThreshold",to_string(threshold)));
							}

							paramList.push_back(make_pair("wX",to_string(wXwYwOut)));
							paramList.push_back(make_pair("wY",to_string(wXwYwOut)));
							paramList.push_back(make_pair("wOut",to_string(wXwYwOut)));
							testStateList.push_back(paramList);
							paramList.clear();
						}

						for(int wX = 4; wX < 57; wX= wX + 2)
						{
							for(int wY = 4; wY < 17; wY++)
							{
								if(target == 0)
								{
									paramList.push_back(make_pair("target","Stratix2"));
								}
								else
								{
									paramList.push_back(make_pair("target","Virtex6"));
								}

								if(pipeline == 0)
								{
									paramList.push_back(make_pair("pipeline","false"));
								}
								else
								{
									paramList.push_back(make_pair("pipeline","true"));
								}

								if(threshold >= 1)
								{
									paramList.push_back(make_pair("hardMultThreshold","1"));
								}
								else
								{
									paramList.push_back(make_pair("hardMultThreshold",to_string(threshold)));
								}

								paramList.push_back(make_pair("wX",to_string(wX)));
								paramList.push_back(make_pair("wY",to_string(wY)));
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


}
