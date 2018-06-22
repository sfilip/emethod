#include "AutoTest.hpp"
#include "../UserInterface.hpp"

#include <vector>
#include <map>
#include <set>

#include <iostream>

namespace flopoco
{

	OperatorPtr AutoTest::parseArguments(Target *target , vector<string> &args)
	{
		string opName;
		bool testDependences;
		UserInterface::parseBoolean(args, "Dependences", &testDependences);
		UserInterface::parseString(args, "Operator", &opName);

		AutoTest AutoTest(opName,testDependences);

		return nullptr;
	}

	void AutoTest::registerFactory()
	{
		UserInterface::add("AutoTest", // name
			"A tester for operators.",
			"AutoTest",
			"", //seeAlso
			"Operator(string): name of the operator to test, All if we need to test all the operators;\
			Dependences(bool)=false: test the operator's dependences;",
			"",
			AutoTest::parseArguments
			) ;
	}

	AutoTest::AutoTest(string opName, bool testDependences)
	{
		system("src/AutoTest/initTests.sh");

		OperatorFactoryPtr opFact;	
		string commandLine;
		string commandLineTestBench;
		set<string> testedOperator;
		set<string>::iterator itOperator;
		vector<string> paramNames;
		vector<string>::iterator itParam;
		map<string,string> unitTestParam;
		map<string,string>::iterator itMap;
		TestList unitTestList;
		TestList::iterator itUnitTestList;
		vector<pair<string,string>>::iterator itUnitTest;
		bool doUnitTest = false;
		bool doRandomTest = false;
		bool allOpTest = false;
		bool testsDone = false;

		if(opName == "All")
		{
			doUnitTest = true;
			doRandomTest = true;
			allOpTest = true;
		}
		else if(opName == "AllUnitTest")
		{
			doUnitTest = true;
			allOpTest = true;
		}
		else if(opName == "AllRandomTest")
		{
			doRandomTest = true;
			allOpTest = true;
		}
		else
		{
			doUnitTest = true;
			doRandomTest = true;
		}

		// Select the Operator(s) to test
		if(allOpTest)
		{
				// We get the operators' names to add them in the testedOperator set
			unsigned nbFactory = UserInterface::getFactoryCount();

			for (unsigned i=0; i<nbFactory ; i++)
			{
				opFact = UserInterface::getFactoryByIndex(i);
				if(opFact->name() != "AutoTest")
					testedOperator.insert(opFact->name());
			}
		}
		else
		{
			opFact = UserInterface::getFactoryByName(opName);
			testedOperator.insert(opFact->name());

				// Do we check for dependences ?
			if(testDependences)		
			{
					// Find all the dependences of the Operator opName
					// Add dependences to the set testedOperator
					// Then we do the same on all Operator in the set testedOperator

				unsigned nbFactory = UserInterface::getFactoryCount();
				set<string> allOperator;

				for (unsigned i=0; i<nbFactory ; i++)
				{
					opFact = UserInterface::getFactoryByIndex(i);
					allOperator.insert(opFact->name());
				}

				for(itOperator = testedOperator.begin(); itOperator != testedOperator.end(); ++itOperator)
				{

					FILE * in;
					char buff[512];
					bool success = true;

						// string grepCommand = "grep '" + *itOperator + "\.o' CMakeFiles/FloPoCoLib.dir/depend.make | awk -F/ '{print $NF}' | awk -F. '{print $1}' | grep ^.*$";

						// Command to get the name of the Operator using the depend file of CMake
					string grepCommand = "grep '" + *itOperator + "\.hpp' CMakeFiles/FloPoCoLib.dir/depend.make | grep -o '.*\.o' | awk -F/ '{print $NF}' | awk -F. '{print $1}'";

					if(!(in = popen(grepCommand.c_str(), "r")))
					{
						success = false;
					}

					if(success)
					{
						while(fgets(buff, sizeof(buff), in) != NULL )
						{
							string opFile = string(buff);
							opFile.erase(opFile.find("\n"));
							if(allOperator.find(opFile) != allOperator.end())
							{
								testedOperator.insert(opFile);
							}
						}
						pclose(in);
					}
				}
			}
		}



		// For each tested Operator, we run a number of tests defined in the Operator's unitTest method
		for(itOperator = testedOperator.begin(); itOperator != testedOperator.end(); ++itOperator)
		{
			testsDone = false;
			system(("src/AutoTest/initOpTest.sh " + (*itOperator)).c_str());
			opFact = UserInterface::getFactoryByName(*itOperator);

			// First we run the unitTest for each tested Operator
			if(doUnitTest)
			{

				unitTestList.clear();
				unitTestList = opFact->unitTestGenerator(-1);

				// Do the unitTestsParamList contains nothing, meaning the unitTest method is not implemented 
				if(unitTestList.size() != 0 )
				{
					testsDone = true;
					//For every Test
					for(itUnitTestList = unitTestList.begin(); itUnitTestList != unitTestList.end(); ++itUnitTestList)
					{
						// Create the flopoco command corresponding to the test
						commandLine = "src/AutoTest/testScript.sh " + (*itOperator);
						commandLineTestBench = "";

						unitTestParam.clear();

									// Fetch all parameters and default values for readability
						paramNames = opFact->param_names();
						for(itParam = paramNames.begin(); itParam != paramNames.end(); ++itParam)
						{
							string defaultValue = opFact->getDefaultParamVal((*itParam));
							unitTestParam.insert(make_pair((*itParam),defaultValue));
						}

						// For every Param
						for(itUnitTest = (*itUnitTestList).begin(); itUnitTest != (*itUnitTestList).end(); ++itUnitTest)
						{
							itMap = unitTestParam.find((*itUnitTest).first);

							if( itMap != unitTestParam.end())
							{
								itMap->second = (*itUnitTest).second;
							}
							else if ((*itUnitTest).first == "TestBench n=")
							{
								commandLineTestBench = " TestBench n=" + (*itUnitTest).second;
							}
							else
							{
								unitTestParam.insert(make_pair((*itUnitTest).first,(*itUnitTest).second));
							}
						}

						if(commandLineTestBench == "")
						{
							//TODO
							commandLineTestBench = defaultTestBenchSize(&unitTestParam);
						}

						for(itMap = unitTestParam.begin(); itMap != unitTestParam.end(); itMap++)
						{
							commandLine += " " + itMap->first + "=" + itMap->second;
						}

						system((commandLine + commandLineTestBench).c_str());	

					}
				}
				else
				{
					cout << "No unitTest method defined" << endl;
				}
			}

			// Then we run random Tests for each tested Operator
			if(doRandomTest)
			{

				unitTestList.clear();
				unitTestParam.clear();
				unitTestList = opFact->unitTestGenerator(0);

				// Do the unitTestsParamList contains nothing, meaning the unitTest method is not implemented 
				if(unitTestList.size() != 0 )
				{
					testsDone = true;
					//For every Test
					for(itUnitTestList = unitTestList.begin(); itUnitTestList != unitTestList.end(); ++itUnitTestList)
					{
						// Create the flopoco command corresponding to the test
						commandLine = "src/AutoTest/testScript.sh " + (*itOperator);
						commandLineTestBench = "";

						unitTestParam.clear();

									// Fetch all parameters and default values for readability
						paramNames = opFact->param_names();
						for(itParam = paramNames.begin(); itParam != paramNames.end(); ++itParam)
						{
							string defaultValue = opFact->getDefaultParamVal((*itParam));
							unitTestParam.insert(make_pair((*itParam),defaultValue));
						}

						// For every Param
						for(itUnitTest = (*itUnitTestList).begin(); itUnitTest != (*itUnitTestList).end(); ++itUnitTest)
						{
							itMap = unitTestParam.find((*itUnitTest).first);

							if( itMap != unitTestParam.end())
							{
								itMap->second = (*itUnitTest).second;
							}
							else if ((*itUnitTest).first == "TestBench n=")
							{
								commandLineTestBench = " TestBench n=" + (*itUnitTest).second;
							}
							else
							{
								unitTestParam.insert(make_pair((*itUnitTest).first,(*itUnitTest).second));
							}
						}

						if(commandLineTestBench == "")
						{
							commandLineTestBench = defaultTestBenchSize(&unitTestParam);
						}

						for(itMap = unitTestParam.begin(); itMap != unitTestParam.end(); itMap++)
						{
							commandLine += " " + itMap->first + "=" + itMap->second;
						}

						system((commandLine + commandLineTestBench).c_str());	

					}
				}
				else
				{
					cout << "No unitTest method defined" << endl;
				}
			}

			if(testsDone)
			{
			// Clean all temporary file
				system(("src/AutoTest/cleanTest.sh " + (*itOperator)).c_str());
			}
		}

		cout << "Tests are finished" << endl;
		exit(EXIT_SUCCESS);		
	}

	string AutoTest::defaultTestBenchSize(map<string,string> * unitTestParam)
	{
		string testBench = " TestBench n=";
		int bitsSum = 0;

		map<string,string>::iterator itParam;

		for(itParam = unitTestParam->begin(); itParam != unitTestParam->end(); ++itParam)
		{
			if(itParam->first.substr(0,1) == "w")
			{
				bitsSum += stoi(itParam->second);
			}
			if(itParam->first.find("In") != string::npos)
			{
				bitsSum += stoi(itParam->second);
			}
		}

		if(bitsSum >= 8)
		{
			testBench += "1000";
		}
		else
		{
			testBench += "-2";
		}

		return testBench;
	}
};
