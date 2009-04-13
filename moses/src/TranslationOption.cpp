// $Id$
// vim:tabstop=2

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

#include "TranslationOption.h"
#include "WordsBitmap.h"
#include "PhraseDictionaryMemory.h"
#include "GenerationDictionary.h"
#include "LMList.h"
#include "StaticData.h"
#include "InputType.h"
#include "DecodeStepTranslation.h"

using namespace std;

//TODO this should be a factory function!
TranslationOption::TranslationOption(const WordsRange &wordsRange
																		, const TargetPhrase &targetPhrase
																		, const InputType &inputType
																		, size_t decodeStepId
																		, const DecodeGraph &decodeGraph)
: m_targetPhrase(targetPhrase)
, m_sourceWordsRange(wordsRange)
, m_subRangeCount(DecodeStepTranslation::GetNumTransStep(), 0)
{
	m_decodeGraphId = decodeGraph.GetId();

	// set score
	m_scoreBreakdown.PlusEquals(targetPhrase.GetScoreBreakdown());

	if (inputType.GetType() == SentenceInput)
	{
		Phrase phrase = inputType.GetSubString(wordsRange);
		m_sourcePhrase = new Phrase(phrase);
	}
	else
	{ // TODO lex reordering with confusion network
		m_sourcePhrase = new Phrase(*targetPhrase.GetSourcePhrase());
	}

	m_subRangeCount[decodeStepId] += targetPhrase.GetSubRangeCount();
	CalcScore();
}
//TODO if (! hieu.isPartying && newYearsEve()){hieu.goParty(beer);}
//TODO this should be a factory function!
TranslationOption::TranslationOption( int /*whatever*/
																		 , const WordsRange &wordsRange
																		 , const TargetPhrase &targetPhrase
																		 , const InputType &inputType)
: m_targetPhrase(targetPhrase)
, m_sourceWordsRange	(wordsRange)
, m_subRangeCount(DecodeStepTranslation::GetNumTransStep(), 0)
{
	const UnknownWordPenaltyProducer *up = StaticData::Instance().GetUnknownWordPenaltyProducer();
  if (up) {
		const ScoreProducer *scoreProducer = (const ScoreProducer *)up; // not sure why none of the c++ cast works
		vector<float> score(1);
		score[0] = FloorScore(-numeric_limits<float>::infinity());
		m_scoreBreakdown.Assign(scoreProducer, score);
	}

	if (inputType.GetType() == SentenceInput)
	{
		Phrase phrase = inputType.GetSubString(wordsRange);
		m_sourcePhrase = new Phrase(phrase);
	}
	else
	{ // TODO lex reordering with confusion network
		m_sourcePhrase = new Phrase(*targetPhrase.GetSourcePhrase());
	}

	// hack. prob should increment all decode step
	m_subRangeCount[0] += targetPhrase.GetSubRangeCount();
	CalcScore();
}

TranslationOption::TranslationOption(const TranslationOption &copy)
: m_targetPhrase(copy.m_targetPhrase)
//, m_sourcePhrase(new Phrase(*copy.m_sourcePhrase)) // TODO use when confusion network trans opt for confusion net properly implemented 
, m_sourcePhrase( (copy.m_sourcePhrase == NULL) ? new Phrase(Input) : new Phrase(*copy.m_sourcePhrase))
, m_sourceWordsRange(copy.m_sourceWordsRange)
, m_futureScore(copy.m_futureScore)
, m_scoreBreakdown(copy.m_scoreBreakdown)
, m_reordering(copy.m_reordering)
, m_subRangeCount(copy.m_subRangeCount)
{}

TranslationOption::TranslationOption(const TranslationOption &copy, const WordsRange &sourceWordsRange)
: m_targetPhrase(copy.m_targetPhrase)
//, m_sourcePhrase(new Phrase(*copy.m_sourcePhrase)) // TODO use when confusion network trans opt for confusion net properly implemented 
, m_sourcePhrase( (copy.m_sourcePhrase == NULL) ? new Phrase(Input) : new Phrase(*copy.m_sourcePhrase))
, m_sourceWordsRange(sourceWordsRange)
, m_futureScore(copy.m_futureScore)
, m_scoreBreakdown(copy.m_scoreBreakdown)
, m_reordering(copy.m_reordering)
, m_subRangeCount(copy.m_subRangeCount)
{}

void TranslationOption::MergeTargetPhrase(const TargetPhrase &targetPhrase
																				 , const ScoreComponentCollection& score
																				 , const std::vector<FactorType>& featuresToAdd
																				 , size_t decodeStepId)
{
  const Phrase &phrase = static_cast<const Phrase&>(targetPhrase);
  MergePhrase(phrase, score, featuresToAdd);

  size_t sourceSize = m_targetPhrase.GetAlignmentPair().GetAlignmentPhrase(Input).GetSize();
  m_targetPhrase.GetAlignmentPair().Merge(targetPhrase.GetAlignmentPair()
                                        , WordsRange(0, sourceSize - 1) 
                                        , WordsRange(0, GetTargetSize() - 1));

	m_subRangeCount[decodeStepId] += targetPhrase.GetSubRangeCount();
	CalcScore();
}

void TranslationOption::MergePhrase(const Phrase& phrase
																				 , const ScoreComponentCollection& score
																				 , const std::vector<FactorType>& featuresToAdd)
{
	assert(phrase.GetSize() == m_targetPhrase.GetSize());
	if (featuresToAdd.size() == 1) 
  {
		m_targetPhrase.MergeFactors(phrase, featuresToAdd[0]);
	}
  else if (featuresToAdd.empty()) 
  {	/* features already there, just update score */ 
  }
  else 
  {
		m_targetPhrase.MergeFactors(phrase, featuresToAdd);
	}
	m_scoreBreakdown.PlusEquals(score);
}

bool TranslationOption::Overlap(const Hypothesis &hypothesis) const
{
	const WordsBitmap &bitmap = hypothesis.GetWordsBitmap();
	return bitmap.Overlap(GetSourceWordsRange());
}

void TranslationOption::CalcScore()
{
	// LM scores
	float ngramScore = 0;
	float retFullScore = 0;

	const LMList &allLM = StaticData::Instance().GetAllLM();

	allLM.CalcScore(GetTargetPhrase(), retFullScore, ngramScore, &m_scoreBreakdown);

	size_t phraseSize = GetTargetPhrase().GetSize();
	// future score
	m_futureScore = retFullScore - ngramScore
								+ m_scoreBreakdown.InnerProduct(StaticData::Instance().GetAllWeights()) - phraseSize * StaticData::Instance().GetWeightWordPenalty();
}

void TranslationOption::CacheReorderingProb(const LexicalReordering &lexreordering
												, const Score &score)
{
	m_reordering.Assign(&lexreordering, score);
}

TO_STRING_BODY(TranslationOption);

// friend
ostream& operator<<(ostream& out, const TranslationOption& transOpt)
{
	out << transOpt.GetTargetPhrase().GetStringRep()
			<< "c=" << transOpt.GetFutureScore()
			<< " [" << transOpt.GetSourceWordsRange() << "]"
			<< transOpt.GetScoreBreakdown() << " " 
			<< transOpt.GetAlignmentPair();
	return out;
}
