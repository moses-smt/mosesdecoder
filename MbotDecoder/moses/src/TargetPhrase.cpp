// $Id: TargetPhrase.cpp,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $

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

#include "util/check.hh"
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include "util/tokenize_piece.hh"

#include "TargetPhrase.h"
#include "PhraseDictionaryMemory.h"
#include "GenerationDictionary.h"
#include "LM/Base.h"
#include "StaticData.h"
#include "ScoreIndexManager.h"
#include "LMList.h"
#include "ScoreComponentCollection.h"
#include "Util.h"
#include "DummyScoreProducers.h"
#include "AlignmentInfoCollection.h"

using namespace std;

namespace Moses
{
TargetPhrase::TargetPhrase(std::string out_string, Phrase sourcePhrase)
  :Phrase(0),m_transScore(0.0), m_fullScore(0.0), m_sourcePhrase(0)
  , m_alignmentInfo(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_matchesSource(false)
{

   //std::cerr << "New TargetPhrase(1)" << std::endl;
  //ACAT
  const StaticData &staticData = StaticData::Instance();
  CreateFromString(staticData.GetInputFactorOrder(), out_string, staticData.GetFactorDelimiter());
  m_sourcePhrase = new Phrase(sourcePhrase);
  //it is useful to have access to the source side of a rule!
}

TargetPhrase::TargetPhrase(Phrase sourcePhrase)
  :Phrase(ARRAY_SIZE_INCR)
  , m_transScore(0.0)
  , m_fullScore(0.0)
  , m_sourcePhrase(0)
  , m_alignmentInfo(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_matchesSource(false)
{
    m_sourcePhrase = new Phrase(sourcePhrase);
   //std::cerr << "New TargetPhrase(2)" << std::endl;
}

TargetPhrase::TargetPhrase(const Phrase &phrase, Phrase sourcePhrase)
  : Phrase(phrase)
  , m_transScore(0.0)
  , m_fullScore(0.0)
  , m_sourcePhrase(0)
  , m_alignmentInfo(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_matchesSource(false)
{
    m_sourcePhrase = new Phrase(sourcePhrase);
   //std::cerr << "New TargetPhrase(3)" << std::endl;
}

//copy constructor for target phrase
TargetPhrase::TargetPhrase(TargetPhrase * tp)
  : Phrase(*tp)
  , m_transScore(tp->GetTranslationScore())
  , m_fullScore(tp->GetFutureScore())
  , m_scoreBreakdown(tp->GetScoreBreakdown())
  , m_sourcePhrase(tp->GetSourcePhrase())
 //BEWARE : Alignment is empty because we only use mbot alignments, to fix
  , m_alignmentInfo(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_matchesSource(false)
{
	m_lhsSource = tp->GetSourceLHS();
	m_lhsTarget = tp->GetTargetLHS();
}

TargetPhrase::~TargetPhrase()
{
    delete m_sourcePhrase;
}

void TargetPhrase::SetScore(const TranslationSystem* system)
{
  // used when creating translations of unknown words:
  m_transScore = 0;
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

  //get the weight list
  unsigned int id = prod->GetScoreBookkeepingID();

  const vector<float> &allWeights = StaticData::Instance().GetAllWeights();

  size_t beginIndex = StaticData::Instance().GetScoreIndexManager().GetBeginIndex(id);
  size_t endIndex = StaticData::Instance().GetScoreIndexManager().GetEndIndex(id);

  vector<float> weights;

  std::copy(allWeights.begin() +beginIndex, allWeights.begin() + endIndex,std::back_inserter(weights));

  //find out how many items are in the score vector for this producer
  size_t numScores = prod->GetNumScoreComponents();

  //divide up the score among all of the score vectors
  vector <float> scoreVector(numScores,score/numScores);

  //Now we have what we need to call the full SetScore method
  SetScore(prod,scoreVector,weights,system.GetWeightWordPenalty(),system.GetLanguageModels());
}

/**
 * used for setting scores for unknown words with input link features (lattice/conf. nets)
 * \param scoreVector input scores
 */
void TargetPhrase::SetScore(const TranslationSystem* system, const Scores &scoreVector)
{
  //we use an existing score producer to figure out information for score setting (number of scores and weights)

  const ScoreProducer* prod = system->GetPhraseDictionaries()[0];

  //get the weight list
  unsigned int id = prod->GetScoreBookkeepingID();
  const vector<float> &allWeights = StaticData::Instance().GetAllWeights();
  size_t beginIndex = StaticData::Instance().GetScoreIndexManager().GetBeginIndex(id);
  size_t endIndex = StaticData::Instance().GetScoreIndexManager().GetEndIndex(id);
  vector<float> weights;
  std::copy(allWeights.begin() +beginIndex, allWeights.begin() + endIndex,std::back_inserter(weights));

  //expand the input weight vector
  CHECK(scoreVector.size() <= prod->GetNumScoreComponents());
  Scores sizedScoreVector = scoreVector;
  sizedScoreVector.resize(prod->GetNumScoreComponents(),0.0f);

  SetScore(prod,sizedScoreVector,weights,system->GetWeightWordPenalty(),system->GetLanguageModels());
}

void TargetPhrase::SetScore(const ScoreProducer* translationScoreProducer,
                            const Scores &scoreVector,
                            const vector<float> &weightT,
                            float weightWP, const LMList &languageModels)
{
  CHECK(weightT.size() == scoreVector.size());
  // calc average score if non-best

  m_transScore = std::inner_product(scoreVector.begin(), scoreVector.end(), weightT.begin(), 0.0f);
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

  m_fullScore = m_transScore + totalFullScore + totalOOVScore
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
  m_transScore = std::inner_product(scoreVector.begin(), scoreVector.end(), weightT.begin(), 0.0f);
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
  m_transScore = 0;
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

  m_transScore = m_scoreBreakdown.PartialInnerProduct(translationScoreProducer, weightT);
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

namespace {
void MosesShouldUseExceptions(bool value) {
  if (!value) {
    std::cerr << "Could not parse alignment info" << std::endl;
    abort();
  }
}
} // namespace

void TargetPhrase::SetAlignmentInfo(const StringPiece &alignString)
{
  set<pair<size_t,size_t> > alignmentInfo;
  for (util::TokenIter<util::AnyCharacter, true> token(alignString, util::AnyCharacter(" \t")); token; ++token) {
    util::TokenIter<util::AnyCharacter, false> dash(*token, util::AnyCharacter("-"));
    MosesShouldUseExceptions(dash);
    size_t sourcePos = boost::lexical_cast<size_t>(*dash++);
    MosesShouldUseExceptions(dash);
    size_t targetPos = boost::lexical_cast<size_t>(*dash++);
    MosesShouldUseExceptions(!dash);

    alignmentInfo.insert(pair<size_t,size_t>(sourcePos, targetPos));
  }
  SetAlignmentInfo(alignmentInfo);
}

void TargetPhrase::SetAlignmentInfo(const std::set<std::pair<size_t,size_t> > &alignmentInfo)
{
  m_alignmentInfo = AlignmentInfoCollection::Instance().Add(alignmentInfo);
}

TO_STRING_BODY(TargetPhrase);

//NIN: old
  //std::ostream& operator<<(std::ostream& os, const TargetPhrase& tp)
  //{
  //os << static_cast<const Phrase&>(tp) << ":" << tp.GetAlignmentInfo();
  //os << ": pC=" << tp.m_transScore << ", c=" << tp.m_fullScore;

  //return os;
//}

//NIN: new
// friend
 std::ostream& operator<<(ostream& out, const TargetPhrase& targetPhrase)
{
  out << "Information from TargetPhrase.cpp " << endl;
  out << "=> m_transScore " << targetPhrase.GetTranslationScore() << endl;
  out << "=> m_fullScore " << targetPhrase.GetFutureScore() << endl;
  out << "=> m_scoreBreakdown " << targetPhrase.GetScoreBreakdown() << endl;
  out << "=> m_alignmentInfo " << targetPhrase.GetAlignmentInfo() << endl;
  out << "=> m_sourcePhrase " << targetPhrase.GetSourcePhrase() << endl;
  out << "=> m_lhsTarget " << targetPhrase.GetTargetLHS() << endl;
  out << "=> m_lhsSource " << targetPhrase.GetSourceLHS() << endl;

  return out;
}


}

