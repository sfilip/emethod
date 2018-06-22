/*
  Bipartite Tables for FloPoCo

  Authors: Florent de Dinechin, Matei Istoan

  This file is part of the FloPoCo project

  Initial software.
  Copyright © INSA-Lyon, INRIA, CNRS, UCBL,
  2008-2014.
  All rights reserved.
  */

#include "BipartiteTable.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <string.h>

#include <gmp.h>
#include <mpfr.h>

#include <gmpxx.h>
#include "../utils.hpp"



using namespace std;

namespace flopoco{


	BipartiteTable::BipartiteTable(Target* target_, string functionName_, int lsbIn_, int msbOut_, int lsbOut_,  map<string, double> inputDelays):
			Operator(target_, inputDelays), functionName(functionName_), lsbIn(lsbIn_), msbOut(msbOut_), lsbOut(lsbOut_)
	{
		srcFileName = "BipartiteTable";

		f = new FixFunction(functionName, lsbIn, msbOut, lsbOut);

		wIn  = -lsbIn;
		wOut = msbOut-lsbOut+1;

		ostringstream name;
		name << "BipartiteTable_f_" << vhdlize(functionName) << "_in_" << vhdlize(lsbIn) << "_out_"
				<< vhdlize(msbOut) << "_" << vhdlize(lsbOut);
		setNameWithFreqAndUID(name.str());

		setCopyrightString("Florent de Dinechin, Matei Istoan (2014)");

		addInput("X"  , wIn);
		addOutput("Y" , wOut , 2);

		useNumericStd();

		//TODO: compute the number of needed guard bits
		g = 2;

		//create the signals to address the tables
		n0 = wIn/3;
		n1 = wIn/3;
		n2 = wIn-n0-n1;

		vhdl << tab << declare("X0", n0) << " <= X" << range(wIn-1, wIn-n0) << ";" << endl;
		vhdl << tab << declare("X1", n1) << " <= X" << range(wIn-n0-1, wIn-n0-n1) << ";" << endl;
		vhdl << tab << declare("X2", n2) << " <= X" << range(wIn-n0-n1-1, wIn-n0-n1-n2) << ";" << endl;
		vhdl << endl;

		vhdl << tab << declare("X2_msb") << " <= X2" << of(n2-1) << ";" << endl;
		vhdl << tab << declare("X2_short", n2-1) << " <= X2" << range(n2-2, 0) << ";" << endl;
		vhdl << tab << declare("X2_short_inv", n2-1) << " <= X2_short xor (" << n2-2 << " downto 0 => X2_msb);" << endl;
		vhdl << endl;

		vhdl << tab << declare("tableTIVaddr", n0+n1) << " <= X0 & X1;" << endl;
		vhdl << tab << declare("tableTOaddr", n0+n2-1) << " <= X0 & X2_short_inv;" << endl;
		vhdl << endl;

		//create the TIV and TO tables

		//	create the values to be stored in the tables
		vector<mpz_class> valuesTIV, valuesTO;

		computeTIVvalues(valuesTIV);
		computeTOvalues(valuesTO);

		//	create TIV
		GenericTable *tivTable = new GenericTable(target_, n0+n1, wOut+g, valuesTIV);

		//	add the table to the operator
		addSubComponent(tivTable);
		//useSoftRAM(tivTable);
		useHardRAM(tivTable);
		inPortMap (tivTable , "X", "tableTIVaddr");
		outPortMap(tivTable , "Y", "tableTIVout");
		vhdl << instance(tivTable , "TIVtable");
		vhdl << endl;

		//	create TO
		GenericTable *toTable = new GenericTable(target_, n0+n2-1, wOut+g-n0-n1, valuesTO);

		//add the table to the operator
		addSubComponent(toTable);
		//useSoftRAM(toTable);
		useHardRAM(toTable);
		inPortMap (toTable , "X", "tableTOaddr");
		outPortMap(toTable , "Y", "tableTOout");
		vhdl << instance(toTable , "TOtable");
		vhdl << endl;

		vhdl << tab << declare("tableTOout_inv", wOut+g-n0-n1) << " <= tableTOout xor (" << wOut+g-n0-n1-1 << " downto 0 => X2_msb);" << endl;
		vhdl << endl;

		vhdl << tab << declareFixPoint("tableTIV_fxp", true, msbOut,  lsbOut-g) << " <= signed(tableTIVout);" << endl;
		vhdl << tab << declareFixPoint("tableTO_fxp", true, msbOut-n0-n1,  lsbOut-g) << " <= signed(tableTOout_inv);" << endl;
		resizeFixPoint("tableTO_fxp_sgnExt", "tableTO_fxp", msbOut, lsbOut-g);
		vhdl << endl;

		vhdl << tab << declareFixPoint("Y_int", true, msbOut,  lsbOut-g) << " <= tableTIV_fxp + tableTO_fxp_sgnExt;" << endl;
		resizeFixPoint("Y_int_short", "Y_int", msbOut, lsbOut-1);
		vhdl << tab << declareFixPoint("Y_rnd", true, msbOut,  lsbOut-1) << " <= Y_int_short + (" << zg(wOut, 0) << " & '1');" << endl;
		vhdl << tab << "Y <= std_logic_vector(Y_rnd" << range(wOut, 1) << ");" << endl;
	}

	void BipartiteTable::computeTIVvalues(vector<mpz_class> &values)
	{

		for(int i=0; i<(1<<n0); i++)
			for(int j=0; j<(1<<n1); j++)
			{
				ostringstream newFunctionName;
				mpfr_t x0, x1, temp, temp2, aux, aux2, fx, fxdelta;
				mpz_class functionValue;

				mpfr_inits2(10000, x0, x1, temp, temp2, aux, aux2, fx, fxdelta, (mpfr_ptr)0);

				//create the value of x0, x1
				mpfr_set_si(x0, i, GMP_RNDN);
				mpfr_mul_2si(x0, x0, -n0, GMP_RNDN);
				mpfr_set_si(x1, j, GMP_RNDN);
				mpfr_mul_2si(x1, x1, -n0-n1, GMP_RNDN);

				//create the value of the points in which to evaluate the function
				mpfr_add(temp, x0, x1, GMP_RNDN);
				mpfr_set_si(aux2, 1, GMP_RNDN);
				mpfr_mul_2si(aux2, aux2, -n0-n1-n2, GMP_RNDN);
				mpfr_set_si(aux, 1, GMP_RNDN);
				mpfr_mul_2si(aux, aux, -n0-n1, GMP_RNDN);
				mpfr_add(temp2, x0, x1, GMP_RNDN);
				mpfr_add(temp2, temp2, aux, GMP_RNDN);
				mpfr_sub(temp2, temp2, aux2, GMP_RNDN);

				//create the string containing the function to evaluate
				newFunctionName << "(" << mpfr_get_ld(temp, GMP_RNDN) << ");";
				//convert the input string into a sollya evaluation tree
				sollya_obj_t node, newArg, newNode;
				node = sollya_lib_parse_string(functionName.c_str());
				newArg = sollya_lib_parse_string(newFunctionName.str().c_str());
				newNode = sollya_lib_substitute(node, newArg);
				//if parse error, throw an exception
				if(sollya_lib_obj_is_error(newNode))
				{
					ostringstream error;
					error << srcFileName << ": Unable to parse string \"" << newFunctionName.str() << "\" as a numeric constant" <<endl;
					throw error.str();
				}
				sollya_lib_get_constant(fx, newNode);

				//create the string containing the function to evaluate at the other end of the interval
				newFunctionName.clear();
				newFunctionName.str("");
				newFunctionName << "(" << mpfr_get_ld(temp2, GMP_RNDN) << ");";
				//convert the input string into a sollya evaluation tree
				node = sollya_lib_parse_string(functionName.c_str());
				newArg = sollya_lib_parse_string(newFunctionName.str().c_str());
				newNode = sollya_lib_substitute(node, newArg);
				//if parse error, throw an exception
				if(sollya_lib_obj_is_error(newNode))
				{
					ostringstream error;
					error << srcFileName << ": Unable to parse string \"" << newFunctionName.str() << "\" as a numeric constant" <<endl;
					throw error.str();
				}
				sollya_lib_get_constant(fxdelta, newNode);

				//create the value to store in the table
				mpfr_add(fx, fx, fxdelta, GMP_RNDN);
				mpfr_div_si(fx, fx, 2, GMP_RNDN);

				//extract the value to store in the table
				mpfr_mul_2si(fx, fx, -lsbOut+g, GMP_RNDN);

				mpfr_get_z(functionValue.get_mpz_t(), fx, GMP_RNDN);

				//store the value in the table
				values.push_back(functionValue);

				//clean-up
				mpfr_clears(x0, x1, temp, temp2, aux, aux2, fx, fxdelta, (mpfr_ptr)0);
			}
	}

	void BipartiteTable::computeTOvalues(vector<mpz_class> &values)
	{

		for(int i=0; i<(1<<n0); i++)
			for(int j=0; j<(1<<(n2-1)); j++)
			{
				mpfr_t x0, x2, delta1, delta2, fx, x2SubDelta, temp;
				ostringstream newFunctionName, newArgName;
				mpz_class functionValue, signConstant, auxMpz;

				mpfr_inits2(10000, x0, x2, delta1, delta2, fx, x2SubDelta, temp, (mpfr_ptr)0);

				//create the value of x0, x2
				mpfr_set_si(x0, i, GMP_RNDN);
				mpfr_mul_2si(x0, x0, -n0, GMP_RNDN);
				mpfr_set_si(x2, j, GMP_RNDN);
				mpfr_mul_2si(x2, x2, -n0-n1-n2, GMP_RNDN);
				//create delta1
				mpfr_set_si(delta1, 1, GMP_RNDN);
				mpfr_div_2si(delta1, delta1, n0+1, GMP_RNDN);
				mpfr_set_si(temp, 1, GMP_RNDN);
				mpfr_div_2si(temp, temp, n0+n1+1, GMP_RNDN);
				mpfr_sub(delta1, delta1, temp, GMP_RNDN);
				//create delta2
				mpfr_set_si(delta2, 1, GMP_RNDN);
				mpfr_div_2si(delta2, delta2, n0+n1+1, GMP_RNDN);
				mpfr_set_si(temp, 1, GMP_RNDN);
				mpfr_div_2si(temp, temp, n0+n1+n2+1, GMP_RNDN);
				mpfr_sub(delta2, delta2, temp, GMP_RNDN);
				//create x2-delta2
				mpfr_sub(x2SubDelta, x2, delta2, GMP_RNDN);
				//create the value of the point at which to evaluate the function
				mpfr_add(temp, x0, delta1, GMP_RNDN);
				mpfr_add(temp, temp, delta2, GMP_RNDN);

				//create the string containing the function to evaluate
				newFunctionName << "diff(" << functionName << ")*(" << mpfr_get_ld(x2SubDelta, GMP_RNDN) << ");";
				newArgName << "(" << mpfr_get_ld(temp, GMP_RNDN) << ");";
				//convert the input string into a sollya evaluation tree
				sollya_obj_t node, newArg, newNode;
				node = sollya_lib_parse_string(newFunctionName.str().c_str());
				newArg = sollya_lib_parse_string(newArgName.str().c_str());
				newNode = sollya_lib_substitute(node, newArg);
				//if parse error, throw an exception
				if(sollya_lib_obj_is_error(newNode))
				{
					ostringstream error;
					error << srcFileName << ": Unable to parse string \"" << newFunctionName.str() << "\" as a numeric constant" <<endl;
					throw error.str();
				}
				sollya_lib_get_constant(fx, newNode);

				//extract the value to store in the table
				mpfr_mul_2si(fx, fx, -lsbOut+g, GMP_RNDN);

				mpfr_get_z(functionValue.get_mpz_t(), fx, GMP_RNDN);

				//handle negative constants
				if(mpfr_sgn(fx) < 0)
				{
					auxMpz = mpz_class(1);
					auxMpz = auxMpz << (msbOut-lsbOut+1+g-n0-n1);
					functionValue = auxMpz + functionValue;
				}

				//store the value in the table
				values.push_back(functionValue);

				mpfr_clears(x0, x2, delta1, delta2, fx, x2SubDelta, temp, (mpfr_ptr)0);
			}
	}



	BipartiteTable::~BipartiteTable() {
		free(f);
	}



	void BipartiteTable::emulate(TestCase* tc)
	{
		//f->emulate(tc);

		mpfr_t mpX, mpR;
		mpz_class svX;
		mpz_class svRu, svRd, auxMpz;
		ostringstream newArgName;
		sollya_obj_t node, newArg, newNode;

		/// Get I/O values
		svX = tc->getInputValue("X");

		mpfr_inits2(10000, mpX, mpR, (mpfr_ptr)0);

		//get the true value of X
		mpfr_set_z(mpX, svX.get_mpz_t(), GMP_RNDN);
		mpfr_div_2si(mpX, mpX, -lsbIn, GMP_RNDN);

		//create the string containing the function to evaluate
		newArgName << "(" << mpfr_get_ld(mpX, GMP_RNDN) << ");";
		//convert the input string into a sollya evaluation tree
		node = sollya_lib_parse_string(functionName.c_str());
		newArg = sollya_lib_parse_string(newArgName.str().c_str());
		newNode = sollya_lib_substitute(node, newArg);
		//if parse error, throw an exception
		if(sollya_lib_obj_is_error(newNode))
		{
			ostringstream error;
			error << srcFileName << ": Unable to parse string \"" << newArgName.str() << "\" as a numeric constant" <<endl;
			throw error.str();
		}
		sollya_lib_get_constant(mpR, newNode);

		//extract the value to store in the table
		mpfr_mul_2si(mpR, mpR, -lsbOut, GMP_RNDN);

		mpfr_get_z(svRd.get_mpz_t(), mpR, GMP_RNDD);
		//handle negative constants
		if(mpfr_sgn(mpR) < 0)
		{
			auxMpz = mpz_class(1);
			auxMpz = auxMpz << (wOut);
			svRd = auxMpz + svRd;
		}
		tc->addExpectedOutput("Y", svRd);
		mpfr_get_z(svRu.get_mpz_t(), mpR, GMP_RNDU);
		//handle negative constants
		if(mpfr_sgn(mpR) < 0)
		{
			auxMpz = mpz_class(1);
			auxMpz = auxMpz << (wOut);
			svRu = auxMpz + svRu;
		}
		tc->addExpectedOutput("Y", svRu);
	}

	void BipartiteTable::buildStandardTestCases(TestCaseList* tcl)
	{
		TestCase *tc;

		tc = new TestCase(this);
		tc->addInput("X", 0);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addInput("X", (mpz_class(1) << f->wIn) -1);
		emulate(tc);
		tcl->add(tc);

	}

}

