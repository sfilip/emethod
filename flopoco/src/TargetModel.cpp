// general c++ library for manipulating streams
#include <iostream>
#include <sstream>

/* header of libraries to manipulate multiprecision numbers
   There will be used in the emulate function to manipulate arbitraly large
   entries */
#include "gmp.h"
#include "mpfr.h"

// include the header of the Operator
#include "TargetModel.hpp"

using namespace std;
namespace flopoco {




	TargetModel::TargetModel(Target* target, int type_) : Operator(target), type(type_) {
		/* constructor of the TargetModel
		   Target is the targeted FPGA : Stratix, Virtex ... (see Target.hpp for more informations)
		*/

		// definition of the source file name, used for info and error reporting using REPORT 
		srcFileName="TargetModel";

		// definition of the name of the operator
		ostringstream name;
		name << "TargetModel";
		setNameWithFreqAndUID(name.str());
		// Copyright 
		setCopyrightString("Florent de Dinechin");

		/* SET UP THE IO SIGNALS
		   Each IO signal is declared by addInput(name,n) or addOutput(name,n) 
		   where name is a string that stands for the name of the variable and 
		   n is an integer (int)   that stands for the length of the corresponding 
		   input/output */

		addInput ("X" , 32);
		addInput ("Y" , 32);
		addOutput("S" , 32);
		if(type==0) {
			// we first put the most significant bit of the result into R
			REPORT(INFO, "Delay should be adderDelay(32) = " << target->adderDelay(32));
			vhdl << tab << "S <= X+Y;" << endl;
		}
	};





	OperatorPtr TargetModel::parseArguments(Target *target, vector<string> &args) {
		 int type;
		 UserInterface::parseInt(args, "type", &type); // param0 has a default value, this method will recover it if it doesnt't find it in args, 
		 return new TargetModel(target, type);
	}
	
	void TargetModel::registerFactory(){
		UserInterface::add("TargetModel", // name
											 "A dummy operator useful when designing a new Target", // description, string
											 "Miscellaneous", // category, from the list defined in UserInterface.cpp
											 "", //seeAlso
											 // Now comes the parameter description string.
											 // Respect its syntax because it will be used to generate the parser and the docs
											 // Syntax is: a semicolon-separated list of parameterDescription;
											 // where parameterDescription is parameterName (parameterType)[=defaultValue]: parameterDescriptionString 
											 "type(int)=0: when 0, build an adder of size 32",
											 // More documentation for the HTML pages. If you want to link to your blog, it is here.
											 "This operator is for FloPoCo developers only. <br> Synthesize this operator, then look at its critical path. <br> Also see Target.hpp.",
											 TargetModel::parseArguments
											 ) ;
	}

}//namespace
