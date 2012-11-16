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

#include <algorithm>
#include <stdlib.h>
#include "util/check.hh"
#include "util/exception.hh"
#include "util/tokenize_piece.hh"

#include "TargetPhrase.h"
#include "PhraseDictionaryMemory.h"
#include "GenerationDictionary.h"
#include "LM/Base.h"
#include "StaticData.h"
#include "LMList.h"
#include "ScoreComponentCollection.h"
#include "Util.h"
#include "DummyScoreProducers.h"
#include "AlignmentInfoCollection.h"

using namespace std;

namespace Moses
{
TargetPhrase::TargetPhrase( std::string out_string)
  :Phrase(0), m_fullScore(0.0), m_sourcePhrase(0)
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
{

  //ACAT
  const StaticData &staticData = StaticData::Instance();
  CreateFromString(staticData.GetInputFactorOrder(), out_string, staticData.GetFactorDelimiter());
}


TargetPhrase::TargetPhrase()
  :Phrase()
  , m_fullScore(0.0)
  ,m_sourcePhrase()
	, m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
	, m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
{
}

TargetPhrase::TargetPhrase(const Phrase &phrase)
  : Phrase(phrase)
  , m_fullScore(0.0)
  , m_sourcePhrase()
	, m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
	, m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
{
}

void TargetPhrase::SetScore(const TranslationSystem* system)
{
  // used when creating translations of unknown words:
  m_fullScore = - system->GetWeightWordPenalty();
}

#ifdef HAVE_PROTOBUF
void TargetPhrase::WriteToRulePB(hgmert::Rule* pb) const
{
  pb->add_trg_words("[X,1]");
  for (size_t pos = 0 ; pos < GetSize() ; pos++)
    pb->add_trg_words(GetWord(pos)[0]->GetString());
}
#endif



void TargetPhrase::SetScore(float score)
{
	//we use an existing score producer to figure out information for score setting (number of scores and weights)
	//TODO: is this a good idea?
    // Assume the default system.
    const TranslationSystem& system =  StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT);
	const ScoreProducer* prod = system.GetPhraseDictionaries()[0];
	
	vector<float> weights = StaticData::Instance().GetWeights(prod);

	
	//find out how many items are in the score vector for this producer	
	size_t numScores = prod->GetNumScoreComponents();

	//divide up the score among all of the score vectors
	vector <float> scoreVector(numScores,score/numScores);
	
	//Now we have what we need to call the full SetScore method
	SetScore(prod, scoreVector, ScoreComponentCollection(), weights, system.GetWeightWordPenalty(), system.GetLanguageModels());
}

/**
 * used for setting scores for unknown words with input link features (lattice/conf. nets)
 * \param scoreVector input scores
 */
void TargetPhrase::SetScore(const TranslationSystem* system, const Scores &scoreVector)
{
	//we use an existing score producer to figure out information for score setting (number of scores and weights)

    const ScoreProducer* prod = system->GetPhraseDictionaries()[0];

	vector<float> weights = StaticData::Instance().GetWeights(prod);
	
	//expand the input weight vector
	CHECK(scoreVector.size() <= prod->GetNumScoreComponents());
	Scores sizedScoreVector = scoreVector;
	sizedScoreVector.resize(prod->GetNumScoreComponents(),0.0f);

	SetScore(prod,sizedScoreVector, ScoreComponentCollection(),weights,system->GetWeightWordPenalty(),system->GetLanguageModels());
}

void TargetPhrase::SetScore(const ScoreProducer* translationScoreProducer,
                            const Scores &scoreVector,
                            const ScoreComponentCollection &sparseScoreVector,
                            const vector<float> &weightT,
                            float weightWP, const LMList &languageModels)
{
  CHECK(weightT.size() == scoreVector.size());
  // calc average score if non-best

  float transScore = std::inner_product(scoreVector.begin(), scoreVector.end(), weightT.begin(), 0.0f);
  m_scoreBreakdown.PlusEquals(translationScoreProducer, scoreVector);
  m_scoreBreakdown.PlusEquals(sparseScoreVector);

  // Replicated from TranslationOptions.cpp
  float totalNgramScore  = 0;
  float totalFullScore   = 0;
  float totalOOVScore    = 0;

  LMList::const_iterator lmIter;
  for (lmIter = languageModels.begin(); lmIter != languageModels.end(); ++lmIter) {
    const LanguageModel &lm = **lmIter;

    if (lm.Useable(*this)) {
      // contains factors used by this LM
      const float weightLM = lm.GetWeight();
      const float oovWeightLM = lm.GetOOVWeight();
      float fullScore, nGramScore;
      size_t oovCount;

      lm.CalcScore(*this, fullScore, nGramScore, oovCount);

      if (StaticData::Instance().GetLMEnableOOVFeature()) {
        vector<float> scores(2);
        scores[0] = nGramScore;
        scores[1] = oovCount;
        m_scoreBreakdown.Assign(&lm, scores);
        totalOOVScore += oovCount * oovWeightLM;
      } else {
        m_scoreBreakdown.Assign(&lm, nGramScore);
      }


      // total LM score so far
      totalNgramScore  += nGramScore * weightLM;
      totalFullScore   += fullScore * weightLM;

    }
  }

  m_fullScore = transScore + totalFullScore + totalOOVScore
                - (this->GetSize() * weightWP);	 // word penalty
}

void TargetPhrase::SetScoreChart(const ScoreProducer* translationScoreProducer,
                                 const Scores &scoreVector
                                 ,const vector<float> &weightT
                                 ,const LMList &languageModels
                                 ,const WordPenaltyProducer* wpProducer)
{
  CHECK(weightT.size() == scoreVector.size());
  
  // calc average score if non-best
  m_scoreBreakdown.PlusEquals(translationScoreProducer, scoreVector);

  // Replicated from TranslationOptions.cpp
  float totalNgramScore  = 0;
  float totalFullScore   = 0;
  float totalOOVScore    = 0;

  LMList::const_iterator lmIter;
  for (lmIter = languageModels.begin(); lmIter != languageModels.end(); ++lmIter) {
    const LanguageModel &lm = **lmIter;

    if (lm.Useable(*this)) {
      // contains factors used by this LM
      const float weightLM = lm.GetWeight();
      const float oovWeightLM = lm.GetOOVWeight();
      float fullScore, nGramScore;
      size_t oovCount;

      lm.CalcScore(*this, fullScore, nGramScore, oovCount);
      fullScore = UntransformLMScore(fullScore);
      nGramScore = UntransformLMScore(nGramScore);

      if (StaticData::Instance().GetLMEnableOOVFeature()) {
        vector<float> scores(2);
        scores[0] = nGramScore;
        scores[1] = oovCount;
        m_scoreBreakdown.Assign(&lm, scores);
        totalOOVScore += oovCount * oovWeightLM;
      } else {
        m_scoreBreakdown.Assign(&lm, nGramScore);
      }

      // total LM score so far
      totalNgramScore  += nGramScore * weightLM;
      totalFullScore   += fullScore * weightLM;
    }
  }

  // word penalty
  size_t wordCount = GetNumTerminals();
  m_scoreBreakdown.Assign(wpProducer, - (float) wordCount * 0.434294482); // TODO log -> ln ??

  m_fullScore = m_scoreBreakdown.GetWeightedScore() - totalNgramScore + totalFullScore + totalOOVScore;
}

void TargetPhrase::SetScore(const ScoreProducer* producer, const Scores &scoreVector)
{
  // used when creating translations of unknown words (chart decoding)
  m_scoreBreakdown.Assign(producer, scoreVector);
  m_fullScore = m_scoreBreakdown.GetWeightedScore();
}


void TargetPhrase::SetWeights(const ScoreProducer* translationScoreProducer, const vector<float> &weightT)
{
  // calling this function in case of confusion net input is undefined
  CHECK(StaticData::Instance().GetInputType()==SentenceInput);

  /* one way to fix this, you have to make sure the weightT contains (in
     addition to the usual phrase translation scaling factors) the input
     weight factor as last element
  */
}

void TargetPhrase::ResetScore()
{
  m_fullScore = 0;
  m_scoreBreakdown.ZeroAll();
}

TargetPhrase *TargetPhrase::MergeNext(const TargetPhrase &inputPhrase) const
{
  if (! IsCompatible(inputPhrase)) {
    return NULL;
  }

  // ok, merge
  TargetPhrase *clone				= new TargetPhrase(*this);
  clone->m_sourcePhrase = m_sourcePhrase;
  int currWord = 0;
  const size_t len = GetSize();
  for (size_t currPos = 0 ; currPos < len ; currPos++) {
    const Word &inputWord	= inputPhrase.GetWord(currPos);
    Word &cloneWord = clone->GetWord(currPos);
    cloneWord.Merge(inputWord);

    currWord++;
  }

  return clone;
}

void TargetPhrase::SetAlignmentInfo(const StringPiece &alignString)
{
	AlignmentInfo::CollType alignTerm, alignNonTerm;
  for (util::TokenIter<util::AnyCharacter, true> token(alignString, util::AnyCharacter(" \t")); token; ++token) {
    util::TokenIter<util::SingleCharacter, false> dash(*token, util::SingleCharacter('-'));

    char *endptr;
    size_t sourcePos = strtoul(dash->data(), &endptr, 10);
    UTIL_THROW_IF(endptr != dash->data() + dash->size(), util::ErrnoException, "Error parsing alignment" << *dash);
    ++dash;
    size_t targetPos = strtoul(dash->data(), &endptr, 10);
    UTIL_THROW_IF(endptr != dash->data() + dash->size(), util::ErrnoException, "Error parsing alignment" << *dash);
    UTIL_THROW_IF(++dash, util::Exception, "Extra gunk in alignment " << *token);


    if (GetWord(targetPos).IsNonTerminal()) {
    	alignNonTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    }
  	else {
  		alignTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
  	}
  }
  SetAlignTerm(alignTerm);
  SetAlignNonTerm(alignNonTerm);

}

void TargetPhrase::SetAlignTerm(const AlignmentInfo::CollType &coll)
{
	const AlignmentInfo *alignmentInfo = AlignmentInfoCollection::Instance().Add(coll);
	m_alignTerm = alignmentInfo;

}

void TargetPhrase::SetAlignNonTerm(const AlignmentInfo::CollType &coll)
{
	const AlignmentInfo *alignmentInfo = AlignmentInfoCollection::Instance().Add(coll);
	m_alignNonTerm = alignmentInfo;
}

TO_STRING_BODY(TargetPhrase);

std::ostream& operator<<(std::ostream& os, const TargetPhrase& tp)
{
  os << static_cast<const Phrase&>(tp) << ":" << tp.GetAlignNonTerm();
  os << ": c=" << tp.m_fullScore;

  return os;
}

}

