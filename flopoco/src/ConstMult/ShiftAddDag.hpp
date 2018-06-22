#ifndef SHIFTADDDAG_HPP
#define SHIFTADDDAG_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "ShiftAddOp.hpp"
#include "../Operator.hpp"


namespace flopoco{

	class IntConstMult;
	class ShiftAddOp;

	class ShiftAddDag {
	public:
		IntConstMult* icm;
		vector<ShiftAddOp*> saolist;  // the shift-and-add operators computed so far
		ShiftAddOp* PX;
		ShiftAddOp* result;

		//support for DAGs with multiple heads
		vector<ShiftAddOp*> saoHeadlist;  // the shift-and-add operators computed so far which are heads of the DAG
		vector<int> saoHeadShift;		  // the shift of the heads of the DAG, relative to the weight of the lsb

		ShiftAddDag(IntConstMult* icm);

		ShiftAddDag(ShiftAddDag* reference); //copy constructor. This perform a deep copy of saolist and PX and copies only the pointer icm (not the instance)

		virtual ~ShiftAddDag();

		/**
		 * @brief This method looks up in the current Dag if the required op
		 * exists, and either returns a pointer to it, or creates the
		 * corresponding node.
		 */
		ShiftAddOp* provideShiftAddOp(ShiftAddOpType op, ShiftAddOp* i, int s, ShiftAddOp* j=NULL);

		mpz_class computeConstant(ShiftAddOpType op, ShiftAddOp* i, int s, ShiftAddOp* j);

		/** @brief This method appends saolist with patch.saolist and returns patch.result. */
		ShiftAddOp* sadAppend(ShiftAddDag* patch);

	};
}

#endif
