/*
  Multipartites Tables for FloPoCo

  Authors: Franck Meyer, Florent de Dinechin

  This file is part of the FloPoCo project

  Initial software.
  Copyright © INSA-Lyon, INRIA, CNRS, UCBL,
  2008-2015.
  All rights reserved.
  */

#include <iostream>
#include <math.h>
#include <string.h>
#include <sstream>
#include <vector>

#include <gmp.h>
#include <gmpxx.h>
#include <mpfr.h>

#include <sollya.h>



#include "../utils.hpp"
#include "FixFunctionByMultipartiteTable.hpp"
#include "Multipartite.hpp"
#include "../BitHeap/BitHeap.hpp"


using namespace std;

namespace flopoco
{
	//----------------------------------------------------------------------------- Constructor/Destructor

	/**
	 @brief The FixFunctionByMultipartiteTable constructor
	 @param[string] functionName_		a string representing the function, input range should be [0,1) or [-1,1)
	 @param[int]    lsbIn_		input LSB weight
	 @param[int]    msbOut_		output MSB weight, used to determine wOut
	 @param[int]    lsbOut_		output LSB weight
	 @param[int]	nbTables_	number of tables which will be created
	 @param[bool]	signedIn_	true if the input range is [-1,1)
	 */
	FixFunctionByMultipartiteTable::FixFunctionByMultipartiteTable(Target *target, string functionName_, int nbTables_, bool signedIn_, int lsbIn_, int msbOut_, int lsbOut_, map<string, double> inputDelays):
		Operator(target, inputDelayMap), nbTables(nbTables_)
	{
		f = new FixFunction(functionName_, signedIn_, lsbIn_, msbOut_, lsbOut_);

		srcFileName="FixFunctionByMultipartiteTable";

		ostringstream name;
		name << "FixFunctionByMultipartiteTable_";
		setNameWithFreqAndUID(name.str());

		setCriticalPath(getMaxInputDelays(inputDelays));

		setCopyrightString("Franck Meyer, Florent de Dinechin (2015)");
		addHeaderComment("-- Evaluator for " +  f->getDescription() + "\n");
		REPORT(DETAILED, "Entering: FixFunctionByMultipartiteTable \"" << functionName_ << "\" " << nbTables_ << " " << signedIn_ << " " << lsbIn_ << " " << msbOut_ << " " << lsbOut_ << " ");
		int wX=-lsbIn_;
		addInput("X", wX);
		int outputSize = msbOut_-lsbOut_+1; // TODO finalRounding would impact this line
		addOutput("Y" ,outputSize , 2);
		useNumericStd();


		obj = new Multipartite(this, f, f->wIn, f->wOut);

		// build the required tables of errors
		buildOneTableError();

		buildGammaiMin();

		bestMP = enumerateDec();
		bestMP->mkTables(target);

		vhdl << endl;

		vhdl << tab << declare("in_tiv", bestMP->alpha) <<" <= X" << range(bestMP->inputSize - 1, bestMP->inputSize - bestMP->alpha) << ";" << endl;

		addSubComponent(bestMP->cTiv);
		//addSubComponent(bestMP->tiv);
		inPortMap(bestMP->cTiv, "X", "in_tiv");
		outPortMap(bestMP->cTiv, "Y1", "out_tiv_comp");
		outPortMap(bestMP->cTiv, "Y2", "out_tiv_corr");
		vhdl << instance(bestMP->cTiv, "cTivTable") << endl;


		//addSubComponent(bestMP->cTiv);
		//vhdl << instance(bestMP->cTiv, "cTivTableTest") << endl;

		ostringstream oss("");
		for(int i = 0; i < bestMP->m; i++)
		{
			oss << "TO" << i << ":" << ((bestMP->outputSizeTOi[i]-1) << ( bestMP->gammai[i]+bestMP->betai[i]-1 ))
				<< " (" + (bestMP->outputSizeTOi[i]-1) << ".2^"
				<< ( bestMP->gammai[i]+bestMP->betai[i]-1 ) << endl;
		}

		oss << "Total:" << bestMP->totalSize << endl;

		REPORT(DEBUG, "g=" << bestMP->guardBits);
		REPORT(DEBUG, "size:" << endl << oss.str());
		;
		// Déclaration des composants xor
		int p = 0;

		for(unsigned int i = 0; i < bestMP->toi.size(); ++i)
		{
			stringstream a("");
			a << "a_" << i;

			stringstream b("");
			b << "b_" << i;

			stringstream out("");
			out << "out_" << i;

			TOXor* xor_toi = new TOXor(target, this, i);
			addSubComponent(xor_toi);

			p += bestMP->betai[i];
			vhdl << declare(a.str(), bestMP->gammai[i]) << " <= X" << range(bestMP->inputSize - 1, bestMP->inputSize - bestMP->gammai[i]) << ";" << endl;
			vhdl << declare(b.str(), bestMP->betai[i]) << " <= X" << range(p - 1, p - bestMP->betai[i]) << ";" << endl;


			inPortMap(xor_toi, "a", a.str());
			inPortMap(xor_toi, "b", b.str());
			outPortMap(xor_toi, "r", out.str());

			ostringstream name("");
			name << "xor_to_" << i;
			vhdl << instance(xor_toi, name.str()) << endl;
		}

		BitHeap *bh = new BitHeap(this, bestMP->outputSize + bestMP->guardBits, false, "final_sum");

		bh->setSignedIO(false);

		bh->addUnsignedBitVector(0, "out_tiv_comp", bestMP->outputSize + bestMP->guardBits);
		bh->addUnsignedBitVector(0, "out_tiv_corr", bestMP->cTiv->wO_corr);

		//vhdl << tab << declare("sum", bestMP->outputSize + bestMP->guardBits) << " <= std_logic_vector(signed(out_tiv)";
		for(unsigned int i = 0; i < bestMP->toi.size(); ++i)
		{
			stringstream ss("");
			ss << "out_" << i;
			bh->addUnsignedBitVector(0, ss.str(), bestMP->outputSize + bestMP->guardBits);
		}
		/*vhdl << " + signed(out_" << i << ")";
		vhdl << ");" << endl;*/

		bh->generateCompressorVHDL();
		//bh->generateSupertileVHDL();

		vhdl << tab << "Y <= " << /* "sum" */bh->getSumName() << range(bestMP->outputSize + bestMP->guardBits - 1, bestMP->guardBits) << ";" << endl;
	}


	FixFunctionByMultipartiteTable::~FixFunctionByMultipartiteTable() {
		delete f;
	}


	//------------------------------------------------------------------------------------- Public methods

	void FixFunctionByMultipartiteTable::buildStandardTestCases(TestCaseList* tcl)
	{
		TestCase *tc;

		tc = new TestCase(this);
		tc->addInput("X", 0);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addInput("X", 1);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addInput("X", (mpz_class(1) << f->wIn) - 1);
		emulate(tc);
		tcl->add(tc);

	}


	void FixFunctionByMultipartiteTable::emulate(TestCase* tc)
	{
		f->emulate(tc);
	}

	//------------------------------------------------------------------------------------ Private classes

	FixFunctionByMultipartiteTable::TOXor::TOXor(Target* target, FixFunctionByMultipartiteTable* mpt, int toIndex, map<string, double> inputDelays):
		Operator(target, inputDelays)
	{
		ostringstream name;
		name << "toXor_" << toIndex;
		setNameWithFreqAndUID(name.str());

		Multipartite* mp = mpt->bestMP;
		Table* toi = mp->toi[toIndex];

		int wI = mp->gammai[toIndex] + mp->betai[toIndex] - 1;
		int wO = mp->outputSizeTOi[toIndex] - 1;

		int wA = mp->gammai[toIndex];
		int wB = mp->betai[toIndex];

		setCopyrightString("Franck Meyer (2015)");

		// Component I/O
		addInput("a", mp->gammai[toIndex]);
		addInput("b", mp->betai[toIndex]);

		addOutput("r", mp->outputSize + mp->guardBits);


		vhdl << tab << declare("sign") << " <= not(b(" << mp->betai[toIndex] - 1 << "));" << endl;

		vhdl << tab << declare("in_t", wI) << range(wI - 1, wB - 1) << " <= a" << range(wA - 1, 0) << ";" << endl;

		for(int i = 0; i < wB - 1; i++)
			vhdl << tab << "in_t(" << i << ") <= b(" << i << ") xor sign;" << endl;

		addSubComponent(toi);
		inPortMap(toi, "X", "in_t");
		outPortMap(toi, "Y", "out_t");
		vhdl << instance(toi, "to_table") << endl;

		vhdl << tab << "r" << range(mp->outputSize + mp->guardBits - 1, wO) << " <= (" << (mp->outputSize + mp->guardBits - 1) << " downto " << wO << " => sign);" << endl;
		for(int i = 0; i < wO; i++)
			vhdl << tab << "r(" << i << ") <= out_t(" << i << ") xor sign;" << endl;
	}

	//------------------------------------------------------------------------------------ Private methods

	// enumerating the alphas is much simpler
	vector<vector<int>> FixFunctionByMultipartiteTable::alphaenum(int alpha, int m)
	{
		vector<vector<int>> res;
		vector<vector<int>> res1;
		int ptr;

		if (m==1)
		{
			res = vector<vector<int>>(alpha, vector<int>(1));
			for(int i = 0; i < alpha; i++)
			{
				res[i][0] = i+1;
			}
		}
		else
		{
			res1 = alphaenum(alpha, m-1);
			res = vector<vector<int>>(alpha*res1.size());
			ptr = 0;

			for(int i = 0; i < alpha; i++)
			{
				for(unsigned int j = 0; j < res1.size(); j++)
				{
					res[ptr] = vector<int>(m);
					res[ptr][0] = i+1;
					for(int k = 1; k < m; k++)
					{
						res[ptr][k] = res1[j][k-1];
					}
					ptr++;
				}
			}
		}
		return res;
	}


	// with a table stating where to start the enumeration
	vector<vector<int>> FixFunctionByMultipartiteTable::alphaenum(int alpha, int m, vector<int> gammaimin)
	{
		//test if it is possible with this alpha
		for (int i=0; i<m; i++)
			if (gammaimin[i]>alpha)
				return vector<vector<int>>(0);// else return an empty enumeration
		//else
		return alphaenumrec(alpha, m, gammaimin);

	}


	vector<vector<int>> FixFunctionByMultipartiteTable::alphaenumrec(int alpha, int m, vector<int> gammaimin)
	{
		vector<vector<int>> res;
		vector<vector<int>> res1;
		int ptr;
		if (m==1)
		{
			res = vector<vector<int>>(alpha-gammaimin[0]+1, vector<int>(1));
			for(int i = gammaimin[0]; i <= alpha; i++)
			{
				res[i-gammaimin[0]][0] = i;
			}
		}
		else
		{
			res1 = alphaenumrec(alpha, m-1, gammaimin);
			res = vector<vector<int>>((alpha-gammaimin[m-1]+1)*res1.size());
			ptr = 0;
			for(int i = gammaimin[m-1]; i <= alpha; i++)
			{
				for(unsigned int j = 0; j < res1.size(); j++)
				{
					res[ptr] = vector<int>(m);
					res[ptr][0] = i;
					for(int k = 1; k < m; k++)
						res[ptr][k] = res1[j][k-1];
					ptr++;
				}
			}
		}
		return res;
	}


	vector<vector<int>> FixFunctionByMultipartiteTable::betaenum (int sum, int size)
	{
		int ptr;
		vector<int> t;
		int totalsize(0);
		vector<vector<int>> res;
		vector<vector<vector<int>>> enums;

		if (size == 1)
		{
			t = vector<int>(size);
			t[0] = sum;
			res = vector<vector<int>>(1);
			res[0] = t;
		}
		else if (2*size>sum)
		{
			res=vector<vector<int>>(0);
		}
		else if (2*size == sum)
		{
			t = vector<int>(size);
			for (int i = 0; i < size; i++)
				t[i] = 2;

			res = vector<vector<int>>(1);
			res[0] = t;
		}
		else
		{ // 2*size < sum
			enums = vector<vector<vector<int>>>(sum-2*size+1);//to flatten at the end

			for (int i = 2; i <= sum-2*(size-1); i++)
			{
				enums[i-2] = betaenum(sum-i, size-1);
				totalsize += enums[i-2].size();
			}
			// now we know the size to alloc
			res = vector<vector<int>>(totalsize);
			// now flatten all these smaller enums
			ptr = 0;
			for (unsigned int i = 0; i < enums.size(); i++)
			{
				for(unsigned int j = 0; j < enums[i].size(); j++)
				{
					res[ptr] = vector<int>(size);
					res[ptr][0] = i+2;
					for(int k = 1; k < size; k++)
					{
						res[ptr][k] = enums[i][j][k-1];
					}
					ptr++;
				}
			}

		}
		return res;
	}


	/** See the article p5, right. */
	void FixFunctionByMultipartiteTable::buildGammaiMin()
	{
		int wi = obj->inputSize;
		int gammai;
		gammaiMin = vector<vector<int>>(wi, vector<int>(wi));
		for(int pi = 0; pi < wi; pi++)
		{
			for(int betai = 1; betai < (wi-pi-1); betai++)
			{
				gammai=1;
				while ( (gammai < wi) && (oneTableError[pi][betai][gammai] > obj->epsilonT) )
					gammai++;

				gammaiMin[pi][betai] = gammai;
			}
		}
	}


	/**
	 * @brief buildOneTableError : Builds the error for every beta_i and gamma_i, to evaluate precision
	 */
	void FixFunctionByMultipartiteTable::buildOneTableError()
	{
		int wi = obj->inputSize;
		int gammai, betai, pi;
		oneTableError = vector<vector<vector<double>>>(wi, vector<vector<double>>(wi, vector<double>(wi))); // there will be holes

		for(pi=0; pi<wi; pi++) {
			for(betai=1; betai<wi-pi-1; betai++) {
				for(gammai=1; gammai<= wi-pi-betai; gammai++) {
					oneTableError[pi][betai][gammai] =
							errorForOneTable(pi, betai,gammai);
				}
			}
		}
	}


	/**
	 * @brief enumerateDec : This function enumerates all the decompositions and returns the smallest one
	 * @return The smallest Multipartite decomposition.
	 * @throw "It seems we could not find a decomposition" if there isn't any decomposition with an acceptable error
	 */
	Multipartite* FixFunctionByMultipartiteTable::enumerateDec()
	{
		Multipartite *smallest, *mpt;
		int beta, p;
		int n = obj->inputSize;
		int alphamin = 1;
		int alphamax = (2*n)/3;
		vector<vector<int>> betaEnum;
		vector<vector<int>> alphaEnum;
		vector<int> gammaimin;
		vector<int> gammai;
		vector<int> betai;

		int sizeMax = obj->outputSize * obj->inputRange;
		smallest = new Multipartite(this, f, f->wIn, f->wOut);
		smallest->totalSize = sizeMax;

		for (int alpha = alphamin; alpha <= alphamax; alpha++)
		{
			beta = n-alpha;
			betaEnum = betaenum(beta, nbTables);
			for(unsigned int e = 0; e < betaEnum.size(); e++) {
				betai = betaEnum[e];

				gammaimin = vector<int>(nbTables);
				p = 0;
				for(int i = nbTables-1; i >= 0; i--) {
					gammaimin[i] = gammaiMin[p][betai[i]];
					p += betai[i];
				}

				alphaEnum = alphaenum(alpha,nbTables,gammaimin);
				for(unsigned int ae = 0; ae < alphaEnum.size(); ae++)
				{
					gammai = alphaEnum[ae];

					mpt = new Multipartite(f, nbTables,
										   alpha, beta,
										   gammai, betai, this);

					if(mpt->mathError < obj->epsilonT)
					{
						mpt->buildGuardBitsAndSizes();
					}
					else
						mpt->totalSize = sizeMax;

					if(mpt->totalSize < smallest->totalSize)
						smallest = mpt;
				}
			}
		}
		/* If  parse error throw an exception */
		if (smallest->totalSize == sizeMax)
			throw("It seems we could not find a decomposition");

		return smallest;
	}


	/** 5th equation implementation */
	double FixFunctionByMultipartiteTable::epsilon(int ci_, int gammai, int betai, int pi)
	{
		int wi = obj->inputSize; // for the notations of the paper
		double ci = (double)ci_; // for the notations of the paper

		double xleft = (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * intpow2(-gammai) * ci;
		double xright = (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * (intpow2(-gammai) * (ci+1) - intpow2(-wi+pi+betai));
		double delta = (f->signedIn ? 2 : 1) * intpow2(pi-wi) * (intpow2(betai) - 1);

		double eps = 0.25 * (f->eval(xleft + delta)
							 - f->eval(xleft)
							 - f->eval(xright+delta)
							 + f->eval(xright)
							 );
		return eps;
	}


	double FixFunctionByMultipartiteTable::epsilon2ndOrder(int Ai, int gammai, int betai, int pi)
	{
		int wi = obj->inputSize;
		double xleft = (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * intpow2(-gammai)  * ((double)Ai);
		double delta = (f->signedIn ? 2 : 1) * ( intpow2(pi-wi) * (intpow2(betai) - 1));
		double epsilon = 0.5 * (f->eval(xleft + (delta-1)/2)
								- (delta-1)/2 * (f->eval(xleft + delta)
												 - f->eval(xleft)));
		return epsilon;
	}


	/** eq 11 */
	double FixFunctionByMultipartiteTable::errorForOneTable(int pi, int betai, int gammai)
	{
		double eps1, eps2, eps;

		// Beware when gammai+betai+pi=obj->inputSize,
		// we then have to compute a second order term for the error
		if(gammai+betai+pi==obj->inputSize)
		{
			eps1 = abs(epsilon2ndOrder(0, gammai, betai, pi));
			eps2 = abs(epsilon2ndOrder( (int)intpow2(gammai) -1, gammai, betai, pi));
			if (eps1 > eps2)
				eps = eps1;
			else
				eps = eps2;
		}
		else
		{
			eps1 = abs(epsilon(0, gammai, betai, pi));
			eps2 = abs(epsilon( (int)intpow2(gammai) -1, gammai, betai, pi));

			if (eps1 > eps2)
				eps = eps1;
			else
				eps = eps2;
		}
		return eps;
	}



	OperatorPtr FixFunctionByMultipartiteTable::parseArguments(Target *target, vector<string> &args) {
		string f;
		bool signedIn;
		int lsbIn, lsbOut, msbOut, nbTables;
		UserInterface::parseString(args, "f", &f);
		UserInterface::parseStrictlyPositiveInt(args, "nbTables", &nbTables);
		UserInterface::parseInt(args, "lsbIn", &lsbIn);
		UserInterface::parseInt(args, "msbOut", &msbOut);
		UserInterface::parseInt(args, "lsbOut", &lsbOut);
		UserInterface::parseBoolean(args, "signedIn", &signedIn);
		return new FixFunctionByMultipartiteTable(target, f, nbTables, signedIn, lsbIn, msbOut, lsbOut);
	}

	void FixFunctionByMultipartiteTable::registerFactory(){
		try
		{
		UserInterface::add("FixFunctionByMultipartiteTable", // name
											 "A function evaluator using the multipartite method.",
											 "FunctionApproximation", // category
											 "",
						   "f(string): function to be evaluated between double-quotes, for instance \"exp(x*x)\";\
nbTables(int): the number of tables used to decompose the function, between 2 (bipartite) to 4 or 5 for large input sizes;\
lsbIn(int): weight of input LSB, for instance -8 for an 8-bit input;\
msbOut(int): weight of output MSB;\
lsbOut(int): weight of output LSB;\
signedIn(bool)=false: defines the input range : [0,1) if false, and [-1,1) otherwise\
",
											 
											 "This operator uses the multipartite table method as introduced in <a href=\"http://perso.citi-lab.fr/fdedinec/recherche/publis/2005-TC-Multipartite.pdf\">this article</a>, with the improvement described in <a href=\"http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=6998028&tag=1\">this article</a>. ",
											 FixFunctionByMultipartiteTable::parseArguments
											 ) ;
		}
		catch(string s)
		{
			cerr << s << endl;
		}

	}
}

