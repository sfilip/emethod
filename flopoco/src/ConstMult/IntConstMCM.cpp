/*
 * IntConstMCM.cpp
 *
 * An multiple constant multiplier for FloPoCo,
 * based on shift-and-add and bitheaps.
 *
 *  Created on: Mar 24, 2015
 *      Author: mistoan
 */

#include "IntConstMCM.hpp"

namespace flopoco {


	IntConstMCM::IntConstMCM(Target* target_, int xsize_, int nbConst_, vector<mpz_class> constants_) :
							IntConstMult(target_, xsize_), nbConst(nbConst_), constants(constants_)
	{
		ostringstream name;

		srcFileName="IntConstMCM";

		setCopyrightString("Florent de Dinechin, Matei Istoan (2015)");

		//C++ wrapper for GMP does not work properly on win32, using mpz2string
		name <<"IntConstMCM_" << xsize;
		for(int i=0; i<nbConst; i++)
			name << "_" << mpz2string(constants[i]);
		setNameWithFreqAndUID(name.str());

		//add the input
		addInput("X", xsize);

		//prepare the constants and the sizes of the results
		nbConstZero = 0;
		for(int i=0; i<nbConst; i++)
		{
			if(constants[i] == 0)
			{
				REPORT(LIST, "Here I am, brain the size of a planet and they ask me to multiply by zero. Call that job satisfaction? 'Cos I don't.");
				REPORT(LIST, "Detected constant equal; will not use it when generating the operator.");
				//increasig the number of inputs equal to zero
				nbConst--;
				nbConstZero++;
				constants.erase(constants.begin()+i);
				i--;

				//add a new output of zero
				addOutput(join("R", i), 1); // multiplier by zero is always zero
				vhdl << tab << "R" << i << " <= \"0\";" <<endl;
			}else
			{
				//add a new output
				int tempRSize;
				tempRSize = intlog2(constants[i] * ((mpz_class(1)<<xsize)-1));
				rsizes.push_back(tempRSize);

				//add a new output of zero
				addOutput(join("R", i), tempRSize);
			}
		}

		//create the forests of trees corresponding to the multiplication by each constant
		for(int i=0; i<nbConst; i++)
		{
			implementation = buildMultBoothTreeBitheap(constants[i], 2);
			implementations.push_back(implementation);
		}

		//merge the trees
		mergeTrees();

		//save the critical path
		double tempCriticalPath = getCriticalPath();

		//create the logic for each of the trees
		double delay;		//used for pipelining
		for(int i=0; i<nbConst; i++)
		{
			//set the critical path
			setCycleFromSignal("X");
			setCriticalPath(tempCriticalPath);

			//create the bitheap that computes the sum
			bitheap = new BitHeap(this, rsizes[i]+1);
			bitheaps.push_back(bitheap);

			for(int j=0; (unsigned)j<implementations[i]->saoHeadlist.size(); j++)
			{
				delay = 0.0;
				build_pipeline(implementations[i]->saoHeadlist[j], delay);

				//add the bits to the bitheap
				vhdl << endl << tab << "-- adding " << implementations[i]->saoHeadlist[j]->name
						<< " shifted by " << implementations[i]->saoHeadShift[j] << " to the bitheap" << endl;
				for(int k=0; k<implementations[i]->saoHeadlist[j]->size-1; k++)
				{
					bitheap->addBit(implementations[i]->saoHeadShift[j]+k,
							join(implementations[i]->saoHeadlist[j]->name, of(k)));
				}
				//sign extend if necessary
				if(implementations[i]->saoHeadlist[j]->n < 0)
				{
					ostringstream tmpStr;

					tmpStr << "not(" << implementations[i]->saoHeadlist[j]->name << of(implementations[i]->saoHeadlist[j]->size-1) << ")";
					bitheap->addBit(implementations[i]->saoHeadShift[j]+implementations[i]->saoHeadlist[j]->size-1,
							tmpStr.str());
					for(int k=implementations[i]->saoHeadlist[j]->size-1; k<(rsizes[i]+1); k++)
					{
						bitheap->addConstantOneBit(implementations[i]->saoHeadShift[j]+k);
					}
				}
				else
				{
					bitheap->addBit(implementations[i]->saoHeadShift[j]+implementations[i]->saoHeadlist[j]->size-1,
							join(implementations[i]->saoHeadlist[j]->name, of(implementations[i]->saoHeadlist[j]->size-1)));
				}
			}

			//compress the bitheap
			bitheap->generateCompressorVHDL();

			// copy the top of the DAG into variable Ri
			vhdl << endl << tab << "R" << i << " <= " << bitheap->getSumName() << range(rsizes[i]-1, 0) << ";" << endl;
			outDelayMap[join("R", i)] = getCriticalPath();
		}

	}


	IntConstMCM::~IntConstMCM()
	{
		delete implementation;
	}


	void IntConstMCM::mergeTrees()
	{
		vector<ShiftAddDag*> newImplementations;

		//nothing to do if there are no trees in implementations
		if(implementations.size() == 0)
		{
			return;
		}

		newImplementations.push_back(implementations[0]);

		for(int i=1; (unsigned)i<implementations.size(); i++)
		{
			//select a new element from the current implementations list,
			ShiftAddDag* tempDag = implementations[i];

			//try to see if the elements of tempDag already exists in the other ShiftAddDags
			//	go through each tree in the forest, and check each of its elements,
			//	replacing the reference to it with a reference to the already existing element
			for(int j=0; (unsigned)j<tempDag->saoHeadlist.size(); j++)
			{
				//check if the root of the current tree already exists
				//	if yes, replace it with the reference to the existing node
				//	if not, start the tree traversal
				//do this process for all the forests in newImplementations
				for(int k=0; (unsigned)k<newImplementations.size(); k++)
				{
					ShiftAddOp* searchResult;

					searchResult = sadExistsInForest(newImplementations[k], tempDag->saoHeadlist[j]);
					if(searchResult != NULL)
					{
						//try to reuse an already implemented negative version of the node
						if(tempDag->saoHeadlist[j]->n == (searchResult->n*(-1)))
						{
							//the node we're trying to replace has a negative equivalent
							//	build an intermediary node that negates it
							ShiftAddOp* searchResultNegative = new ShiftAddOp(tempDag, ShiftAddOpType::Neg, searchResult, 0, NULL);
							searchResult->parentList.push_back(searchResultNegative);
							tempDag->saoHeadlist[j] = searchResultNegative;
							break;
						}else
						{
							//the node itself already exists in the tree
							tempDag->saoHeadlist[j] = searchResult;
							break;
						}
					}else
					{
						if(tempDag->saoHeadlist[j]->i != NULL)
							replaceSadInForest(newImplementations[k], tempDag->saoHeadlist[j]->i);
						if(tempDag->saoHeadlist[j]->j != NULL)
							replaceSadInForest(newImplementations[k], tempDag->saoHeadlist[j]->j);
					}
				}
			}

			//try to eliminate negative/positive couples of nodes from the new forest itself
			for(int j=0; (unsigned)j<tempDag->saoHeadlist.size(); j++)
			{
				//check if the root of the current tree already exists
				//	if yes, replace it with the reference to the existing node
				//	if not, start the tree traversal
				//do this process for all the forests in newImplementations
				ShiftAddOp* searchResult;

				searchResult = sadExistsInForest(tempDag, tempDag->saoHeadlist[j], tempDag->saoHeadlist[j]);
				if((searchResult != NULL) && (searchResult != tempDag->saoHeadlist[j]))
				{
					//try to reuse an already implemented version of the node with the opposite sign
					if(tempDag->saoHeadlist[j]->n == (searchResult->n*(-1)))
					{
						//the node we're trying to replace has an opposite sign equivalent
						//	build an intermediary node that negates it
						//keep the positive node
						if(searchResult->n > 0)
						{
							//keep the newly found node
							ShiftAddOp* searchResultNegative = new ShiftAddOp(tempDag, ShiftAddOpType::Neg, searchResult, 0, NULL);
							searchResult->parentList.push_back(searchResultNegative);
							tempDag->saoHeadlist[j] = searchResultNegative;
							break;
						}else
						{
							//replace the found node with the current node
							ShiftAddOp* searchResultNegative = new ShiftAddOp(tempDag, ShiftAddOpType::Neg, tempDag->saoHeadlist[j], 0, NULL);
							tempDag->saoHeadlist[j]->parentList.push_back(searchResultNegative);

							for(int k=0; (unsigned)k<searchResult->parentList.size(); k++)
							{
								ShiftAddOp* parent = searchResult->parentList[k];

								if(parent->i == searchResult)
									parent->i = searchResultNegative;
								else
									parent->j = searchResultNegative;
							}

							break;
						}
					}else
					{
						tempDag->saoHeadlist[j] = searchResult;
						break;
					}
				}else
				{
					if(tempDag->saoHeadlist[j]->i != NULL)
						replaceSadInForest(tempDag, tempDag->saoHeadlist[j]->i, tempDag->saoHeadlist[j]);
					if(tempDag->saoHeadlist[j]->j != NULL)
						replaceSadInForest(tempDag, tempDag->saoHeadlist[j]->j, tempDag->saoHeadlist[j]);
				}
			}


			//add the new forest to the new list of forests of trees
			newImplementations.push_back(tempDag);
		}

		//update the implementations with the merged version
		implementations = newImplementations;
	}

	void IntConstMCM::replaceSadInForest(ShiftAddDag* forest, ShiftAddOp* root, ShiftAddOp* searchLimit)
	{
		if((root == NULL) || (forest == NULL) || (forest->saoHeadlist.size() == 0))
			return;

		ShiftAddOp* searchResult;

		searchResult = sadExistsInForest(forest, root, searchLimit);
		if((searchResult == NULL) || (searchResult == root))
		{
			//the node does not already exist in any of the other trees,
			//or we found a reference to the same node
			//	try to replace it's children, recursively
			if(root->i != NULL)
				replaceSadInForest(forest, root->i, searchLimit);
			if(root->j != NULL)
				replaceSadInForest(forest, root->j, searchLimit);
		}else
		{
			//the node already exists in one of the previous trees
			//	replace the child of the parent with the reference to the existing already node
			//keep the positive node
			ShiftAddOp *replacedNode, *replacementNode;

			if(searchResult->n < 0)
			{
				replacedNode = searchResult;
				replacementNode = root;
			}else
			{
				replacedNode = root;
				replacementNode = searchResult;
			}

			for(int i=0; (unsigned)i<replacedNode->parentList.size(); i++)
			{
				ShiftAddOp* parent = replacedNode->parentList[i];

				if((parent->i != NULL) && ((parent->i->n == replacementNode->n) || (parent->i->n == ((-1)*replacementNode->n))))
				{
					//check if the node that we're replacing is the child i of the parent
					if(parent->i->n == root->n)
					{
						//the node itself already exists
						parent->i = replacementNode;
						return;
					}else
					{
						//the node we're trying to replace has a negative equivalent
						//	build an intermediary node that negates it
						ShiftAddOp* replacementNodeNegative = new ShiftAddOp(forest, ShiftAddOpType::Neg, replacementNode, 0, NULL);
						replacementNode->parentList.push_back(replacementNodeNegative);
						parent->i = replacementNodeNegative;
						return;
					}
				}else if((parent->j != NULL) && ((parent->j->n == replacementNode->n) || (parent->j->n == ((-1)*replacementNode->n))))
				{
					//check if the node that we're replacing is the child j of the parent
					if(parent->j->n == root->n)
					{
						//the node itself already exists
						parent->j = replacementNode;
						return;
					}else
					{
						//the node we're trying to replace has a negative equivalent
						//	build an intermediary node that negates it
						ShiftAddOp* replacementNodeNegative = new ShiftAddOp(forest, ShiftAddOpType::Neg, replacementNode, 0, NULL);
						replacementNode->parentList.push_back(replacementNodeNegative);
						parent->j = replacementNodeNegative;
						return;
					}
				}else
				{
					//node detected as existing, but none of the children of the parent match the node
					//	THIS SHOULD NEVER HAPPEN
					THROWERROR("Error in replaceSadInForest: node does not match any of the children of its parent");
				}
			}
		}
	}

	ShiftAddOp* IntConstMCM::sadExistsInForest(ShiftAddDag* forest, ShiftAddOp* node, ShiftAddOp* searchLimit)
	{
		ShiftAddOp* foundNode = NULL;

		if((forest == NULL) || (forest->saoHeadlist.size() == 0) || (node == NULL))
			return NULL;

		for(int i=0; (((unsigned)i<forest->saoHeadlist.size()) && (foundNode == NULL)); i++)
		{
			if(forest->saoHeadlist[i] == NULL)
				continue;
			if((searchLimit != NULL) && (forest->saoHeadlist[i] == searchLimit))
				break;

			foundNode = sadExistsInTree(forest->saoHeadlist[i], node);
		}

		return foundNode;
	}

	ShiftAddOp* IntConstMCM::sadExistsInTree(ShiftAddOp* tree, ShiftAddOp* node)
	{
		ShiftAddOp *left, *right;

		if((tree == NULL) || (node == NULL))
			return NULL;

		if(tree->n == node->n)
			return tree;

		left = sadExistsInTree(tree->i, node);
		right = sadExistsInTree(tree->j, node);

		if((left != NULL) && (left->n == node->n))
			return left;
		else if((right != NULL) && (right->n == node->n))
			return right;
		else if(tree->n == (node->n*(-1)))
			return tree;
		else if((left != NULL) && (left->n == (node->n *(-1))))
			return left;
		else if((right != NULL) && (right->n == (node->n *(-1))))
			return right;
		else
			return NULL;
	}




	void IntConstMCM::emulate(TestCase *tc){
		mpz_class svX = tc->getInputValue("X");

		for(int i=0; i<nbConst; i++)
		{
			mpz_class svR = svX * constants[i];
			tc->addExpectedOutput(join("R", i), svR);
		}
	}



	void IntConstMCM::buildStandardTestCases(TestCaseList* tcl){
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
	}


}




























