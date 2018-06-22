/*
  A wrapper generator for FloPoCo.

  A wrapper is a VHDL entity that places registers before and after
  an operator, so that you can synthesize it and get delay and area,
  without the synthesis tools optimizing out your design because it
  is connected to nothing.

  Author: Florent de Dinechin

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2010.
  All rights reserved.
 */

#include <iostream>
#include <sstream>
#include "Operator.hpp"
#include "Wrapper.hpp"

namespace flopoco{

	Wrapper::Wrapper(Target* target, Operator *op):
		Operator(target), op_(op)
	{
		setCopyrightString("Florent de Dinechin (2007)");
		/* the name of the Wrapped operator consists of the name of the operator to be
			wrapped followd by _Wrapper */
		setNameWithFreqAndUID(op_->getName() + "_Wrapper");

		//this operator is a sequential one	even if Target is unpipelined
		setSequential();

		// Add the wrapped operator to the subcomponent list
		addVirtualSubComponent(op);

		//support for fixed point
		if(op_->getStdLibType() == -1)
			useStdLogicSigned();
		else if(op_->getStdLibType() == 0)
			useStdLogicUnsigned();
		else if(op_->getStdLibType() == 1)
			useNumericStd();
		else if(op_->getStdLibType() == 2)
			useNumericStd_Signed();
		else
			useNumericStd_Unsigned();


		// Copy the signals of the wrapped operator
		// This replaces addInputs and addOutputs
		for(int i=0; i < op->getIOListSize(); i++)	{
			Signal* s = op->getIOListSignal(i);
			if(s->type() == Signal::in)
			{
				if(s->isFix())
					addFixInput(s->getName(), s->isFixSigned(), s->MSB(), s->LSB());
				else
					addInput(s->getName(), s->width(), s->isBus());
			}
			if(s->type() == Signal::out)
			{
				if(s->isFix())
					addFixOutput(s->getName(), s->isFixSigned(), s->MSB(), s->LSB(), s->getNumberOfPossibleValues());
				else
					addOutput(s->getName(), s->width(), s->isBus());
			}

		}


 		string idext;

		// copy inputs
		for(int i=0; i < op->getIOListSize(); i++){
			Signal* s = op->getIOListSignal(i);
			 if(s->type() == Signal::in) {
				 idext = "i_"+s->getName();
				 if(s->isFix())
					 vhdl << tab << declareFixPoint(idext, s->isFixSigned(), s->MSB(), s->LSB());
				 else
					 vhdl << tab << declare(idext, s->width(), s->isBus());
				 vhdl << " <= " << s->getName() << ";" << endl;
			}
		}

		// register inputs
		setCycle(1);

		for(int i=0; i < op->getIOListSize(); i++){
			Signal* s = op->getIOListSignal(i);
			 if(s->type() == Signal::in) {
				 idext = "i_"+s->getName();
				 inPortMap (op, s->getName(), idext);
			 }
		}


		// port map the outputs
		for(int i=0; i < op->getIOListSize(); i++){
			Signal* s = op->getIOListSignal(i);
			if(s->type() == Signal::out) {
				idext = "o_" + s->getName();
				outPortMap (op, s->getName(), idext);
			}
		}

		// The VHDL for the instance
		vhdl << instance(op, "test");

		// Advance cycle to the cycle of the outputs
		syncCycleFromSignal(idext, false); // this is the last output
		nextCycle();

		// copy the outputs
		for(int i=0; i < op->getIOListSize(); i++){
			Signal* s = op->getIOListSignal(i);
			if(s->type() == Signal::out) {
				string idext = "o_" + s->getName();
				vhdl << tab << s->getName() << " <= " << use(idext) << ";" <<endl;
			}
		}

	}

	Wrapper::~Wrapper() {
	}

	OperatorPtr Wrapper::parseArguments(Target *target, vector<string> &args) {
		if(UserInterface::globalOpList.empty()){
			throw("ERROR: Wrapper has no operator to wrap (it should come after the operator it wraps)");
		}

		Operator* toWrap = UserInterface::globalOpList.back();
		//		UserInterface::globalOpList.pop_back();

		return new Wrapper(target, toWrap);
	}

	void Wrapper::registerFactory(){
			UserInterface::add("Wrapper", // name
								 "Wraps the preceding operator between registers (for frequency testing).",
								 "TestBenches",
								 "fixed-point function evaluator; fixed-point", // categories
								 "",
								 "",
								 Wrapper::parseArguments
								 ) ;
	}

}
