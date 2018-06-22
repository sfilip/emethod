
#include "CommandLineParser.hpp"

namespace emethod {

	CommandLineParser::CommandLineParser() {
		versionFileName = "version";
		configFileName  = "";
	}

	CommandLineParser::~CommandLineParser() {
		// TODO Auto-generated destructor stub
	}

	void CommandLineParser::parseCmdLine(int argc, char* argv[])
	{
		try
		{
			// declare a group of options that will be allowed only on command line
			options_description generic("Generic options");
			generic.add_options()
				("version,v", "print the version string")
				("help,h", "produce a help message")
				("help-all", "produce a detailed help message, including hidden options")
				("configFile,c", value<string>(&configFileName)->default_value("emethodHW.cfg"), "name of the file containing the configuration.")
				;

			// declare a group of options that will be allowed both on command line and in config file
			options_description config("Configuration");
			config.add_options()
				//function f set to default to sqrt(1+(9x/2)^4)
				//("function,f", value<string>(&fStr)->required()->default_value("((x * 4.5)^4 + 1)^0.5"), "function to evaluate")
				("function,f", value<string>(&fStr)->required()->default_value("((x * 4.5)^4 + 1)^0.5"), "function to evaluate")
				//weight w set to default to 1
				("weight,w", value<string>(&wStr)->required()->default_value("1.0"), "weight of the function to evaluate")
				//delta set by default to 1/8=0.125
				("delta,d", value<string>(&deltaStr)->required()->default_value("0.125"), "value of delta")
				//scaling factor set by default to 32; all coefficients will be multiplied by 1/scaleFactor
				("scalingFactor", value<int>(&scalingFactor)->required()->default_value(32), "value of the scaling factor")
				//radix r set by default to 2
				("radix,r", value<int>(&r)->required()->default_value(2), "value of the radix")
				//labInOut set by default to -32
				("lsbInOut,l", value<int>(&lsbInOut)->required()->default_value(-32), "value of the input/output LSB weight")
				;

			// hidden options, will be allowed both on command line and in configuration file, but will not be shown to the user
			options_description hidden("Hidden options");
			hidden.add_options()
				//xi computed to need to be set by default to 9/16=0.5625
				("xi,x", value<string>(&xiStr)->default_value("0.5625"), "value of xi")
				//alpha computed to need to be set by default to 7/32=0.21875
				("alpha,a", value<string>(&alphaStr)->default_value("0.21875"), "value of alpha")
				//scaleInput set by default to true
				("scaleInput", value<bool>(&scaleInput)->default_value(true), "select weather the input to the circuit is scaled, or not")
				//inputScalingFactor computed to be set by default to 1/32=0.03125
				("inputScalingFactor", value<string>(&inputScalingFactorStr)->default_value("0.03125"), "value of the scale factor for the input to the circuit")
				//msbInOut set by default to 0 (input is a signed number in [-1,1])
				("msbInOut,m", value<int>(&msbInOut)->default_value(0), "value of the input/output MSB weight")
				//verbosity set by default to 2=DEBUG level informations
				("verbosity", value<int>(&verbosity)->default_value(2), "verbosity level")
				//pipeline set by default to true
				("pipeline", value<bool>(&isPipelined)->default_value(true), "select between a pipelined or a combinatorial circuit")
				//pipeline set by default to true
				("frequency", value<int>(&frequency)->default_value(400), "set the target frequency of the circuit in MHz")
				//testbench set by default to 1000
				("testbench", value<int>(&nbTests)->default_value(1000), "set the number of tests to be generated; set to 0 to disable test generation")
				;

			//create positional options
			//	arguments interpreted as: function, delta, scalingFactor, radix, lsbInOut
			positional_options_description positional;
			positional.add("function", 1).add("weight", 1).add("delta", 1).add("scalingFactor", 1).add("radix", 1).add("lsbInOut", 1);

			// group together options that are available on the command line
			options_description cmdline_options;
			cmdline_options.add(generic).add(config).add(hidden);
			// group together options that are available in the config. file and the hidden options
			options_description config_file_options;
			config_file_options.add(config).add(hidden);
			// group together options that will be displayed for the user
			options_description visible("Allowed options");
			visible.add(generic).add(config);

			// create a variables map, to store the options
			variables_map vm;
			// store the options
			//store(parse_command_line(argc, argv, cmdline_options), vm);
			store(command_line_parser(argc, argv).options(cmdline_options).positional(positional).run(), vm);
			// transfer the options to the variables map
			notify(vm);

			// check if the configuration file exists
			configFile.open(configFileName.c_str(), std::ifstream::in);
			if(!configFile)
			{
				cout << "Warning: could not find a configurations file (set by default to " << configFileName << ")." << endl
						<< "Reverting to defaults and command line options." << endl;
			}else
			{
				// read configurations from the config file
				store(parse_config_file(configFile, config_file_options), vm);
				// transfer the options to the variables map
				notify(vm);
			}
			configFile.close();

			// parse the options

			// help option
			if(vm.count("help")) {
				cout << visible << endl;
				exit(0);
			}

			// detailed help option
			if(vm.count("help-all")) {
				cout << cmdline_options << endl;
				exit(0);
			}

			// version option
			if(vm.count("version")) {
				versionFile.open(versionFileName.c_str(), ios::in);
				if(!versionFile.is_open())
				{
					cout << "Error: unable to open version file!" << endl;
					exit(1);
				}
				string versionStr;
				getline(versionFile, versionStr);

				cout << "emethodHW, version " << versionStr << endl;

				ifstream flopocoVersionFile;
				flopocoVersionFile.open("../flopoco/VERSION", ios::in);
				if(flopocoVersionFile.is_open())
				{
					getline(flopocoVersionFile, versionStr);
					cout << "this program uses:" << endl
							<< " a custom version of FloPoCo " << versionStr << endl
							<< " a custom version of the efrac library" << endl;
				}else{
					cout << "this program uses:" << endl
							<< " a custom version of FloPoCo " << endl
							<< " a custom version of the efrac library" << endl;
				}

				exit(0);
			}
		}
		catch(exception& e)
		{
			cout << "Error while parsing the command line options: " << e.what() << endl;
			return;
		}

		return;
	}

	GeneratorData* CommandLineParser::populateData()
	{
		return new GeneratorData(
				fStr,
				wStr,
				deltaStr,
				scalingFactor,
				r,
				lsbInOut,
				xiStr,
				alphaStr,
				msbInOut,
				scaleInput,
				inputScalingFactorStr,
				verbosity,
				isPipelined,
				frequency,
				nbTests
				);
	}

} /* namespace emethod */
