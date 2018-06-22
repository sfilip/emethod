/*
   A multiplier by an integer constant for FloPoCo

   This file is part of the FloPoCo project developed by the Arenaire
   team at Ecole Normale Superieure de Lyon

Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
2008-2011.
All rights reserved.

*/

#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../utils.hpp"
#include "../Operator.hpp"
#include <climits>

#include "ShiftAddOp.hpp"
#include "ShiftAddDag.hpp"

#include "IntConstMult.hpp"
//#include "rigo.h"

//#define DEBUG_SET
//#define USE_EXPERIMENTAL_HEURISTIC_1

using namespace std;

namespace flopoco{

	void reset_visited(ShiftAddOp* sao) {
		if (sao!=NULL) {
			sao->already_visited=false;
			switch(sao->op) {
				case X:
					break;
				case Add:
				case Sub:
				case RSub:
					reset_visited(sao->i);
					reset_visited(sao->j);
					break;
				case Shift:
				case Neg:
					reset_visited(sao->i);
					break;
			}
		}
	}





	// No longer used
	int compute_tree_depth(ShiftAddOp* sao) {
		int ipd, jpd;
		if (sao==NULL)
			return 0;
		else {
			switch(sao->op) {
				case X:
					return 0;
				case Add:
				case Sub:
				case RSub:
					ipd = compute_tree_depth(sao->i);
					jpd = compute_tree_depth(sao->j);
					return 1+max(ipd, jpd);
				case Shift:
					ipd = compute_tree_depth(sao->i);
					return ipd;
				case Neg:
					ipd = compute_tree_depth(sao->i);
					return 1+ipd;
			}
			return 0;
		}
	}





	// Do not forget to call reset_visited before calling this one.
	int compute_total_cost_rec(ShiftAddOp* sao) {
		if (sao==NULL || sao->already_visited)
			return 0;
		else {
			sao->already_visited=true;
			switch(sao->op) {
				case X:
					return 0;
				case Add:
				case Sub:
				case RSub:
					return sao->cost_in_full_adders + compute_total_cost_rec(sao->i) + compute_total_cost_rec(sao->j);
				case Shift:
				case Neg:
					return sao->cost_in_full_adders + compute_total_cost_rec(sao->i);
			}
		}
		throw string("This exception in IntConstMult::compute_total_cost_rec should never happen");
	}



	int compute_total_cost(ShiftAddOp* sao) {
		reset_visited(sao);
		return compute_total_cost_rec(sao);
	}


	/** gives the cost of a ShiftAddDag, using its depth and surface*/
	//FIXME: find a better cost function (for the client)
	int costF(ShiftAddDag* sao, int priorityFlag=0 ){
		if (!priorityFlag)
			return ( compute_total_cost(sao->result)*compute_tree_depth(sao->result) );
		else if (priorityFlag+1) //priorityFlag=1
			return ( compute_total_cost(sao->result) );
		else
			return ( compute_tree_depth(sao->result) );
	}




	/**
	 * Finds and returns the index of the best ShiftAddDag in an array considering the priority flag. 
	 * If the flag is not set, we consider both latency and surface.
	 * If the flag is 1, we only consider the surface
	 * If the flag is -1, w consider only the latency
	//WARNING: if two ShiftAddDags have the same characteristics, the returned one will be the ShiftAddDag with the smallest index.
	//WARNING: if the flag was badly set, it returns 0;
	//
	 */
	unsigned int find_best_implementation( ShiftAddDag **implemTries, const unsigned int numberOfTries, const int priorityFlag=0) {
		unsigned int bestShiftAddDag=0;
		int tryCost, bestCost;

		bestCost=costF(implemTries[0],priorityFlag);
		for (unsigned int i=1; i<numberOfTries; i++) {
			tryCost=costF(implemTries[i],priorityFlag);

			if ( tryCost < bestCost )
			{
				bestShiftAddDag=i;
				bestCost=costF(implemTries[bestShiftAddDag],priorityFlag);
			}
			else if ( tryCost == bestCost )
				if ( costF(implemTries[i],0) < costF(implemTries[bestShiftAddDag],0) )
				{
					bestShiftAddDag=i;
					bestCost=costF(implemTries[bestShiftAddDag],priorityFlag);
				}
		}

		return bestShiftAddDag;
	}

	/**
	 * Depth-first traversal of the DAG to build the pipeline.
	 * @param partial_delay accumulates the delays of several stages
	 *
	 * Starting from the leaves, we accumulate partial delays until target_period is reached.
	 * Then pipeline level will be inserted.
	 */
	void IntConstMult::build_pipeline(ShiftAddOp* sao, double &partial_delay) {
		string iname, jname, isignal, jsignal;
		double idelay=0,jdelay=0, max_children_delay;
		int size, isize, jsize, shift, adder_size; 
		bool use_pipelined_adder;
		IntAdder* adder=0;

		if (sao==NULL)
			return;
		else {
			// First check that the sub-DAG has not been already visited
			bool already_visited=true;
			try { 
				getSignalByName(sao->name);
			} catch (std::string s) {
				already_visited=false;
			}
			if(already_visited)
				return;

			// A few variables to make the code below easier to read
			ShiftAddOpType op = sao->op;
			size = sao->size;
			shift = sao->s;

			switch(op) {
				case X:
					partial_delay=0;
					setCycle(0, false);
					return;

				case Add:
				case Sub:
				case RSub:
					// A few variables to make the code below easier to read
					isize = sao->i->size;
					jsize = sao->j->size;

					build_pipeline(sao->i, idelay);
					if(sao->i != sao->j) {
						build_pipeline(sao->j, jdelay);
					}
					iname = sao->i->name; 
					jname = sao->j->name; 

					adder_size = sao->cost_in_full_adders+1;
					vhdl << endl << tab << "-- " << *sao <<endl; // comment what we're doing
					setCycleFromSignal(iname, false);
					syncCycleFromSignal(jname, false);

					max_children_delay = max(idelay,jdelay);

					// Now decide what kind of adder we will use, and compute the remaining delay
					use_pipelined_adder=false;
					if (isSequential()) {
						// First case: using a plain adder fits within the current pipeline level
						double tentative_delay = max_children_delay + getTarget()->adderDelay(adder_size) + getTarget()->localWireDelay();
						if(tentative_delay <= 1./getTarget()->frequency()) {
							use_pipelined_adder=false;
							partial_delay = tentative_delay;					
						}
						else { 
							// register the children 
							nextCycle();
							// Is a standard adder OK ?
							tentative_delay = getTarget()->ffDelay() + getTarget()->localWireDelay() + getTarget()->adderDelay(adder_size);
							if(tentative_delay <= 1./getTarget()->frequency()) {
								use_pipelined_adder=false;
								partial_delay = tentative_delay;					
							}
							else { // Need to instantiate an IntAdder
								use_pipelined_adder=true;
								adder = new IntAdder(getTarget(), adder_size);
								adder->changeName(getName() + "_" + sao->name + "_adder");
								addSubComponent(adder);

								partial_delay =  (adder->getOutDelayMap())["R"]; //  getTarget()->adderDelay(adder->getLastChunkSize());
							}
						}
					}

					// Now generate the VHDL
					if(shift==0) { // Add with no shift -- this shouldn't happen with current DAGs so the following code is mostly untested
						if(op==Sub || op==RSub)
							throw string("In IntConstMult::build_pipeline, Sub and RSub with zero shift currently unimplemented"); // TODO
						isignal = sao->name + "_High_L";  
						jsignal = sao->name + "_High_R"; 

						// The i part
						vhdl << tab << declare(isignal, size) << " <= ";
						if(size>isize+1) // need to sign-extend x
							vhdl <<"(" << size-1 << " downto " << isize <<" => '" << (sao->i->n >= 0 ? "0" : "1" ) << "') & ";
						vhdl << iname << ";" << endl;
						// the y part
						vhdl << tab << declare(jsignal, size) << " <= ";
						if(size>jsize) // need to sign-extend y
							vhdl << "(" << size-1 <<" downto " << jsize <<" => '" << (sao->j->n >= 0 ? "0" : "1" ) << "') & ";
						vhdl << jname << ";" << endl;

						if(use_pipelined_adder) { // Need to use an IntAdder subcomponent
							inPortMap  (adder, "X", isignal);
							inPortMap  (adder, "Y", jsignal);
							inPortMapCst  (adder, "Cin", "'0'");
							outPortMap (adder, "R",sao->name);
							vhdl << instance(adder, sao->name + "_adder");
						}
						else
							vhdl << tab << declare(sao->name, size) << " <= " << isignal << " + " << jsignal << ";" << endl;
					}


					else { // Add with actual shift
						if(op == Add || op==RSub) {
							if(shift >= jsize) { // Simpler case when the two words to add are disjoint; size=isize+s+1
								//                        jjjjjj
								//             +/-  iiii
								// TODO perf: use an IntAdder here when needed
								// The lower bits of the sum are those of y, possibly sign-extended but otherwise untouched
								vhdl << tab << declare(sao->name, sao->size) << "("<< shift - 1 <<" downto 0) <= " ;
								if(shift>jsize) {
									vhdl << "(" <<  shift-1 <<" downto " << jsize << " => ";
									if(sao->j->n >= 0)  vhdl << "'0'"; // pad with 0s 
									else                vhdl << jname << "(" << jsize-1 << ")";// sign extend
									vhdl << ") & ";
								}
								vhdl << jname << ";   -- lower bits untouched"<<endl;

								if(op == Add) {
									// The higher bits (size-1 downto s) of the result are those of x, possibly plus 11...1 if y was negative
									vhdl << tab << sao->name << "("<<sao->size-1<<" downto "<< shift<<") <= " << iname ;
									if(sao->j->n < 0) { 
										vhdl <<" + (" << sao->size-1 <<" downto " <<  shift <<" => " << jname << "(" << jsize-1 << ")) "
											<< ";   -- sum of higher bits"<<endl;
									}
									else 
										vhdl << ";   -- higher bits also untouched"<<endl;
								}
								else {// op == RSub
									// The higher bits (size-1 downto s) of the result are those of -x, possibly plus 11...1 if y was negative
									vhdl << tab << sao->name << "("<<sao->size-1<<" downto "<< shift<<") <= " ;
									if(sao->j->n < 0) 
										vhdl <<"(" << sao->size-1 << " downto " <<  shift <<" => " << jname << "(" << jsize-1 << ")) ";
									else
										vhdl <<"(" << sao->size-1 << " downto " <<  shift <<" => '0') ";
									vhdl << " - " << iname << ";   -- sum of higher bits"<<endl;
								}
							} // end if (shift >= jsize)
							else{ 
								// jsize>s.        Cases:      xxxxx              xxxxxx
								//                                yyyyyyyyyy             yyyyyyyyyyyy
								// so we may need to sign-extend Vx, or Vy, or even both.
								// The higher bits of the result are sum/diff
								isignal = sao->name + "_High_L";  
								jsignal = sao->name + "_High_R"; 
								// The x part
								vhdl << tab << declare(isignal,  size - shift) << " <= ";
								if(size >= isize +  shift +1) { // need to sign-extend vx. If the constant is positive, padding with 0s is enough
									vhdl <<" (" << size-1 << " downto " << isize +  shift <<" => ";
									if(sao->i->n >= 0)   vhdl << "'0'";// pad with 0s 
									else                 vhdl << iname << "(" << isize-1 << ")"; // sign extend
									vhdl << ") & ";
								}
								vhdl << iname << "("<< isize -1 <<" downto 0) ;" << endl;
								// the y part
								vhdl << tab << declare(jsignal,  size - shift) << " <= ";
								if(size >= jsize+1) {// need to sign-extend vy. If the constant is positive padding with 0s is enough
									vhdl <<" (" << size-1 << " downto " << jsize <<" => ";
									if(sao->j->n >= 0)  vhdl << "'0'"; // pad with 0s 
									else                vhdl << jname << "(" << jsize-1 << ")";// sign extend
									vhdl << ") & ";
								}
								vhdl << jname << "("<< jsize -1 <<" downto " <<  shift << "); " << endl;

								// do the sum
								if(use_pipelined_adder) {
									inPortMap  (adder, "X", jsignal);
									if(op==Add) {
										inPortMap  (adder, "Y", isignal);
										inPortMapCst  (adder, "Cin", "'0'");
									} 
									else { // RSub
										string isignalneg = isignal+"_neg"; 
										vhdl << declare(isignalneg, size - shift) << " <= not " << isignal;
										inPortMap  (adder, "Y", isignal);
										inPortMapCst  (adder, "Cin", "'1'");
									}
									string resname=sao->name+"_h";
									outPortMap (adder, "R",resname);
									vhdl << instance(adder, sao->name + "_adder");

									syncCycleFromSignal(resname, false);
									//nextCycle();
									vhdl << tab << declare(sao->name, sao->size) << "("<<size-1<<" downto " <<  shift << ") <= " << resname + ";" << endl;
								}
								else
									vhdl << tab << declare(sao->name, sao->size) << "("<<size-1<<" downto " <<  shift << ") <= " // vz (size-1 downto s)
										<< jsignal << (op==Add ? " + " : "-") << isignal << ";   -- sum of higher bits" << endl; 

								// In both cases the lower bits of the result (s-1 downto 0) are untouched
								vhdl << tab << sao->name << "("<<shift-1<<" downto 0) <= " << jname <<"("<< shift-1<<" downto 0);   -- lower bits untouched"<<endl;

							} // end if (shift >= jsize) else
						} // end if(op == Add || op == RSub) 
						else { // op=Sub 
							// Do a normal subtraction of size size
							isignal = sao->name + "_L";  
							jsignal = sao->name + "_R"; 
							vhdl << tab << declare(isignal,  size) << " <= ";
							if(size > isize+shift) {// need to sign-extend vx. If the constant is positive padding with 0s is enough
								vhdl <<" (" << size-1 << " downto " << isize+shift <<" => ";
								if(sao->i->n >= 0)   vhdl << "'0'";// pad with 0s 
								else                 vhdl << iname << "(" << isize-1 << ")";// sign extend
								vhdl << ") & ";
							}
							vhdl << iname << " & (" << shift-1 << " downto 0 => '0');" << endl;

							vhdl << tab << declare(jsignal,  size) << " <= ";
							vhdl <<" (" << size-1 << " downto " << jsize <<" => ";
							if(sao->j->n >= 0)   vhdl << "'0'";// pad with 0s 
							else                 vhdl << jname << "(" << jsize-1 << ")";// sign extend
							vhdl << ") & ";
							vhdl << jname << ";" << endl;

							// do the subtraction
							if(use_pipelined_adder) {
								string jsignalneg = jsignal+"_neg"; 
								vhdl << declare(jsignalneg, size) << " <= not " << jsignal;
								inPortMap  (adder, "X", isignal);
								inPortMap  (adder, "Y", jsignalneg);
								inPortMapCst  (adder, "Cin", "'1'");
								string resname=sao->name+"_h";
								outPortMap (adder, "R",resname);
								vhdl << instance(adder, sao->name + "_adder");

								syncCycleFromSignal(resname, false);
								//nextCycle();
								vhdl << tab << declare(sao->name, size) << " <=  " << resname + ";" << endl;

							}
						}
					}

					return;

					// shift and neg almost identical
				case Shift:
				case Neg:
					isize = sao->i->size;

					double local_delay;
					if(op == Neg){   
						local_delay = getTarget()->adderDelay(sao->cost_in_full_adders);
					}
					else 
						local_delay=0;

					build_pipeline(sao->i, idelay);

					iname = sao->i->name; 
					setCycleFromSignal(iname, false);

					if(isSequential() 
							&& idelay +  getTarget()->localWireDelay() + local_delay > 1./getTarget()->frequency()
							&& sao->i->op != X)
					{
						// This resets the partial delay to that of this ShiftAddOp
						nextCycle();
						partial_delay =  getTarget()->ffDelay() + getTarget()->adderDelay(sao->cost_in_full_adders);
					}
					else{ // this ShiftAddOp and its child will be in the same pipeline level
						partial_delay = idelay + getTarget()->localWireDelay() + local_delay;
					}
					vhdl << tab << declare(sao->name, size) << " <= " ;
					// TODO use a pipelined IntAdder when necessary
					if(op == Neg)   
						vhdl << "("<< size -1 <<" downto 0 => '0') - " << iname <<";"<<endl; 
					else { // Shift
						if (shift == 0) 
							vhdl << iname <<";"<<endl; 
						else
							vhdl << iname <<" & ("<< shift - 1 <<" downto 0 => '0');"<<endl;
					}
					break;

			}   
		}
	}











	IntConstMult::IntConstMult(Target* _target, int _xsize, mpz_class n) :
		Operator(_target), n(n), xsize(_xsize)
	{
			ostringstream name; 

			srcFileName="IntConstMult";
			setCopyrightString("Florent de Dinechin, Antoine Martinet (2007-2013)");

			//C++ wrapper for GMP does not work properly on win32, using mpz2string
			name <<"IntConstMult_"<<xsize<<"_"<<mpz2string(n);
			setNameWithFreqAndUID(name.str());


			if (!mpz_cmp_ui(n.get_mpz_t(),0)){
				REPORT(LIST, "Here I am, brain the size of a planet and they ask me to multiply by zero. Call that job satisfaction? 'Cos I don't.");
				// TODO: replace with a REPORT (keep the HGG quotation), and build a multiplier by 0:
				//			this constructor will be called by automatic generators, and should perform the required operation in all cases.
				addInput("X", xsize);
				addOutput("R", 1); // multiplier by zero is always zero
				vhdl << tab << "R <= \"0\";" <<endl;	
			}

			else{
				rsize = intlog2(n * ((mpz_class(1)<<xsize)-1));

				addInput("X", xsize);
				addOutput("R", rsize);

#if 1
				// adder tree, no bitheaps
			 
				// Build the implementation trees for the constant multiplier
				ShiftAddDag* implem_try_right = buildMultBoothTreeFromRight(n);
				ShiftAddDag* implem_try_left = buildMultBoothTreeFromLeft(n);
				ShiftAddDag* implem_try_balanced = buildMultBoothTreeToMiddle(n);
				ShiftAddDag* implem_try_euclidean0 = buildEuclideanTree(n);
				ShiftAddDag* implem_try_Shifts = buildMultBoothTreeSmallestShifts(n);

				REPORT (DETAILED,"Building from the left: cost="<<costF(implem_try_left)<<" surface="<<costF(implem_try_left,1)<<" latency="<<costF(implem_try_left,-1) );
				REPORT (DETAILED,"Building from the right: cost="<<costF(implem_try_right)<<" surface="<<costF(implem_try_right,1)<<" latency="<<costF(implem_try_right,-1) );
				REPORT (DETAILED,"Building balanced: cost="<<costF(implem_try_balanced)<<" surface="<<costF(implem_try_balanced,1)<<" latency="<<costF(implem_try_balanced,-1) );
				REPORT (DETAILED,"Building with Euclid 0: cost="<<costF(implem_try_euclidean0)<<" surface="<<costF(implem_try_euclidean0,1)<<" latency="<<costF(implem_try_euclidean0,-1) );
				REPORT (DETAILED,"Building using shifts: cost="<<costF(implem_try_Shifts)<<" surface="<<costF(implem_try_Shifts,1)<<" latency="<<costF(implem_try_Shifts,-1) );

				ShiftAddDag* tries[5];
				tries[1]=implem_try_left;
				tries[0]=implem_try_right;
				tries[2]=implem_try_balanced;
				tries[3]=implem_try_euclidean0;
				tries[4]=implem_try_Shifts;

				// Build in implementation a tree for the constant multiplier
				unsigned int position=find_best_implementation( tries, 4);
				switch (position) {
					case 1: implementation=implem_try_left;
							REPORT( DETAILED,"Building "<<n.get_mpz_t()<<" from the left was better amelioration against right-building= "<< costF(implem_try_right,1)-costF(implem_try_left,1) << " amelioration on latency= " << costF(implem_try_right,-1)-costF(implem_try_left,-1) );
							delete implem_try_right;
							delete implem_try_balanced;
							delete implem_try_euclidean0;
							delete implem_try_Shifts;
							break;
					case 0: implementation=implem_try_right;
							REPORT( DETAILED,"Building "<<n.get_mpz_t()<<" from the right was better");
							delete implem_try_left;
							delete implem_try_balanced;
							delete implem_try_euclidean0;
							delete implem_try_Shifts;
							break;
					case 2: implementation=implem_try_balanced;
							REPORT( DETAILED,"Building "<<n.get_mpz_t()<<" with balanced method was better amelioration against right-building= "<< costF(implem_try_right,1)-costF(implem_try_balanced,1) << " amelioration on latency= " << costF(implem_try_right,-1)-costF(implem_try_balanced,-1) );
							delete implem_try_right;
							delete implem_try_left;
							delete implem_try_euclidean0;
							delete implem_try_Shifts;
							break;
					case 3: implementation=implem_try_euclidean0;
							REPORT( DETAILED,"Building "<<n.get_mpz_t()<<" with euclide 0 was better amelioration against right-building= "<< costF(implem_try_right,1)-costF(implem_try_euclidean0,1) << " amelioration on latency= " << costF(implem_try_right,-1)-costF(implem_try_euclidean0,-1) );
							delete implem_try_right;
							delete implem_try_left;
							delete implem_try_balanced;
							delete implem_try_Shifts;
							break;
					case 4: implementation=implem_try_Shifts;
							REPORT( DETAILED,"Building "<<n.get_mpz_t()<<" targetting smallest shifts was better amelioration against right-building= "<< costF(implem_try_right,1)-costF(implem_try_Shifts,1) << " amelioration on latency= " << costF(implem_try_right,-1)-costF(implem_try_Shifts,-1) );
							delete implem_try_right;
							delete implem_try_left;
							delete implem_try_balanced;
							delete implem_try_euclidean0;
							break;
					default:
							throw string("Unknown error encountered");
				}

				//delete implementation;
				//implementation=buildMultBoothTreeFromRight(n);

				if(UserInterface::verbose>=DETAILED) showShiftAddDag();

				int cost=compute_total_cost(implementation->result);
				REPORT(INFO, "Estimated bare cost (not counting pipeline overhead) : " << cost << " FA/LUT" );
				REPORT(INFO, "Depth of the DAG : " << compute_tree_depth(implementation->result) );

				double delay=0.0;
				// recursively build the pipeline in the vhdl stream
				build_pipeline(implementation->result, delay);

				// copy the top of the DAG into variable R
				vhdl << endl << tab << "R <= " << implementation->result->name << "("<< rsize-1 <<" downto 0);"<<endl;
				outDelayMap["R"] = delay;
#else
				//experimenting with bitheaps
				double delay;
				delete implementation;
				implementation = buildMultBoothTreeBitheap(n, 1);

				//create the bitheap that computes the sum
				bitheap = new BitHeap(this, rsize+1);

				for(int i=0; (unsigned)i<implementation->saoHeadlist.size(); i++)
				{
					delay = 0.0;
					build_pipeline(implementation->saoHeadlist[i], delay);

					//add the bits to the bitheap
					vhdl << endl << tab << "-- adding " << implementation->saoHeadlist[i]->name
							<< " shifted by " << implementation->saoHeadShift[i] << " to the bitheap" << endl;
					for(int j=0; j<implementation->saoHeadlist[i]->size-1; j++)
					{
						bitheap->addBit(implementation->saoHeadShift[i]+j,
								join(implementation->saoHeadlist[i]->name, of(j)));
					}
					//sign extend if necessary
					if(implementation->saoHeadlist[i]->n < 0)
					{
						ostringstream tmpStr;

						tmpStr << "not(" << implementation->saoHeadlist[i]->name << of(implementation->saoHeadlist[i]->size-1) << ")";
						bitheap->addBit(implementation->saoHeadShift[i]+implementation->saoHeadlist[i]->size-1,
									tmpStr.str());
						for(int j=implementation->saoHeadlist[i]->size-1; j<(rsize+1); j++)
						{
							bitheap->addConstantOneBit(implementation->saoHeadShift[i]+j);
						}
					}
					else
					{
						bitheap->addBit(implementation->saoHeadShift[i]+implementation->saoHeadlist[i]->size-1,
								join(implementation->saoHeadlist[i]->name, of(implementation->saoHeadlist[i]->size-1)));
					}
				}

				//compress the bitheap
				bitheap->generateCompressorVHDL();

				// copy the top of the DAG into variable R
				vhdl << endl << tab << "R <= " << bitheap->getSumName() << range(rsize-1, 0) << ";" << endl;
				outDelayMap["R"] = getCriticalPath();
#endif // bitheap or not
			}
		}





	// The constructor for rational constants

	IntConstMult::IntConstMult(Target* _target, int _xsize, mpz_class n, mpz_class period, int periodMSBZeroes, int periodSize, mpz_class header, int headerSize, int i, int j) :
		Operator(_target), xsize(_xsize){
			ostringstream name; 

			srcFileName="IntConstMult (periodic)";
			setCopyrightString("Florent de Dinechin (2007-2011)");
			name <<"IntConstMultPeriodic_"<<xsize<<"_"<<mpz2string(header)<<"_"<<headerSize
				<<"_"<<mpz2string(period<<periodMSBZeroes)<<"_"<<periodSize<<"_"<<i<<"_";
			if (j<0) 
				name << "M" << -j;
			else
				name << j;
			setNameWithFreqAndUID(name.str());

			//implementation = new ShiftAddDag(this);

			rsize = intlog2(n * ((mpz_class(1)<<xsize)-1));
			REPORT(INFO, "Building a periodic DAG for  " << n );

			addInput("X", xsize);
			addOutput("R", rsize);

			// Build in implementation a tree for the constant multiplier

			ShiftAddOp* powerOfTwo[1000]; // Should be enough for anybody

			// Build the multiplier by the period
			implementation = buildMultBoothTreeFromRight(period);
			powerOfTwo[0] = implementation->result;

			// Example: actual period 11000, periodSize=5, is represented by period=11, periodMSBZeroes=3
			// powerOfTwo[0] will build mult by 11
			// powerOfTwo[1] will build mult by 1100011: shift=periodSize
			// powerOfTwo[2] will build mult by 11000110001100011: shift=2*periodSize

			for (int k=1; k<=i; k++){
				powerOfTwo[k]=  new ShiftAddOp(implementation, Add, powerOfTwo[k-1], (periodSize<<(k-1)), powerOfTwo[k-1] );
			}


			if(header==0)  {
				if(j==-1) //just repeat the period 2^i times
					implementation->result = 	powerOfTwo[i];
				else
					implementation->result = new ShiftAddOp(implementation, Add, powerOfTwo[j], (periodSize<<i), powerOfTwo[i] );
			}
			else {
				// REPORT(DEBUG, "DAG before adding header and zero=" << zeroLSBs);
				// if(verbose>=DETAILED) showShiftAddDag();
				REPORT(DEBUG, "Header not null: header="<<header);
				ShiftAddOp* headerSAO;
				headerSAO=implementation->sadAppend(buildMultBoothTreeFromRight(header));
				if(j==-1)//just repeat the period 2^i times
					implementation->result = 	new ShiftAddOp(implementation, Add, headerSAO, (periodSize<<i) - periodMSBZeroes, powerOfTwo[i] );
				else { // Here we should generate both trees and use the smaller. The following static decision is probably always the good one
					if (i==j) {// I know this case should be equivalent to j=-1 and i+1, but it seems to happen
						powerOfTwo[i+1] = implementation->provideShiftAddOp(Add, powerOfTwo[i], (periodSize<<i), powerOfTwo[i] );
						implementation->result = new ShiftAddOp(implementation, Add, headerSAO, (periodSize<<(i+1)) - periodMSBZeroes,powerOfTwo[i+1]);
					}
					else{
						ShiftAddOp* tmp = implementation->provideShiftAddOp(Add, headerSAO, (periodSize<<j) - periodMSBZeroes, powerOfTwo[j] );
						implementation->result = new ShiftAddOp(implementation, Add, tmp, (periodSize<<i), powerOfTwo[i] );
					}
				}
			}

			if(UserInterface::verbose>=DETAILED) showShiftAddDag();

			int cost=compute_total_cost(implementation->result);
			REPORT(INFO, "Estimated bare cost (not counting pipeline overhead) : " << cost << " FA/LUT" );
			REPORT(INFO, "Depth of the DAG : " << compute_tree_depth(implementation->result) );

			double delay=0.0;
			// recursively build the pipeline in the vhdl stream
			build_pipeline(implementation->result, delay);

			// copy the top of the DAG into variable R
			vhdl << endl << tab << "R <= " << implementation->result->name << "("<< rsize-1 <<" downto 0);"<<endl;
			outDelayMap["R"] = delay;

		}


	IntConstMult::IntConstMult(Target* _target, int _xsize) :
			Operator(_target), xsize(_xsize)
	{

	}




	// One hand-coded Lefevre multiplier, for comparison purposes -- to be inserted somewhere in the constructor

#if 0
	if(false && n==mpz_class("254876276031724631276054471292942"))
	{
		const int PX=0;
		cerr<<"Optimization by rigo.c"<< endl;//                    x    s    y
		/*
		*/
		implementation->computeVarSizes(); 
		implementation->result = implementation->sao.size()-1;
	}
	else 
		if(n==mpz_class("1768559438007110"))
		{
			const int PX=0;
			cerr<<"Optimization by rigo.c"<< endl;//                   
			implementation->addOp( new ShiftAddOp(implementation, Neg,   PX) );       // 1  mx = -u0
			implementation->addOp( new ShiftAddOp(implementation, Add,   0, 19,  0) );        // 2  u3 = u0 << 19 + u0
			implementation->addOp( new ShiftAddOp(implementation, Shift,   2, 20) );          // 3  u103 = u3 << 20
			implementation->addOp( new ShiftAddOp(implementation, Add,   3, 4,   3) );        // 4  u203 = u103 << 4  + u103
			implementation->addOp( new ShiftAddOp(implementation, Add,   0, 14,  1) );        // 5  u7 = u0 << 14 + mx
			implementation->addOp( new ShiftAddOp(implementation, Add,   5, 6,  0) );         // 6  u6 = u7 << 6 + u0
			implementation->addOp( new ShiftAddOp(implementation, Add,   6, 10,  0) );        // 7  u5 = u6 << 10 + u0
			implementation->addOp( new ShiftAddOp(implementation, Shift, 7,  16    ));         // 8  u1 = u5 << 16
			implementation->addOp( new ShiftAddOp(implementation, Add,   8, 0,   4) ) ;       // 9  u101 = u1 + u203
			implementation->addOp( new ShiftAddOp(implementation, Add,   0, 21,  1) );        // 10 u107 = u0 << 21 + mx
			implementation->addOp( new ShiftAddOp(implementation, Add,   10, 18,  0) );       // 11 u106 = u107 << 18 + u0
			implementation->addOp( new ShiftAddOp(implementation, Add,   11, 4,   1) );       // 12 u105 = u106 << 4 + mx
			implementation->addOp( new ShiftAddOp(implementation, Add,   12, 5,   0) );       // 13 u2 = u105 << 5 + u0
			implementation->addOp( new ShiftAddOp(implementation, Shift, 13, 1) );            // 14 u102 = u2 << 1
			implementation->addOp( new ShiftAddOp(implementation, Neg,   14) );       // 15 mu102 = - u102
			implementation->addOp( new ShiftAddOp(implementation, Add,   14, 2,   15) );       // 16 u202 = u102 << 2  + mu102
			implementation->addOp( new ShiftAddOp(implementation, Add,   9, 0,   16) );        // R = u101 + u202

			/*
			   0  u0 = x
			   1  mx = -u0
			   2  u3 = u0 << 19 + u0
			   3  u103 = u3 << 20
			   4  u203 = u103 << 4  + u103
			   5  u7 = u0 << 14 + mx
			   6  u6 = u7 << 6 + u0
			   7  u5 = u6 << 10 + u0
			   8  u1 = u5 << 16
			   9  u101 = u1 + u203
			   10 u107 = u0 << 21 + mx
			   11 u106 = u107 << 18 + u0
			   12 u105 = u106 << 4 + mx
			   13 u2 = u105 << 5 + u0
			   14 u102 = u2 << 1
			   15 mu102 = - u102
			   16 u202 = u102 << 2  + mu102
			   R = u101 + u202
			   */
			implementation->computeVarSizes(); 
			implementation->result = implementation->saolist.size()-1;
		}
	//	else
#endif


	IntConstMult::~IntConstMult() {
		delete implementation;
	}





	string IntConstMult::printBoothCode(int* BoothCode, int size) {
		ostringstream s;
		for (int i=size-1; i>=0; i--) {
			if(BoothCode[i]==0)       s << "0"; 
			else if(BoothCode[i]==1)  s << "+" ;
			else if(BoothCode[i]==-1) s << "-" ;   
		}
		return s.str();
	}



	bool needsMinusX(int * boothCode, int n) {
		for(int i=0; i<n; i++)
			if(boothCode[i]==-1)
				return true;
		return false;
	}

	/**
	 * This function initializes divider, quotient and remainder,
	 * and returns them choosing the divider that minimizes
	 * the size of quotient plus the size of remainder
	 */
	// FIXME: this should look at the boothcode of the quotient and remainder and make them minimal.
	//			The improvement is started but does not work it is set in the #define USE_EXPERIMENTAL_HEURISTIC_1.
	bool IntConstMult::findBestDivider(mpz_class n, mpz_t & divider, mpz_t & quotient, mpz_t & remainder) {

		REPORT(FULL,"Entering findBestDivider()");

		mpz_t sizeTest;
		mpz_init(sizeTest);

		//check the size of the constant. if it is a power of two,
		//	we have to check if constant-1 is a good divider too or not.
		size_t sizeOfmaxDivider=mpz_sizeinbase(n.get_mpz_t(),2);

		mpz_ui_pow_ui(sizeTest,2,sizeOfmaxDivider-1);
		mpz_sub_ui(sizeTest,sizeTest,1); //computing the test

		// if the constant is smaller than the power of two minus one, then we are allowed to check
		//		directly the smaller dividers. Else we have to check the divider which size matches the size of the constant.
		if ( mpz_cmp(n.get_mpz_t(), sizeTest) < 0 )
			sizeOfmaxDivider--;
		mpz_clear(sizeTest);

		REPORT(DEBUG,"					sizeOfmaxDivider="<<sizeOfmaxDivider);


		mpz_t q,r;
		mpz_init(q);
		mpz_init(r);

		//we initialize the three outputs to the constant because we want them to be smaller (in bits) than it.
		mpz_init_set(divider,n.get_mpz_t());
		mpz_init_set(quotient,n.get_mpz_t());
		mpz_init_set(remainder,n.get_mpz_t());


		mpz_t *potentialDividers = new mpz_t[2*(sizeOfmaxDivider-1)];
		for (unsigned int i=0; i<2*(sizeOfmaxDivider-1); i++)
		{
			mpz_init(potentialDividers[i]); //initialisation of the array
		}

		for (unsigned int i=0; i<2*(sizeOfmaxDivider-1);i+=2)
		{
			// the first box is filled with 2^i+1
			mpz_ui_pow_ui(potentialDividers[i],2,sizeOfmaxDivider-1-(i/2));
			mpz_add_ui(potentialDividers[i],potentialDividers[i],1);
			if (mpz_cmp(potentialDividers[i],n.get_mpz_t())>0)
				mpz_set_ui(potentialDividers[i],3); //we set it to three to avoid problems trying to divide a constant by a superior divider.
			REPORT(DEBUG,"potentialDividers["<<i<<"]="<<potentialDividers[i]);

			//the second one with 2^i-1
			mpz_ui_pow_ui(potentialDividers[i+1],2,sizeOfmaxDivider-1-(i/2));
			mpz_sub_ui(potentialDividers[i+1],potentialDividers[i+1],1);
			REPORT(DEBUG,"potentialDividers["<<i+1<<"]="<<potentialDividers[i+1]);
		}

#if defined USE_EXPERIMENTAL_HEURISTIC_1
			int *fBC= new int [intlog2(n)+1];
			int sizeOfCurrentOperands, sizeOfNewOperands;
#endif

		for (unsigned int i=0; i<2*(sizeOfmaxDivider-1);i++)
		{
			//computing euclidean division
			mpz_tdiv_qr(q,r,n.get_mpz_t(),potentialDividers[i]);
			//			REPORT(DEBUG,"q="<<q<<" r="<<r);

#if defined USE_EXPERIMENTAL_HEURISTIC_1
			//FIXME: this does not work and finishes with an infinite loop state
			mpz_class quot(q);
			mpz_class rem(r);
			mpz_class fquot(quotient);
			mpz_class frem(remainder);

			//avoid infinite loop taking the following try when it's possible, and taking the precedent if we are at the end.
			if (!mpz_cmp(q, n.get_mpz_t()) || !mpz_cmp(r, n.get_mpz_t())){
				i++;
				if (i==2*(sizeOfmaxDivider-1)){
					i--;
					mpz_tdiv_qr(q,r,n.get_mpz_t(),potentialDividers[i]);
					mpz_set(divider,potentialDividers[i]);
					mpz_set(quotient,q);
					mpz_set(remainder,r);
					break;
				}
			}
			sizeOfCurrentOperands=recodeBooth(fquot, fBC)+recodeBooth(frem, fBC);
			sizeOfNewOperands=recodeBooth(quot,fBC)+recodeBooth(rem,fBC);
			
			if (  ( sizeOfNewOperands ) < ( sizeOfCurrentOperands )  )
			{	//if the result is smaller than the current result, then set the current result to the result and carry on
				mpz_set(divider,potentialDividers[i]);
				mpz_set(quotient,q);
				mpz_set(remainder,r);
			REPORT(DEBUG, "				I am in the test. I managed to set the operands. remainder="<<remainder<<" quotient="<<quotient<<" divider="<<divider <<" n="<<n.get_mpz_t());
			}
#endif

#if !defined USE_EXPERIMENTAL_HEURISTIC_1
			if ( (mpz_sizeinbase(q,2)+mpz_sizeinbase(r,2)) < (mpz_sizeinbase(quotient,2)+mpz_sizeinbase(remainder,2)) )
			{
				//if the result is smaller than the current result, then set the current result to the result and carry on
				mpz_set(divider,potentialDividers[i]);
				mpz_set(quotient,q);
				mpz_set(remainder,r);
			}
#endif
		}

#if defined USE_EXPERIMENTAL_HEURISTIC_1
			delete [] fBC;
#endif
		//deleting the mess with your memory
		for (unsigned int i=0; i<2*(sizeOfmaxDivider-1); i++){
			mpz_clear(potentialDividers[i]);
		}
		mpz_clears(q,r,NULL);
		//if we didn'tchanged anything since we set the result to n (value of the constant to deal with), return false: the result is unusable
		if ( (!mpz_cmp(quotient,n.get_mpz_t())) || (!mpz_cmp(remainder,n.get_mpz_t())) )
			return false;
		return true;
	}

	/** Recodes input n, returns the number of non-zero bits */
	int IntConstMult::recodeBooth(mpz_class n, int* BoothCode) {
		int i;
		int *b, *c;
		int nonZeroInBoothCode = 0;
		int nsize = intlog2(n);

		// Build the binary representation -- I'm sure there is a mpz method for it.
		//		Search around mpz_array_init(), mpz_export(), or mpz_get_ui()
		//		(this last one should be useless because of the too restricted size)
		mpz_class nn = n;
		int l = 0;
		int nonZero=0;
		b = new int[nsize+1];
		while(nn!=0) {
			b[l] = (nn.get_ui())%2;  
			nonZero+=b[l]; // count the ones
			l++;
			nn = nn>>1;
		}
		b[nsize]=0;

		ostringstream o;
		for (int i=nsize-1; i>=0; i--)    o << ((int) b[i]);   
		REPORT(DETAILED, "Constant binary is  " << o.str() << " with " << nonZero << " ones"); 

		int needMinusX=0;
		c = new int[nsize+1];

		c[0] = 0;
		for (i=0; i<nsize; i++) {
			if (b[i] + b[i+1] + c[i] >= 2)
				c[i+1] = 1;
			else 
				c[i+1] = 0;
			BoothCode[i] = b[i] + c[i] -2*c[i+1];
			if(BoothCode[i]==-1)
				needMinusX=1;
		}
		BoothCode[nsize] = c[nsize];


		for (i=0; i<=nsize; i++) {
			if (BoothCode[i] != 0) 
				nonZeroInBoothCode ++;
		}

		REPORT(DETAILED, "Booth recoding is  " << printBoothCode(BoothCode, nsize+1)  << " with " << nonZeroInBoothCode << " non-zero digits"); 

		// If there is no savings in terms of additions, discard the Booth code 
		if (nonZeroInBoothCode+needMinusX >= nonZero) {
			REPORT(DETAILED, "Reverting to non-Booth"); 
			for (i=0; i<nsize; i++)
				BoothCode[i] = b[i];
			nonZeroInBoothCode=nonZero;
			REPORT(DETAILED, "Booth recoding is   " << printBoothCode(BoothCode, nsize)  << " with " << nonZeroInBoothCode << " non-zero digits"); 
		}

		delete [] c; delete [] b;
		return nonZeroInBoothCode;
	}







#if 0
	// The simple, sequential version: the DAG is a rake
	void  IntConstMult::buildMultBooth(){
		int k,i;
		ShiftAddOp *z;
		i=0;

		// build the opposite of the input
		ShiftAddOp* MX = new ShiftAddOp(implementation, Neg, implementation->PX);

		while(0==BoothCode[i]) i++;
		// first op is a shift, possibly of 0
		if (1==BoothCode[i]) {
			if(i==0) // no need to add a variable
				z = implementation->PX;
			else {
				z = new ShiftAddOp(implementation, Shift,   implementation->PX, i);
			}
		}
		else {
			if(i==0) 
				z = MX;
			else {
				z = new ShiftAddOp(implementation, Shift,   MX, i);
			}
		}
		i++;

		for (k=1; k<nonZeroInBoothCode; k++) {
			while(0==BoothCode[i]) i++;
			if (1==BoothCode[i]) 
				z = new ShiftAddOp(implementation, Add,   implementation->PX, i,  z);
			else
				z = new ShiftAddOp(implementation, Add,   MX, i,  z);
			i++;
		}
		// Which variable number holds the result?
		implementation->result = z;
	}
#endif

	/** builds the euclidean DAG using the method buildEuclideanDAG. */
	ShiftAddDag* IntConstMult::buildEuclideanTree(const mpz_class n){

		REPORT(DEBUG,"Entering BuildEuclideanTree for "<<n);

		ShiftAddDag *tree_try = new ShiftAddDag(this);

		mpz_t resized_n;
		mpz_init(resized_n);

		mp_bitcnt_t globalShift = mpz_scan1(n.get_mpz_t(),0); //find global shift gatting the last significant non-zero bit. The rest of the constant will be computed recursively.
		mpz_fdiv_q_2exp(resized_n,n.get_mpz_t(),globalShift); //right shift to set the computed constant.

		mpz_class nn = mpz_class(resized_n); //buildEuclideanDag takes an mpz_class in and we can't change n, of course

		tree_try->result = tree_try->provideShiftAddOp(Shift,buildEuclideanDag(nn,tree_try),globalShift); //build recursively

		return tree_try;


	}


	/**this builds a ShiftAddDag built following the decomposition of a constant C as C=AQ+R, with A=(2^i +/- 1) and R<A */
	//FIXME: The end of the recursion is not optimally set. Find the right stop
	// to try: compute the cost at each level for several ways to build the DAG and chose the best (warning: it could be tricky)
	ShiftAddOp* IntConstMult::buildEuclideanDag(const mpz_class n, ShiftAddDag* constant){

		REPORT(FULL,"Entering buildEuclideanDag for "<<n.get_mpz_t());
		ShiftAddOp *finalQ, *pmQ, *finalAQ, *finalR; //*res;
		ShiftAddDag *computeQ, *computeR;
		mpz_t q,d,r,test;

		if (mpz_cmp_ui(n.get_mpz_t(),1)<=0)
		{
			return constant->PX;
		}
		if ( mpz_cmp_ui(n.get_mpz_t(),3)==0 )
			return constant->provideShiftAddOp(Add, constant->PX, 1, constant->PX); //3 and 1 are particular cases, so they are computed separately

		bool too_small = !findBestDivider(n, d, q, r);

		if (too_small){
			//would have to  stay here until we find no more errors in this method...
			throw string("ERROR in IntConstMult::buildEuclideanDag: Tried to divide a constant lower than 3: this will make no sense. Aborting DAG construction\n");
			return NULL;
		};

		mpz_class R=mpz_class(r);
		mpz_class Q=mpz_class(q);

		//this variable is here to know in a relatively easy way if d is 2^i+1 or 2^i-1.
		//	What we do is this: we add 1 to test, which was previously initialized as the same value as d,
		//	the best divider for n. So, if d is 2^i-1, adding 1 increases the size of test.
		//	Otherwise, test's size is not modified. Then we check if it has changed and apply the appropriate case.
		mpz_init_set(test, d);
		mpz_add_ui(test, test, 1);

		// compute Q
		if (mpz_cmp_ui(q,3)<0){ //if q<3
			computeQ = buildMultBoothTreeFromRight(Q); //end of recursion construct the right operator
			finalQ = constant->sadAppend(computeQ);
		}
		else {
			finalQ = buildEuclideanDag( Q, constant ); //else recurse
		}
		//compute lowest part of A
		if( mpz_sizeinbase(test,2) != mpz_sizeinbase(d, 2) ) //if size of test+1 is not equal to size of d, and in this case, d=2^i-1
			pmQ = constant->provideShiftAddOp(Neg, finalQ, 0);
		else
			pmQ = finalQ;

		//compute AQ
		if(!mpz_cmp_ui(d,3)) //3 is a particular case
			finalAQ = constant->provideShiftAddOp(Add,finalQ,1,finalQ);
		else
			finalAQ = constant->provideShiftAddOp(Add, finalQ, mpz_sizeinbase(test,2)-1, pmQ);

		//compute R
		if (mpz_cmp_ui(r,0)==0){
			finalR = NULL;
		}
		else if (mpz_cmp_ui(r,3)<0){ //if r<3
			computeR = buildMultBoothTreeFromRight(R); //end of recursion
			finalR = constant->sadAppend(computeR);
		}
		else{
			finalR = buildEuclideanDag( R, constant ); //else recurse
		}

		if (finalR == NULL)
			return constant->provideShiftAddOp(Shift, finalAQ, 0);

		return constant->provideShiftAddOp(Add, finalAQ, 0, finalR);
	}

	void mpz_add_si(mpz_t &rop, mpz_t op1, signed long int op2){
		if (op2<0)
			mpz_sub_ui(rop, op1, -op2);
		else
			mpz_add_ui(rop, op1, op2);
	}


	/** prepares all the features needed for all the methods building a ShiftAddDag */
	int  IntConstMult::prepareBoothTree(mpz_class &n, ShiftAddDag* &tree_try, ShiftAddOp** &level, ShiftAddOp* &result,
			ShiftAddOp* &MX, unsigned int* &shifts, int &nonZeroInBoothCode, int &globalshift){
		int i,j;

		int nsize = intlog2(n);

		if((mpz_class(1) << (nsize-1)) == n) { // n is a power of two
			REPORT(DEBUG, "Power of two");
			result= tree_try->provideShiftAddOp(Shift, tree_try->PX, intlog2(n)-1);
			globalshift = nsize-1;
			return 1;
		}
		else {
			int* BoothCode;
			BoothCode = new int[nsize+1]; 
			nonZeroInBoothCode = recodeBooth(n, BoothCode);

			i=0;
			while(0==BoothCode[i]) i++;
			globalshift=i;
			if (nonZeroInBoothCode==1) { // a power of two
				if(i==0) // no need to add a variable, because BoothCode=1
					result = tree_try->PX;
				else {
					result = new ShiftAddOp(tree_try, Shift, tree_try->PX, i);
				}
				delete [] BoothCode;
				return 2;
			}
			else { // at least two non-zero bits

				// build the opposite of the input
				if(needsMinusX(BoothCode, nsize))
					MX = new ShiftAddOp(tree_try, Neg, tree_try->PX);

				// fill an initial array with Xs and MXs. Objective is to perform the additions with the right weight.
				level = new ShiftAddOp*[nonZeroInBoothCode];
				shifts = new unsigned int[nonZeroInBoothCode];
				for (j=0; j<nonZeroInBoothCode-1; j++) {
					if (1==BoothCode[i]) // i was previously initialized as the weight of the first non-zero bit
						level[j] = tree_try->PX; // j corresponds to the non-zero bit we are looking at. then we have to set its shift in the shift array
					else
						level[j] = MX; 
					shifts[j] = i - globalshift;
					i++;
					while(0==BoothCode[i]) i++; // set the shift to the right value
				}
				// BoothCode's MSB is always positive, because we perform unsigned multiplication.
				//		For signed multiplication, we just need to negate or not the result.
				// Check this: deduction propagation could increase cost.
				level[j] = tree_try->PX; 
				shifts[j] = i-globalshift;
				delete [] BoothCode;
				return 0;

			}
		}
	}

	/**
	 * Builds a balanced tree.
	 */
	// Assumes: implementation is initialized
	ShiftAddDag*  IntConstMult::buildMultBoothTreeFromRight(mpz_class n){
		ShiftAddDag* tree_try= new ShiftAddDag(this);
		int k,j,nk,nonZeroInBoothCode,globalshift;
		ShiftAddOp *result;
		ShiftAddOp *MX=0;
		ShiftAddOp**  level;
		unsigned int* shifts;


		REPORT(DEBUG, "Entering buildMultBoothTreeFromRight for "<< n);		

		if ( !prepareBoothTree(n,tree_try, level, result, MX, shifts, nonZeroInBoothCode, globalshift) ){
			REPORT(DEBUG, "nonZeroInBoothCode=" <<nonZeroInBoothCode<<" globalshift=" << globalshift);
			k=nonZeroInBoothCode;
			while(k!=1) {
				nk=k>>1;
				for (j=0; j<nk; j++) {

					if (  ( level[2*j+1]==tree_try->PX ) && ( level[2*j]==MX ) && ( (shifts[2*j+1]-shifts[2*j])==2 )  )
						level[j] = tree_try->provideShiftAddOp(Add, tree_try->PX, 1,  tree_try->PX); // the multiplier is three: implementation cost is lower when we do it as ++ than when we do as +0-.
					else
						level[j] = tree_try->provideShiftAddOp(Add, level[2*j+1], (shifts[2*j+1]-shifts[2*j]),  level[2*j]); // construct the addition tree with right weight
					shifts[j] = shifts[2*j];
				}
				if(nk<<1 != k) { // if k_LSB=1
					level[j] = level[2*j];
					shifts[j] = shifts[2*j];
					k=nk+1;
				}
				else 
					k=nk;
			}
			if(globalshift==0)
				result = level[0];
			else
				result= tree_try->provideShiftAddOp(Shift, level[0], globalshift);

			delete level;
			delete shifts;
		}
		REPORT(DETAILED,  "Number of adders: "<<tree_try->saolist.size() );
		tree_try->result=result;
		return tree_try;
	}

	/**
	 * Builds a DAG using the same method as buildMultBoothTreeFromRight,
	 * but starting from the left (MSB)
	 */
	// Assumes: implementation is initialized
	ShiftAddDag*  IntConstMult::buildMultBoothTreeFromLeft(mpz_class n){
		ShiftAddDag* tree_try= new ShiftAddDag(this);
		int k,j,nk,nonZeroInBoothCode,globalshift; 
		ShiftAddOp *result;
		ShiftAddOp *MX=0;
		ShiftAddOp**  level;
		unsigned int* shifts;


		REPORT(DEBUG, "Entering buildMultBoothTreeFromLeft for "<< n);

		if ( !prepareBoothTree(n,tree_try, level, result, MX, shifts, nonZeroInBoothCode, globalshift) ){
			k=nonZeroInBoothCode;
			while(k!=1) {
				nk=k>>1;
				for (j=0; j<nk; j++) {

					if (  ( level[nonZeroInBoothCode-1 - (2*j)]==tree_try->PX ) && ( level[nonZeroInBoothCode-1 - (2*j+1)]==MX ) && ( (shifts[nonZeroInBoothCode-1 - (2*j)]-shifts[nonZeroInBoothCode-1 - (2*j+1)])==2 )  )
						level[nonZeroInBoothCode-1 - (j)] = tree_try->provideShiftAddOp(Add, tree_try->PX, 1, tree_try->PX); // the multiplier is 3. See the explanation above in buildMultBoothTreeFromRight.
					else
						level[nonZeroInBoothCode-1 - (j)] = tree_try->provideShiftAddOp(Add, level[nonZeroInBoothCode-1 - (2*j)], (shifts[nonZeroInBoothCode-1 - (2*j)]-shifts[nonZeroInBoothCode-1 - (2*j+1)]),  level[nonZeroInBoothCode-1 - (2*j+1)]); // construct the addition tree with right weight
					shifts[nonZeroInBoothCode-1 - j] = shifts[nonZeroInBoothCode-1 - (2*j+1)];
				}
				if(nk<<1 != k) { // if k_LSB=1
					level[nonZeroInBoothCode-1 - (j)] = level[nonZeroInBoothCode-1 - (2*j)];
					shifts[nonZeroInBoothCode-1 - (j)] = shifts[nonZeroInBoothCode-1 - (2*j)];
					k=nk+1;
				}
				else 
					k=nk;
			}
			if(globalshift==0)
				result = level[nonZeroInBoothCode-1 - (0)];
			else
				result= tree_try->provideShiftAddOp(Shift, level[nonZeroInBoothCode-1 - (0)], globalshift);

			delete level;
			delete shifts;

		}

		REPORT(DETAILED,  "Number of adders: "<<tree_try->saolist.size() );
		tree_try->result=result;
		return tree_try;
	}

	/**
	 * Builds a DAG starting from both extremities
	 */
	// Assumes: implementation is initialized
	ShiftAddDag*  IntConstMult::buildMultBoothTreeToMiddle(mpz_class n){
		ShiftAddDag* tree_try= new ShiftAddDag(this);
		int k,j,nk,nonZeroInBoothCode,globalshift; 
		ShiftAddOp *result;
		ShiftAddOp *MX=0;
		ShiftAddOp**  level;
		unsigned int* shifts;


		REPORT(DEBUG, "Entering buildMultBoothTreeToMiddle for "<< n);

		if ( !prepareBoothTree(n,tree_try, level, result, MX, shifts, nonZeroInBoothCode, globalshift) ){
			ShiftAddOp **fillLevel;
			unsigned int* shiftsLevel;
			unsigned int sizeOfFillLevel;
			k=nonZeroInBoothCode;

			while(k>3) {
				nk=k>>1;
				if ( (nk<<1) != k )
					sizeOfFillLevel = (k>>1) + 1;
				else
					sizeOfFillLevel = k>>1;

				fillLevel = new ShiftAddOp*[sizeOfFillLevel];
				shiftsLevel = new unsigned int[sizeOfFillLevel];
				for ( j=0; j<(k>>1); j+=2 ) { //we construct the DAG starting from the extreme sides and go to the middle.
					if ((j+1)<(k>>1))
					{
						if (  ( level[j+1]==tree_try->PX ) && ( level[j]==MX ) && ( (shifts[j+1]-shifts[j])==2 )  )
							fillLevel[(j>>1)] = tree_try->provideShiftAddOp(Add, tree_try->PX, 1, tree_try->PX); //multiplier by 3, 'don't wanna be a parrot
						else
							fillLevel[(j>>1)] = tree_try->provideShiftAddOp(Add, level[j+1], shifts[j+1]-shifts[j],level[j]);
						shiftsLevel[(j>>1)] = shifts[j];
					}

					if (  ( level[k-1 -j]==tree_try->PX ) && ( level[k-1 -j-1]==MX ) && ( (shifts[k-1 -j]-shifts[k-1 -j-1])==2 )  )
						fillLevel[sizeOfFillLevel-1 -(j>>1)] = tree_try->provideShiftAddOp(Add, tree_try->PX,1,tree_try->PX);
					else
						fillLevel[sizeOfFillLevel-1 -(j>>1)] = tree_try->provideShiftAddOp(Add, level[k-1 -j],shifts[k-1 -j]-shifts[k-1 -j-1],level[k-1 -j-1]);
					shiftsLevel[sizeOfFillLevel-1 -(j>>1)] = shifts[k-1 -j-1];
				}

				if ( (nk<<1) != k ) { //case were k is odd
					if ( j-1<(k>>1) ){ //the remaining bit is not on the center
						fillLevel[sizeOfFillLevel>>1]=level[nk];
						shiftsLevel[sizeOfFillLevel>>1]=shifts[nk];
					}
					else {//the remaining bit is on the center
						fillLevel[(sizeOfFillLevel>>1)-1]=level[nk-1];
						shiftsLevel[(sizeOfFillLevel>>1)-1]=shifts[nk-1];
					}

				}

				delete level;
				delete [ ] shifts;
				level = fillLevel;
				shifts = shiftsLevel;
				k=sizeOfFillLevel;
			}
			if (k==3){ // three remaining bits at the end
				level[1] = tree_try->provideShiftAddOp(Add, level[1], shifts[1], level[0]);
				level[0] = tree_try->provideShiftAddOp(Add, level[2], shifts[2], level[1]);
			}
			else // one remaining bit at the end
				level[0] = tree_try->provideShiftAddOp(Add, level[1], shifts[1]-shifts[0], level[0]);

			if (globalshift==0)
				result = level[0];
			else
				result = tree_try->provideShiftAddOp(Shift, level[0], globalshift);

			delete level;
			delete [ ] shifts;


		}

		REPORT(DETAILED,  "Number of adders: "<<tree_try->saolist.size() );
		tree_try->result=result;
		return tree_try;
	}

	/**
	 * searches the smallest shift in an array of integers
	 */
	unsigned int searchSmallestShift(unsigned int* shifts, unsigned int size ) {
		if ( size<2 )
			return 0;

		unsigned int smallestShift=shifts[size-1]+1;
		for ( unsigned int i=0; i<size-1; i++ ) {
			if ( (shifts[i+1]-shifts[i]) < smallestShift )
				smallestShift=shifts[i+1]-shifts[i];
		}
		return smallestShift;
	};

	/**
	 * computes the number of buildable ShiftAddOps in an array (there are token in pairs)
	 */
	unsigned int computeFillLevelSize(unsigned int* shifts, unsigned int size, unsigned int shift ) {
		unsigned int sizeOfFillLevel=size;
		if (size>=2) {
			for ( unsigned int i=0; i<size-1; i++ ){
				if ( (shifts[i+1]-shifts[i]) == shift ){
					sizeOfFillLevel--;
					i++;
				}
			}
		}
		return sizeOfFillLevel;
	};



	/**
	 * Builds a balanced tree, starts building the smallest shifts, leave the others and then iterate
	 */
	// Assumes: implementation is initialized
	ShiftAddDag* IntConstMult::buildMultBoothTreeSmallestShifts(mpz_class n){
		ShiftAddDag* tree_try= new ShiftAddDag(this);
		int k,j,nonZeroInBoothCode,globalshift;
		ShiftAddOp *result;
		ShiftAddOp *MX=0;
		ShiftAddOp**  level;
		unsigned int* shifts;


		REPORT(DEBUG, "Entering buildMultBoothTreeSmallestShifts for "<< n);
		//each iteration we search the smallest shift between two non-zero bits
		//	and compute all the operator including this shift, leaving the others for the next step
		if ( !prepareBoothTree(n,tree_try, level, result, MX, shifts, nonZeroInBoothCode, globalshift) ){
			ShiftAddOp **fillLevel;
			unsigned int* shiftsLevel;
			unsigned int sizeOfFillLevel;
			k=nonZeroInBoothCode;
			unsigned int shiftPlace=0;
			unsigned int smallestShift;
			while (k>1)
			{
				smallestShift = searchSmallestShift(shifts, k);
				sizeOfFillLevel = computeFillLevelSize(shifts, k, smallestShift);
				fillLevel = new ShiftAddOp*[sizeOfFillLevel];
				shiftsLevel = new unsigned int[sizeOfFillLevel];

				for ( j=0; j<k; j++ )
				{
					if ( (shifts[j+1]-shifts[j]) == smallestShift )
					{//if the two bits we are looking at are distanced by the smallest shift, then construct the operator
						if ( (level[j+1]==tree_try->PX) && (level[j]==MX) && (smallestShift==2) ) //3, you know what
							fillLevel[j-shiftPlace]=tree_try->provideShiftAddOp( Add, tree_try->PX, 1, tree_try->PX);
						else
							fillLevel[j-shiftPlace]=tree_try->provideShiftAddOp( Add, level[j+1], smallestShift, level[j] );
						shiftsLevel[j-(shiftPlace++)]=shifts[j];
						j++;
					}
					else {//else copy the non zero bit to the next step
						fillLevel[j-shiftPlace]=level[j];
						shiftsLevel[j-shiftPlace]=shifts[j];
					}
				}
				k=sizeOfFillLevel;
				shiftPlace=0;
				delete level;
				delete [ ] shifts;
				level=fillLevel;
				shifts=shiftsLevel;
			}


			if (globalshift==0)
				result = level[0];
			else
				result = tree_try->provideShiftAddOp(Shift, level[0], globalshift);

			delete level;
			delete [ ] shifts;

		}

		REPORT(DETAILED,  "Number of adders: "<<tree_try->saolist.size() );
		tree_try->result=result;
		return tree_try;
	}


	void IntConstMult::showShiftAddDag(){
		REPORT(DETAILED, " ShiftAddDag:");
		for (uint32_t i=0; i<implementation->saolist.size(); i++) {
			REPORT(DETAILED, "  "<<*(implementation->saolist[i]));
		}
	};




	/**
	 * Builds an implementation tree, using bitheaps
	 */
	// Assumes: implementation is initialized
	ShiftAddDag* IntConstMult::buildMultBoothTreeBitheap(mpz_class n, int levels)
	{
		ShiftAddDag* tree_try= new ShiftAddDag(this);
		int k,j,nk,nonZeroInBoothCode;
		int globalshift = 0;
		ShiftAddOp *result = NULL;
		ShiftAddOp *MX=0;
		ShiftAddOp**  level;
		unsigned int* shifts;
		vector<int> tempHeadShifts;

		REPORT(DEBUG, "Entering buildMultBoothTreeBitheap for "<< n);

		if ( !prepareBoothTree(n,tree_try, level, result, MX, shifts, nonZeroInBoothCode, globalshift) )
		{
			REPORT(DEBUG, "nonZeroInBoothCode=" <<nonZeroInBoothCode<<" globalshift=" << globalshift);
			k=nonZeroInBoothCode;

			//if there is no reutilization
			if(levels == 0)
			{
				for(int count=0; count<k; count++)
					tree_try->saoHeadlist.push_back(level[count]);
				for(int count=0; count<k; count++)
					tree_try->saoHeadShift.push_back(shifts[count]+globalshift);
				return tree_try;
			}

			//initialize the shifts of the heads of the DAG
			for(int count=0; count<k; count++)
				tempHeadShifts.push_back(shifts[count]+globalshift);
			if(k == 1)
			{
				tree_try->saoHeadShift = tempHeadShifts;
			}

			while((k!=1) && (levels>0))
			{
				//reset the delays for the heads of the DAG
				tree_try->saoHeadShift.clear();

				nk=k>>1;
				for(j=0; j<nk; j++)
				{
					if (  ( level[2*j+1]==tree_try->PX ) && ( level[2*j]==MX ) && ( (shifts[2*j+1]-shifts[2*j])==2 )  )
						// the multiplier is three: implementation cost is lower when we do it as ++ than when we do as +0-.
						level[j] = tree_try->provideShiftAddOp(Add, tree_try->PX, 1,  tree_try->PX);
					else
						// construct the addition tree with right weight
						level[j] = tree_try->provideShiftAddOp(Add, level[2*j+1], (shifts[2*j+1]-shifts[2*j]),  level[2*j]);

					level[2*j]->parentList.clear();
					level[2*j]->parentList.push_back(level[j]);
					level[2*j+1]->parentList.clear();
					level[2*j+1]->parentList.push_back(level[j]);

					shifts[j] = shifts[2*j];

					tree_try->saoHeadShift.push_back(tempHeadShifts[2*j]);
				}
				if(nk<<1 != k)
				{
					// if k_LSB=1
					level[j] = level[2*j];
					shifts[j] = shifts[2*j];
					k=nk+1;

					tree_try->saoHeadShift.push_back(tempHeadShifts[2*j]);
				}
				else
					k=nk;

				levels--;
				tempHeadShifts = tree_try->saoHeadShift;
			}

			//update the DAG head list
			tree_try->saoHeadlist.clear();
			for(j=0; j<k; j++)
			{
				tree_try->saoHeadlist.push_back(level[j]);
			}

			delete level;
			delete shifts;
		}
		else
		{
			tree_try->saoHeadlist.push_back(result);
			tree_try->saoHeadShift.push_back(globalshift);
		}
		REPORT(DETAILED, "Number of adders: " << tree_try->saolist.size());

		//there's no need for the result, as there might several heads for the DAG (stored in saoHeadlist)
		tree_try->result = NULL;

		return tree_try;
	}








	void optimizeLefevre(const vector<mpz_class>& constants) {


	};


	void IntConstMult::emulate(TestCase *tc){
		mpz_class svX = tc->getInputValue("X");
		mpz_class svR = svX * n;
		tc->addExpectedOutput("R", svR);
	}



	void IntConstMult::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;

		tc = new TestCase(this); 
		tc->addInput("X", mpz_class(0));
		emulate(tc);
		tc->addComment("Multiplication by 0");
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addInput("X", mpz_class(1));
		emulate(tc);
		tc->addComment("Multiplication by 1");
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addInput("X", mpz_class(2));
		emulate(tc);
		tc->addComment("Multiplication by 2");
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addInput("X", (mpz_class(1) << xsize) -1);
		emulate(tc);
		tc->addComment("Multiplication by the max positive value");
		tcl->add(tc);

		//	tc = new TestCase(this); 
		//	tc->addInput("X", (mpz_class(1) << (xsize) -1) + mpz_class(1));
		//	emulate(tc);
		//	tc->addComment("Multiplication by 10...01");
		//	tcl->add(tc);


	}





	// if index==-1, run the unit tests, otherwise just compute one single test state  out of index, and return it
	TestList IntConstMult::unitTest(int index)
	{
		throw ("TestList IntConstDiv::unitTest : TODO, plz FIXME");
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;

		return testStateList;
	}



	OperatorPtr IntConstMult::parseArguments(Target *target, vector<string> &args) {
		int wIn;
		string	n;
		UserInterface::parseStrictlyPositiveInt(args, "wIn", &wIn); 
		UserInterface::parseString(args, "n", &n);
		mpz_class nz(n); // TODO catch exceptions here?
		return new IntConstMult(target, wIn, nz);
	}

	void IntConstMult::registerFactory(){
		UserInterface::add("IntConstMult", // name
			"Integer multiplier by a constant using a shift-and-add tree.",
			"ConstMultDiv",
											 "FixRealKCM,IntConstDiv", // seeAlso
											 "wIn(int): input size in bits; \
											 n(int): constant to multiply by",
											 "An early version of this operator is described in <a href=\"bib/flopoco.html#BrisebarreMullerDinechin2008:ASAP\">this article</a>.",
											 IntConstMult::parseArguments,
											 IntConstMult::unitTest
											 ) ;
	}


}
