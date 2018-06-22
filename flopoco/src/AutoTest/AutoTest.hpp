#ifndef AutoTest_hpp
#define AutoTest_hpp

#include <stdio.h>
#include <string>
#include "../UserInterface.hpp"

using namespace std;

namespace flopoco{

	class AutoTest
	{

	public:

		static OperatorPtr parseArguments(Target *target , vector<string> &args);

		static void registerFactory();

		AutoTest(string opName, bool testDependences = false);

	private:

		string defaultTestBenchSize(map<string,string> * unitTestParam);

	};
};
#endif
