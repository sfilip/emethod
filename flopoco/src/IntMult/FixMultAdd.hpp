#ifndef FixMultAdd_HPP
#define FixMultAdd_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "IntMultiplier.hpp"
#include "BitHeap/BitHeap.hpp"


namespace flopoco {



	/**
	 * The FixMultAdd class computes A+X*Y
	 * X*Y may be placed anywhere with respect to A;
	 * the product will be truncated when relevant.
	 * The result is specified as its LSB, MSB.
	 *
	 * Note on signed numbers:
	 * The bit heap manages signed addition in any case, so whether the addend is signed or not is irrelevant
	 * The product may be signed, or not.
	*/

	class FixMultAdd : public Operator {

	public:
		/**
		 * The FixMultAdd generic constructor computes x*y+a, faithful to outLSB.
		 * @param[in] target            target device
		 * @param[in] x                 Signal (should be of fixed-point type)
		 * @param[in] y                 Signal (should be of fixed-point type)
		 * @param[in] a                 Signal (should be of fixed-point type)
		 * @param[in] outMSB            weight of the MSB of the product
		 * @param[in] outLSB            weight of the LSB of the product
		 * @param[in] enableSuperTiles  if true, supertiles will decrease resource consumption but increase latency
		 **/
		FixMultAdd(Target* target, Signal* x, Signal* y, Signal* a, int outMSB, int outLSB,
		           bool enableSuperTiles=true, map<string, double> inputDelays = emptyDelayMap);


		/**
		 *  Destructor
		 */
		~FixMultAdd();

		/**
		 * Generates a component, and produces VHDL code for the instance inside an operator.
		 * The inputs signal names (x|y|a)SignalName are names of existing signals of the FloPoCo fixed-point types.
		 * This method reads the fixed-point parameters from them. 
		 * It then declares two new signals: rSignalName as a numeric_std (parameters isSigned, rMSB, rLSB),
		 * and rSignalName+"_slv" is the equivalent standard_logic_vector 
		 */
		static FixMultAdd* newComponentAndInstance(
																							 Operator* op,
																							 string instanceName,
																							 string xSignalName,
																							 string ySignalName,
																							 string aSignalName,
																							 string rSignalName,
																							 int rMSB,
																							 int rLSB
																							 );
		
		/**
		 * The emulate function.
		 * @param[in] tc               a test-case
		 */
		void emulate ( TestCase* tc );

		void buildStandardTestCases(TestCaseList* tcl);

		Signal* x;
		Signal* y;
		Signal* a;

		int wX;                     /**< X input width */
		int wY;                     /**< Y input width */
		int wA;                     /**< A input width */

		int outMSB;                 /**< output MSB */
		int outLSB;                 /**< output LSB */
		int wOut;                   /**< size of the result */
		int lsbPfull;               /**< equal to msbP - wX -wY */
		int lsbA;                  	/**< weight of the LSB of A */

		bool signedIO;              /**< if true, inputs and outputs are signed. */
		bool enableSuperTiles;     	/**< if true, supertiles are built (fewer resources, longer latency */

		string xname;              	/**< X input VHDL name */
		string yname;              	/**< Y input VHDL name */
		string aname;              	/**< A input VHDL name */
		string rname;              	/**< R output VHDL name */

		int g ;                    	/**< the number of guard bits if the product is truncated */
		int maxWeight;             	/**< The max weight for the bit heap of this multiplier, wOut + g*/
		int wOutP;                 	/**< size of the product (not counting the guard bits) */
		double maxError;     		/**< the max absolute value error of this multiplier, in ulps of the result. Should be 0 for untruncated, 1 or a bit less for truncated.*/
		double initialCP;     		/**< the initial delay, getMaxInputDelays ( inputDelays_ ).*/
		int possibleOutputs;  		/**< 1 if the operator is exact, 2 if it is faithfully rounded */

	private:
		Operator* parentOp;  		/**< For a virtual multiplier, adding bits to some external BitHeap,
		                        			this is a pointer to the Operator that will provide the actual vhdl stream etc. */
		BitHeap* bitHeap;    		/**< The heap of weighted bits that will be used to do the additions */
		IntMultiplier* mult; 		/**< the virtual multiplier */
		Plotter* plotter;

		int pMSB;					/**< MSB of the product */
		int pLSB;					/**< LSB of the product */
		int workPMSB;				/**< MSB of the product, aligned with the output precision */
		int workPLSB;				/**< LSB of the product, aligned with the output precision */
		int workAMSB;				/**< MSB of the addend, aligned with the output precision */
		int workALSB;				/**< LSB of the addend, aligned with the output precision */

	};

}
#endif
