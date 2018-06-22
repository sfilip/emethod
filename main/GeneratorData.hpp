
#ifndef GENERATORDATA_HPP_
#define GENERATORDATA_HPP_

#include <mpreal.h>

#include <functional>
#include <stdio.h>

#include <vector>
#include <string>

#include "tokenizer.h"
#include "shuntingyard.h"

using namespace std;
using mpfr::mpreal;

	namespace emethod {

		class GeneratorData {
		public:
			GeneratorData(
					string fStr,
					string wStr,
					string deltaStr,
					int scalingFactor,
					int r,
					int lsbInOut,
					string xiStr,
					string alphaStr,
					int msbInOut,
					bool scaleInput,
					string inputScalingFactorStr,
					int verbosity,
					bool isPipelined,
					int frequency,
					int nbTests);
			virtual ~GeneratorData();

		public:
			function<mpreal(mpreal)> f;
			function<mpreal(mpreal)> w;
			mpreal delta;
			mpreal scalingFactor;
			mpreal r;

			mpreal xi;
			pair<int, int> type;
			pair<mpreal, mpreal> dom;
			mpreal alpha;
			mpreal d2;
			mpreal d1;

			int lsbInOut;
			int msbInOut;

			bool scaleInput;
			mpreal inputScalingFactor;

			int verbosity;
			bool isPipelined;
			int frequency;
			int nbTests;

			vector<string> ftokens;
			vector<string> wtokens;
			shuntingyard sh;

		};

	} /* namespace emethod */

#endif /* GENERATORDATA_HPP_ */
