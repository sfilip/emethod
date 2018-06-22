#include "main.hpp"

using namespace emethod;
using std::getline;

int main(int argc, char* argv[])
{

    mpreal::set_default_prec(500);
    parser = new CommandLineParser();
    parser->parseCmdLine(argc, argv);
    genData = parser->populateData();

    QSexactStart();
    QSexact_set_precision(500);
    {
    	vector<mpreal> num;
    	vector<mpreal> den;
    	mpreal numScalingFactor;

    	mpreal errorEstimation;
    	efrac(errorEstimation, num, den, numScalingFactor,
          genData->f, genData->w, genData->delta, genData->xi,
          genData->d1, genData->d2, genData->type, genData->dom,
          genData->scalingFactor);

    }
    QSexactClear();

    ifstream coeffsFile;
    coeffsFile.open("discCoeffsExample.txt", ios::in);
    if(!coeffsFile.is_open())
    {
    	cout << "Error: unable to open coefficients file!" << endl;
    	exit(1);
    }

    int n, m;
    string line;

    getline(coeffsFile, line);
    n = stoi(line, nullptr, 10);
    getline(coeffsFile, line);
    m = stoi(line, nullptr, 10);
    //skip over delta
    getline(coeffsFile, line);

    for(int i=0; i <= n; i++)
    {
    	std::getline(coeffsFile, line);
    	coeffsP.push_back(line);
    }
    for(int i=0; i <= m; i++)
    {
    	getline(coeffsFile, line);
    	coeffsQ.push_back(line);
    }

    coeffsFile.close();

    outputFileName = "EMethod.vhdl";

    ofstream file;
    file.open(outputFileName.c_str(), ios::out);

    ui.verbose = genData->verbosity;
    ui.setOutputFileName(outputFileName);

    target = new Virtex6();
    target->setGenerateFigures(true);
    target->setPipelined(genData->isPipelined);
    //frequency input in MHz at the command line, so needs to be
    //converted to HZ here
    target->setFrequency((double)genData->frequency * 1.0e6);

    try
    {
    	op = new FixEMethodEvaluator(target,            //target
    							(int)genData->r,                    //radix
									(int)genData->r-1,                  //maximum digit
									genData->msbInOut,                  //msbInOut
									genData->lsbInOut,                  //lsbInOut
									coeffsP,                            //coeffsP
									coeffsQ,                            //coeffsQ
									(double)genData->delta,             //delta
									genData->scaleInput,                //scaleInput
									(double)genData->inputScalingFactor //inputScaleFactor
    	);
    }
    catch(string& e)
    {
    	cout << endl << "Error creating the operator: " << e << endl;
    	exit(1);
    }

    if(genData->nbTests > 0)
    {
		try
		{
			tb = new TestBench(target, op, genData->nbTests, true);
		}
		catch(string& e)
		{
			cout << endl << "Error creating the testbench: " << e << endl;
			exit(1);
		}
    }

    ui.addToGlobalOpList(op);
    if(genData->nbTests > 0)
    {
    	ui.addToGlobalOpList(tb);
    }

    ui.outputVHDLToFile(file);

    file.close();

    UserInterface::finalReport(cerr);
}












