#ifndef FLOPOCO_HPP
#define FLOPOCO_HPP


// FIXME: temporary solution to get a slimmer version of FloPoCo
//		 all the files that have been removed from the copmpilation chain are marked with ##(followed by a space)
//		 ATTENTION: COMMENT AND UNCOMMENT THE FOLLOWING IN CONJUNCTION WITH THE CORRESPONDING LINES IN CMAKELISTS.TXT


// TODO: I guess we should at some point copy here only the public part of each class,
// to provide a single self-contained include file.

// support the autotools-generated config.h
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Operator.hpp"
#include "UserInterface.hpp"
#include "FlopocoStream.hpp"

/* targets ---------------------------------------------------- */
#include "Target.hpp"
#include "TargetModel.hpp"
#include "Targets/DSP.hpp"

#include "Targets/Spartan3.hpp"
#include "Targets/Virtex4.hpp"
#include "Targets/Virtex5.hpp"
#include "Targets/Virtex6.hpp"

#include "Targets/StratixII.hpp"
#include "Targets/StratixIII.hpp"
#include "Targets/StratixIV.hpp"
#include "Targets/StratixV.hpp"

#include "Targets/CycloneII.hpp"
#include "Targets/CycloneIII.hpp"
#include "Targets/CycloneIV.hpp"
#include "Targets/CycloneV.hpp"

#include "TestBenches/TestBench.hpp"

/* regular pipelined integer adder/ adder+subtracter --------- */
#include "IntAddSubCmp/IntAdder.hpp" // includes several other .hpp

/* Integer and fixed-point multipliers ------------------------ */
#include "IntMult/IntMultiplier.hpp"
#include "IntMult/FixMultAdd.hpp"


/* Constant multipliers and dividers ------------------------ */
#include "ConstMult/IntConstMult.hpp"
#include "ConstMult/IntConstMCM.hpp"
#include "ConstMult/IntIntKCM.hpp"
#include "ConstMult/FixRealKCM.hpp"


/* Fixed-point function generators ---------------------*/

#include "FixFunctions/FixFunction.hpp"
#include "FixFunctions/BasicPolyApprox.hpp"
#include "FixFunctions/PiecewisePolyApprox.hpp"
#include "FixFunctions/FixFunctionByTable.hpp"
#include "FixFunctions/FixFunctionBySimplePoly.hpp"
#include "FixFunctions/FixFunctionByPiecewisePoly.hpp"

#include "FixFunctions/GenericTable.hpp"
#include "FixFunctions/BipartiteTable.hpp"
#include "FixFunctions/FixFunctionByMultipartiteTable.hpp"

#include "FixFunctions/E-method/GenericSimpleSelectionFunction.hpp"
#include "FixFunctions/E-method/GenericComputationUnit.hpp"
#include "FixFunctions/E-method/FixEMethodEvaluator.hpp"

// AutoTest
#include "AutoTest/AutoTest.hpp"



/* misc ------------------------------------------------------ */
#include "TestBenches/Wrapper.hpp"
#include "UserDefinedOperator.hpp"


#endif //FLOPOCO_HPP
