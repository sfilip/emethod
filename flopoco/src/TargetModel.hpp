/* This is a dummy operator that should be used to build a new Target.
	 Syntesize it, then look at the critical path and transfer the obtained information in YourTarget.hpp and YourTarget.cpp  
*/
#include "Operator.hpp"

/* This file contains a lot of useful functions to manipulate vhdl */
#include "utils.hpp"


namespace flopoco {

	class TargetModel : public Operator {
	public:


	public:
		TargetModel(Target* target, int type);

		~TargetModel() {};


		/** Factory method that parses arguments and calls the constructor */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);
		
		/** Factory register method */ 
		static void registerFactory();

	private:
		int type; /**< The type of feature we want to model. See the operator docstring for options */

	};


}//namespace
