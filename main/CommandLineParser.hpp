
#ifndef COMMANDLINEPARSER_HPP_
#define COMMANDLINEPARSER_HPP_

#include <mpreal.h>

#include <functional>
#include <vector>
#include <string>
#include <stdio.h>
#include <fstream>

#include <boost/program_options.hpp>
using namespace boost::program_options;

#include "GeneratorData.hpp"

using namespace std;
using mpfr::mpreal;

	namespace emethod {

		class CommandLineParser {
		public:
			CommandLineParser();
			virtual ~CommandLineParser();

		public:
			void parseCmdLine(int argc, char* argv[]);
			GeneratorData* populateData();

		private:
			string fStr;
			string wStr;
			string deltaStr;
			int scalingFactor;
			int r;

			string xiStr;
			string alphaStr;

			bool scaleInput;
			string inputScalingFactorStr;

			int msbInOut;
			int lsbInOut;

			int verbosity;
			bool isPipelined;
			int frequency;
			int nbTests;

			string configFileName;
			ifstream configFile;
			string versionFileName;
			ifstream versionFile;
		};

	} /* namespace emethod */

#endif /* COMMANDLINEPARSER_HPP_ */
