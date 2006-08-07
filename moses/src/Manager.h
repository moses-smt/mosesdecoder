// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#pragma once

#include <vector>
#include <list>
#include "Input.h"
#include "Hypothesis.h"
#include "StaticData.h"
#include "TranslationOption.h"
#include "HypothesisCollection.h"
#include "TranslationOptionCollection.h"
#include "LatticePathList.h"
#include "SquareMatrix.h"
#include "WordsBitmap.h"
//#include "UnknownWordHandler.h"

class LatticePath;
class TranslationOptionCollection;

/** The Manager class implements a stack decoding algorithm.
  * Hypotheses are organized in stacks. One stack contains all hypothesis that have 
  * the same number of foreign words translated.  The data structure for hypothesis 
  * stacks is the class HypothesisCollection. The data structure for a hypothesis 
  * is the class Hypothesis. 
  *
  * The main decoder loop in the function ProcessSentence() consists of the steps: 
  * - Create the list of possible translation options. In phrase-based decoding 
  *   (and also the first mapping step in the factored model) is a phrase translation 
  *   from the source to the target. Given a specific input sentence, only a limited 
  *   number of phrase translation can be applied. For efficient lookup of the 
  *   translation options later, these optuions are first collected in the function 
  *   CreateTranslationOption (for more information check the class 
  *   TranslationOptionCollection) 
  * - Create initial hypothesis: Hypothesis stack 0 contains only one empty hypothesis. 
  * - Going through stacks 0 ... (sentence_length-1): 
  *   - The stack is pruned to the maximum size 
  *   - Going through all hypotheses in the stack 
  *     - Each hypothesis is expanded by ProcessOneHypothesis() 
  *     - Expansion means applying a translation option to the hypothesis to create 
  *       new hypotheses 
  *     - What translation options may be applied depends on reordering limits and 
  *       overlap with already translated words 
  *     - With a applicable translation option and a hypothesis at hand, a new 
  *       hypothesis can be created in ExpandHypothesis() 
  *     - New hypothesis are either discarded (because they are too bad), added to 
  *       the appropriate stack, or re-combined with existing hypotheses 
 **/

class Manager
{
  Manager();
  Manager(Manager const&);
  void operator=(Manager const&);
protected:	
	// data
	InputType const& m_source; /**< source sentence to be translated */

	std::vector < HypothesisCollection > m_hypoStack; /**< stacks to store hypothesis (partial translations) */ 
		// no of elements = no of words in source + 1
	StaticData &m_staticData; /**< holds various kinds of constants, counters, and global data structures */
	TranslationOptionCollection &m_possibleTranslations; /**< pre-computed list of translation options for the phrases in this sentence */
	TargetPhrase m_initialTargetPhrase; /**< used to seed 1st hypo */
	
	// functions for creating hypotheses
	void ProcessOneHypothesis(const Hypothesis &hypothesis);
	void ExpandAllHypotheses(const Hypothesis &hypothesis,const TranslationOptionList &transOptList);
	void ExpandHypothesis(const Hypothesis &hypothesis,const TranslationOption &transOpt);

	// logging
	void OutputHypoStack(int stack = -1);
	void OutputHypoStackSize();
public:
	Manager(InputType const& source, StaticData &staticData);
	~Manager();

	void ProcessSentence();
	const Hypothesis *GetBestHypothesis() const;
	void CalcNBest(size_t count, LatticePathList &ret) const;
};

