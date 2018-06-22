#ifndef IntMultiplierS_HPP
#define IntMultiplierS_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "Table.hpp"
#include "BitHeap/BitHeap.hpp"

#include "IntMult//MultiplierBlock.hpp"

namespace flopoco {


	/*
	  Definition of the DSP use threshold in Target.hpp
	*/

	/*
			Corner cases that one has to understand and support:

			related to g, so stand-alone only: in general, g>0 when wOut<wX+wY.
			However, one situation where g=0 and wOut!=wX+wY is the tabulation of a small rounded multiplier.
			To manage this case, 1/ neededGuardBits should anticipate it and 2/ the rounding should always be at the bit 0 of the bit heap.



			Final rounding: to do only in case of a stand-alone operator. Therefore, it shouldn't be in fillBitHeap()


			Workflow for the stand-alone and virtual multipliers
			STAND-ALONE:
			inputs wOut, computes g out of it, computes lsbWeightInBitHeap, instantiates a bit heap, and throws bits in it.
			In case of truncation, needs to add a round bit

			VIRTUAL:
			inputs lsbWeightInBitHeap (which may have been computed by parentOp after calls to neededGuardBits) lsbFullMultWeightInBitheap
			and throws bits in it. No notion of guard bit: this belongs to parentOp.
			No addition of rounding bits: this is the responsibility of parentOp

		As a consequence, attributes of IntMultiplier:
			- there should be no global attribute g, because it is used only in the stand-alone case.
			- there should be no global attribute wOut, because it is used only in the stand-alone case.
			Actually there is a global attribute wOut, but to be used only by emulate()
			- fillBitHeap and the other methods should not refer to g or wOut
	 */



	/** The IntMultiplier class, getting rid of Bogdan's mess.
	 */
			class IntMultiplier : public Operator {

			public:
		/** An elementary LUT-based multiplier, written as a table so that synthesis tools don't infer DSP blocks for it*/
				class SmallMultTable: public Table {
				public:
					int dx, dy, wO;
					bool negate, signedX, signedY;
					SmallMultTable(Target* target, int dx, int dy, int wO, bool negate=false, bool signedX=false, bool signedY=false );
					mpz_class function(int x);
				};

		/**
		 * The IntMultiplier constructor
		 * @param[in] target           the target device
		 * @param[in] wX             X multiplier size (including sign bit if any)
		 * @param[in] wY             Y multiplier size (including sign bit if any)
		 * @param[in] wOut           wOut size for a truncated multiplier (0 means full multiplier)
		 * @param[in] signedIO       false=unsigned, true=signed
		 **/
		 IntMultiplier(Target* target, int wX, int wY, int wOut=0, bool signedIO = false,
		 	map<string, double> inputDelays = emptyDelayMap,bool enableSuperTiles=false);


		/**
		 * The virtual IntMultiplier constructor adds all the multiplier bits to some bitHeap, but no more.
		 * @param[in] parentOp      the Operator to which VHDL code will be added
		 * @param[in] bitHeap       the BitHeap to which bits will be added
		 * @param[in] x            a Signal from which the x input should be taken
		 * @param[in] y            a Signal from which the y input should be taken
		 * @param[in] lsbWeightInBitHeap     the weight, within this BitHeap, corresponding to the LSB of the multiplier output.
		 *                          Note that there should be enough bits below for guard bits in case of truncation.
		 *                          The method neededGuardBits() provides this information.
		 *                          For a stand-alone multiplier lsbWeightInBitHeap=g, otherwise lsbWeightInBitHeap>=g
		 * @param[in] negate     if true, the multiplier result is subtracted from the bit heap
		 * @param[in] signedIO     false=unsigned, true=signed
		 **/
		// FIXME: for now, lsbFullMultWeightInBitheap so as no to break compatibility with the rest of the code
		 IntMultiplier (Operator* parentOp, BitHeap* bitHeap,  Signal* x, Signal* y,
		 	int lsbWeightInBitHeap,
		 	bool negate, bool signedIO,
		 	int lsbFullMultWeightInBitheap = 0);

		/** How many guard bits will a truncated multiplier need? Needed to set up the BitHeap of an operator using the virtual constructor */
		 static int neededGuardBits(Target* target, int wX, int wY, int wOut);




		/** A constructor for fix-point use
		 * Generates a component, and produces VHDL code for the instance inside an operator.
		 * The inputs signal names (x|y)SignalName are names of existing signals of the FloPoCo fixed-point types.
		 * This method reads the fixed-point parameters from them.
		 * It then declares two new signals: rSignalName as a numeric_std (parameters isSigned, rMSB, rLSB),
		 * and rSignalName+"_slv" is the equivalent standard_logic_vector
		 */
		 static IntMultiplier* newComponentAndInstance(
		 	Operator* op,
		 	string instanceName,
		 	string xSignalName,
		 	string ySignalName,
		 	string rSignalName,
		 	int rMSB,
		 	int rLSB
		 	);


		/**
		 *  Destructor
		 */
		 ~IntMultiplier();

		/**
		 * The emulate function.
		 * @param[in] tc               a test-case
		 */
		 void emulate ( TestCase* tc );

		 void buildStandardTestCases(TestCaseList* tcl);

		/** Factory method that parses arguments and calls the constructor */
		 static OperatorPtr parseArguments(Target *target , vector<string> &args);

		/** Factory register method */ 
		 static void registerFactory();

		 static TestList unitTest(int index);


		protected:
		// add a unique identifier for the multiplier, and possibly for the block inside the multiplier
			string addUID(string name, int blockUID=-1);


			string PP(int i, int j, int uid=-1);
			string PPTbl( int i, int j, int uid=-1);
			string XY(int i, int j, int uid=-1);

		/** Can we pack a multiplier of this size in a table? */
			static bool tabulatedMultiplierP(Target* target, int wX, int wY);


		/** Fill the bit heap with all the contributions from this multiplier */
			void fillBitHeap();


			void buildLogicOnly();
			void buildTiling();






		/**	builds the logic block ( smallMultTables)
		 *@param lsbX, lsbY -top right coordinates
		 *@param msbX, msbY -bottom left coordinates
		 *@param uid is just a number which helps to form the signal names (for multiple calling of the method
		 )	*/
		 void buildHeapLogicOnly(int lsbX, int lsbY, int msbX, int msbY, int uid=-1);

		/**	builds the heap using DSP blocks)
		 */
		 void buildXilinxTiling();

		 void buildAlteraTiling( int blockTopX, int blockTopY, int blockBottomX, int blockBottomY, int dspIndex, bool signedX, bool signedY);

		 void buildFancy41x41Tiling();


		/**
		 * checks the DSPThreshold, if only DSPs should be used, only logic, or maybe both, and applies it
		 * is called when no more dsp-s fit in a row, because of the truncation line
		 * @param topX, topY, botX, botY the coordinates of two opposite corners of the multiplication block
		 * 								one with the lsb coodinates (top) and the other with the msb coordinates (bot)
		 * 								Historical choice, because the paving is done from the corner
		 * 								with larger coordinates to the corner with smaller coordinates
		 */
		 bool worthUsingOneDSP(int topX, int topY, int botX, int botY, int wxDSP, int wyDSP);

		/**
		 * checks against the DSPThreshold the given block and adds a DSP or logic
		 * @param isDSPImplementable whether the block has already been checked using worthUsingDSP
		 * 			(so as to avoid an extra call that does the same thing)
		 */
		 void addExtraDSPs(int lsbX, int lsbY, int msbX, int msbY, int wxDSP, int wyDSP, bool isDSPImplementable = false);

		/**
		 * checks how many DSPs will be used in case of a tiling
		 */
		 int checkTiling(int wxDSP, int wyDSP, int& horDSP, int& verDSP);



		int wXdecl;                     /**< the width for X as declared*/
		int wYdecl;                     /**< the width for Y  as declared*/
		int wX;                         /**< the width for X after possible swap such that wX>wY */
		int wY;                         /**< the width for Y after possible swap such that wX>wY */
		int wFullP;                     /**< size of the full product: wX+wY  */
		int wOut;                       /**< size of the output, to be used only in the standalone constructor and emulate.  */
		int lsbWeightInBitHeap;       	/**< the weight in the bit heap of the lsb of the multiplier result ; equals g for standalone multipliers */
		int lsbFullMultWeightInBitheap;	/**< the weight in the bit heap of the full multiplication result (used for truncated multiplications)  */

		private:
		void initialize();     			/**< initialization stuff common to both constructors*/

		int wxDSP, wyDSP;               /**< the width on X/Y in DSP(s)*/
		Operator* parentOp;  			/**< For a virtual multiplier, adding bits to some external BitHeap,
												this is a pointer to the Operator that will provide the actual vhdl stream etc. */
		BitHeap* bitHeap;    			/**< The heap of weighted bits that will be used to do the additions */
		//Plotter* plotter;
		// TODO the three following variable pairs seem ugly redundant
		Signal* x;
		Signal* y;
		string xname;
		string yname;
		string inputName1;
		string inputName2;
		bool negate;                    /**< if true this multiplier computes -xy */
		int signedIO;                   /**< true if the IOs are two's complement */
		bool enableSuperTiles;     		/**< if true, supertiles are built (fewer resources, longer latency */
		int multiplierUid;

		vector<MultiplierBlock*> localSplitVector;
		vector<int> multWidths;
		//vector<DSP*> dsps;
		//ofstream fig;

	};

}
#endif
