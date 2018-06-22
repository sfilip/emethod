/*

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Author : Florent de Dinechin, Florent.de-Dinechin@insa-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL, INSA,
  2008-2014.
  All rights reserved.

*/
#ifndef __FIXHORNEREVALUATOR_HPP
#define __FIXHORNEREVALUATOR_HPP
#include <vector>
#include <sstream>

#include "Operator.hpp"
#include "Table.hpp"
#include "DualTable.hpp"
#include "IntMult/FixMultAdd.hpp"


namespace flopoco{

	/**
	 * An Horner polynomial evaluator computing just right.
	 * It assumes the input X is an signed number in [-1, 1[ so msbX=-wX.
	*/

  class FixHornerEvaluator : public Operator
  {
  public:
    /**
     * The constructor with manual control of all options.
     * @param   lsbIn input lsb weight,
     * @param   msbOut  output MSB weight, used to determine wOut
     * @param   lsbOut  output LSB weight
     * @param   degree  degree of the polynomial
     * @param   msbCoeff vector (of size degree+1) holding the MSB of the polynomial coefficients
     * @param   lsbCoeff holding the LSB of the polynomial coefficients
     * @param   roundingErrorBudget The rounding error budget, excluding final rounding.
     * 				If -1, will be set to 2^(lsbOut-2)
     * @param   signedXandCoeffs  true if the coefficients are signed numbers (usually true)
     * @param   finalRounding: if false, the operator outputs its guard bits as well, saving the half-ulp rounding error.
			    	This makes sense in situations that further process the result with further guard bits.

     */
	FixHornerEvaluator(Target* target,
			  int lsbIn,
			  int msbOut,
			  int lsbOut,
			  int degree,
			  vector<int> msbCoeff,
			  int lsbCoeff,
			  double roundingErrorBudget=-1,
			  bool signedXandCoeffs=true,
			  bool finalRounding=true,
			  map<string, double> inputDelays = emptyDelayMap);


	/**
	 * An optimized constructor if the caller has been able to compute the signs and MSBs of the sigma terms
	 */
	FixHornerEvaluator(Target* target,
			  int lsbIn,
			  int msbOut,
			  int lsbOut,
			  int degree,
			  vector<int> msbCoeff,
			  int lsbCoeff,
			  vector<int> sigmaSign, vector<int> sigmaMSB,
			  double roundingErrorBudget=-1,
			  bool signedXandCoeffs=true,
			  bool finalRounding=true,
			  map<string, double> inputDelays = emptyDelayMap);

    ~FixHornerEvaluator();


  private:
    int degree;                       /**< degree of the polynomial */
    int lsbIn;                        /**< LSB of input. Input is assumed in [0,1], so unsigned and MSB=-1 */
    int msbOut;                       /**< MSB of output  */
    int lsbOut;                       /**< LSB of output */
    vector<int> msbCoeff;             /**< vector of MSB weights for each coefficient */
    int lsbCoeff;                     /**< LSB weight shared by each coefficient */
    double roundingErrorBudget;
    bool signedXandCoeffs;            /**< if false, all the coeffs are unsigned and the operator may use unsigned arithmetic.
											Usually true unless known Taylor etc */
    bool finalRounding;               /**< If true, the operator returns a rounded result (i.e. add the half-ulp then truncate)
											If false, the operator returns the full, unrounded results including guard bits */
    vector<int> coeffSize;            /**< vector of the sizes of the coefficients, computed out of MSB and LSB.
    										See FixConstant.hpp for the constant format */

    // internal architectural parameters; max degree = 1000 should be enough for anybody
    vector <int> signSigma;
    vector<int> msbSigma;
    vector <int>  msbP;
    vector <int> lsbSigma;
    vector <int> lsbP;
    vector <int> lsbXTrunc;

    void computeLSBs();               /**< error analysis that ensures the rounding budget is met */
    void initialize();                /**< initialization factored out between various constructors */
    void generateVHDL();              /**< generation of the VHDL once all the parameters have been computed */

  };

}
#endif
