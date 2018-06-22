/*
  This file is part of the FloPoCo project

  Author : Florent de Dinechin, Matei Istoan

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2014.

  All rights reserved.
*/

#include "GenericTable.hpp"


using namespace std;

namespace flopoco{

	// A table for the KCM Multiplication
	// the size of the input to the table will be the number of inputs of a LUT
	GenericTable::GenericTable(Target* target_, int wIn_, int wOut_, vector<mpz_class> values_, map<string, double> inputDelays) :
			Table(target_, wIn_, wOut_, 0, -1, false, inputDelays), wIn(wIn_), wOut(wOut_), values(values_)
	{
		ostringstream name;

		srcFileName="GenericTable";
		name <<"GenericTable_" << wIn << "_" << wOut;
		setNameWithFreqAndUID(name.str());
	}

	GenericTable::~GenericTable() {}

	mpz_class GenericTable::function(int x) {

		return  values[x];
	}

}
