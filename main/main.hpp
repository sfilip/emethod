
#ifndef EMETHOD_MAIN_HPP
#define EMETHOD_MAIN_HPP

#include <mpreal.h>
#include "../efrac/efrac.h"
#include "../efrac/tokenizer.h"
#include "../efrac/shuntingyard.h"

#include <vector>
#include <string>
#include <stdio.h>
#include "../flopoco/src/FloPoCo.hpp"

#include "GeneratorData.hpp"
#include "CommandLineParser.hpp"

using namespace std;
using mpfr::mpreal;
using namespace flopoco;

	namespace emethod
	{
		vector<string> tokens;
		shuntingyard sh;

		GeneratorData *genData;
		CommandLineParser *parser;

		vector<string> coeffsP, coeffsQ;

		string outputFileName;

		UserInterface ui;
		Target* target;
		Operator *op;
		Operator *tb;
	}

#endif /* EMETHOD_MAIN_HPP_ */
