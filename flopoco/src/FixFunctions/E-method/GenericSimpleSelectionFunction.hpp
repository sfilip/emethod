/*
 * SimpleSelectionFunction.hpp
 *
 *  Created on: 8 Dec 2017
 *      Author: Matei Istoan
 */

#ifndef _GENERICSIMPLESELECTIONFUNCTION_HPP_
#define _GENERICSIMPLESELECTIONFUNCTION_HPP_

#include <vector>
#include <sstream>
#include <iostream>
#include <string>

#include <sollya.h>
#include <gmpxx.h>

#include "Operator.hpp"
#include "Signal.hpp"
#include "utils.hpp"

#define RADIX8plusSUPPORT 1

namespace flopoco {

	class GenericSimpleSelectionFunction : public Operator
	{
	public:
		/**
		 * A simple constructor.
		 * Currently only implementing radix 2.
		 * Higher radixes (4, 8 etc.) to come.
		 * @param   radix          the radix being used
		 * @param   maxDigit       the maximum digit in the redundant digit set
		 * @param   W              the input signal
		 */
		GenericSimpleSelectionFunction(Target* target,
				int    radix,
				int    maxDigit,
				Signal *W,
				map<string, double> inputDelays = emptyDelayMap);

		/**
		 * Class destructor
		 */
		~GenericSimpleSelectionFunction();

		/**
		 * Compute the format of the W^ signal, from the used radix
		 * and the maximum digit in the digit set
		 */
		static void getWHatFormat(int radix, int maxDigit, int *msb, int *lsb);

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
		int maxDigit;                         /**< the maximum digit in the redundant digit set */
		int msbIn;                            /**< MSB of the input */
		int lsbIn;                            /**< LSB of the input */
		int msbWHat;                          /**< MSB of the WHat signal */
		int lsbWHat;                          /**< LSB of the WHat signal */

		size_t wHatSize;                      /**< size of the W^Hat signal */

		int outputSize;                       /**< size of the output */

	};

} /* namespace flopoco */

#endif /* _GenericSIMPLESELECTIONFUNCTION_HPP_ */
