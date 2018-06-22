/*
An integer adder for FloPoCo

It may be pipelined to arbitrary frequency.
Also useful to derive the carry-propagate delays for the subclasses of Target

Authors:  Bogdan Pasca, Florent de Dinechin

This file is part of the FloPoCo project
developed by the Arenaire team at Ecole Normale Superieure de Lyon

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
2008-2010.
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
#include "IntAdder.hpp"

#include "IntAdderClassical.hpp"
#include "IntAdderAlternative.hpp"
#include "IntAdderShortLatency.hpp"

  using namespace std;
  namespace flopoco {

  	IntAdder::IntAdder ( Target* target, int wIn, map<string, double> inputDelays, int optimizeType, bool srl, int implementation):
  	Operator ( target, inputDelays), wIn_ ( wIn )  {
  		ostringstream name;
  		srcFileName="IntAdder";
  		setCopyrightString ( "Bogdan Pasca, Florent de Dinechin (2008-2010)" );

  		name << "IntAdder_" << wIn_<<"_f"<<target->frequencyMHz()<<"_uid"<<getNewUId();

		// Set up the IO signals
  		addInput ( "X"  , wIn_, true );
  		addInput ( "Y"  , wIn_, true );
  		addInput( "Cin");
  		addOutput ( "R"  , wIn_, 1 , true );

  		REPORT(DETAILED, "Implementing IntAdder " << wIn << " implementation="<<implementation);

  		Operator* intAdderInstantiation;

		if (implementation == -1){ // we must explore
			intAdderInstantiation = new IntAdderClassical(target, wIn, inputDelays, optimizeType, srl);
			addImplementationList.push_back(intAdderInstantiation);

			intAdderInstantiation = new IntAdderAlternative(target, wIn, inputDelays, optimizeType, srl);
			addImplementationList.push_back(intAdderInstantiation);

//			intAdderInstantiation = new IntAdderShortLatency(target, wIn, inputDelays, optimizeType, srl);
//			addImplementationList.push_back(intAdderInstantiation);
		}else{
			switch (implementation){
				case 0:
				intAdderInstantiation = new IntAdderClassical(target, wIn, inputDelays, optimizeType, srl);
				addImplementationList.push_back(intAdderInstantiation);
				break;
				case 1:
				intAdderInstantiation = new IntAdderAlternative(target, wIn, inputDelays, optimizeType, srl);
				addImplementationList.push_back(intAdderInstantiation);
				break;
				case 2:
				intAdderInstantiation = new IntAdderShortLatency(target, wIn, inputDelays, optimizeType, srl);
				addImplementationList.push_back(intAdderInstantiation);
				break;
				default:
				intAdderInstantiation = new IntAdderClassical(target, wIn, inputDelays, optimizeType, srl);
				addImplementationList.push_back(intAdderInstantiation);
			}
		}

		int currentCost = 16384;
		selectedVersion = 0;
		for (unsigned j=0; j< addImplementationList.size(); j++)
			if (currentCost > addImplementationList[j]->getOperatorCost()){
				currentCost = addImplementationList[j]->getOperatorCost();
				selectedVersion = j;
			}

			cloneOperator(addImplementationList[selectedVersion]);
			changeName ( name.str() );

			REPORT(DETAILED, "Selected implementation for IntAdder"<< wIn << " is "<<selectedVersion<<" with cost="<<currentCost);

//		//cleanup; clear the oplist of the components that will be unused, and the components used therein
//		for (unsigned j=0; j< addImplementationList.size(); j++){
//			REPORT(DEBUG, "deleting version "<<int(j));
//			cleanup(&oplist, addImplementationList[j]);
//		}
//		REPORT(DEBUG, "Finished implementing the adder");
		}


		void IntAdder::updateParameters ( Target* target, int &alpha, int &beta, int &k )
		{
		target->suggestSlackSubaddSize ( alpha , wIn_, target->ffDelay() + target->localWireDelay() ); /* chunk size */
			if ( wIn_ == alpha )
		{ /* addition requires one chunk */
				beta = 0;
			k    = 1;
		}
		else
		{
			beta = ( wIn_ % alpha == 0 ? alpha : wIn_ % alpha );
			k    = ( wIn_ % alpha == 0 ? wIn_ / alpha : int ( ceil ( double ( wIn_ ) / double ( alpha ) ) ) );
		}
	}

	void IntAdder::updateParameters ( Target* target, map<string, double> inputDelays, int &alpha, int &beta, int &gamma, int &k )
	{
		int typeOfChunks = 1;
		bool status = target->suggestSlackSubaddSize ( gamma , wIn_, getMaxInputDelays(inputDelays) ); // the first chunk size
		if (!status){ /* well, it will not work in this case, we will have to register the inputs */
		k     = -1;
		alpha =  0;
		beta  =  0;
		gamma =  0;
	}
	else if (wIn_ - gamma > 0)
		{ //more than 1 chunk
			target->suggestSlackSubaddSize (alpha, wIn_-gamma, target->ffDelay() + target->localWireDelay());
			if (wIn_ - gamma == alpha)
				typeOfChunks++;
			else
				typeOfChunks+=2; /* beta will have to be computed as well */

				if (typeOfChunks == 3)
					beta = ( (wIn_-gamma) % alpha == 0 ? alpha : ( wIn_-gamma ) % alpha );
				else
					beta = alpha;

				if ( typeOfChunks==2 )
					k = 2;
				else
					k = 2 + int ( ceil ( double ( wIn_ - beta - gamma ) / double ( alpha ) ) );
			}
			else
		{ /* in thiis case there is only one chunk type: gamma */
				alpha = 0;
			beta  = 0;
			k     = 1;
		}
	}

	void IntAdder::updateParameters ( Target* target, map<string, double> inputDelays, int &alpha, int &beta, int &k )
	{
		bool status = target->suggestSlackSubaddSize ( alpha , wIn_,  getMaxInputDelays ( inputDelays ) ); /* chunk size */
		if ( !status ) {
			k=-1;
			alpha=0;
			beta=0;
		}
		else if ( wIn_ == alpha )
		{
			/* addition requires one chunk */
			beta = 0;
			k    = 1;
		}
		else
		{
			beta = ( wIn_ % alpha == 0 ? alpha : wIn_ % alpha );
			k    = ( wIn_ % alpha == 0 ? wIn_ / alpha : int ( ceil ( double ( wIn_ ) / double ( alpha ) ) ) );
		}
	}
	
	/*************************************************************************/
	IntAdder::~IntAdder() {
	}
	
	/*************************************************************************/
	void IntAdder::emulate ( TestCase* tc ) {
		// get the inputs from the TestCase
		mpz_class svX = tc->getInputValue ( "X" );
		mpz_class svY = tc->getInputValue ( "Y" );
		mpz_class svC = tc->getInputValue ( "Cin" );

		// compute the multiple-precision output
		mpz_class svR = svX + svY + svC;
		// Don't allow overflow: the output is modulo 2^wIn
		svR = svR & ((mpz_class(1)<<wIn_)-1);

		// complete the TestCase with this expected output
		tc->addExpectedOutput ( "R", svR );
	}

	
	OperatorPtr IntAdder::parseArguments(Target *target, vector<string> &args) {
		int wIn;
		int arch;
		int optObjective;
		bool srl;
		UserInterface::parseStrictlyPositiveInt(args, "wIn", &wIn, false);
		UserInterface::parseInt(args, "arch", &arch, false);
		UserInterface::parseInt(args, "optObjective", &optObjective, false);
		UserInterface::parseBoolean(args, "SRL", &srl, false);
		return new IntAdder(target, wIn,emptyDelayMap,optObjective,srl,arch);
	}

	void IntAdder::registerFactory(){
		UserInterface::add("IntAdder", // name
			"Integer adder. In modern VHDL, integer addition is expressed by a + and one usually needn't define an entity for it. However, this operator will be pipelined if the addition is too large to be performed at the target frequency.",
											 "BasicInteger", // category
											 "",
											 "wIn(int): input size in bits;\
											 arch(int)=-1: -1 for automatic, 0 for classical, 1 for alternative, 2 for short latency;\
											 optObjective(int)=2: 0 to optimize for logic, 1 to optimize for register, 2 to optimize for slice/ALM count;\
											 SRL(bool)=true: optimize for shift registers",
											 "",
											 IntAdder::parseArguments,
											 IntAdder::unitTest
											 ) ;

	}

	TestList IntAdder::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;
		
		if(index==-1) 
		{ // The unit tests

			for(int wIn=4; wIn<65; wIn+=1) 
			{ // test various input widths
				paramList.push_back(make_pair("wIn", to_string(wIn) ));	
				testStateList.push_back(paramList);
				paramList.clear();
			}
		}
		else     
		{
				// finite number of random test computed out of index
		}	

		return testStateList;
	}

//    void IntAdder::changeName(std::string operatorName){
//		Operator::changeName(operatorName);
//		addImplementationList[selectedVersion]->changeName(operatorName);
//		//		cout << "changin IntAdder name to " << operatorName << endl;
//    }

}


