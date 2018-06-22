/*
  Multipartites Tables for FloPoCo

  Authors: Franck Meyer, Florent de Dinechin

  This file is part of the FloPoCo project

  Initial software.
  Copyright © INSA-Lyon, INRIA, CNRS, UCBL,
  2008-2014.
  All rights reserved.
  */

#include "Multipartite.hpp"
#include "FixFunctionByMultipartiteTable.hpp"
#include "FixFunction.hpp"

#include "../utils.hpp"
#include "../BitHeap/BitHeap.hpp"
//#include "QuineMcCluskey.h"
#include "GenericTable.hpp"

#include <map>
#include <sstream>
#include <vector>
using namespace std;
using namespace flopoco;

namespace flopoco
{
	//--------------------------------------------------------------------------------------- Constructors
	Multipartite::Multipartite(FixFunctionByMultipartiteTable* mpt_, FixFunction *f_, int inputSize_, int outputSize_):
		f(f_), inputSize(inputSize_), outputSize(outputSize_), mpt(mpt_)
	{
		inputRange = intpow2(inputSize_);
		epsilonT = 1 / (intpow2(outputSize+1));
	}

	Multipartite::Multipartite(FixFunction *f_, int m_, int alpha_, int beta_, vector<int> gammai_, vector<int> betai_, FixFunctionByMultipartiteTable *mpt_):
		f(f_), m(m_), alpha(alpha_), beta(beta_), gammai(gammai_), betai(betai_), mpt(mpt_)
	{
		inputRange = mpt_->obj->inputRange;
		inputSize = mpt_->obj->inputSize;
		outputSize = mpt_->obj->outputSize;
		pi = vector<int>(m);
		pi[0] = 0;
		for (int i = 1; i < m; i++)
			pi[i] = pi[i-1] + betai[i-1];

		epsilonT = 1 / (intpow2(outputSize+1));
		computeMathErrors();
	}

	//------------------------------------------------------------------------------------ Private methods

	void Multipartite::computeGuardBits()
	{
		guardBits =  (int) floor(-outputSize - 1
								 + log2(m /
										(intpow2(-outputSize - 1) - mathError)));
	}


	/** Computes the math errors from the values precalculated in the multipartiteTable */
	void Multipartite::computeMathErrors()
	{
		mathError=0;
		for (int i=0; i<m; i++)
		{
			mathError += mpt->oneTableError[pi[i]][betai[i]][gammai[i]];
		}
	}


	void Multipartite::computeSizes()
	{
		int size = (int) intpow2(alpha) * (outputSize + guardBits);
		outputSizeTOi = vector<int>(m);
		sizeTOi = vector<int>(m);
		for (int i=0; i<m; i++)
		{
			computeTOiSize(i);
			size += sizeTOi[i];
		}
		totalSize = size;
	}


	void Multipartite::computeTOiSize(int i)
	{
		double r1, r2, r;
		double delta = deltai(i);
		r1 = abs( delta * si(i,0) );
		r2 = abs( delta * si(i, (int)(intpow2(gammai[i]) - 1)));
		if (r1 > r2)
			r = r1;
		else
			r = r2;
		outputSizeTOi[i]= (int)ceil( outputSize + guardBits + log2(r));

		sizeTOi[i] = (int)intpow2( gammai[i]+betai[i]-1 ) * (outputSizeTOi[i]-1);
	}


	/** Just as in the article */
	double Multipartite::deltai(int i)
	{
		return mui(i, (int)(intpow2(betai[i]) - 1)) - mui(i, 0);
	}


	/** Just as in the article  */
	double Multipartite::mui(int i, int Bi)
	{
		int wi = inputSize;
		return  (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) *  intpow2(-wi+pi[i]) * Bi;
	}


	/** Just as in the article */
	double Multipartite::si(int i, int Ai)
	{
		int wi = inputSize;
		double xleft = (f->signedIn ? -1 : 0)
				+ (f->signedIn ? 2 : 1) * intpow2(-gammai[i])  * ((double)Ai);
		double xright= (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * ((intpow2(-gammai[i]) * ((double)Ai+1)) - intpow2(-wi+pi[i]+betai[i]));
		double delta = deltai(i);
		double si =  (f->eval(xleft + delta)
					  - f->eval(xleft)
					  + f->eval(xright+delta)
					  - f->eval(xright) )    / (2*delta);
		return si;
	}

	static int countBits(int val)
	{
		int calcval = abs(val);
		int count = 0;
		while((calcval >> count) != 0)
			count++;
		if(val >= 0)
			return count;
		return count + 1;
	}

	void Multipartite::compressAndUpdateTIV(int inputSize, int outputSize)
	{
		vector<int> values;
		//vector<expression> exprs;

		for(int i = 0; i < (1 << inputSize); i++)
		{
			values.push_back(tiv->function(i).get_ui());
		}

		int maxBitsCounted;
		int size = -1;
		int betterS;
		for(int s = 0; s < inputSize; s++)
		{
			int maxBits = 0;
			for(int i = 0; i < intpow2(inputSize - s); i++)
			{
				int value = values[intpow2(s) * i];
				for(int j = 0; j < intpow2(s); j++)
				{
					maxBits = max(maxBits, countBits(values[i * intpow2(s) + j] - value));
				}
			}
			int sSize = intpow2(inputSize - s) * outputSize + intpow2(inputSize) * maxBits;

			if(sSize < size || size < 0)
			{
				betterS = s;
				size = sSize;
				maxBitsCounted = maxBits;
			}
		}

		string srcFileName = mpt->getSrcFileName();
		REPORT(DEBUG, "TIV compression : s=" << betterS << ", w'=" << maxBitsCounted << ", size=" << size);

		vector<mpz_class> valsa;
		vector<mpz_class> valsw;
		for(int i = 0; i < intpow2(inputSize - betterS); i++)
		{
			int value = values[intpow2(betterS) * i];
			valsa.push_back(mpz_class(value));
			for(int j = 0; j < intpow2(betterS); j++)
			{
				valsw.push_back(mpz_class((values[i * intpow2(betterS) + j] - value) & ((1 << (maxBitsCounted+1)) - 1)));
			}
		}

		GenericTable* raTiV = new GenericTable(mpt->getTarget(), inputSize - betterS, outputSize, valsa);

		GenericTable* rwTiV = new GenericTable(mpt->getTarget(), inputSize, maxBitsCounted, valsw);

		cTiv = new compressedTIV(mpt->getTarget(), raTiV, rwTiV, betterS, maxBitsCounted, inputSize, outputSize);
	}



	//------------------------------------------------------------------------------------- Public methods

	void Multipartite::buildGuardBitsAndSizes()
	{
		computeGuardBits();
		computeSizes();
	}


	void Multipartite::mkTables(Target* target)
	{
		tiv = new TIV(this, target, alpha, f->wOut + guardBits, 0, intpow2(alpha) - 1);
		toi = vector<TOi*>(m);
		for(int i = 0; i < m; ++i)
		{
			toi[i] = new TOi(this, i, target, gammai[i] + betai[i] - 1, outputSizeTOi[i]-1, 0, intpow2(gammai[i] + betai[i] - 1) - 1);
		}

		compressAndUpdateTIV(alpha, f->wOut + guardBits);
	}



	//------------------------------------------------------------------------------------- Public classes

	Multipartite::TOi::TOi(Multipartite *mp_, int tableIndex, Target *target, int wIn, int wOut, int min, int max):
		Table(target, wIn, wOut, min, max), mp(mp_), ti(tableIndex)
	{
		stringstream name("");
		name << "TOi_table_" << tableIndex;
		setNameWithFreqAndUID(name.str());
	}


	mpz_class Multipartite::TOi::function(int x)
	{
		int TOi;
		double dTOi;

		double y, slope;
		int Ai, Bi;

		// to lighten the notation and bring them closer to the paper
		int wI = mp->inputSize;
		int wO = mp->outputSize;
		int g = mp->guardBits;

		Ai = x >> (mp->betai[ti]-1);
		Bi = x - (Ai << (mp->betai[ti]-1));
		slope = mp->si(ti,Ai); // mathematical slope

		y = slope * intpow2(-wI + mp->pi[ti]) * (Bi+0.5);
		dTOi = y * intpow2(wO+g) * intpow2(mp->f->lsbIn - mp->f->lsbOut) * intpow2(mp->inputSize - mp->outputSize);
		TOi = (int)floor(dTOi);

		return mpz_class(TOi);
	}




	Multipartite::TIV::TIV(Multipartite *mp, Target* target, int wIn, int wOut, int min, int max):
		Table(target, wIn, wOut, min, max), mp(mp)
	{
		setNameWithFreqAndUID("TIV_table");
	}


	mpz_class Multipartite::TIV::function(int x)
	{
		int TIVval;
		double dTIVval;

		double y, yl, yr;
		double offsetX(0);
		double offsetMatula;

		// to lighten the notation and bring them closer to the paper
		int wO = mp->outputSize;
		int g = mp->guardBits;


		for (unsigned int i = 0; i < mp->pi.size(); i++) {
			offsetX+= intpow2(mp->pi[i]) * (intpow2(mp->betai[i]) -1);
		}

		offsetX = offsetX / ((double)mp->inputRange);

		if (mp->m % 2 == 1) // odd
					offsetMatula = 0.5*(mp->m-1);
				else //even
					offsetMatula = 0.5 * mp->m;

		offsetMatula += intpow2(g-1); //for the final rounding

		double xVal = (mp->f->signedIn ? -1 : 0) + (mp->f->signedIn ? 2 : 1) * x * intpow2(-mp->alpha);
		// we compute the function at the left and at the right of
		// the interval
		yl = mp->f->eval(xVal) * intpow2(mp->f->lsbIn - mp->f->lsbOut) * intpow2(mp->inputSize - mp->outputSize);
		yr = mp->f->eval(xVal+offsetX) * intpow2(mp->f->lsbIn - mp->f->lsbOut) * intpow2(mp->inputSize - mp->outputSize);

		// and we take the mean of these values
		y =  0.5 * (yl + yr);
		dTIVval = y * intpow2(g + wO);

		if(mp->m % 2 == 1)
			TIVval = (int) round(dTIVval + offsetMatula);
		else
			TIVval = (int) floor(dTIVval + offsetMatula);

		return mpz_class(TIVval);
	}



	Multipartite::compressedTIV::compressedTIV(Target *target, GenericTable *compressedAlpha, GenericTable *compressedout, int s, int wOC, int wI, int wO)
		: Operator(target), wO_corr(wOC)
	{
		stringstream name("");
		name << "Compressed_TIV_decoder_" << getuid();
		setNameWithFreqAndUID(name.str());

		setCopyrightString("Franck Meyer (2015)");

		addInput("X", wI);
		addOutput("Y1", wO);
		addOutput("Y2", wOC);

		vhdl << tab << declare("XComp", wI-s) << " <= X" << range(wI-1, s) << ";" << endl;
		addSubComponent(compressedAlpha);
		inPortMap(compressedAlpha, "X", "XComp");
		outPortMap(compressedAlpha, "Y", "Y1", false);

		vhdl << tab << instance(compressedAlpha, "TIV_compressed_part");


		addSubComponent(compressedout);
		inPortMap(compressedout, "X", "X");
		outPortMap(compressedout, "Y", "Y2", false);

		vhdl << tab << instance(compressedout, "TIV_correction_part");
	}



}

