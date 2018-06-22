#ifndef FIXREALKCM_HPP
#define FIXREALKCM_HPP
#include "../Operator.hpp"
#include "../Table.hpp"
#include "../BitHeap/BitHeap.hpp"



namespace flopoco{

	// Here is the principle for a bit-heap version.
	// The parent first calls the constructor, which predicts the architecture (shift or tables, how many tables) and deduces errorInUlps.
	// The parent sums these errors, and computes the actual g out of this.
	// The asumption is that the error of each architecture will be multiplied by 2^-g.
	// The parent then defines the bit heap.
	// Then it calls addToBitHeap(bitHeap, g) for all its KCMs. This is where VHDL is generated.

	// Only small sub-optimality: for c=0.5 and lsbOut=0, errorInUlps will be counted as 1 (bit lost in the shift, whereas if the bit heap has at least one guard bit the error becomes 0.
	// Never mind.
	
	class FixRealKCMTable;

	class FixRealKCM : public Operator
	{
	public:

		/**
		 * @brief Standalone version of KCM. Input size will be msbIn-lsbIn+1
		 * @param target : target on which we want the KCM to run
		 * @param signedInput : true if input are 2'complement fixed point numbers
		 * @param msbin : power of two associated with input msb. For unsigned 
		 * 				  input, msb weight will be 2^msb, for signed input, it
		 * 				  will be -2^msb
		 * 	@param lsbIn : power of two of input least significant bit
		 * 	@param lsbOut : Weight of the least significant bit of the output
		 * 	@param constant : string that describes the constant in sollya syntax
		 * 	@param targetUlpError : exiged error bound on result. Difference
		 * 							between result and real value should be
		 * 							lesser than targetUlpError * 2^lsbOut.
		 * 							Value has to be in ]0.5 ; 1] (if 0.5 wanted,
		 * 							please consider to create a one bit more
		 * 							precise KCM with a targetUlpError of 1 and
		 * 							truncate the result
		 */
		FixRealKCM(
							 Target* target, 
							 bool signedInput, 
							 int msbIn, 
							 int lsbIn, 
							 int lsbOut, 
							 string constant, 
							 double targetUlpError = 1.0
							 );

		/**
		  @brief A version of KCM that throws its results on an external bit heap. 
			@param parentOp : operator frow which the KCM is a subentity
			@param multiplicandX : signal which will be KCM input (must be a fixed-point signal)
			@param lsbOut : desired output precision i.e. output least 
								significant bit has a weight of 2^lsbOut
			@param constant : string that describes the constant with sollya
									syntax
			@param targetUlpError: target error bound on result, in ulps.
					 The value of the ulp will ultimately be 2^-(lsbOut-g). Difference
										between result and real value should be
										lesser than targetUlpError * 2^lsbOut.
										Value has to be in ]0.5 ; 1] 


	Constructor for an operator incorporated into a global bit heap,
	for use as part of a bigger operator.
	It works in "dry run" mode: it computes g but does not generate any VHDL
	In this case, the flow is typically (see FixFIR)
	1/ call the constructor below. It stops before the VHDL generation 
	2/ have it report its ulp error using getErrorInUlps
	3/ the bigger operator accumulates these ulp errors, and computes	 the global g out of the sum
	4/ It  builds the bit heap, then calls buildForBitHeap(bitHeap, g)

	WARNING : nothing is added to the bitHeap when constant is set to 0.

		 */
		FixRealKCM(
							 Operator* parentOp, 
							 string multiplicandX,
							 bool signedInput,
							 int msbIn,
							 int lsbIn,
							 int lsbOut, 
							 string constant,
							 bool addRoundBit,
							 double targetUlpError = 1.0
							 );

		/** return the error in bits */
		double getErrorInUlps();
		
		
		/**
		 * @brief handle operator initialisation, constant parsing
		 */
		void init();




		/**
		 * @brief do the actual VHDL generation for the bitHeap KCM.
		 */
		void addToBitHeap(BitHeap* bitHeap, int g);



		// Overloading the virtual functions of Operator

		void emulate(TestCase* tc);

		static OperatorPtr parseArguments(
				Target* target,
				vector<string>& args
			);

		static TestList unitTest(int index);

		static void registerFactory();
		
		Operator*	parentOp; 		/**< The operator which envelops this constant multiplier */
		bool signedInput;
		bool signedOutput; /**< computed: true if the constant is negative or the input is signed */
		int msbIn;
		int lsbIn;
		int msbOut;
		int lsbOut;
		string constant;
		float targetUlpError;
		bool addRoundBit; /**< If false, do not add the round bit to the bit heap: somebody else will */ 
		mpfr_t mpC;
		mpfr_t absC;
		int msbC;
		double errorInUlps; /**< These are ulps at position lsbOut-g. 0 if the KCM is exact, 0.5 if it has one table, more if there are more tables. computed by init(). */
		int g; /**< computed late, either by the parent operator, or out of errorInUlps */
		bool negativeConstant;
		bool constantRoundsToZero;
		bool constantIsPowerOfTwo;
		int extraBitForNegativePowerOfTwo;
;



		/** The heap of weighted bits that will be used to do the additions */
		BitHeap*	bitHeap;    	

		/** The input signal. */
		string inputSignalName;
		
		int numberOfTables;
		vector<int> m; /**< MSB of chunk i; m[0] == msbIn */
		vector<int> l; /**< LSB of chunk i; l[numberOfTables-1] = lsbIn, or maybe >= lsbIn if not all the input bits are used due to a small constant */
		vector<int> tableOutputSign; /**< +1: table is always positive. -1: table output is always negative. 0: table output is signed */

		void computeGuardBits();

	private:
		bool specialCasesForBitHeap();
		void buildTablesForBitHeap();
		string createShiftedPowerOfTwo(string resultSignalName);

	};



	class FixRealKCMTable : public Table
	{
		public:
			FixRealKCMTable(
					Target* target, 
					FixRealKCM* mother, 
					int i
					);

			mpz_class function(int x);
			FixRealKCM* mother;
			int index;
			//Weight of input lsb
			int lsbInWeight;
			bool negativeSubproduct;
	};

}


#endif
