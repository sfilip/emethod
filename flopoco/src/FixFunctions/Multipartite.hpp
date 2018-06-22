#ifndef MULTIPARTITE_H
#define MULTIPARTITE_H

#include <vector>
#include <map>

#include "FixFunction.hpp"
#include "../Table.hpp"
#include "../Operator.hpp"
#include "GenericTable.hpp"


using namespace std;

namespace flopoco
{
	class FixFunctionByMultipartiteTable;
	typedef vector<vector<int>> expression;

	class Multipartite
	{
		public:

			//---------------------------------------------------------------------------- Constructors/Destructor

			Multipartite(FixFunction *f_, int m_, int alpha_, int beta_, vector<int> gammai_, vector<int> betai_, FixFunctionByMultipartiteTable* mpt_);

			Multipartite(FixFunctionByMultipartiteTable* mpt_, FixFunction* f_, int inputSize_, int outputSize_);

			//------------------------------------------------------------------------------------- Public classes

			/**
			 * @brief The TOi class : Used to store one of the multipartite method TO tables
			 */
			class TOi : public Table
			{
				public:
					TOi(Multipartite *mp_, int tableIndex, Target* target, int wIn, int wOut, int min, int max);

					mpz_class function(int x);

					Multipartite* mp;
					int ti;
			};

			/**
			 * @brief The TIV class : Used to store the multipartite method TIV table
			 */
			class TIV : public Table
			{
				public:
					TIV(Multipartite *mp, Target* target, int wIn, int wOut, int min, int max);

					mpz_class function(int x);


					Multipartite* mp;
			};

			/**
			 * @brief The compressedTIV class : Used to store a slightly compressed version of the TIV table
			 */
			class compressedTIV : public Operator
			{
				public:
					compressedTIV(Target* target, GenericTable* compressedAlpha, GenericTable* compressedout, int s, int wOC, int wI, int wO);

					int wO_corr;
			};

			//------------------------------------------------------------------------------------- Public methods

			/**
			 * @brief buildGuardBitsAndSizes : Builds decomposition's guard bits and tables sizes
			 */
			void buildGuardBitsAndSizes();

			/**
			 * @brief mkTables : Creates the TIV and TOs tables (they will be automatically filled)
			 * @param target : The target FPGA
			 */
			void mkTables(Target* target);

			//---------------------------------------------------------------------------------- Public attributes
			FixFunction* f;

			int inputSize;
			int inputRange;
			int outputSize;

			double epsilonT;

			/** The number of tables of intermediate values */
			int m;
			/** Just as in the article  */
			int alpha;
			/** Just as in the article */
			int beta;
			/** Just as in the article */
			vector<int> gammai;
			/** Just as in the article */
			vector<int> betai;
			/** Just as in the article */
			vector<int> pi;


			/** The Table of Initial Values, just as the ARITH 15 article */
			TIV* tiv;
			compressedTIV* cTiv;

			/** The m Tables of Offset , just as the ARITH 15 article */
			vector<TOi*> toi;

			double mathError;

			int guardBits;
			vector<int> outputSizeTOi;
			vector<int> sizeTOi;
			int totalSize;

			// holds precalculated TOi math errors. Valid as long as we don't change m!
			FixFunctionByMultipartiteTable *mpt;

		private:

			//------------------------------------------------------------------------------------ Private methods
			void computeGuardBits();
			void computeMathErrors();
			void computeSizes();
			void computeTOiSize (int i);

			double deltai(int i);
			double mui(int i, int Bi);
			double si(int i, int Ai);

			void compressAndUpdateTIV(int inputSize, int outputSize);

	};

}
#endif // MULTIPARTITE_H
