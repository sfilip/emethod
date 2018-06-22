#ifndef SHIFTREG_HPP
#define SHIFTREG_HPP

#include "Operator.hpp"
#include "utils.hpp"

namespace flopoco{ 

	
	class ShiftReg : public Operator {
	  
		public:
			/* Costructor : w is the input and output size; n is the number of taps */
			ShiftReg(Target* target, int w, int n, map<string, double> inputDelays = emptyDelayMap); 

			/* Destructor */
			~ShiftReg();

			// Below all the functions needed to test the operator
			/* the emulate function is used to simulate in software the operator
			  in order to compare this result with those outputed by the vhdl opertator */
			void emulate(TestCase * tc);

			/* function used to create Standard testCase defined by the developper */
			void buildStandardTestCases(TestCaseList* tcl);

	  	private:
			int w; // input and output size
			int n; // number of tap in ShiftReg
	};

}

#endif
