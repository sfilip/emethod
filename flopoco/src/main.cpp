/*
  the FloPoCo command-line interface

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Authors : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr
			Bogdan Pasca, Bogdan.Pasca@ens-lyon.org

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL, INSA-Lyon
  2008-2014.
  All rights reserved.

*/

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <mpfr.h>
#include <sollya.h>
#include <cstdlib>


#include "FloPoCo.hpp"
#include "utils.hpp"
#include "main.hpp"

using namespace std;
using namespace flopoco;

// FIXME: temporary solution to get a slimmer version of FloPoCo
//		 all the files that have been removed from the copmpilation chain are marked with ##(followed by a space)
//		 ATTENTION: COMMENT AND UNCOMMENT THE FOLLOWING IN CONJUNCTION WITH THE CORRESPONDING LINES IN CMAKELISTS.TXT

int main(int argc, char* argv[] )
{
	try {

		AutoTest::registerFactory();

		IntAdder::registerFactory();
		IntMultiplier::registerFactory();

		IntConstMult::registerFactory();
		FixFunctionByTable::registerFactory();
		FixFunctionBySimplePoly::registerFactory();
		FixFunctionByPiecewisePoly::registerFactory();
		FixFunctionByMultipartiteTable::registerFactory();
		BasicPolyApprox::registerFactory();
		PiecewisePolyApprox::registerFactory();

		GenericSimpleSelectionFunction::registerFactory();
		GenericComputationUnit::registerFactory();
		FixEMethodEvaluator::registerFactory();

		FixRealKCM::registerFactory();
		TestBench::registerFactory();
		Wrapper::registerFactory();

		TargetModel::registerFactory();
		// Uncomment me to play within FloPoCo operator development
	  UserDefinedOperator::registerFactory();
	}
	catch (const std::exception &e) {
		cerr << "Error while registering factories: " << e.what() <<endl;
		exit(EXIT_FAILURE);
	}
	// cout << UserInterface::getFactoryCount() << " factories registered " << endl ;

	try {
		UserInterface::main(argc, argv);
	}
	catch (const std::exception &e) {
		cerr << "Error in main: " << e.what() <<endl;
		exit(EXIT_FAILURE);
	}

	return 0;
}



#if 0

	//------------ Resource Estimation --------------------------------
	int reLevel;
	bool resourceEstimationDebug = false;
	//-----------------------------------------------------------------

	//------------------ Floorplanning --------------------------------
	bool floorplanning = false;
	bool floorplanningDebug = false;
	ostringstream floorplanMessages;
	//-----------------------------------------------------------------


	//------------------------ Resource Estimation ---------------------
	for (vector<Operator*>::iterator it = oplist->begin(); it!=oplist->end(); ++it) {
		Operator* op = *it;

		if(reLevel!=0){
			if(op->reActive)
				cerr << op->generateStatistics(reLevel);
			else{
				cerr << "Resource estimation option active for an operator that has NO estimations in place." << endl;
			}
		}
	}
	//------------------------------------------------------------------


	//------------------------------------------------------------------
#endif




