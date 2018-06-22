/*
 * Table Generator unit for FloPoCo
 *
 * Author : Mioara Joldes
 *
 * This file is part of the FloPoCo project developed by the Arenaire
 * team at Ecole Normale Superieure de Lyon

 Initial software.
 Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
 2008-2010.
  All rights reserved.

 All rights reserved
*/


// TODO replace all the 165 with some variable

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <math.h>
#include <string.h>
#include <gmp.h>
#include <mpfr.h>
#include <cstdlib>
#include <gmpxx.h>
#include "../utils.hpp"
#include "../Operator.hpp"

#ifdef HAVE_SOLLYA
#include "PolyCoeffTable.hpp"

using namespace std;

namespace flopoco{

  PolyCoeffTable::PolyCoeffTable(Target* target, PiecewiseFunction* pf, int targetAccuracy_, int degree_): 
    Table(target), pwf(pf), degree(degree_), targetAccuracy(targetAccuracy_) {
		
    setCopyrightString("Mioara Joldes (2010), F. de Dinechin(2011-2013)");
    srcFileName = "PolyCoeffTable";

    ostringstream cacheFileName;
    cacheFileName << "Poly_"<<vhdlize(pf->getName()) << "_" << degree << "_" << targetAccuracy << ".cache";

    // Test existence of cache file
    fstream file;
    file.open(cacheFileName.str().c_str(), ios::in);
		
    // check for bogus .cache file
    if(file.is_open() && file.peek() == std::ifstream::traits_type::eof())
      {
	file.close();
	std::remove(cacheFileName.str().c_str());
	file.open(cacheFileName.str().c_str(), ios::in);
      }

    if(!file.is_open()){
      //********************** Do the work, then write the cache ********************* 
      REPORT(INFO, "No polynomial data cache found, creating " << cacheFileName.str());
      file.open(cacheFileName.str().c_str(), ios::out);

      /* Start initialization */
      sollya_lib_set_prec(sollya_lib_constant_from_int(165));
      int nrMaxIntervals=1024*1024;

      int	nrFunctions=(pwf->getPiecewiseFunctionArray()).size();
      int	iter;
      int guardBits =1;
		
			
      mpfr_t a;
      mpfr_t b;
      mpfr_t eps;
      mpfr_init2(a, 165);
      mpfr_init2(b, 165);
      mpfr_init2(eps, 165);
	
      /* Convert parameters to their required type */
      mpfr_set_d(a,0.,GMP_RNDN);
      mpfr_set_d(b,1.,GMP_RNDN);
      mpfr_set_ui(eps, 1, GMP_RNDN);
      mpfr_mul_2si(eps, eps, -targetAccuracy-1-guardBits, GMP_RNDN); /* eps< 2^{-woutX-1} */
	
      vector<sollya_obj_t> polys;
      vector<double> errPolys; // TODO some day: a vector of mpfr instead
	
      sollya_obj_t function=0, zeroSollya, polynomial, nDiff=0, sX=0, f_translated, ai_s, interval, accuracy;
      sollya_obj_t precListS=0;
	
      mpfr_t ai;
      mpfr_t bi;
      mpfr_t zero;
      mpfr_t mpErr;
      mpfr_t dummy;
      double err;
      int nrIntCompleted=-1;
	

      mpfr_init2(ai, 165);
      mpfr_init2(bi, 165);
      mpfr_init2(zero, 165);
      accuracy = sollya_lib_parse_string("1b-10");
      mpfr_init2(mpErr,53);	
      mpfr_init2(dummy,53);	
 
      int k, errBoundBool=0;
      zeroSollya = sollya_lib_parse_string("0"); /*ct part*/
      degreeS = sollya_lib_constant_from_int(degree); /* used to be a monomial list, but simpler this way*/
 
      // initialize the size list so we can use it as an array
      for (k=0;k<=degree;k++)	
	sizeList.push_back(0);

      for (iter=0; iter<nrFunctions;iter++){    // Loop on the functions of the function list
	Function *fi;
	fi=(pwf->getPiecewiseFunctionArray(iter));
			
	REPORT(INFO, "Now approximating "<<fi->getName());
	/*start with one interval; subdivide until the error is satisfied for all functions involved*/

	int nrIntervals = 256;
	int precShift=7;

	//				int nrIntervals = 1;
	//				int precShift=0;

	errBoundBool =0;
		
	/*Convert the input string into a sollya evaluation tree*/ 
	function = sollya_lib_copy_obj(fi->getSollyaNode());
		
	while((errBoundBool==0)&& (nrIntervals <=nrMaxIntervals)){   // Loop increasing the number of intervals
	  errBoundBool=1; 
	  REPORT(DETAILED, "Now trying with "<< nrIntervals <<" intervals");
	  
	  // Compute the precision list for this splitting
	  
	  //	  precList = makePrecList(targetAccuracy+1+guardBits, degree+1, precShift); //precision
	  int precList[degree+1];
	  precList[0] = targetAccuracy+1+guardBits; // accuracy of coeff of degree 0;
	  for (i=1; i< degree+1; i++){
	    precList[i] = precList[i-1]-precshift;
	  }
	  // conversion to a sollya list is long and painful
	  sollya_obj_t  precListStemp[degree+1];
	  for (i=1; i< degree+1; i++){
	    precListStemp[i] = sollya_lib_constant_from_int(precList[i]);
	  }
	  sollya_obj_t numberCoefS = sollya_lib_constant_from_int(degree+1);
	  precListS = sollya_lib_list(precListStemp, numberCoefS);
	  // cleanup immediately or we will never do it
	  sollya_lib_clear(numberCoefS);
	  for (i=1; i< degree+1; i++){
	    sollya_lib_clear(precListStemp[i]);
	  }
	  // TODO clear precListS at the right point

	  for (k=0; k<nrIntervals; k++){
	    // build the subinterval [ai, ai+bi]
	    mpfr_set_ui(ai,k,GMP_RNDN);
	    mpfr_div_ui(ai, ai, nrIntervals, GMP_RNDN);

	    mpfr_set_ui(bi,1,GMP_RNDN);
	    mpfr_div_ui(bi, bi, nrIntervals, GMP_RNDN);		

	    mpfr_set_ui(zero, 0, GMP_RNDN);
			
	    // translate the function: define f(y)= f(x+ai)
	    ai_s = sollya_lib_constant(ai);
	    sX = SOLLYA_ADD(SOLLYA_X_,  ai_s);
	    f_translated = sollya_lib_substitute(function, sX);
	    if (f_translated == 0)
	      THROWERROR("Sollya error when performing range mapping.");
	    interval = sollya_lib_range_from_bounds(zero, bi);
	    if(verbose>=DEBUG){
	      sollya_lib_printf("translated function : %b over %b with prec shift %b \n", f_translated, interval, precShift);
	    }

	    polynomial = sollya_lib_fpminimax(f_translated, degreeS, precListS, NULL, interval, sollya_lib_fixed(), sollya_lib_absolute(), zeroSollya,NULL);
	    polys.push_back(polynomial);
				
	    if (verbose>=DEBUG){
	      sollya_lib_printf("infinite norm: %b\n", polynomial);
	    }

	    /*Compute the error*/
	    sollya_obj_t supnormRange = sollya_lib_supnorm(polynomial, f_translated, interval, sollya_lib_absolute(), accuracy);
	    sollya_lib_get_bounds_from_range(dummy, mpErr, supnormRange);
	    sollya_lib_clear_obj(supnormRange);
	    err = mpfr_get_d(mpErr, GMP_RNDU); 
	    errPolys.push_back(err);
	      

	    if (verbose>=DEBUG){
	      sollya_lib_printf("infinite norm: %v", mpErr);
	    }
	    if (mpfr_cmp(mpErr, eps)>0) {
	      errBoundBool=0; 
	      REPORT(DETAILED, tab << "we have found an interval where the error is not small enough");
	      REPORT(DETAILED, tab << "failed at interval "<< k+1 << "/" << nrIntervals);
	      REPORT(DETAILED, tab << "proceed to splitting"); 
	      /*erase the polys and the errors put so far for this function; keep the previous good ones intact*/
	      polys.resize(nrIntCompleted+1);
	      errPolys.resize(nrIntCompleted+1);
	      nrIntervals=2 * nrIntervals;
	      precShift=precShift+1;
	      break;
	    }
	  }
	}

	sollya_lib_clear_obj(precListS);
	
	if (errBoundBool==1){ 
	  /*we have the good polynomials for one function*/
	  REPORT(DETAILED, "the number of intervals is:"<< nrIntervals); 
	  REPORT(DETAILED, "We proceed to the next function");
	  /*we have to update the total size of the coefficients*/
#if 0
	  int ii=0;
	  while (precList!=NULL){
	    int sizeNew=*((int *)first(precList));
	    precList=tail(precList);
	    if (sizeNew>sizeList[ii] )sizeList[ii]= sizeNew;
	    ii++;
	  }
#endif


	  nrIntCompleted=nrIntCompleted+nrIntervals;	
	  nrIntervalsArray.push_back(intlog2(nrIntervals-1));
	} 
      }
      /*here we have the good polynomials for all the piecewise functions*/
      /*clear some vars used*/

	
      if (errBoundBool==1){
	if(verbose>=DEBUG){
	  cout<< "the total number of intervals is:"<< nrIntCompleted<<endl; 
	  cout<< "We proceed to the extraction of the coefficients:"<<endl; 
	}

	/*Get the maximum error*/
	maxError =errPolys[0];
		
	for (k=1; (unsigned)k<errPolys.size(); k++){
	  if (maxError < errPolys[k])
	    maxError =errPolys[k];
	}
		
	REPORT(DEBUG, "maximum error = " << maxError);



	/*Extract coefficients*/
	vector<FixedPointCoefficient*> fpCoeffVector;
		
	k=0;
	for (k=0;(unsigned)k<polys.size();k++){

#if 0 // lazy to convert it to modern sollya lib
	  if (verbose>=DEBUG){
	    cout<<"\n----"<< k<<"th polynomial:----"<<endl;
	    printTree(polys[k]);
	  }
#endif			
	  fpCoeffVector = getPolynomialCoefficients(polys[k]);
	  polyCoeffVector.push_back(fpCoeffVector);
	}
	/*setting of Table parameters*/
	wIn=intlog2(polys.size()-1);
	minIn=0;
	maxIn=polys.size()-1;
	wOut=0;
	for(k=0; (unsigned)k<coeffParamVector.size();k++){
	  wOut=wOut+(*coeffParamVector[k]).getSize()+(*coeffParamVector[k]).getWeight()+1; /*a +1 is necessary for the sign*/
	}

	/* Setting up the actual array of values */ 
	buildActualTable();

	REPORT(INFO, "Writing the cache file");
	file << "Polynomial data cache for " << cacheFileName.str() << endl;
	file << "Erasing this file is harmless, but do not try to edit it." <<endl; 
	file << wIn  << endl;
	file << wOut << endl;
	// The approx error
	file << maxError << endl;
	// The size of the nrIntervalsArray
	file << nrIntervalsArray.size() << endl;  
	// The values of nrIntervalsArray
	for(k=0; (unsigned)k<nrIntervalsArray.size(); k++) {
	  file << nrIntervalsArray[k] << endl;
	}
	// The size of the actual table
	file << actualTable.size() << endl;  
	// The compact array
	for(k=0; (unsigned)k<actualTable.size(); k++) {
	  mpz_class r = actualTable[k];
	  file << r << endl;
	}
	// The coefficient details
	file << coeffParamVector.size()  << endl;
	for(k=0; (unsigned)k<coeffParamVector.size(); k++) {
	  file << coeffParamVector[k]->getSize() << endl;
	  file << coeffParamVector[k]->getWeight() << endl;
	}


      }else{
	/*if we didn't manage to have the good polynomials for all the piecewise functions*/
	cout<< "PolyCoeffTable error: Could not approximate the given function(s)"<<endl; 
      }
	
      mpfr_clear(ai);
      mpfr_clear(bi);
      mpfr_clear(eps);
      mpfr_clear(zero);
      mpfr_clear(dummy);
      mpfr_clear(mpErr);

      sollya_lib_clear_obj(accuracy);
      sollya_lib_clear_obj(function);
      sollya_lib_clear_obj(zeroSollya);
      sollya_lib_clear_obj(nDiff);
      sollya_lib_clear_obj(sX);
      sollya_lib_clear_obj(degreeS);
    }
    else{
      REPORT(INFO, "Polynomial data cache found: " << cacheFileName.str());
      //********************** Just read the cache ********************* 
      string line;
      int nrIntervals, nrCoeffs;
      getline(file, line); // ignore the first line which is a comment
      getline(file, line); // ignore the second line which is a comment
      file >> wIn; // third line
      file >> wOut; // fourth line
      // The approx error
      file >> maxError;
      // The size of the nrIntervalsArray
      int nrIntervalsArraySize;
      file >> nrIntervalsArraySize;
      // The values of nrIntervalsArray
      for(int k=0; k<nrIntervalsArraySize; k++) {
	int nrInter;
	file >> nrInter;
	nrIntervalsArray.push_back(nrInter);
      }
      // The size of the actual table
      file >> nrIntervals;
      // The compact array
      for (int i=0; i<nrIntervals; i++){
	mpz_class t;
	file >> t;
	actualTable.push_back(t);
      }
      // these are two attributes of Table.
      minIn=0;
      maxIn=actualTable.size()-1;
      // Now in the file: the coefficient details
      file >> nrCoeffs;
      for (int i=0; i<nrCoeffs; i++){
	FixedPointCoefficient *c;
	int size, weight;
	file >> size;
	file >> weight;
	c = new FixedPointCoefficient(size, weight);
	coeffParamVector.push_back(c);
      }

      //generateDebugPwf();
      if (verbose>=INFO){	
	printPolynomialCoefficientsVector();
	REPORT(DETAILED, "Parameters for polynomial evaluator:");
	printCoeffParamVector();
      }

    }
			
    ostringstream name;
    /*Set up the name of the entity */
    name <<"PolyCoeffTable_"<<wIn<<"_"<<wOut;
    setNameWithFreqAndUID(name.str());
		
    /*Set up the IO signals*/
    addInput ("X"	, wIn, true);
    addOutput ("Y"	, wOut, true,1);
		
    /* And that's it: this operator inherits from Table all the VHDL generation */
    nextCycle();//this operator has one cycle delay, in order to be inferred by synthesis tools

    file.close();

  }


  /*This constructor receives the function to be approximated as a string
    Look above to find one that receives the function as a Piecewise function already parsed*/
  PolyCoeffTable::PolyCoeffTable(Target* target, string func,  int targetAccuracy, int degree):
    Table(target) {

    /*parse the string, create a list of functions, create an array of f's, compute an approximation on each interval*/
    PiecewiseFunction *pf=new PiecewiseFunction(func);
    PolyCoeffTable(target, pf,  targetAccuracy, degree);
  }







  PolyCoeffTable::~PolyCoeffTable() {
  }


  double PolyCoeffTable::getMaxApproxError() {
    return maxError;
  };



#if 0 // is this useful?
  MPPolynomial* PolyCoeffTable::getMPPolynomial(sollya_obj_t t){
    int degree=1;
    int i;
    sollya_obj_t *nCoef;
    mpfr_t *coef;
		
    //printTree(t);
    getCoefficients(&degree, &nCoef, t);
    //cout<<degree<<endl;
    coef = (mpfr_t *) safeCalloc(degree+1,sizeof(mpfr_t));
		
			
    for (i = 0; i <= degree; i++){
      mpfr_init2(coef[i], 165);
      //cout<< i<<"th coeff:"<<endl;
      //printTree(getIthCoefficient(t, i));
      evaluateConstantExpression(coef[i], getIthCoefficient(t, i), 165);
      if (verbose>=DEBUG){
	cout<< i<<"th coeff:"<<sPrintBinary(coef[i])<<endl;
      }
    }
    MPPolynomial* mpPx = new MPPolynomial(degree, coef);
    //Cleanup 
    for (i = 0; i <= degree; i++)
      mpfr_clear(coef[i]);
    free(coef);
		
    return mpPx;
  }
#endif



  vector<FixedPointCoefficient*> PolyCoeffTable::getPolynomialCoefficients(sollya_obj_t poly){
    int i,size, weight;
    vector<FixedPointCoefficient*> coeffVector;
    FixedPointCoefficient* zz;
#if 0
    sollya_obj_t *nCoef;
    mpfr_t *coef;
    sollya_obj_list_t cc;
		
    //printTree(t);
    getCoefficients(&degree, &nCoef, t);
    //cout<<degree<<endl;
    coef = (mpfr_t *) safeCalloc(degree+1,sizeof(mpfr_t));
    cc=c;
			
    for (i = 0; i <= degree; i++)
      {
	mpfr_init2(coef[i], 165);

	evaluateConstantExpression(coef[i], getIthCoefficient(t, i), 165);
	if (verbose>=DEBUG){
	  cout<< i<<"th coeff:"<<sPrintBinary(coef[i])<<endl;
	}
	size=*((int *)first(cc));
	cc=tail(cc);
	if (mpfr_sgn(coef[i])==0){
	  weight=0;
	  size=1;
	} 
	else 
	  weight=mpfr_get_exp(coef[i]);
			
	zz= new FixedPointCoefficient(size, weight, coef[i]);
	coeffVector.push_back(zz);
	updateMinWeightParam(i,zz);
      }
			
    //Cleanup 
    for (i = 0; i <= degree; i++)
      mpfr_clear(coef[i]);
    free(coef);
			
#else


    mpfr_t coef;
    
    // get each coeff along with its size
    for (i = 0; i <= degree; i++) {
	mpfr_init2(coef, 165);  // TODO should be the proper size, we have it out there somewhere
	sollya_obj_t iS, coefS;
	iS = sollya_lib_constant_from_int(i);
	coefS = sollya_lib_coeff(poly, iS);
	sollya_lib_get_constant(coef, coefS);
	sollya_lib_clear_obj(iS);
	sollya_lib_clear_obj(coefS);

	REPORT(DEBUG, i<<"th coeff: "<<sPrintBinary(coef)<<endl);

	if (mpfr_sgn(coef)==0){
	  weight=0;
	  size=1;
	} 
	else 
	  weight=mpfr_get_exp(coef);
			
	zz= new FixedPointCoefficient(sizeList[i], weight, coef);
	coeffVector.push_back(zz);
	updateMinWeightParam(i,zz);
      }
    

#endif

    return coeffVector;
  }






  void PolyCoeffTable::updateMinWeightParam(int i, FixedPointCoefficient* zz)
  {
    if (coeffParamVector.size()<=(unsigned)i) {
      coeffParamVector.push_back(zz);
    }
    else if (mpfr_sgn((*(*coeffParamVector[i]).getValueMpfr()))==0) coeffParamVector[i]=zz;
    else if ( ((*coeffParamVector[i]).getWeight() <(*zz).getWeight()) && (mpfr_sgn(*((*zz).getValueMpfr()))!=0) )
      coeffParamVector[i]=zz;
  }


  vector<vector<FixedPointCoefficient*> > PolyCoeffTable::getPolynomialCoefficientsVector(){
    return polyCoeffVector;
  }


  void PolyCoeffTable::printPolynomialCoefficientsVector(){
    int i,j,nrIntervals, degree;
    vector<FixedPointCoefficient*> pcoeffs;
    nrIntervals=polyCoeffVector.size();

    for (i=0; i<nrIntervals; i++){	
      pcoeffs=polyCoeffVector[i];
      degree= pcoeffs.size();
      REPORT(DEBUG, "polynomial "<<i<<": ");
      for (j=0; j<degree; j++){
	REPORT(DEBUG, " "<<(*pcoeffs[j]).getSize()<< " "<<(*pcoeffs[j]).getWeight());
      }
    }
  }


  vector<FixedPointCoefficient*> PolyCoeffTable::getCoeffParamVector(){
    return coeffParamVector;
  }


  void PolyCoeffTable::printCoeffParamVector(){
    int j, degree;
    degree= coeffParamVector.size();
    for (j=0; j<degree; j++){		
      REPORT(DETAILED, " "<<(*coeffParamVector[j]).getSize()<< " "<<(*coeffParamVector[j]).getWeight()); 
    }
  }



#if 0
  void PolyCoeffTable::generateDebug(){
    int j;
    cout<<"f=";
#if 0 // lazy to convert it to modern sollya lib
    printTree(simplifyTreeErrorfree(f->getSollyaNode()));
#endif // lazy to convert it to modern sollya lib
    cout<<" wOut="<<(-1)*targetAccuracy_<<endl;
    cout<<"k="<<polyCoeffVector.size()<<" d="<<coeffParamVector.size()<<endl;
    cout<<"The size of the coefficients is:"<<endl;
    for (j=0; (unsigned)j<coeffParamVector.size(); j++){
      cout<<"c"<<j<<":"<<(*coeffParamVector[j]).getSize()+(*coeffParamVector[j]).getWeight()+1<<endl; 
    }
  }

  vector<int> PolyCoeffTable::getNrIntArray(){
    return nrIntervalsArray;
  }


  void PolyCoeffTable::generateDebugPwf(){
    int j;
    cout<<"pwf=";
    cout<<pwf->getName()<<endl;
    cout<<" wOut="<<(-1)*targetAccuracy_<<endl;
    cout<<"k="<<polyCoeffVector.size()<<" d="<<coeffParamVector.size()<<endl;
    cout<<"The size of the branch is:"<<endl;
    for (j=0; (unsigned)j<getNrIntArray().size(); j++){
      cout<<j<<":"<<(getNrIntArray())[j]<<endl;
    }

    cout<<"The size of the coefficients is:"<<endl;
    for (j=0; (unsigned)j<coeffParamVector.size(); j++){
      cout<<"c"<<j<<":"<<(*coeffParamVector[j]).getSize()+(*coeffParamVector[j]).getWeight()+1<<endl; 
    }
  }

#endif

#if 0 
  // TODO rewrite from scratch on sounder bases
  // n is probably the degree of the polynomial + 1
  // m is some precision
  sollya_obj_t PolyCoeffTable::makePrecList(int m, int n, int precshift) {
    int i;
    sollya_obj_t c;
    int tempTable[n];
    int tempTableS[n];

    // first build the int table
    tempTable[0]=m;
    for (i=1; i<n; i++){
      tempTable[i] = tempTable[i-1]-precshift;
    }

    tempTable[3]+=1; // TODO ?????????????!????????????!!! This looks like an ugly patch
    // either remove it, or add 1 to all the entries


    // Then copy it into an array of sollya_obj_t
	
    for (i=0; i<n; i++){
      tempTableS[i] = sollya_lib_constant_from_int(tempTable[i]);
    }
    // Then make a list of it
    c = sollya_lib_list(tempTableS, degreeS)
  return c;
  }


#endif 



  /****************************************************************************************/
  /************Implementation of virtual methods of Class Table***************************/
  int PolyCoeffTable::double2input(double x){
    int result;
    cerr << "???	PolyCoeffTable::double2input not yet implemented ";
    exit(1);
    return result;
  }


  double	PolyCoeffTable::input2double(int x) {
    double y;
    cerr << "??? PolyCoeffTable::double2input not yet implemented ";
    exit(1);
    return(y);
  }


  mpz_class	PolyCoeffTable::double2output(double x){
    cerr << "???	PolyCoeffTable::double2input not yet implemented ";
    exit(1);
    return 0;
  }


  double	PolyCoeffTable::output2double(mpz_class x) {
    double y;
    cerr << "???	PolyCoeffTable::double2input not yet implemented ";
    exit(1);
	
    return(y);
  }


  void PolyCoeffTable::buildActualTable() {
    unsigned int x;
    mpz_class r=0; mpfr_t *cf; mpz_t c; char * z;
    int amount,j,nrIntervals, degree,trailingZeros,numberSize;
    vector<FixedPointCoefficient*> pcoeffs;
    nrIntervals=polyCoeffVector.size();
    for(x=0; x<(unsigned)nrIntervals; x++){
      r=0;
      pcoeffs=polyCoeffVector[(unsigned)x];
      degree= pcoeffs.size();
      amount=0;
      //cout<<"polynomial "<<x<<": "<<endl;
      //r=mpz_class( 133955332 )+(mpz_class( 65664 )<< 27 )+(mpz_class( 64 )<< 44 )
      for (j=0; j<degree; j++){		
	//cout<<" "<<(*pcoeffs[j]).getSize()<< " "<<(*pcoeffs[j]).getWeight()<<endl; 
	r=r+(mpz_class(0)<<amount);
				
				
	cf=(*pcoeffs[j]).getValueMpfr();
				
	//cout<< j<<"th coeff:"<<sPrintBinary(*cf)<<endl;
	z=sPrintBinaryZ(*cf);
	//cout<< j<<"th coeff:"<<z<<" "<<strlen(z)<<endl;
	mpz_init(c);
	if (mpfr_sgn(*cf)!=0) {
					
	  trailingZeros=(*coeffParamVector[j]).getSize()+(*pcoeffs[j]).getWeight()-strlen(z);
	  //cout<<"Trailing zeros="<< trailingZeros<<endl;
	  numberSize= (*coeffParamVector[j]).getSize()+(*coeffParamVector[j]).getWeight()+1 ;
	  //mpz_set_str (mpz_t rop, char *str, int base) 
	  mpz_set_str (c,z,2);
	  if (mpfr_sgn(*cf)<0) {
	    if (j==0)
	      r=r+(((mpz_class(1)<<numberSize) -	((mpz_class(c)<<trailingZeros) - (mpz_class(1)<<2) ) )<<amount);
	    else
	      r=r+(((mpz_class(1)<<numberSize) -	(mpz_class(c)<<trailingZeros)	)<<amount);
	  }
	  else{
	    if (j==0)
	      r=r+(((mpz_class(c)<<trailingZeros) + (mpz_class(1)<<2) )<<amount);
	    else
	      r=r+((mpz_class(c)<<trailingZeros)<<amount);							
	  }
					
	}
	else{
	  if (j==0)
	    r=r+(((mpz_class(0)) + (mpz_class(1)<<2))<<amount);
	  else
	    r=r+(mpz_class(0)<<amount);
	} 
				
	amount=amount+(*coeffParamVector[j]).getSize()+(*coeffParamVector[j]).getWeight()+1;
      }
      //cout << x << "  " << r << endl;
      actualTable.push_back(r);
    }
  }

  /***************************************************************************************/
  /********************This is the implementation of the actual mapping*******************/
  mpz_class	PolyCoeffTable::function(int x)
  {  
    // if ((x<0) ||(x>=nrIntervals)) {
    // 	//TODO an error here
    // }
    return actualTable[x];
  }
  /***************************************************************************************/


#if 0
  // The following is cut from UtilSollya, to be removed in next cleanup

  sollya_obj_list_t makeIntPtrChainToFromBy(int m, int n, int k) {
    int i,j;
    int *elem;
    sollya_obj_list_t c;
  
    c = NULL;
    i=m-(n-1)*k;
    for(j=0;j<n;j++) {
      elem = (int *) malloc(sizeof(int));
      *elem = i;
      c = addElement(c,elem);
      i=i+k;
    }

    return c;
  }

  sollya_obj_list_t makeIntPtrChainFromArray(int m, int *a) {
    int j;
    int *elem;
    sollya_obj_list_t c;
  
    c = NULL;
  
    for(j=0;j<m;j++) {
      elem = (int *) malloc(sizeof(int));
      *elem = a[j];
      c = addElement(c,elem);
   
    }

    return c;
  }

#endif



  void removeTrailingZeros(char *outbuf, char *inbuf) {
    char *temp, *temp2, *temp3;

    temp = inbuf; temp2 = outbuf; temp3 = outbuf;
    while ((temp != NULL) && (*temp != '\0')) {
      *temp2 = *temp;
      if (*temp2 != '0') {
	temp3 = temp2;
      }
      temp2++;
      temp++;
    }
    temp3++;
    *temp3 = '\0';
  }



  // TODO get rid of this and use mpfr functions instead
  char *sPrintBinary(mpfr_t x) {
    mpfr_t xx;
    int negative;
    mp_prec_t prec;
    mp_exp_t expo;
    char *raw, *formatted, *temp1, *temp2, *str3;
    char *temp3=NULL;
    char *resultStr;
    int len;

    prec = mpfr_get_prec(x);
    mpfr_init2(xx,prec);
    mpfr_abs(xx,x,GMP_RNDN);
    negative = 0;
    if (mpfr_sgn(x) < 0) negative = 1;
    raw = mpfr_get_str(NULL,&expo,2,0,xx,GMP_RNDN);
    if (raw == NULL) {
      printf("Error: unable to get a string for the given number.\n");
      return NULL;
    } else {
      formatted =(char *) safeCalloc(strlen(raw) + 3, sizeof(char));
      temp1 = raw; temp2 = formatted;
      if (negative) {
	*temp2 = '-';
	temp2++;
      }
      *temp2 = *temp1; temp2++; temp1++;
      if (*temp1 != '\0') { 
	*temp2 = '.'; 
	temp2++;
      }
      while (*temp1 != '\0') {
	*temp2 = *temp1;
	temp2++; temp1++;
      }
      str3 = (char *) safeCalloc(strlen(formatted)+2,sizeof(char));
      removeTrailingZeros(str3,formatted);
      len = strlen(str3) - 1;
      if (str3[len] == '.') {
	str3[len] = '\0';
      }
      if (!mpfr_zero_p(x)) {
	if (mpfr_number_p(x)) {
	  temp3 = (char *) safeCalloc(strlen(str3)+74,sizeof(char));
	  if ((((int) expo)-1) != 0) 
	    sprintf(temp3,"%s_2 * 2^(%d)",str3,((int)expo)-1);  
	  else
	    sprintf(temp3,"%s_2",str3);  
	} else {
	  temp3 = (char *) safeCalloc(strlen(raw) + 2,sizeof(char));
	  if (negative) 
	    sprintf(temp3,"-%s",raw); 
	  else 
	    sprintf(temp3,"%s",raw); 
	}
      }
      else {
	temp3 = (char *) safeCalloc(2,sizeof(char));
	sprintf(temp3,"0");
      }
      free(formatted);
      free(str3);
    }
    mpfr_free_str(raw);  
    mpfr_clear(xx);
    resultStr = (char *) safeCalloc(strlen(temp3) + 1,sizeof(char));
    sprintf(resultStr,"%s",temp3);
    free(temp3);
    return resultStr;
  }


  char *sPrintBinaryZ(mpfr_t x) {
    mpfr_t xx;
    int negative;
    mp_prec_t prec;
    mp_exp_t expo;
    char *raw, *formatted, *temp1, *temp2, *str3;
    char *temp3=NULL;
    char *resultStr;
    int len;

    prec = mpfr_get_prec(x);
    mpfr_init2(xx,prec);
    mpfr_abs(xx,x,GMP_RNDN);
    negative = 0;
    if (mpfr_sgn(x) < 0) negative = 1;
    raw = mpfr_get_str(NULL,&expo,2,0,xx,GMP_RNDN);
    if (raw == NULL) {
      printf("Error: unable to get a string for the given number.\n");
      return NULL;
    } else {
      formatted =(char *) safeCalloc(strlen(raw) + 3, sizeof(char));
      temp1 = raw; temp2 = formatted;
      /*if (negative) {
       *temp2 = '-';
       temp2++;
       }*/
      *temp2 = *temp1; temp2++; temp1++;
      /*if (*temp1 != '\0') { 
       *temp2 = '.'; 
       temp2++;
       }*/
      while (*temp1 != '\0') {
	*temp2 = *temp1;
	temp2++; temp1++;
      }
      str3 = (char *) safeCalloc(strlen(formatted)+2,sizeof(char));
      removeTrailingZeros(str3,formatted);
      len = strlen(str3) - 1;
      if (str3[len] == '.') {
	str3[len] = '\0';
      }
      if (!mpfr_zero_p(x)) {
	if (mpfr_number_p(x)) {
	  temp3 = (char *) safeCalloc(strlen(str3)+74,sizeof(char));
	  if ((((int) expo)-1) != 0) 
	    //sprintf(temp3,"%s_2 * 2^(%d)",str3,((int)expo)-1);  
	    sprintf(temp3,"%s",str3);  
	  else
	    sprintf(temp3,"%s",str3);  
	} else {
	  temp3 = (char *) safeCalloc(strlen(raw) + 2,sizeof(char));
	  if (negative) 
	    // sprintf(temp3,"-%s",raw); 
	    sprintf(temp3,"%s",raw); 
	  else 
	    sprintf(temp3,"%s",raw); 
	}
      }
      else {
	temp3 = (char *) safeCalloc(2,sizeof(char));
	sprintf(temp3,"0");
      }
      free(formatted);
      free(str3);
    }
    mpfr_free_str(raw);  
    mpfr_clear(xx);
    resultStr = (char *) safeCalloc(strlen(temp3) + 1,sizeof(char));
    sprintf(resultStr,"%s",temp3);
    free(temp3);
    return resultStr;
  }


}
#endif //HAVE_SOLLYA


