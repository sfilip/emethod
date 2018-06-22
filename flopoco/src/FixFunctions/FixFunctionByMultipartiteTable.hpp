#ifndef MULTIPARTITETABLE_H
#define MULTIPARTITETABLE_H


#include <mpfr.h>

#include <gmp.h>
#include <gmpxx.h>

#include <vector>

using namespace std;



#include "FixFunction.hpp"
#include "../Operator.hpp"


namespace flopoco
{
	class Multipartite;

	class FixFunctionByMultipartiteTable : public Operator
	{
			// Multipartite needs to access some private fields of FixFunctionByMultipartiteTable, so we declare it as a friend...
			friend class Multipartite;

		public:

			//----------------------------------------------------------------------------- Constructor/Destructor
			/**
			 * @brief The FixFunctionByMultipartiteTable constructor
			 * @param[string] functionName_		a string representing the function, input range should be [0,1) or [-1,1)
			 * @param[int]    lsbIn_		input LSB weight
			 * @param[int]    msbOut_		output MSB weight, used to determine wOut
			 * @param[int]    lsbOut_		output LSB weight
			 * @param[int]	nbTables_	number of tables which will be created
			 * @param[bool]	signedIn_	true if the input range is [-1,1)
			 */
			FixFunctionByMultipartiteTable(Target* target, string function, int nbTables, bool signedIn,
							  int lsbIn, int msbOut, int lsbOut, map<string, double> inputDelays = emptyDelayMap);

			virtual ~FixFunctionByMultipartiteTable();

			//------------------------------------------------------------------------------------- Public methods
			void buildStandardTestCases(TestCaseList* tcl);
			void emulate(TestCase * tc);

			static OperatorPtr parseArguments(Target *target, vector<string> &args);
			static void registerFactory();

		private:

			//------------------------------------------------------------------------------------ Private classes
			/**
			 * @brief The TOXor class defines the xor operator needed to retrieve the correct output from the TO table with the given input
			 */
			class TOXor : public Operator
			{
				public:
					TOXor(Target* target, FixFunctionByMultipartiteTable* mpt, int toIndex, map<string, double> inputDelays = emptyDelayMap);

			};

			//------------------------------------------------------------------------------------ Private methods

			/**
			 * @brief buildOneTableError : Builds the error for every beta_i and gamma_i, to evaluate precision
			 */
			void buildOneTableError();

			void buildGammaiMin();

			/**
			 * @brief enumerateDec : This function enumerates all the decompositions and returns the smallest one
			 * @return The smallest Multipartite decomposition.
			 * @throw "It seems we could not find a decomposition" if there isn't any decomposition with an acceptable error
			 */
			Multipartite* enumerateDec();


			/** Some needed methods, extracted from the article */

			static vector<vector<int>> betaenum (int sum, int size);

			static vector<vector<int>> alphaenum(int alpha, int m);
			static vector<vector<int>> alphaenum(int alpha, int m, vector<int> gammaimin);
			static vector<vector<int>> alphaenumrec(int alpha, int m, vector<int> gammaimin);

			double errorForOneTable(int pi, int betai, int gammai);

			double epsilon(int ci_, int gammai, int betai, int pi);
			double epsilon2ndOrder(int Ai, int gammai, int betai, int pi);

			//--------------------------------------------------------------------------------- Private attributes

			FixFunction *f;
			Multipartite* obj;
			Multipartite* bestMP;

			vector<vector<vector<double>>> oneTableError;
			vector<vector<int>> gammaiMin;

			int g;				/**< the number of extra guard bits to be used for the computations */
			int nbTables;		/**< The number of tables used */


	};

}

#endif // MULTIPARTITETABLE_H
