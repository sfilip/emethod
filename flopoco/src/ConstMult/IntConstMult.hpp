#ifndef INTCONSTMULT_HPP
#define INTCONSTMULT_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>

#include "Operator.hpp"
#include "ShiftAddOp.hpp"
#include "ShiftAddDag.hpp"
#include "IntAddSubCmp/IntAdder.hpp"

#include "BitHeap/BitHeap.hpp"

/**
	@brief Integer constant multiplication.

	See also ShiftAddOp, ShiftAddDag
	ShiftAddOp defines a shift-and-add operation for IntConstMult.

	ShiftAddDag deals with the implementation of an IntConstMult as a
	vector of ShiftAddOp. It defines the intermediate variables with
	their bit sizes and provide methods for evaluating the cost of an
	implementation.

*/


namespace flopoco{

	class IntConstMult : public Operator
	{
	public:
		/** @brief The standard constructor, inputs the number to implement */
		IntConstMult(Target* target, int xsize, mpz_class n);

		/** @brief A constructor for constants defined as a header and a period (significands of rational constants).
			The actual periodic pattern is given as (period << periodMSBZeroes)
				Parameters i and j are such that the period must be repeated 2^i + 2^j times.
				If j==-1, just repeat the period 2^i times
		 */
		IntConstMult(Target* _target, int _xsize, mpz_class n,
					 mpz_class period, int periodMSBZeroes, int periodSize,
					 mpz_class header, int headerSize,
					 int i, int j);

		/** @brief The bare-bones constructor, used by inheriting classes */
		IntConstMult(Target* target, int xsize);

		~IntConstMult();

		mpz_class n;  /**< The constant */
		int xsize;
		int rsize;
		ShiftAddDag* implementation;




		// Overloading the virtual functions of Operator

		void emulate(TestCase* tc);
		void buildStandardTestCases(TestCaseList* tcl);

		/** Factory method that parses arguments and calls the constructor */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);

		static TestList unitTest(int index);

		/** Factory register method */ 
		static void registerFactory();

		/** @brief Recodes input n; returns the number of non-zero bits */
		int recodeBooth(mpz_class n, int* BoothCode);

		// void buildMultBooth();      /**< Build a rectangular (low area, long latency) implementation */
		ShiftAddDag* buildMultBoothTreeFromRight(mpz_class n);  /**< Build a balanced tree implementation as per the ASAP 2008 paper, right to left */
		ShiftAddDag* buildMultBoothTreeFromLeft(mpz_class n);  /**<  Build a balanced tree implementation as per the ASAP 2008 paper, left to right */
		ShiftAddDag* buildEuclideanTree(const mpz_class n); /**< Build a tree using the lower cost (in terms of size on the FPGA) recursive euclidean division */
		ShiftAddDag* buildMultBoothTreeToMiddle(mpz_class n);  /**< Build a the same tree, but starting from the left and the right joining the middle */

		/** A wrapper that tests the various build*Tree and picks up the best */
		ShiftAddDag* buildMultBoothTreeSmallestShifts(mpz_class n);

		/**
		 * @brief Build a tree implementation using a bitheap for the compression
		 * @param n the constant
		 * @param levels the number of stages of re-utilization
		 */
		ShiftAddDag* buildMultBoothTreeBitheap(mpz_class n, int levels);

		/** @brief Build an optimal tree for rational constants
		 Parameters are such that n = headerSize + (2^i + 2^j)periodSize */
		void buildTreeForRational(mpz_class header, mpz_class period, int headerSize, int periodSize, int i, int j);

	protected:
		bool findBestDivider(const mpz_class n, mpz_t & divider, mpz_t & quotient, mpz_t & remainder);
		void build_pipeline(ShiftAddOp* sao, double& delay);
		string printBoothCode(int* BoothCode, int size);
		void showShiftAddDag();
		void optimizeLefevre(const vector<mpz_class>& constants);
		ShiftAddOp* buildEuclideanDag(const mpz_class n, ShiftAddDag* constant);
		int prepareBoothTree(mpz_class &n, ShiftAddDag* &tree_try, ShiftAddOp** &level, ShiftAddOp* &result, ShiftAddOp* &MX, unsigned int* &shifts, int& nonZeroInBoothCode, int& globalshift);

		BitHeap* bitheap;
	};
}
#endif
