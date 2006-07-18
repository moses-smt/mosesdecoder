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

#include "DeletionHypothesis.h"



/***
 * calculate the score due to source words dropped; set the appropriate elements of m_score
 */
void DeletionHypothesis::CalcDeletionScore(const Sentence& sourceSentence, const WordsRange& sourceWordsRange, const WordDeletionTable& wordDeletionTable)
{
	m_score[ScoreType::DeletedWords] = wordDeletionTable.GetDeletionCost(sourceSentence.GetSubString(sourceWordsRange));
}

/***
 * Set the total-score field from the various individual score parts
 * (not necessarily using all of them)
 */
void DeletionHypothesis::SumIndividualScores(const StaticData& staticData)
{
	m_score[ScoreType::Total] = m_score[ScoreType::PhraseTrans]
								+ m_score[ScoreType::Generation]			
								+ m_score[ScoreType::LanguageModelScore]
								+ m_score[ScoreType::Distortion]					* staticData.GetWeightDistortion()
								+ m_score[ScoreType::WordPenalty]				* staticData.GetWeightWordPenalty()
								+ m_score[ScoreType::DeletedWords]
								+ m_score[ScoreType::FutureScoreEnum];
}

void DeletionHypothesis::CalcScore(const StaticData& staticData, const SquareMatrix &futureScore, const Sentence &source)
{
	// DISTORTION COST
	CalcDistortionScore();
	
	// LANGUAGE MODEL COST
	CalcLMScore(staticData.GetLanguageModel(Initial), staticData.GetLanguageModel(Other));

	// WORD PENALTY
	m_score[ScoreType::WordPenalty] = - (float) GetSize();

	// FUTURE COST
	CalcFutureScore(futureScore);
	
	//cost for deleting source words
//	CalcDeletionScore(source, GetCurrSourceWordsRange(), staticData.GetWordDeletionTable());
	
	//LEXICAL REORDERING COST
	CalcLexicalReorderingScore();

	// TOTAL COST
	SumIndividualScores(staticData);
}
