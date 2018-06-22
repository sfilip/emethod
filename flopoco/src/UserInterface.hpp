/**
A generic user interface class that manages the command line and the documentation for various operators
Includes an operator factory inspired by David Thomas

For typical use, see src/ExpLog/FPExp.*

Author : David Thomas, Florent de Dinechin

Initial software.
Copyright © INSA-Lyon, ENS-Lyon, INRIA, CNRS, UCBL,  
2015-.
  All rights reserved.S

*/
#ifndef UserInterface_hpp
#define UserInterface_hpp

#include "Operator.hpp"
#include <memory>

// Operator Factory, based on the one by David Thomas, with a bit of clean up.
// For typical use, see src/ShiftersEtc/Shifter  or   src/ExpLog/FPExp

  namespace flopoco
  {
  	typedef OperatorPtr (*parser_func_t)(Target *, vector<string> &);	 	//this defines parser_func_t as a pointer to a function to a function taking as parameters Target* etc., and returning an OperatorPtr
  	class OperatorFactory;
  	typedef shared_ptr<OperatorFactory> OperatorFactoryPtr;


	/* There is a bit of refactoring to do: all the pair=values CLI options should be encapsulated in something like
	
	class UIOption
	{
	public:
		string name;
		vector<string> type;
		vector<string> possibleValues;
		vector<string> description;
		vector<string> defaultValue;
	}

	Currently we have this inside the operator factory for the operator parameters, but global options are managed completely differently.
	*/

	// so this would disappear
	typedef pair<string, vector<string>> option_t; 

	
	/** This is the class that manages a list of OperatorFactories, and the overall command line and documentation.
			Each OperatorFactory is responsible for the command line and parsing for one Operator sub-class. */
	class UserInterface
	{
	public:

		/** The function that does it all */
		static void main(int argc, char* argv[]);


		/**  main initialization function */
		static	void initialize();
		/** parse all the operators passed on the command-line */
		static void buildAll(int argc, char* argv[]);

		/** generates the code to the default file */
		static void outputVHDL();

		/** generates a report for operators in globalOpList, and all their subcomponents */
		static void finalReport(ostream & s);
		

		/**a helper factory function. For the parameter documentation, see the OperatorFactory constructor */ 
		static void add(
			string name,
			string description, 
			string category,
			string seeAlso,
			string parameterList, 
			string extraHTMLDoc,  
			parser_func_t parser,
			unitTest_func_t unitTest = nullptr	);
		
		static unsigned getFactoryCount();
		static OperatorFactoryPtr getFactoryByIndex(unsigned i);
		static OperatorFactoryPtr getFactoryByName(string operatorName);



		////////////////// Helper parsing functions to be used in each Operator parser ///////////////////////////////
		static void parseBoolean(vector<string> &args, string key, bool* variable, bool genericOption=false);
		static void parseInt(vector<string> &args, string key, int* variable, bool genericOption=false);
		static void parsePositiveInt(vector<string> &args, string key, int* variable, bool genericOption=false);
		static void parseStrictlyPositiveInt(vector<string> &args, string key, int* variable, bool genericOption=false);
		static void parseFloat(vector<string> &args, string key, double* variable, bool genericOption=false);
		static void parseString(vector<string> &args, string key, string* variable, bool genericOption=false);

		/** Provide a string with the full documentation.*/
		static string getFullDoc();

		/** add an operator to the global (first-level) list, which is stored in its Target (not really its place, sorry).
				This method should be called by 
				1/ the main / top-level, or  
				2/ for sub-components that are really basic operators, 
				expected to be used several times, *in a way that is independent of the context/timing*.
				Typical example is a table designed to fit in a LUT or parallel row of LUTs
		*/

				static void addToGlobalOpList(OperatorPtr op);

		/** generates the code for operators in oplist, and all their subcomponents */
				static void outputVHDLToFile(vector<OperatorPtr> oplist, ofstream& file);

		/** generates the code for operators in globalOpList, and all their subcomponents */
				static void outputVHDLToFile(ofstream& file);

		/** set the name of the output VHDL file */
		static void setOutputFileName(string outputFileName);
				


			private:
		/** register a factory */
				static void registerFactory(OperatorFactoryPtr factory);

		/** error reporting */
				static void throwMissingArgError(string opname, string key);
		/** parse all the generic options such as name, target, verbose, etc. */
				static void parseGenericOptions(vector<string>& args);

		/** Build operators.html directly into the doc directory. */
				static void buildHTMLDoc();

		/** Build flopoco bash autocompletion file **/
				static void buildAutocomplete();

			public:
		static vector<OperatorPtr>  globalOpList;  /**< Level-0 operators. Each of these can have sub-operators */
				static int    verbose;
			private:
				static string outputFileName;
				static string entityName;
				static string targetFPGA;
				static double targetFrequencyMHz;
				static bool   pipeline;
				static bool   clockEnable;
				static bool   useHardMult;
				static bool   plainVHDL;
				static bool   generateFigures;
				static double unusedHardMultThreshold;
				static int    resourceEstimation;
				static bool   floorplanning;
				static bool   reDebug;
				static bool   flpDebug;
		static vector<pair<string,OperatorFactoryPtr>> factoryList; // used to be a map, but I don't want them listed in alphabetical order
		static const vector<pair<string,string>> categories;

		static const vector<string> known_fpgas;
		static const vector<string> special_targets;
		static const vector<option_t> options;

	};

	/** This is the abstract class that each operator factory will inherit. 
			Each OperatorFactory is responsible for the command line and parsing for one Operator sub-class.  */
	class OperatorFactory
	{
		friend UserInterface;
	private:
		
		string m_name; /**< see constructor doc */ 
		string m_description;  /**< see constructor doc */
		string m_category;  /**< see constructor doc */
		string m_seeAlso; /**< see constructor doc */
		vector<string> m_paramNames;  /**< list of paramater names */
		map<string,string> m_paramType;  /**< type of parameters listed in m_paramNames */
		map<string,string> m_paramDoc;  /**< description of parameters listed in m_paramNames */
		map<string,string> m_paramDefault; /* If equal to "", this parameter is mandatory (no default). Otherwise, default value (as a string, to be parsed) */
		string m_extraHTMLDoc; 
		parser_func_t m_parser;
		unitTest_func_t m_unitTest;

	public:
		
		/** Implements a no-frills factory
				\param name         Name for the operator. 
				\param description  The short documentation
				\param category     A category used to organize the doc.
				\param parameters   A semicolon-separated list of parameter description, each being name(type)[=default]:short_description
				\param parser       A function that can parse a vector of string arguments into an Operator instance
				\param extraHTMLDoc Extra information to go to the HTML doc, for instance links to articles or details on the algorithms 
		**/
				OperatorFactory(
					string name,
					string description, 
					string category,
					string seeAlso,
					string parameters,  
					string extraHTMLDoc,  
					parser_func_t parser,
					unitTest_func_t unitTest	);

		virtual const string &name() const // You can see in this prototype that it was not written by Florent
		{ return m_name; } 
		
		/** Provide a string with the full documentation. */
		string getFullDoc();
		/** Provide a string with the full documentation in HTML. */
		string getHTMLDoc();

		const vector<string> &param_names(void) const;

		string getOperatorFunctions(void);

		/** get the default value associated to a parameter (empty string if there is no default)*/
		string getDefaultParamVal(const string& key);

		/*! Consumes zero or more string arguments, and creates an operator
			\param args The offered arguments start at index 0 of the vector, and it is up to the
			factory to check the types and whether there are enough.
			\param consumed On exit, the factory indicates how many of the arguments are used up.
		*/
		virtual OperatorPtr parseArguments(Target* target, vector<string> &args	);
		
		/*! Generate a list of arg-value pairs out of the index value
		*/
		virtual TestList unitTestGenerator(int index);


		};
}; // namespace flopoco

#endif

