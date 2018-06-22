/**
  FloPoCo Stream for VHDL code including cycle information

  This file is part of the FloPoCo project developed by the Arenaire
  team at Ecole Normale Superieure de Lyon
  
  Authors :   Bogdan Pasca, Nicolas Brunie

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved. */


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
#include "utils.hpp"
#include "FlopocoStream.hpp"


using namespace std;

namespace flopoco{

	/** The FlopocoStream class.  */
	FlopocoStream::FlopocoStream(){
		vhdlCode.str("");
		vhdlCodeBuffer.str("");
		currentCycle_ = 0;
	}


	FlopocoStream::~FlopocoStream(){
	}

	FlopocoStream & operator<<(FlopocoStream& output, FlopocoStream fs) {
		output.vhdlCodeBuffer << fs.str();
		return output; 
	}
	
	FlopocoStream& operator<<( FlopocoStream& output, UNUSED(ostream& (*f)(ostream& fs)) ){
		output.vhdlCodeBuffer << std::endl;
		return output;
	}
	
	string FlopocoStream::str(){
		flush(currentCycle_);
		return vhdlCode.str();
	}

	string FlopocoStream::str(string UNUSED(s) ){
		vhdlCode.str("");
		vhdlCodeBuffer.str("");
		return "";
	}

	void FlopocoStream::flush(int currentCycle){
		if (! disabledParsing ){
			ostringstream bufferCode;
			if ( vhdlCodeBuffer.str() != string("") ){
				/* do processing if buffer is not empty */

				/* scan buffer sequence and annotate ids */
				bufferCode << annotateIDs( currentCycle );

				/* the newly processed code is appended to the existing one */
				vhdlCode << bufferCode.str();

			}
		}else{
			vhdlCode << vhdlCodeBuffer.str();				
		}
		/* reset buffer */
		vhdlCodeBuffer.str(""); 
	}

	void FlopocoStream::flush(){
		flush ( currentCycle_ );
	}

	string FlopocoStream::annotateIDs( int currentCycle ){
		//				vhdlCode << "-- CurrentCycle is = " << currentCycle << endl;
		ostringstream vhdlO;
		istringstream in( vhdlCodeBuffer.str() );
		/* instantiate the flex++ object  for lexing the buffer info */
		LexerContext* lexer = new LexerContext(&in, &vhdlO);
		/* This variable is visible from within the flex++ scanner class */
		lexer->yyTheCycle = currentCycle;
		/* call the FlexLexer ++ on the buffer. The lexing output is
		   in the variable vhdlO. Additionally, a temporary table contating
		   the tuples <name, cycle> is created */
		lexer->lex();
		/* the temporary table is used to update the member of the FlopocoStream
		   class useTable */

		updateUseMap(lexer);
		/* the annotated string is returned */
		return vhdlO.str();
	}

	void FlopocoStream::updateUseTable(vector<pair<string,int> > tmpUseTable){
		vector<pair<string, int> >::iterator iter;
		for (iter = tmpUseTable.begin(); iter!=tmpUseTable.end();++iter){
			pair < string, int> tmp;
			tmp.first  =  (*iter).first;
			tmp.second = (*iter).second;
			useTable.push_back(tmp);
		}			
	}

	void FlopocoStream::updateUseMap(LexerContext* lexer){
		updateUseTable(lexer->theUseTable);
		lexer->theUseTable.erase(lexer->theUseTable.begin(), lexer->theUseTable.end());
	}


	void FlopocoStream::setCycle(int cycle){
		currentCycle_=cycle;
	}


	void FlopocoStream::setSecondLevelCode(string code){
		vhdlCode.str("");
		vhdlCode << code;
	}


	vector<pair<string, int> > FlopocoStream::getUseTable(){
		return useTable;
	}


	void FlopocoStream::disableParsing(bool s){
		disabledParsing = s;
	}

	
	bool FlopocoStream::isParsing(){
		return !disabledParsing;
	}


	bool FlopocoStream::isEmpty(){
		return ((vhdlCode.str()).length() == 0 && (vhdlCodeBuffer.str()).length() == 0 );
	}
}
