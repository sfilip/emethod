/*
  Function object for FloPoCo

  Authors: Florent de Dinechin

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  

  All rights reserved.

*/

#include "Function.hpp"
#include <sstream>

namespace flopoco{

	Function::Function(string name_, double xmin, double xmax, double scale)
	{
		/* Convert the input string into a sollya evaluation tree */
		sollya_obj_t sF = sollya_lib_parse_string(name_.c_str());

		// Name HAS to be unique!
		// will cause weird bugs otherwise
		ostringstream complete_name;
		complete_name << name_ << " * " << scale << " on [" << xmin << ", " << xmax << "]"; 
		name = complete_name.str();

		/* If conversion did not succeed (i.e. parse error)
		 * throw an exception */
		if (sF == 0)
			throw "Unable to parse input function.";
		
		/* Map the input to the [0,1[ range */
		// g(y) = scale * f(y * (xmax - xmin) + xmin)
		sollya_obj_t sXW = sollya_lib_constant_from_double(xmax - xmin);
		sollya_obj_t sXMin = sollya_lib_constant_from_double(xmin);
	
		sollya_obj_t sX = SOLLYA_ADD(SOLLYA_MUL(SOLLYA_X_, sXW), sXMin);
		sollya_obj_t sG = sollya_lib_substitute(sF, sX);
	
		node = SOLLYA_MUL(sollya_lib_constant_from_double(scale), sG);
		if (node == 0)
			throw "Sollya error when performing range mapping.";
	
		// No need to free memory for Sollya subexpressions (?)
	}

	Function::~Function()
	{
	  sollya_lib_free(node); // Not sure of this one
	}

	string Function::getName() const
	{
		return name;
	}

	void Function::eval(mpfr_t r, mpfr_t x) const
	{
		sollya_lib_evaluate_function_at_point(r, node, x, NULL);
	}

	double Function::eval(double x) const
	{
		mpfr_t mpX, mpR;
		double r;

		mpfr_inits(mpX, mpR, NULL);
		mpfr_set_d(mpX, x, GMP_RNDN);
		sollya_lib_evaluate_function_at_point(mpR, node, mpX, NULL);
		r = mpfr_get_d(mpR, GMP_RNDN);
		mpfr_clears(mpX, mpR, NULL);

		return r;
	}

	sollya_obj_t Function::getSollyaNode() const
	{
		return node;
	}

}
