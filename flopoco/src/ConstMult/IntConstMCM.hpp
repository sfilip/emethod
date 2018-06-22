#ifndef INTCONSTMCM_HPP
#define INTCONSTMCM_HPP
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

#include "ConstMult/IntConstMult.hpp"

#include "BitHeap/BitHeap.hpp"

/**
	@brief Integer multiple (parallel) constant multiplication

	See also ShiftAddOp, ShiftAddDag, IntConstMult.
	ShiftAddOp defines a shift-and-add operation for IntConstMult.
	ShiftAddDag defines the DAG for IntConstMult.
	IntConstMult is a single constant multiplication, whose DAGs will be merged
		in order to create the IntConstMCM.

	ShiftAddDag deals with the implementation of an IntConstMult as a
	vector of ShiftAddOp. It defines the intermediate variables with
	their bit sizes and provide methods for evaluating the cost of an
	implementation.

*/


namespace flopoco{

	class IntConstMCM : public IntConstMult
	{
	public:
		/** @brief The standard constructor, inputs the number to implement */
		IntConstMCM(Target* target, int xsize, int nbConst, vector<mpz_class> constants);

		~IntConstMCM();

		vector<int> rsizes;
		vector<ShiftAddDag*> implementations;
		int nbConst;
		int nbConstZero;
		vector<mpz_class> constants;  /**< The constants */

		// Overloading the virtual functions of Operator

		void emulate(TestCase* tc);
		void buildStandardTestCases(TestCaseList* tcl);

	private:
		/**
		 * @brief Merge the ShiftAddDags contained in implementations, in order to have
		 * reuse between the constant multipliers.
		 *
		 * Note: this function assumes that the trees for each of the constant multiplications
		 * has already been created, i.e. that implementations has already been initialized
		 */
		void mergeTrees();

		/**
		 * @brief Try to replace the nodes of a tree with the root at @param root
		 * with nodes already existing in @param forest
		 * @param forest an existing forest of trees
		 * @param root the root of the tree in which we're trying to replace elements
		 * @param searchLimit the tree at which to end the searches in the forest of trees given by @param forest
		 */
		void replaceSadInForest(ShiftAddDag* forest, ShiftAddOp* root, ShiftAddOp* searchLimit = NULL);

		/**
		 * @brief Check if a node already exists in a given forest of trees.
		 * @param forest an existing forest of trees
		 * @param node the element we're trying to find
		 * @param searchLimit the tree at which to end the searches in the forest of trees given by @param forest
		 * @return a reference to the existing node, or NULL if the node doesn't already exist
		 */
		ShiftAddOp* sadExistsInForest(ShiftAddDag* forest, ShiftAddOp* node, ShiftAddOp* searchLimit = NULL);

		/**
		 * @brief Check if a node already exists in a given tree.
		 * @param tree an existing tree/subtree
		 * @param node the element we're trying to find
		 * @return a reference to the existing node, or NULL if the node doesn't already exist
		 */
		ShiftAddOp* sadExistsInTree(ShiftAddOp* tree, ShiftAddOp* node);


		vector<BitHeap*> bitheaps;

	};
}
#endif
