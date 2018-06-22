#ifndef Zynq7000_HPP
#define Zynq7000_HPP
#include "../Target.hpp"
#include <iostream>
#include <sstream>
#include <vector>


namespace flopoco{

	/** Class for representing an Zynq7000 target */
	class Zynq7000 : public Target
	{
	public:
		/** The default constructor. */  
		Zynq7000() : Target()	{
			id_             		= "Zynq7000";
			vendor_         		= "Xilinx";

			maxFrequencyMHz_		= 500;

			/////// Architectural parameters
			lutInputs_ = 6;
			multXInputs_    		= 25;
			multYInputs_    		= 18;
			sizeOfBlock_ 			= 18432;	// the size of a primitive block is 2^11 * 9
			        // The blocks are 36kb configurable as dual 18k so I don't know.


			//////// Delay parameters, copypasted from Vivado timing reports
			lutDelay_ = 0.124 e-9;
			carry4Delay_ = 0.117 e-9;
			
			//---------------Floorplanning related----------------------
			// TODO this was copypasted from another one: update
			multiplierPosition.push_back(15);
			multiplierPosition.push_back(47);
			multiplierPosition.push_back(55);
			multiplierPosition.push_back(107);
			multiplierPosition.push_back(115);
			multiplierPosition.push_back(147);
						
			memoryPosition.push_back(7);
			memoryPosition.push_back(19);
			memoryPosition.push_back(27);
			memoryPosition.push_back(43);
			memoryPosition.push_back(59);
			memoryPosition.push_back(103);
			memoryPosition.push_back(119);
			memoryPosition.push_back(135);
			memoryPosition.push_back(143);
			memoryPosition.push_back(155);
			memoryPosition.push_back(169);
			
			topSliceX = 169;
			topSliceY = 359;
			
			lutPerSlice = 4;
			ffPerSlice = 8;
			
			dspHeightInLUT = 3;		//3, actually
			ramHeightInLUT = 5;
			
			dspPerColumn = 143;
			ramPerColumn = 71;
			//----------------------------------------------------------

		}

		/** The destructor */
		virtual ~Zynq7000() {}

		/** overloading the virtual functions of Target
		 * @see the target class for more details 
		 */
		double carryPropagateDelay();
		double adderDelay(int size);
		double adder3Delay(int size){return 0;}; // currently irrelevant for Xilinx
		double eqComparatorDelay(int size);
		double eqConstComparatorDelay(int size);
		
		double DSPMultiplierDelay(){ return DSPMultiplierDelay_;}
		double DSPAdderDelay(){ return DSPAdderDelay_;}
		double DSPCascadingWireDelay(){ return DSPCascadingWireDelay_;}
		double DSPToLogicWireDelay(){ return DSPToLogicWireDelay_;}
		double LogicToDSPWireDelay(){ return DSPToLogicWireDelay_;}
		void   delayForDSP(MultiplierBlock* multBlock, double currentCp, int& cycleDelay, double& cpDelay);
		
		double RAMDelay() { return RAMDelay_; }
		double RAMToLogicWireDelay() { return RAMToLogicWireDelay_; }
		double LogicToRAMWireDelay() { return RAMToLogicWireDelay_; }
		
		void   getAdderParameters(double &k1, double &k2, int size);
		double localWireDelay(int fanout = 1);
		double lutDelay();
		double ffDelay();
		double distantWireDelay(int n);
		bool   suggestSubmultSize(int &x, int &y, int wInX, int wInY);
		bool   suggestSubaddSize(int &x, int wIn);
		bool   suggestSubadd3Size(int &x, int wIn){return 0;}; // currently irrelevant for Xilinx
		bool   suggestSlackSubaddSize(int &x, int wIn, double slack);
		bool   suggestSlackSubadd3Size(int &x, int wIn, double slack){return 0;}; // currently irrelevant for Xilinx
		bool   suggestSlackSubcomparatorSize(int &x, int wIn, double slack, bool constant);
		
		int    getIntMultiplierCost(int wInX, int wInY);
		long   sizeOfMemoryBlock();
		DSP*   createDSP(); 
		int    getEquivalenceSliceDSP();
		int    getNumberOfDSPs();
		void   getDSPWidths(int &x, int &y, bool sign = false);
		int    getIntNAdderCost(int wIn, int n);	
	
	private:


		double lutDelay_;       /**< The delay between two LUTs */
		double carry4Delay_;    /**< The delay of the fast carry chain */
#if 0 // unplugged for now
		double elemWireDelay_;  /**< The elementary wire dealy (for computing the distant wire delay) */
	
		double lut2_;           /**< The LUT delay for 2 inputs */
		double lut3_;           /**< The LUT delay for 3 inputs */
		double lut4_;           /**< The LUT delay for 4 inputs */
		double fdCtoQ_;         /**< The delay of the FlipFlop. Also contains an approximate Net Delay experimentally determined */
		double muxcyStoO_;      /**< The delay of the carry propagation MUX, from Source to Out*/
		double muxcyCINtoO_;    /**< The delay of the carry propagation MUX, from CarryIn to Out*/
		double ffd_;            /**< The Flip-Flop D delay*/
		double muxf5_;          /**< The delay of the almighty mux F5*/
		double slice2sliceDelay_;       /**< This is approximate. It approximates the wire delays between Slices */
		double xorcyCintoO_;    /**< the S to O delay of the xor gate */
		int nrDSPs_;			/**< Number of available DSPs on this target */
		int dspFixedShift_;		/**< The amount by which the DSP block can shift an input to the ALU */
		
		double DSPMultiplierDelay_;
		double DSPAdderDelay_;
		double DSPCascadingWireDelay_;
		double DSPToLogicWireDelay_;
		
		double RAMDelay_;
		double RAMToLogicWireDelay_;;
#endif

	};

}
#endif
