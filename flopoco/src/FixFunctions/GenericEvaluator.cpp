/*
  Generic function evaluator, using either HOTBM or polynomial evaluation

  Authors: Sylvain Collange

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  

  All rights reserved.

*/

#include "GenericEvaluator.hpp"
#include "../FixedPointFunctions/HOTBM.hpp"
#include "../FixedPointFunctions/FunctionEvaluator.hpp"
#include <string>
#include <sstream>

namespace flopoco {

static std::string BuildFxName(char const * func, double xmin, double xmax, double scale)
{
	std::ostringstream name;
	name << func << ", " << xmin << ", " << xmax << ", " << scale;
	return name.str();
}

Operator * NewEvaluator(Target * target, char const * func, std::string const & uniqueName,
	int wI, int wO, int o, double xmin, double xmax, double scale,
	EvaluationMethod method)
{
	switch(method) {
	case Hotbm:
		return new HOTBM(target, func, uniqueName, wI, wO, o, xmin, xmax, scale);
	case Polynomial:
		return new FunctionEvaluator(target,
		                             BuildFxName(func, xmin, xmax, scale),
		                             wI, wO, o);
		                             
	default:
		throw std::string("Generic evaluator: invalid method");
	}
}

}
