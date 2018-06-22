#include <iostream>
#include <sstream>
#include "FixConstant.hpp"
#include "utils.hpp"

using namespace std;

namespace flopoco{
#define THROWERROR(stream) {{ostringstream o; o << " ERROR in FixConstant: " << stream << endl; throw o.str();}}

	FixConstant::FixConstant(const int MSB_, const int LSB_, const bool isSigned_, const mpfr_t val_) :
		isSigned(isSigned_), MSB(MSB_), LSB(LSB_), width(MSB_-LSB_+1) {
		mpfr_init2(fpValue, width);
		mpfr_set(fpValue, val_, GMP_RNDN); // TODO check no error?
		isZeroP = mpfr_zero_p(val_);
		if( (! isSigned) && (mpfr_sgn(fpValue)<0) )
			THROWERROR("In FixConstant constructor: Negative constant " << printMPFR(fpValue) << " cannot be represented as an unsigned constant");
	}


	FixConstant::FixConstant(const int MSB_, int LSB_, const bool isSigned_, const mpz_class zval):
		isSigned(isSigned_), MSB(MSB_), LSB(LSB_), width(MSB_-LSB_+1)
	{
		// cout <<  "Entering FixConstant "<< zval  <<endl;
		mpfr_init2(fpValue, width);
		mpfr_set_z(fpValue, zval.get_mpz_t(), GMP_RNDN); // TODO check no error?
		mpfr_mul_2si(fpValue, fpValue, LSB, GMP_RNDN); // exact
		isZeroP = mpfr_zero_p(fpValue);
		if( (! isSigned) && (mpfr_sgn(fpValue)<0) )
			THROWERROR("In FixConstant constructor: Negative constant " << printMPFR(fpValue) << " cannot be represented as an unsigned constant");
		// cout <<  "Exiting FixConstant "<< zval <<endl;
	}


#if 0 // Warning the following code is unfinished and untested
	FixConstant::FixConstant(const bool signed_, const mpfr_t val_) :
		isSigned(signed_) {

		bool sign;
		mpz_class zval;
		LSB =  mpfr_get_z_exp (zval.get_mpz_t(),val_);

		// conversion to (sign, absval)
		if(isSigned) {
			if(zval<0) {
				sign=true;
				zval=-zval;
			}
			else
				sign=false;
		}
		else {// unsigned
			if(zval<0) {
				throw("Signed input to FixConstant(unsigned)");
			}
		}

		// Normalisation?
		cout << "Normalisation";
		while(zval & mpz_class(1) == 0) {
			cout << ".";
			zval = zval>>1;
			LSB++;
		}
		MSB=0;
		while(zval!=0) {
			cout << "*";
			zval = zval>>1;
			MSB++;
		}

		if(isSigned)
			MSB++;

		mpfr_init2(fpValue, width);
		mpfr_set(fpValue, val_, GMP_RNDN); // TODO check no error?
	}
#endif



	FixConstant::~FixConstant(){
		mpfr_clear(fpValue);
	}

	mpz_class FixConstant::getBitVectorAsMPZ() {
		mpz_class h;
		mpfr_t x;
		mpfr_init2(x, mpfr_get_prec(fpValue));
		mpfr_set(x,fpValue,GMP_RNDN);
		mpfr_mul_2si(x, x, -LSB, GMP_RNDN); // exact
		mpfr_get_z(h.get_mpz_t(), x,  GMP_RNDN); // rounding could take place here, but should not
		if(h<0){
			if(isSigned) {
				h += (mpz_class(1)) << (width);
			}
			else {
				THROWERROR("In getBitVectorAsMPZ, negative constant " << printMPFR(fpValue) << " cannot be represented as a signed constant");
			}
		}
		mpfr_clear(x);
		return h;
	}

	std::string FixConstant::getBitVector(int margins){
		//		cout <<  "in FixConstant::getBitVector, fpValue=" << printMPFR(fpValue) << endl;
		// use the function in utils.hpp
		if(isSigned)
			return signedFixPointNumber(fpValue, MSB, LSB);
		else
			return unsignedFixPointNumber(fpValue, MSB, LSB);
	}

	bool FixConstant::isZero(){
		//		return mpfr_zero_p(fpValue);
		return isZeroP;
	}

	void FixConstant::changeMSB(int newMSB){
		if(newMSB<MSB){
			// TODO: check that the number fits its new size, and bork otherwise.
			THROWERROR("FixConstant::changeMSB: old MSB is "<<MSB<< " newMSB="<<newMSB<< " ... hence TODO");
		}
		MSB=newMSB;
		width = (MSB-LSB+1);
	}

	void FixConstant::changeLSB(int newLSB){
		if(newLSB>LSB){
			// TODO: check that the number fits its new size, and bork otherwise.
			throw(string("FixConstant::changeLSB: TODO: The case when the size is reduced is not yet implemented (rounding to do, etc)"));
		}

		LSB = newLSB;
		width = MSB - LSB +1;
	}

	void  FixConstant::addRoundBit(int weight){
		if(isZeroP) {
			isZeroP=false;
			mpfr_init2(fpValue,2);
			mpfr_set_ui_2exp(fpValue, 1, weight, GMP_RNDN); //exact
			MSB=weight+1;
			LSB=weight;
			width=2;
		}
		else {
			if(weight<LSB) {
#if 0				
				ostringstream e;
				e << "in FixConstant::addRoundBit, weight of the round bit is "<< weight << ", lower than LSB=" << LSB;
				throw e.str();
#else
				LSB=weight;
				width=MSB-LSB+1;
				// no need to change the fpvalue
#endif
			}
			mpfr_t b,s;
			mpfr_init2(b,16);
			mpfr_init2(s,width+1);
			mpfr_set_ui_2exp(b, 1, weight, GMP_RNDN); //exact
			mpfr_add(s, fpValue, b, GMP_RNDN);
			if(mpfr_get_exp(s) != mpfr_get_exp(fpValue)) {
			//cerr << "FixConstant::addRoundBit has increased MSB";
				MSB++;
				width++;
				mpfr_set_prec(fpValue, width);
			}
			// just in case we did  get a zero
			isZeroP = mpfr_zero_p(fpValue);
			mpfr_set(fpValue, s,  GMP_RNDN); //exact
			mpfr_clears(s,b, NULL);
		}
	}

	std::string FixConstant::report(){
		ostringstream s;
		s << "(MSB=" << MSB << ", LSB=" << LSB << ")   "<< printMPFR(fpValue);
		return s.str();
	}


}
