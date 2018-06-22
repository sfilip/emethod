#include <iostream>
#include <sstream>

#include "gmp.h"
#include "mpfr.h"

#include "ShiftReg.hpp"

using namespace std;

namespace flopoco {

	ShiftReg::ShiftReg(Target* target, int w_, int n_, map<string, double> inputDelays)
		: Operator(target, inputDelays), w(w_), n(n_)
	{
		srcFileName="ShiftReg";
		setCopyrightString ( "Louis Beseme, Florent de Dinechin (2014)" );
		useNumericStd_Unsigned();

		ostringstream name;
		name << "ShiftReg_"<< w_;
		setNameWithFreqAndUID( name.str() );

		addInput("X", w, true);

		for(int i=0; i<n; i++) {
			addOutput(join("Xd", i), w, true);
		}

		setCriticalPath(getMaxInputDelays(inputDelays));

		vhdl << tab << declare("XX", w, false, Signal::registeredWithAsyncReset) << " <= X;" << endl;

		
		for(int i=0; i<n; i++) {
			vhdl << tab << join("Xd", i) << " <= XX;" << endl;
			if (i<n-1)
				nextCycle(false);
		}
		setPipelineDepth(0);
	};

	ShiftReg::~ShiftReg(){

	};

	void ShiftReg::emulate(TestCase * tc){

	};

	void ShiftReg::buildStandardTestCases(TestCaseList* tcl){

	};

}
	
