/*
   A multiply-and-add in a single bit heap

	Author:  Florent de Dinechin, Matei Istoan

	This file is part of the FloPoCo project

	Initial software.
	Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
	2012-2014.
	All rights reserved.
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
#include "FixMultAdd.hpp"
#include "IntMultiplier.hpp"

using namespace std;

namespace flopoco {


	// The constructor for a stand-alone operator
	FixMultAdd::FixMultAdd(Target* target, Signal* x_, Signal* y_, Signal* a_, int outMSB_, int outLSB_,
												 bool enableSuperTiles_, map<string, double> inputDelays_):
		Operator ( target, inputDelays_ ),
		x(x_), y(y_), a(a_),
		outMSB(outMSB_),
		outLSB(outLSB_),
		wOut(outMSB_- outLSB_ + 1),
		enableSuperTiles(enableSuperTiles_)
	{

		srcFileName="FixMultAdd";
		setCopyrightString ( "Florent de Dinechin, Matei Istoan, 2012-2014" );

		// Set up the VHDL library style
		useNumericStd();

		wX = x->MSB() - x->LSB() +1;
		wY = y->MSB() - y->LSB() +1;
		wA = a->MSB() - a->LSB() +1;

		signedIO = (x->isFixSigned() && y->isFixSigned());
		// TODO: manage the case when one is signed and not the other.
		if((x->isFixSigned() && !y->isFixSigned()) || (!x->isFixSigned() && y->isFixSigned()))
		{
			THROWERROR("Different signs: x is "
								 << (x->isFixSigned()?"":"un")<<"signed and y is "
								 << (y->isFixSigned()?"":"un")<<"signed. This case is currently not supported." );
		}

		// Set the operator name
		{
			ostringstream name;
			name <<"FixMultAdd_";
			name << wX << "x" << wY << "p" << wA << "r" << wOut << "" << (signedIO?"signed":"unsigned");
			setNameWithFreqAndUID(name.str());
			REPORT(DEBUG, "Building " << name.str());
		}

		// Set up the IO signals
		xname="X";
		yname="Y";
		aname="A";
		rname="R";

		// Determine the msb and lsb of the full product X*Y
		pLSB = x->LSB() + y->LSB();
		pMSB = pLSB + (wX + wY + (signedIO ? -1 : 0)) - 1;

		//use 1 guard bit as default when we need to truncate A (and when either pLSB>aLSB or aMSB>pMSB)
		g = ((a->LSB()<outLSB && pLSB>a->LSB() && pLSB>outLSB)
				|| (a->LSB()<outLSB && a->MSB()>pMSB && pMSB<outLSB)) ? 1 : 0;

		// Determine the actual msb and lsb of the product,
		// from the output's msb and lsb, and the (possible) number of guard bits
		//lsb
		if(((pLSB <= outLSB-g) && (g==1)) || ((pLSB < outLSB) && (g==0)))
		{
			//the result of the multiplication will be truncated
			workPLSB = outLSB-g;

			possibleOutputs = 2;
			REPORT(DETAILED, "Faithfully rounded architecture");

			ostringstream dbgMsg;
			dbgMsg << "Multiplication of " << wX << " by " << wY << " bits, with the result truncated to weight " << workPLSB;
			REPORT(DETAILED, dbgMsg.str());
		}
		else
		{
			//the result of the multiplication will not be truncated
			workPLSB = pLSB;

			possibleOutputs = 1; // No faithful rounding
			REPORT(DETAILED, "Exact architecture");

			ostringstream dbgMsg;
			dbgMsg << "Full multiplication of " << wX << " by " << wY;
			REPORT(DETAILED, dbgMsg.str());
		}
		//msb
		if(pMSB <= outMSB)
		{
			//all msbs of the product are used
			workPMSB = pMSB;
		}
		else
		{
			//not all msbs of the product are used
			workPMSB = outMSB;
		}

		//compute the needed guard bits and update the lsb
		if(pMSB >= outLSB-g)
		{
			//workPLSB += ((g==1 && !(workPLSB==workPMSB && pMSB==(outLSB-1))) ? 1 : 0);			//because of the default g=1 value
			g = IntMultiplier::neededGuardBits(target, wX, wY, workPMSB-workPLSB+1);
			workPLSB -= g;
		}

		// Determine the actual msb and lsb of the addend,
		// from the output's msb and lsb
		//lsb
		if(a->LSB() < outLSB)
		{
			//truncate the addend
			workALSB = (outLSB-g < a->LSB()) ? a->LSB() : outLSB-g;
			//need to correct the LSB of A (the corner case when, because of truncating A, some extra bits of X*Y are required, which are needed only for the rounding)
			if((a->LSB()<outLSB) && (a->MSB()>pMSB) && (pMSB<outLSB))
			{
				workALSB += (pMSB==(outLSB-1) ? -1 : 0);
			}
		}
		else
		{
			//keep the full addend
			workALSB = a->LSB();
		}
		//msb
		if(a->MSB() <= outMSB)
		{
			//all msbs of the addend are used
			workAMSB = a->MSB();
		}
		else
		{
			//not all msbs of the product are used
			workAMSB = outMSB;
		}

		//create the inputs and the outputs of the operator
		addInput (xname,  wX);
		addInput (yname,  wY);
		addInput (aname,  wA);
		addOutput(rname,  wOut, possibleOutputs);

		//create the bit heap
		{
			ostringstream dbgMsg;
			dbgMsg << "Using " << g << " guard bits" << endl;
			dbgMsg << "Creating bit heap of size " << wOut+g << ", out of which " << g << " guard bits";
			REPORT(DETAILED, dbgMsg.str());
		}
		bitHeap = new BitHeap(this,								//parent operator
							  wOut+g,							//size of the bit heap
							  enableSuperTiles);				//whether super-tiles are used
		bitHeap->setSignedIO(signedIO);

		//FIXME: are the guard bits included in the bits output by the multiplier?
		//create the multiplier
		//	this is a virtual operator, which uses the bit heap to do all its computations
		//cerr << "Before " << getCurrentCycle() << endl;
		if(pMSB >= outLSB-g)
		{
			mult = new IntMultiplier(this,							//parent operator
									 bitHeap,						//the bit heap that performs the compression
									 getSignalByName(xname),		//first input to the multiplier (a signal)
									 getSignalByName(yname),		//second input to the multiplier (a signal)
									 pLSB-(outLSB-g),				//offset of the LSB of the multiplier in the bit heap
									 false /*negate*/,				//whether to subtract the result of the multiplication from the bit heap
									 signedIO);
		}
		//cerr << "After " << getCurrentCycle() << endl;

		//add the addend to the bit heap
		int addendWeight;

		//if the addend is truncated, no shift is needed
		//	else, compute the shift from the output lsb
		//the offset for the signal in the bitheap can be either positive (signal's lsb < than bitheap's lsb), or negative
		//	the case of a negative offset is treated with a correction term, as it makes more sense in the context of a bitheap
		addendWeight = a->LSB()-(outLSB-g);
		//needed to correct the value of g=1, when the multiplier s truncated as well
		if((workALSB >= a->LSB()) && (g>1))
		{
			addendWeight += (pMSB==(outLSB-1) ? 1 : 0);
		}
		//only add A to the bitheap if there is something to add
		if((a->MSB() >= outLSB-g) && (workAMSB>=workALSB))
		{
			if(signedIO)
			{
				bitHeap->addSignedBitVector(addendWeight,			//weight of signal in the bit heap
											aname,					//name of the signal
											workAMSB-workALSB+1,	//size of the signal added
											workALSB-a->LSB(),		//index of the lsb in the bit vector from which to add the bits of the addend
											(addendWeight<0));		//if we are correcting the index in the bit vector with a negative weight
			}
			else
			{
				bitHeap->addUnsignedBitVector(addendWeight,			//weight of signal in the bit heap
											  aname,				//name of the signal
											  workAMSB-workALSB+1,	//size of the signal added
											  (a->MSB()-a->LSB())-(workAMSB-a->MSB()),
																	//index of the msb in the actual bit vector from which to add the bits of the addend
											  workALSB-a->LSB(),	//index of the lsb in the actual bit vector from which to add the bits of the addend
											  (addendWeight<0));	//if we are correcting the index in the bit vector with a negative weight
			}
		}

		//correct the number of guard bits, if needed, because of the corner cases
		if((a->LSB()<outLSB) && (a->MSB()>pMSB) && (pMSB<outLSB))
		{
			if(pMSB==(outLSB-1))
			{
				g++;
			}
		}

		//add the rounding bit
		if(g>0)
			bitHeap->addConstantOneBit(g-1);

		//compress the bit heap
		bitHeap -> generateCompressorVHDL();
		//assign the output
		vhdl << tab << rname << " <= " << bitHeap->getSumName() << range(wOut+g-1, g) << ";" << endl;
		
	}




	FixMultAdd::~FixMultAdd()
	{
		if(mult)
			free(mult);
		if(plotter)
			free(plotter);
	}


	FixMultAdd* FixMultAdd::newComponentAndInstance(
													Operator* op,
													string instanceName,
													string xSignalName,
													string ySignalName,
													string aSignalName,
													string rSignalName,
													int rMSB,
													int rLSB )
	{
		FixMultAdd* f = new FixMultAdd(
										 op->getTarget(),
										 op->getSignalByName(xSignalName),
										 op->getSignalByName(ySignalName),
										 op->getSignalByName(aSignalName),
										 rMSB, rLSB
										 );
		op->addSubComponent(f);

		op->inPortMap(f, "X", xSignalName);
		op->inPortMap(f, "Y", ySignalName);
		op->inPortMap(f, "A", aSignalName);

		op->outPortMap(f, "R", join(rSignalName,"_slv"));  // here rSignalName_slv is std_logic_vector
		op->vhdl << op->instance(f, instanceName);
		// hence a sign mismatch in next iteration.
		
		op-> syncCycleFromSignal(join(rSignalName,"_slv")); //, f->getOutputDelay("R"));
		op->vhdl << tab << op->declareFixPoint(rSignalName, f->signedIO, rMSB, rLSB)
				<< " <= " <<  (f->signedIO ? "signed(" : "unsigned(") << (join(rSignalName, "_slv")) << ");" << endl;

		return f;
	}






	//FIXME: is this right? emulate function needs to be checked
	//		 it makes no assumptions on the signals, except their relative alignments,
	//		 so it should work for all combinations of the inputs
	void FixMultAdd::emulate ( TestCase* tc )
	{
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");
		mpz_class svA = tc->getInputValue("A");

		mpz_class svP, svR, svRAux;
		mpz_class twoToWR = (mpz_class(1) << (wOut));
		mpz_class twoToWRm1 = (mpz_class(1) << (wOut-1));

		if(!signedIO)
		{
			int outShift;

			svP = svX * svY;

			//align the product and the addend
			if(a->LSB() < pLSB)
			{
				svP = svP << (pLSB-a->LSB());
				outShift = outLSB - a->LSB();
			}
			else
			{
				svA = svA << (a->LSB()-pLSB);
				outShift = outLSB - pLSB;
			}

			svR = svP + svA;

			//align the multiply-and-add with the output format
			if(outShift > 0)
			{
				svR = svR >> outShift;
				possibleOutputs = 2;
			}
			else
			{
				svR = svR << (-outShift);
				possibleOutputs = 1;
			}

			//add only the bits corresponding to the output format
			svRAux = svR & (twoToWR -1);

			tc->addExpectedOutput("R", svRAux);
			if(possibleOutputs==2)
			{
				svR++;
				svR &= (twoToWR -1);
				tc->addExpectedOutput("R", svR);
			}
		}
		else
		{
			int outShift;

			// Manage signed digits
			mpz_class twoToWX = (mpz_class(1) << (wX));
			mpz_class twoToWXm1 = (mpz_class(1) << (wX-1));
			mpz_class twoToWY = (mpz_class(1) << (wY));
			mpz_class twoToWYm1 = (mpz_class(1) << (wY-1));
			mpz_class twoToWA = (mpz_class(1) << (wA));
			mpz_class twoToWAm1 = (mpz_class(1) << (wA-1));

			if (svX >= twoToWXm1)
				svX -= twoToWX;

			if (svY >= twoToWYm1)
				svY -= twoToWY;

			if (svA >= twoToWAm1)
				svA -= twoToWA;

			svP = svX * svY; //signed

			//align the product and the addend
			if(a->LSB() < pLSB)
			{
				svP = svP << (pLSB-a->LSB());
				outShift = outLSB - a->LSB();
			}
			else
			{
				svA = svA << (a->LSB()-pLSB);
				outShift = outLSB - pLSB;
			}

			svR = svP + svA;

			//align the multiply-and-add with the output format
			if(outShift > 0)
			{
				svR = svR >> outShift;
				possibleOutputs = 2;
			}
			else
			{
				svR = svR << (-outShift);
				possibleOutputs = 1;
			}

			// manage two's complement at output
			if(svR < 0)
				svR += twoToWR;

			//add only the bits corresponding to the output format
			svRAux = svR & (twoToWR -1);

			tc->addExpectedOutput("R", svRAux);
			if(possibleOutputs == 2)
			{
				svR++;
				svR &= (twoToWR -1);
				tc->addExpectedOutput("R", svR);
			}
		}
	}



	void FixMultAdd::buildStandardTestCases(TestCaseList* tcl)
	{
		//TODO
	}




}
