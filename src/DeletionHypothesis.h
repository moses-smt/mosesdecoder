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

#include "Sentence.h"
#include "WordsBitmap.h"
#include "WordsRange.h"
#include "WordDeletionTable.h"
#include "StaticData.h"
#include "Hypothesis.h"
#include "TranslationOption.h"

/***
 * Describe a hypothesis extension that involves translating a source phrase to the empty phrase
 * (ie dropping the source words)
 */
class DeletionHypothesis : public Hypothesis
{
	friend class Hypothesis; //for the factory functions
	
	protected:
	
		DeletionHypothesis(const WordsBitmap &initialCoverage) : Hypothesis(Phrase(), initialCoverage) {}
		DeletionHypothesis(const Hypothesis &prevHypo, const TranslationOption &transOpt) : Hypothesis(prevHypo, transOpt) {}
		virtual ~DeletionHypothesis() {}
	
		/***
		 * calculate the score due to source words dropped; set the appropriate elements of m_score
		 */
		void CalcDeletionScore(const Sentence& sourceSentence, const WordsRange& sourceWordsRange, const WordDeletionTable& wordDeletionTable);
		
		/***
		 * Set the total-score field from the various individual score parts
		 * (not necessarily using all of them)
		 */
		virtual void SumIndividualScores(const StaticData& staticData);
	
	public:
		
		virtual void CalcScore(const StaticData& staticData, const SquareMatrix &futureScore, const Sentence &source);
};
