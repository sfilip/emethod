/*
 * GenericComputationUnit.hpp
 *
 *  Created on: 13 Jan 2018
 *      Author: Matei Istoan
 */

#ifndef _GENERICCOMPUTATIONUNIT_HPP_
#define _GENERICCOMPUTATIONUNIT_HPP_

#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>

#include <sollya.h>
#include <gmpxx.h>

#include "Operator.hpp"
#include "Signal.hpp"
#include "BitHeap/BitHeap.hpp"
#include "ConstMult/FixRealKCM.hpp"
#include "IntMult/IntMultiplier.hpp"
#include "utils.hpp"

//how will the multiplication X*D_{i+1} be implemented
//	MULTIMODE = 0 - using a multiplexer, a choice from the inputs
//	MULTIMODE = 1 - using a multiplier, by multiplying the inputs
#define MULTMODE_CU 0

//the precision used to parse the constant
#define LARGEPREC 10000

#define RADIX8plusSUPPORT 1

namespace flopoco {

	class GenericComputationUnit : public Operator
	{
	public:
		/**
		 * A simple constructor.
		 *
		 * Performs the following computation:
		 *     w_i[j] = radix * (w_i[j-1] - d_0[j-1]*q_i - d_i[j-1] + d_{i+1}[j-1]*x)
		 * where
		 *     w_i[j] is the output of the computation unit
		 *     w_i[j-1], d_0[j-1], d_i[j-1], d_{i+1}[j-1], x are inputs
		 *     q_i is a constant, also an input
		 *     d_0, d_i and d_{i+1} are digits in the input radix
		 *
		 * Currently only implementing radix 2.
		 * Higher radixes (4, 8 etc.) to come.
		 * @param   radix          the radix being used
		 * @param   maxDigit       the maximum digit in the used digit set
		 * @param   index          the index of the unit
		 * @param   specialCase    the flag indicating special cases:
		 *                           specialCase = 0  corresponds to indices 1-(n-1)
		 *                           specialCase =-1  corresponds to index 0
		 *                           specialCase = 1  corresponds to index n
		 * @param   W              the input signal W
		 * @param   X              the input signal X
		 * @param   Di             the input signal Di
		 * @param   qi             the coefficient q_i
		 */
		GenericComputationUnit(Target* target,
				int    radix,
				int    maxDigit,
				int    index,
				int    specialCase,
				Signal *W,
				Signal *X,
				Signal *Di,
				string qi,
				map<string, double> inputDelays = emptyDelayMap);

		/**
		 * Class destructor
		 */
		~GenericComputationUnit();


		/**
		 * Test case generator
		 */
		void emulate(TestCase * tc);

		// User-interface stuff
		/**
		 * Factory method
		 */
		static OperatorPtr parseArguments(Target *target, std::vector<std::string> &args);

		static void registerFactory();

		static TestList unitTest(int index);

	private:
		int radix;                            /**< the radix of the digit set being used */
		int maxDigit;                         /**< the maximum digit in the used digit set */

		int index;                            /**< the index of the unit */
		int specialCase;                      /**< the flag indicating special cases (iteration 0 and n) */

		int msbW;                             /**< MSB of the W signal */
		int lsbW;                             /**< LSB of the W signal */
		int msbX;                             /**< MSB of the X signal */
		int lsbX;                             /**< LSB of the X signal */
		int msbD;                             /**< MSB of the D signals */
		int lsbD;                             /**< LSB of the D signals */

		int msbInt;                           /**< MSB used for the internal computations */
		int lsbInt;                           /**< LSB used for the internal computations */

		int msbDiMX;                          /**< MSB used of the DiMultX signals */
		int lsbDiMX;                          /**< LSB used of the DiMultX signals */

		string qi;                            /**< the q_i coefficient */
		mpfr_t mpQi;                          /**< the q_i coefficient as an MPFR number */

		BitHeap *bitheap;                     /**< the bitheap used for the computations */
	};

} /* namespace flopoco */

#endif /* _GENERICCOMPUTATIONUNIT_HPP_ */
