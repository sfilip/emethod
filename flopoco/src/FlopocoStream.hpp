#ifndef FlopocoStream_HPP
#define FlopocoStream_HPP
#include <vector>
#include <sstream>
#include <utility>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>
#include <string.h>

//#include "VHDLLexer.hpp"
#include "LexerContext.hpp"

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

namespace flopoco{


	/** 
	 * The FlopocoStream class.
	 * Segments of code having the same pipeline informations are scanned 
	 * on-the-fly using flex++ to find the VHDL signal IDs. The found IDs
	 * are augmented with pipeline depth information __IDName__PipelineDepth__
	 * for example __X__2__.
	 */
	class FlopocoStream{
	
		/* Methods for overloading the output operator available on streams */
		/*friend FlopocoStream& operator <= (FlopocoStream& output, string s) {

			output.vhdlCodeBuffer << " <= " << s;
			return output;
		}*/


		template <class paramType> friend FlopocoStream& operator <<(
				FlopocoStream& output, 
				paramType c
			){
			output.vhdlCodeBuffer << c;
			return output;
		}

		
		friend FlopocoStream & operator<<(FlopocoStream& output, FlopocoStream fs); 
		
		friend FlopocoStream& operator<<( FlopocoStream& output, UNUSED(ostream& (*f)(ostream& fs))) ;
		
		public:
			/**
			 * FlopocoStream constructor. 
			 * Initializes the two streams: vhdlCode and vhdlCodeBuffer
			 */
			FlopocoStream();

			/**
			 * FlopocoStream destructor
			 */
			~FlopocoStream();

			/**
			 * method that does the similar thing as str() does on ostringstream objects.
			 * Processing is done using a buffer (vhdlCodeBuffer). 
			 * The output code is = previously transformed code (vhdlCode) + 
			 *  newly transformed code ( transform(vhdlCodeBuffer) );
			 * The transformation on vhdlCodeBuffer is done when the buffer is 
			 * flushed 
			 * @return the augmented string encapsulated by FlopocoStream  
			 */
			string str();

			/** 
			 * Resets both the buffer and the code stream. 
			 * @return returns empty string for compatibility issues.
			 */ 
			string str(string UNUSED(s));
			
			/**
			 * the information from the buffer is flushed when cycle information
			 * changes. In order to annotate signal IDs with with cycle information
			 * the cycle information needs to be passed to the flush method 
			 * @param[in] currentCycle the current pipeline level for the vhdl code
			 *            from the buffer
			 */
			void flush(int currentCycle);
			
			/** 
			 * Function used to flush the buffer (annotate the last IDs from the
			 * buffer) when constructor has finished writing into the vhdl stream
			 */ 
			void flush();
			
			/**
			 * Function that annotates the signal IDs from the buffer with 
			 * __IDName__CycleInfo__
			 * @param[in] currentCycle Cycle Information
			 * @return the string containing the annotated information
			 */
			string annotateIDs(int currentCycle);
			
			/**
			 * The extern useTable rewritten by flex for each buffer is used 
			 * to update a useTable contained locally as a member variable of 
			 * the FlopocoStream class
			 * @param[in] tmpUseTable a vector of pairs which will be copied 
			 *            into the member variable useTable 
			 */
			void updateUseTable(vector<pair<string,int> > tmpUseTable);

			/**
			 * A wrapper for updateUseTable
			 * The external table is erased of information
			 */			
			void updateUseMap(LexerContext* lexer);
			
			void setCycle(int cycle);

			/**
			 * member function used to set the code resulted after a second parsing
			 * was perfromed
			 * @param[in] code the 2nd parse level code 
			 */
			void setSecondLevelCode(string code);
			
			/**
			 * Returns the useTable
			 */  
			vector<pair<string, int> > getUseTable();


			void disableParsing(bool s);
			
			bool isParsing();
			
			bool isEmpty();


			ostringstream vhdlCode;              /**< the vhdl code afte */
			ostringstream vhdlCodeBuffer;        /**< the vhdl code buffer */
			
			int currentCycle_;                   /**< the current cycle is used in the picewise code scanning */
	
			vector<pair<string, int> > useTable; /**< table contating <id, cycle> info */

		protected:
		
			bool disabledParsing;
	};
}
#endif
