/*
 * Utility for converting a FP number into its binary representation,
 * for testing etc
 *
 * Author : Florent de Dinechin
 *
 * This file is part of the FloPoCo project developed by the Arenaire
 * team at Ecole Normale Superieure de Lyon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
*/

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <gmpxx.h>
#include <mpfr.h>
#include <cstdlib>

#include"../utils.hpp"
using namespace std;
using namespace flopoco;


static void usage(char *name){
  cerr << endl << "Usage: "<<name<<" wE wF x" << endl ;
  cerr << "  x is a binary string of (wE+wF+1) bits, in the IEEE-754 format:" << endl ;
  cerr << "    <sign><exponent><significand>" << endl ;
  cerr << "    sign:  0: positive, 1: negative" << endl ;
  cerr << "    exponent is on wE bits, biased such that 0 is coded by 011...11" << endl ;
  cerr << "    significand is on wF bits and codes 1.significand (implicit leading one)" << endl ;
  exit (EXIT_FAILURE);
}

int check_strictly_positive(char* s, char* cmd) {
  int n=atoi(s);
  if (n<=0){
    cerr<<"ERROR: got "<<s<<", expected strictly positive number."<<endl;
    usage(cmd);
  }
  return n;
}

int check_special_cases(char* n, int lengthWE, int lengthWF) {
  char* pt=n+1;
  bool isZero=true;
  bool isSubNormal=true;
  bool isInfinity=true;
  bool isNaN=true;

  for(int i=0; i<lengthWE && (isInfinity || isZero || isNaN || isSubNormal); i++){
    if(*pt=='1'){
      isZero=false;
      isSubNormal=false;
    }else{
      isInfinity=false;
      isNaN=false;
    }
    pt++;
  }
  
  if(isZero || isSubNormal){
    for(int i=0; i<lengthWF && isZero && isSubNormal; i++){
      if(*pt=='1'){
        isZero=false;
        break;
      }
      pt++;
    }
  }

  if(isInfinity || isNaN){
    for(int i=0; i<lengthWF && isInfinity && isNaN; i++){
      if(*pt=='1'){
        isInfinity=false;
        break;
      }
      pt++;
    }
  }

  if(isZero)
    return 1;
  else if(isSubNormal)
    return 2;
  else if(isInfinity)
    return 3;
  else if(isNaN)
    return 4;

  return 0;
}

int main(int argc, char* argv[] )
{
  if(argc != 4) usage(argv[0]);
  int wE = check_strictly_positive(argv[1], argv[0]);
  int wF = check_strictly_positive(argv[2], argv[0]);
  char* x = argv[3];

  char *p=x;
  int l=0;
  while (*p){
    if(*p!='0' && *p!='1') {
      cerr<<"ERROR: expecting a binary string, got "<<argv[3]<<endl;
     usage(argv[0]);
    }
    p++; l++;
  }
  if(l != wE+wF+1) {
    cerr<<"ERROR: binary string of size "<< l <<", should be of size "<<wE+wF+1<<endl;
    usage(argv[0]);
  }


  //check special case
  switch(check_special_cases(x, wE, wF)){    
    case 0: // Normal case
      {
      // significand
      mpfr_t sig, one, two;
      mpfr_init2(one, 2);
      mpfr_set_d(one, 1.0, GMP_RNDN);
      mpfr_init2(two, 2);
      mpfr_set_d(two, 2.0, GMP_RNDN);

      mpfr_init2(sig, wF+1);
      mpfr_set_d(sig, 1.0, GMP_RNDN); // this will be the implicit one

      p=x+1+wE;//----------------------------------------------------------------------------------//
      for (int i=0; i< wF; i++) {
        mpfr_mul(sig, sig, two, GMP_RNDN);
        if (*p=='1') {
          mpfr_add(sig, sig, one, GMP_RNDN);
        }
        p++;
      }

      // set sign
      if(x[0]=='1')
        mpfr_neg(sig, sig, GMP_RNDN);

      // bring back between 1 and 2: multiply by 2^-wF
      mpfr_mul_2si (sig, sig, -wF, GMP_RNDN);

      // exponent
      int exp=0;
      p=x+1;
      for (int i=0; i< wE; i++) {
        exp = exp<<1;
        if (*p=='1') {
          exp+=1;
        }
        p++;
      }
      // subtract bias
      exp -= ((1<<(wE-1)) -1);

      // scale sig according to exp
      mpfr_mul_2si (sig, sig, exp, GMP_RNDN);

      // output on enough bits
      mpfr_out_str (0, // std out
        10, // base
        0, // enough digits so that number may be read back
        sig, 
        GMP_RNDN);

      mpfr_clear(one);
      mpfr_clear(two);
      mpfr_clear(sig);
    }
    break;
    
    case 1: // Zero case
      if(x[0]=='1')
        cout<<"-0.0";
      else
        cout<<"+0.0";
    break;
    
    case 2: // Subnormal case
    {
      mpfr_t sig, one, two;
      mpfr_init2(one, 2);
      mpfr_set_d(one, 1.0, GMP_RNDN);
      mpfr_init2(two, 2);
      mpfr_set_d(two, 2.0, GMP_RNDN);

      mpfr_init2(sig, wF+1);
      mpfr_set_d(sig, 0.0, GMP_RNDN); // this will be the implicit zero

      p=x+1+wE;//----------------------------------------------------------------------------------//
      for (int i=0; i< wF; i++) {
        mpfr_mul(sig, sig, two, GMP_RNDN);
        if (*p=='1') {
          mpfr_add(sig, sig, one, GMP_RNDN);
        }
        p++;
      }

      // set sign
      if(x[0]=='1')
        mpfr_neg(sig, sig, GMP_RNDN);

      // bring back between 1 and 2: multiply by 2^-wF
      mpfr_mul_2si (sig, sig, -(wF-1), GMP_RNDN);

      // exponent
      int exp=0;
      p=x+1;
      for (int i=0; i< wE; i++) {
        exp = exp<<1;
        if (*p=='1') {
          exp+=1;
        }
        p++;
      }
      // subtract bias
      exp -= ((1<<(wE-1)) -1);

      // scale sig according to exp
      mpfr_mul_2si (sig, sig, exp, GMP_RNDN);

      // output on enough bits
      mpfr_out_str (0, // std out
        10, // base
        0, // enough digits so that number may be read back
        sig, 
        GMP_RNDN);

      mpfr_clear(one);
      mpfr_clear(two);
      mpfr_clear(sig);
    }
    break;
    
    case 3: // Infinity case
      if(x[0]=='1')
        cout<<"-Infinity";
      else
        cout<<"+Infinity";
    break;

    case 4: // NaN case
        cout<<"NaN";
    break;

  }

  cout << endl;
  
  return 0;
}
