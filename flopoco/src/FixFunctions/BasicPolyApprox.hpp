#ifndef _BASICPOLYAPPROX_HPP_
#define _BASICPOLYAPPROX_HPP_

#include <string>
#include <iostream>

#include <sollya.h>
#include <gmpxx.h>

#include "../Operator.hpp" // mostly for reporting
#include "../UserInterface.hpp"
#include "FixFunction.hpp"
#include "FixConstant.hpp"

using namespace std;

/* Stylistic convention here: all the sollya_obj_t have names that end with a capital S */
namespace flopoco{

	/** The BasicPolyApprox object builds and maintains a machine-efficient polynomial approximation to a fixed-point function over [0,1]
			Fixed point, hence only absolute errors/accuracy targets.

			There are two families of constructors:
			one that inputs target accuracy and computes degree and approximation error (calling the buildApproxFromTargetAccuracy method)
			one that inputs degree and computes approximation error (calling the buildApproxFromDegreeAndLSBs method).

			The first is useful in standalone, or to evaluate degree/MSBs etc.
			The second is needed in the typical case of a domain split, where the degree is determined when determining the split.

			Sketch of the algo for  buildApproxFromTargetAccuracy:
		  	guessDegree gives a tentative degree.
			target_accuracy defines the best-case LSB of the constant part of the polynomial.
			if  addGuardBitsToConstant, we add g=ceil(log2(degree+1)) bits to the LSB of the constant:
			this provides a bit of freedom to fpminimax, for free in terms of evaluation.
			And we call fpminimax with that, and check what it provided.
			Since x is in [0,1], the MSB of coefficient i is then exactly the MSB of a_i.x^i
			For the same reason, all the coefficients should have the same target LSB

			No domain splitting or other range reduction here: these should be managed in different classes
			No good handling of functions with zero coefficients for now.
			- a function with zero constant should be transformed into a "proper" one outside this class.
			   Example: sin, log(1+x)
		  	- a function with odd or even Taylor should be transformed as per the Muller book.

			To implement a generic approximator we will need to lift these restrictions, but it is unclear that it needs to be in this class.
			A short term TODO is to detect such cases.
	*/

	class BasicPolyApprox {
	public:


		/** A minimal constructor inputting target accuracy.
				@param f: defines the function and if the input interval is [0,1] or [-1,1]
				@param addGuardBits:
				if >=0, add this number of bits to the LSB of each coeff
				if -1, add to each coeff a number of LSB bits that corresponds to the bits needed for a faithful Horner evaluation based on faithful (truncated) multipliers
		 */
		BasicPolyApprox(FixFunction* f, double targetAccuracy, int addGuardBits=-1);

		/** A minimal constructor that inputs a sollya_obj_t function, inputting target accuracy
				@param addGuardBits:
				if >=0, add this number of bits to the LSB of each coeff
				if -1, add to each coeff a number of LSB bits that corresponds to the bits needed for a faithful Horner evaluation based on faithful (truncated) multipliers
				@param signedInput:  if true, we consider an approximation on [-1,1]. If false, it will be on [0,1]
		 */
		BasicPolyApprox(sollya_obj_t fS, double targetAccuracy, int addGuardBits=-1, bool signedInput =false);

		/** A minimal constructor that parses a sollya string, inputting target accuracy
				@param addGuardBits:
				if >=0, add this number of bits to the LSB of each coeff
				if -1, add to each coeff a number of LSB bits that corresponds to the bits needed for a faithful Horner evaluation based on faithful (truncated) multipliers
				@param signedInput:  if true, we consider an approximation on [-1,1]. If false, it will be on [0,1]
		 */
		BasicPolyApprox(string sollyaString, double targetAccuracy, int addGuardBits=-1, bool signedInput=false);


		/** A minimal constructor that inputs a sollya_obj_t function, a degree and the weight of the LSBs.
				This one is mostly for "internal" use by classes that compute degree separately, e.g. PiecewisePolyApprox
				@param signedx:  if true, we consider an approximation on [-1,1]. If false, it will be on [0,1]

		 */
		BasicPolyApprox(sollya_obj_t fS, int degree, int LSB, bool signedInput =false);


		/** A constructor for the case where you already have the coefficients, e.g. you read them from a file. Beware, f is un-initialized in this case
		 */
		BasicPolyApprox(int degree, vector<int> MSB, int LSB, vector<mpz_class> coeff);


		virtual ~BasicPolyApprox();

		vector<FixConstant*> coeff;       /**< polynomial coefficients in a hardware-ready form */
		int degree;                       /**< degree of the polynomial approximation */
		double approxErrorBound;          /**< guaranteed upper bound on the approx error of the approximation provided. Should be smaller than targetAccuracy */
		void buildApproxFromDegreeAndLSBs(); /**< build an approximation of a certain degree, LSB being already defined, then computes the approx error.
																						Essentially a wrapper for Sollya fpminimax() followed by supnorm()*/
		int LSB;                          /**< weight of the LSB of the polynomial approximation. Also weight of the LSB of each constant, since x \in [0,1) */

		/** A wrapper for Sollya guessdegree */
		static	void guessDegree(sollya_obj_t fS, sollya_obj_t rangeS, double targetAccuracy, int* degreeInfP, int* degreeSupP);

		static OperatorPtr parseArguments(Target *target, vector<string> &args);

		static void registerFactory();


		FixFunction *f;                   /**< The function to be approximated */

	private:
		sollya_obj_t polynomialS;         /**< The polynomial approximating it */


		string srcFileName;   /**< useful only to enable same kind of reporting as for FloPoCo operators. */
		string uniqueName_;   /**< useful only to enable same kind of reporting as for FloPoCo operators. */
		bool needToFreeF;     /**< in an ideal world, this should not exist */
		void initialize();    /**< initialization of various constant objects for Sollya */
		void buildApproxFromTargetAccuracy(double targetAccuracy, int addGuardBitsToConstant); /**< constructor code for the general case factored out. */
		void buildFixFormatVector(); /**< Build coeff, the vector of coefficients, out of polynomialS, the sollya polynomial. Constructor code, factored out */


		sollya_obj_t fixedS;        /**< a constant sollya_obj_t */
		sollya_obj_t absoluteS;     /**< a constant sollya_obj_t */

	};

}
#endif // _POLYAPPROX_HH_


// Garbage below
