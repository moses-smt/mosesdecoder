// $Id: Implementation.cpp,v 1.3 2013/01/15 14:10:54 braunefe Exp $

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
#include <limits>
#include <iostream>
#include <memory>
#include <sstream>
#include "FFState.h"
#include "LM/Implementation.h"
#include "Util.h"
#include "Manager.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "StaticData.h"
#include "ChartManager.h"
#include "ChartHypothesis.h"
#include "ChartHypothesisMBOT.h"
#include "ProcessedNonTerminals.h"
#include "ContextFactor.h"

#include <boost/shared_ptr.hpp>

using namespace std;

namespace Moses
{

void LanguageModelImplementation::ShiftOrPush(std::vector<const Word*> &contextFactor, const Word &word) const
{
  if (contextFactor.size() < GetNGramOrder()) {
    contextFactor.push_back(&word);
  } else {
    // shift
    for (size_t currNGramOrder = 0 ; currNGramOrder < GetNGramOrder() - 1 ; currNGramOrder++) {
      contextFactor[currNGramOrder] = contextFactor[currNGramOrder + 1];
    }
    contextFactor[GetNGramOrder() - 1] = &word;
  }
}

LMResult LanguageModelImplementation::GetValueGivenState(
  const std::vector<const Word*> &contextFactor,
  FFState &state) const
{
  return GetValueForgotState(contextFactor, state);
}

void LanguageModelImplementation::GetState(
  const std::vector<const Word*> &contextFactor,
  FFState &state) const
{
  GetValueForgotState(contextFactor, state);
}

// update score of a phrase.
void LanguageModelImplementation::CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, size_t &oovCount) const {
  fullScore  = 0;
  ngramScore = 0;
  oovCount = 0;

  size_t phraseSize = phrase.GetSize();
  if (!phraseSize) return;

  vector<const Word*> contextFactor;
  contextFactor.reserve(GetNGramOrder());
  std::auto_ptr<FFState> state(NewState((phrase.GetWord(0) == GetSentenceStartArray()) ?
                               GetBeginSentenceState() : GetNullContextState()));
  size_t currPos = 0;
  while (currPos < phraseSize) {
    const Word &word = phrase.GetWord(currPos);

    if (word.IsNonTerminal()) {
      // do nothing. reset ngram. needed to score target phrases during pt loading in chart decoding
      if (!contextFactor.empty()) {
        // TODO: state operator= ?
        state.reset(NewState(GetNullContextState()));
        contextFactor.clear();
      }
    } else {
      ShiftOrPush(contextFactor, word);
      CHECK(contextFactor.size() <= GetNGramOrder());

      if (word == GetSentenceStartArray()) {
        // do nothing, don't include prob for <s> unigram
        if (currPos != 0) {
          abort();
        }
      } else {
        LMResult result = GetValueGivenState(contextFactor, *state);
        fullScore += result.score;
        if (contextFactor.size() == GetNGramOrder())
          ngramScore += result.score;
        if (result.unknown) ++oovCount;
      }
    }
    currPos++;
  }
}


//Initialize LM with first MBOT phrase (will always be processed first in MBOT LM)
//Multiply all MBOT phrases
void LanguageModelImplementation::CalcScoreMBOT(const TargetPhraseMBOT &mbotPhrase, float &fullScore, float &ngramScore, size_t &oovCount) {

  //for MBOT phrases multiply all terminals
  float allfullScores = 0.0;
  float allngramScores = 0.0;
  float allOovCounts = 0.0;

  fullScore  = 0;
  ngramScore = 0;
  oovCount = 0;

  if (!mbotPhrase.GetMBOTPhrases().size()) return;

  ContextFactor * contextFactors = new ContextFactor(GetNGramOrder(),mbotPhrase.GetMBOTPhrases().size());

  size_t nbrOfMbot = 0;

  //Store phrases into external structure
  while(nbrOfMbot < mbotPhrase.GetMBOTPhrases().size())
  {
	  contextFactors->AddPhrase(mbotPhrase.GetMBOTPhrase(nbrOfMbot));
	  contextFactors->IncrementMbotPosition();
	  nbrOfMbot++;
  }
  contextFactors->ResetMbotPosition();

  std::auto_ptr<FFState> state(NewState((mbotPhrase.GetMBOTPhrases()[0].GetWord(0) == GetSentenceStartArray()) ?
                                        GetBeginSentenceState() : GetNullContextState()));


      while(contextFactors->GetMbotPosition() < mbotPhrase.GetMBOTPhrases().size())
      {
    	  //Check that current phrase is not empty
          if(contextFactors->IsContextEmpty())
          return;

          int currPos = 0;

          while (currPos < contextFactors->GetPhraseSize()) {

            if (contextFactors->GetPhrase()->GetWord(currPos).IsNonTerminal()) {
              if (!contextFactors->IsContextEmpty()) {
                state.reset(NewState(GetNullContextState()));
                contextFactors->Clear();
              }
            } else {

              if(contextFactors->GetPhrase()->GetWord(currPos) == GetSentenceStartArray()) {
                if (currPos != 0) {
                  std::cerr << "Your data contains <s> in a position other than the first word." << std::endl;
                  abort();
                }
              } else {
            	contextFactors->ShiftOrPushFromCurrent(currPos);
            	CHECK(contextFactors->GetNumberWords() <= GetNGramOrder());
            	vector<const Word*> contextFactorsForEval = contextFactors->GetContextFactor();
                LMResult result = GetValueGivenState(contextFactorsForEval, *state);
                fullScore += result.score;
                if (contextFactors->GetNumberWords() == GetNGramOrder())
                  ngramScore += result.score;
                if (result.unknown) ++oovCount;
              }
            }
            currPos++;
        }
        allfullScores += fullScore;
        allngramScores += ngramScore;
        allOovCounts += oovCount;

        fullScore  = 0;
        ngramScore = 0;
        oovCount = 0;
        contextFactors->IncrementMbotPosition();
      }

    fullScore = allfullScores;
    ngramScore = allngramScores;
    oovCount = allOovCounts;

    delete contextFactors;
}

FFState *LanguageModelImplementation::Evaluate(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out, const LanguageModel *feature) const {
  // In this function, we only compute the LM scores of n-grams that overlap a
  // phrase boundary. Phrase-internal scores are taken directly from the
  // translation option.

  // In the case of unigram language models, there is no overlap, so we don't
  // need to do anything.
  if(GetNGramOrder() <= 1)
    return NULL;

  clock_t t = 0;
  IFVERBOSE(2) {
    t = clock();  // track time
  }

  // Empty phrase added? nothing to be done
  if (hypo.GetCurrTargetLength() == 0)
    return ps ? NewState(ps) : NULL;

  const size_t currEndPos = hypo.GetCurrTargetWordsRange().GetEndPos();
  const size_t startPos = hypo.GetCurrTargetWordsRange().GetStartPos();

  // 1st n-gram
  vector<const Word*> contextFactor(GetNGramOrder());
  size_t index = 0;
  for (int currPos = (int) startPos - (int) GetNGramOrder() + 1 ; currPos <= (int) startPos ; currPos++) {
    if (currPos >= 0)
      contextFactor[index++] = &hypo.GetWord(currPos);
    else {
      contextFactor[index++] = &GetSentenceStartArray();
    }
  }
  FFState *res = NewState(ps);
  float lmScore = ps ? GetValueGivenState(contextFactor, *res).score : GetValueForgotState(contextFactor, *res).score;

  // main loop
  size_t endPos = std::min(startPos + GetNGramOrder() - 2
                           , currEndPos);
  for (size_t currPos = startPos + 1 ; currPos <= endPos ; currPos++) {
    // shift all args down 1 place
    for (size_t i = 0 ; i < GetNGramOrder() - 1 ; i++)
      contextFactor[i] = contextFactor[i + 1];

    // add last factor
    contextFactor.back() = &hypo.GetWord(currPos);

    lmScore += GetValueGivenState(contextFactor, *res).score;
  }

  // end of sentence
  if (hypo.IsSourceCompleted()) {
    const size_t size = hypo.GetSize();
    contextFactor.back() = &GetSentenceEndArray();

    for (size_t i = 0 ; i < GetNGramOrder() - 1 ; i ++) {
      int currPos = (int)(size - GetNGramOrder() + i + 1);
      if (currPos < 0)
        contextFactor[i] = &GetSentenceStartArray();
      else
        contextFactor[i] = &hypo.GetWord((size_t)currPos);
    }
    lmScore += GetValueForgotState(contextFactor, *res).score;
  }
  else
  {
    if (endPos < currEndPos) {
      //need to get the LM state (otherwise the last LM state is fine)
      for (size_t currPos = endPos+1; currPos <= currEndPos; currPos++) {
        for (size_t i = 0 ; i < GetNGramOrder() - 1 ; i++)
          contextFactor[i] = contextFactor[i + 1];
        contextFactor.back() = &hypo.GetWord(currPos);
      }
      GetState(contextFactor, *res);
    }
  }
  if (feature->OOVFeatureEnabled()) {
    vector<float> scores(2);
    scores[0] = lmScore;
    scores[1] = 0;
    out->PlusEquals(feature, scores);
  } else {
    out->PlusEquals(feature, lmScore);
  }


  IFVERBOSE(2) {
    hypo.GetManager().GetSentenceStats().AddTimeCalcLM( clock()-t );
  }
  return res;
}

namespace {

// This is the FFState used by LanguageModelImplementation::EvaluateChart.
// Though svn blame goes back to heafield, don't blame me.  I just moved this from LanguageModelChartState.cpp and ChartHypothesis.cpp.
class LanguageModelChartState : public FFState
{
private:
  float m_prefixScore;
  FFState* m_lmRightContext;

  Phrase m_contextPrefix, m_contextSuffix;

  size_t m_numTargetTerminals; // This isn't really correct except for the surviving hypothesis

  const ChartHypothesis &m_hypo;

  /** Construct the prefix string of up to specified size
   * \param ret prefix string
   * \param size maximum size (typically max lm context window)
   */
  size_t CalcPrefix(const ChartHypothesis &hypo, int featureID, Phrase &ret, size_t size) const
  {
    const TargetPhrase &target = hypo.GetCurrTargetPhrase();
    const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
      target.GetAlignmentInfo().GetNonTermIndexMap();

    // loop over the rule that is being applied
    for (size_t pos = 0; pos < target.GetSize(); ++pos) {
      const Word &word = target.GetWord(pos);

      // for non-terminals, retrieve it from underlying hypothesis
      if (word.IsNonTerminal()) {
        size_t nonTermInd = nonTermIndexMap[pos];
        const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermInd);
        size = static_cast<const LanguageModelChartState*>(prevHypo->GetFFState(featureID))->CalcPrefix(*prevHypo, featureID, ret, size);
      }
      // for words, add word
      else {
        ret.AddWord(target.GetWord(pos));
        size--;
      }

      // finish when maximum length reached
      if (size==0)
        break;
    }

    return size;
  }

  /** Construct the suffix phrase of up to specified size
   * will always be called after the construction of prefix phrase
   * \param ret suffix phrase
   * \param size maximum size of suffix
   */
  size_t CalcSuffix(const ChartHypothesis &hypo, int featureID, Phrase &ret, size_t size) const
  {
    CHECK(m_contextPrefix.GetSize() <= m_numTargetTerminals);

    // special handling for small hypotheses
    // does the prefix match the entire hypothesis string? -> just copy prefix
    if (m_contextPrefix.GetSize() == m_numTargetTerminals) {
      size_t maxCount = std::min(m_contextPrefix.GetSize(), size);
      size_t pos= m_contextPrefix.GetSize() - 1;

      for (size_t ind = 0; ind < maxCount; ++ind) {
        const Word &word = m_contextPrefix.GetWord(pos);
        ret.PrependWord(word);
        --pos;
      }

      size -= maxCount;
      return size;
    }
    // construct suffix analogous to prefix
    else {
      const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
        hypo.GetCurrTargetPhrase().GetAlignmentInfo().GetNonTermIndexMap();
      for (int pos = (int) hypo.GetCurrTargetPhrase().GetSize() - 1; pos >= 0 ; --pos) {
        const Word &word = hypo.GetCurrTargetPhrase().GetWord(pos);

        if (word.IsNonTerminal()) {
          size_t nonTermInd = nonTermIndexMap[pos];
          const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermInd);
          size = static_cast<const LanguageModelChartState*>(prevHypo->GetFFState(featureID))->CalcSuffix(*prevHypo, featureID, ret, size);
        }
        else {
          ret.PrependWord(hypo.GetCurrTargetPhrase().GetWord(pos));
          size--;
        }

        if (size==0)
          break;
      }

      return size;
    }
  }

public:
  LanguageModelChartState(const ChartHypothesis &hypo, int featureID, size_t order)
      :m_lmRightContext(NULL)
      ,m_contextPrefix(order - 1)
      ,m_contextSuffix( order - 1)
      ,m_hypo(hypo)
  {
    m_numTargetTerminals = hypo.GetCurrTargetPhrase().GetNumTerminals();

    for (std::vector<const ChartHypothesis*>::const_iterator i = hypo.GetPrevHypos().begin(); i != hypo.GetPrevHypos().end(); ++i) {
      // keep count of words (= length of generated string)
      m_numTargetTerminals += static_cast<const LanguageModelChartState*>((*i)->GetFFState(featureID))->GetNumTargetTerminals();
    }

    CalcPrefix(hypo, featureID, m_contextPrefix, order - 1);
    CalcSuffix(hypo, featureID, m_contextSuffix, order - 1);
  }

  ~LanguageModelChartState() {
    delete m_lmRightContext;
  }

  void Set(float prefixScore, FFState *rightState) {
    m_prefixScore = prefixScore;
    m_lmRightContext = rightState;
  }

  float GetPrefixScore() const { return m_prefixScore; }
  FFState* GetRightContext() const { return m_lmRightContext; }

  size_t GetNumTargetTerminals() const {
    return m_numTargetTerminals;
  }

  const Phrase &GetPrefix() const {
    return m_contextPrefix;
  }
  const Phrase &GetSuffix() const {
    return m_contextSuffix;
  }

  int Compare(const FFState& o) const {
    //std::cout << "Making chart compare" << std::endl;
    const LanguageModelChartState &other =
      dynamic_cast<const LanguageModelChartState &>( o );

    // prefix
    if (m_hypo.GetCurrSourceRange().GetStartPos() > 0) // not for "<s> ..."
    {
      int ret = GetPrefix().Compare(other.GetPrefix());
      if (ret != 0)
        return ret;
    }

    // suffix
    size_t inputSize = m_hypo.GetManager().GetSource().GetSize();
    if (m_hypo.GetCurrSourceRange().GetEndPos() < inputSize - 1)// not for "... </s>"
    {
      int ret = other.GetRightContext()->Compare(*m_lmRightContext);
      if (ret != 0)
        return ret;
    }
    return 0;
  }
};

} // namespace

namespace {

class LanguageModelMBOTState : public FFState
{
private:
  float m_mbotPrefixScore;
  FFState* m_mbotLmRightContext;

  vector<Phrase> m_mbotContextPrefixes;
  vector<Phrase> m_mbotContextSuffixes;

  //inherited from parent
  Phrase m_mbotContextPrefix, m_mbotContextSuffix;

  //size_t m_mbotNumTargetTerminals; // This isn't really correct except for the surviving hypothesis
  vector<size_t> m_mbotNumTargetTerminals; // We have a number of TargetTerminals for each MBOT phrase
  const ChartHypothesisMBOT &m_mbotHypo;
  ProcessedNonTerminals* m_processedNonTerms;
  ContextFactor * m_contextFactor;

  size_t CalcTerminals(const ChartHypothesisMBOT &hypo, ProcessedNonTerminals * processedNT, int nbrMBOTPhrase) const
  {
        processedNT->IncrementRec();

        // add word as parameter
        const ChartHypothesisMBOT * hypo_ptr = &hypo;
        processedNT->AddHypothesis(hypo_ptr);

        processedNT->AddStatus(hypo.GetId(),nbrMBOTPhrase +1);
        nbrMBOTPhrase = 0;

        const ChartHypothesisMBOT * currentHypo = processedNT->GetHypothesis();

        //1. Get MBOT phrases
        //2. Get MBOT alignments
        size_t mbotSize = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases().size();
        const std::vector<const AlignmentInfoMBOT*> *alignedTargets = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTAlignments();

        int currentlyProcessed = processedNT->GetStatus(currentHypo->GetId()) -1;

        CHECK(currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases().size() > currentlyProcessed);
        Phrase currentPhrase = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases()[currentlyProcessed];

        for (size_t pos = 0; pos < currentPhrase.GetSize(); ++pos) {

                const Word &word = currentPhrase.GetWord(pos);

                if (word.IsNonTerminal()) {

                        const AlignmentInfoMBOT::NonTermIndexMapPointer nonTermIndexMap =
                        //get non term index map for non terminals only
                        alignedTargets->at(currentlyProcessed)->GetNonTermIndexMap();
                        // non-term. fill out with prev hypo
                        size_t nonTermInd = nonTermIndexMap->at(pos);

                        const ChartHypothesisMBOT *prevHypo = currentHypo->GetPrevHypoMBOT(nonTermInd);
                        CalcTerminals(*prevHypo, processedNT, nbrMBOTPhrase);
                    }
                    else {
                    processedNT->IncrementTermNum();
                    }
                    if(
                        mbotSize > (processedNT->GetStatus(currentHypo->GetId()))
                        && (pos == currentPhrase.GetSize() - 1) )
                        {

                            processedNT->IncrementStatus(currentHypo->GetId());
                     }

                }
            processedNT->DecrementRec();
            size_t ret = processedNT->GetTermNum();
            return ret;
  }

  size_t CalcMBOTPrefix(const ChartHypothesisMBOT &hypo, int featureID, Phrase &ret, size_t size, ProcessedNonTerminals * processedNT, int nbrMBOTPhrase) const
  {
        processedNT->IncrementRec();

        const ChartHypothesisMBOT * hypo_ptr = &hypo;
        processedNT->AddHypothesis(hypo_ptr);
        processedNT->AddStatus(hypo.GetId(),nbrMBOTPhrase +1);
        nbrMBOTPhrase = 0;

        const ChartHypothesisMBOT * currentHypo = processedNT->GetHypothesis();

        size_t mbotSize = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases().size();

        const std::vector<const AlignmentInfoMBOT*> *alignedTargets = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTAlignments();

        int currentlyProcessed = processedNT->GetStatus(currentHypo->GetId()) - 1;

        CHECK(currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases().size() > currentlyProcessed);
        Phrase currentPhrase = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases()[currentlyProcessed];

        for (size_t pos = 0; pos < currentPhrase.GetSize(); ++pos) {

                const Word &word = currentPhrase.GetWord(pos);

                if (word.IsNonTerminal()) {

                        //get non term index map for non terminals only
                        const AlignmentInfoMBOT::NonTermIndexMapPointer nonTermIndexMap =
                        alignedTargets->at(currentlyProcessed)->GetNonTermIndexMap();
                        size_t nonTermInd = nonTermIndexMap->at(pos);
                        const ChartHypothesisMBOT *prevHypo = currentHypo->GetPrevHypoMBOT(nonTermInd);
                        prevHypo = currentHypo->GetPrevHypoMBOT(nonTermInd);
                        size = static_cast<const LanguageModelMBOTState*>(prevHypo->GetFFState(featureID))->CalcMBOTPrefix(*prevHypo, featureID, ret, size, processedNT, nbrMBOTPhrase);
                    }
                    else {
                    ret.AddWord(word);
                    }
                    if (size==0)
                    {
                        return size;
                    }
                    if(
                        mbotSize > (processedNT->GetStatus(currentHypo->GetId()) ) && size > 0
                        && (pos == currentPhrase.GetSize() - 1) )
                        {
                            processedNT->IncrementStatus(currentHypo->GetId());
                     }

                }
            processedNT->DecrementRec();
        return size;
  }

 /** Fabienne Braune. We don't use suffixes for computing LM scores but only prefixes. Seems simpler to me.*/

public:
  LanguageModelMBOTState(const ChartHypothesisMBOT &hypo, int featureID, size_t order)
      :m_mbotLmRightContext(NULL)
      ,m_mbotContextPrefix(order - 1)
      ,m_mbotContextSuffix(0) //Fabienne Braune : we don't use suffixes
      ,m_mbotHypo(hypo){

        m_processedNonTerms = new ProcessedNonTerminals();

        size_t sizeOfMbot = hypo.GetCurrTargetPhraseMBOT().GetMBOTPhrases().size();

        //store multiple phrases
        m_contextFactor = new ContextFactor(order,sizeOfMbot);

        //Threre is still an error here
        size_t currentMBOTTarget;
        size_t numTerminals = 0;
        std::vector<int> count;


        for(currentMBOTTarget = 0; currentMBOTTarget < sizeOfMbot; currentMBOTTarget++)
        {
            //get number of terminals for the current hypo
            CalcTerminals(hypo, m_processedNonTerms, currentMBOTTarget);
            m_mbotNumTargetTerminals.push_back(m_processedNonTerms->GetTermNum());
            m_processedNonTerms->ResetTermNum();
            //m_processedNonTerms->Reset();
        }

        m_processedNonTerms->Reset();

         //Compute prefix and suffix for each MBOT target phrase
        size_t currentMBOT;
        for(currentMBOT = 0; currentMBOT < sizeOfMbot; currentMBOT++)
        {
            //first push_back then modify it
            CalcMBOTPrefix(hypo, featureID, m_mbotContextPrefix, order - 1, m_processedNonTerms, currentMBOT);
            //std::cerr << "COMPUTED PREFIX : " << m_mbotContextPrefix << std::endl;
            m_mbotContextPrefixes.push_back(m_mbotContextPrefix);
            m_mbotContextPrefix.Clear();
        }
      }

    ~LanguageModelMBOTState() {
    delete m_mbotLmRightContext;
    delete m_processedNonTerms;
    delete m_contextFactor;
    //delete processedNonterms
  }

  ProcessedNonTerminals * GetProcessedNT(){
    return m_processedNonTerms;
  }

  ContextFactor * GetContextFactor(){
	  return m_contextFactor;
  }

  void Set(float prefixScore, FFState *rightState) {
    m_mbotPrefixScore = prefixScore;
    m_mbotLmRightContext = rightState;
  }

  float GetPrefixScore() const { return m_mbotPrefixScore; }
  FFState* GetRightContext() const {
      return m_mbotLmRightContext;
      }

  size_t GetNumTargetTerminals() const {
  }

  std::vector<size_t> GetNumTargetTerminalsMBOT() const {
    return m_mbotNumTargetTerminals;
  }

  //Forbid access to m_mbotContextPrefix
  const Phrase &GetPrefix() const {
     std::cout << "GET PREFIX NOT IMPLEMENTED IN TARGET PHRASE MBOT " << std::endl;
  }

  const vector<Phrase> &GetPrefixesMBOT() const {
    return m_mbotContextPrefixes;
  }

  //Fabienne Braune : we don't use suffixes
  //GetSuffix() not implemented

  const vector<Word> &GetTargetLHSMBOT() const {
	  return m_mbotHypo.GetTargetLHSMBOT();
  }

  int Compare(const FFState& o) const {

    const LanguageModelMBOTState &other =
      dynamic_cast<const LanguageModelMBOTState &>( o );
      CHECK(GetPrefixesMBOT().size() != 0);

      //if number of target phrases is not the same then return false
      int ret = (GetPrefixesMBOT().size() == other.GetPrefixesMBOT().size());

      // compare first of all
     if (m_mbotHypo.GetCurrSourceRange().GetStartPos() > 0) // not for "<s> ..."
     {
        vector<Phrase>::const_iterator itr_phrase;
        vector<Phrase>::const_iterator itr_other_phrase;
        for(
            itr_phrase = GetPrefixesMBOT().begin(), itr_other_phrase = other.GetPrefixesMBOT().begin();
            itr_phrase != GetPrefixesMBOT().end(), itr_other_phrase != other.GetPrefixesMBOT().end();
            itr_phrase++,itr_other_phrase++)
        {
            int ret = (*itr_phrase).Compare(*itr_other_phrase);
            if(ret != 0)
            return ret;
        }
    }
    size_t inputSize = m_mbotHypo.GetManager().GetSource().GetSize();
    if (m_mbotHypo.GetCurrSourceRange().GetEndPos() < inputSize - 1)// not for "... </s>"
    {
      int ret = other.GetRightContext()->Compare(*m_mbotLmRightContext);
      if (ret != 0)
        return ret;
    }
    return 0;
  }
};

}//namespace

FFState* LanguageModelImplementation::EvaluateChart(const ChartHypothesis& hypo, int featureID, ScoreComponentCollection* out, const LanguageModel *scorer) const {
  LanguageModelChartState *ret = new LanguageModelChartState(hypo, featureID, GetNGramOrder());
  // data structure for factored context phrase (history and predicted word)
  vector<const Word*> contextFactor;
  contextFactor.reserve(GetNGramOrder());

  // initialize language model context state
  FFState *lmState = NewState( GetNullContextState() );

  // initial language model scores
  float prefixScore = 0.0;    // not yet final for initial words (lack context)
  float finalizedScore = 0.0; // finalized, has sufficient context

  // get index map for underlying hypotheses
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    hypo.GetCurrTargetPhrase().GetAlignmentInfo().GetNonTermIndexMap();

  // loop over rule
  for (size_t phrasePos = 0, wordPos = 0;
       phrasePos < hypo.GetCurrTargetPhrase().GetSize();
       phrasePos++)
  {
    // consult rule for either word or non-terminal for each MBOT target phrase
    const Word &word = hypo.GetCurrTargetPhrase().GetWord(phrasePos);

    // regular word
    if (!word.IsNonTerminal())
    {

      ShiftOrPush(contextFactor, word);

      if (word == GetSentenceStartArray())
      {
        CHECK(phrasePos == 0);
        delete lmState;
        lmState = NewState( GetBeginSentenceState() );
      }
      // score a regular word added by the rule
      else
      {
        updateChartScore( &prefixScore, &finalizedScore, UntransformLMScore(GetValueGivenState(contextFactor, *lmState).score), ++wordPos );
      }
    }

    // non-terminal, add phrase from underlying hypothesis
    else
    {
      // look up underlying hypothesis
      size_t nonTermIndex = nonTermIndexMap[phrasePos];
      const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndex);

      const LanguageModelChartState* prevState =
        static_cast<const LanguageModelChartState*>(prevHypo->GetFFState(featureID));

      size_t subPhraseLength = prevState->GetNumTargetTerminals();

      // special case: rule starts with non-terminal -> copy everything
      if (phrasePos == 0) {

        // get prefixScore and finalizedScore
        prefixScore = prevState->GetPrefixScore();
        finalizedScore = prevHypo->GetScoreBreakdown().GetScoresForProducer(scorer)[0] - prefixScore;

        // get language model state
        delete lmState;
        lmState = NewState( prevState->GetRightContext() );

        // push suffix
        int suffixPos = prevState->GetSuffix().GetSize() - (GetNGramOrder()-1);
        if (suffixPos < 0) suffixPos = 0; // push all words if less than order
        for(;(size_t)suffixPos < prevState->GetSuffix().GetSize(); suffixPos++)
        {
          const Word &word = prevState->GetSuffix().GetWord(suffixPos);
          ShiftOrPush(contextFactor, word);
          wordPos++;
        }
      }

      // internal non-terminal
      else
      {
        // score its prefix
        for(size_t prefixPos = 0;
            prefixPos < GetNGramOrder()-1 // up to LM order window
              && prefixPos < subPhraseLength; // up to length
            prefixPos++)
        {
          const Word &word = prevState->GetPrefix().GetWord(prefixPos);
          ShiftOrPush(contextFactor, word);
          updateChartScore( &prefixScore, &finalizedScore, UntransformLMScore(GetValueGivenState(contextFactor, *lmState).score), ++wordPos );
        }

        // check if we are dealing with a large sub-phrase
        if (subPhraseLength > GetNGramOrder() - 1)
        {
          // add its finalized language model score
          finalizedScore +=
            prevHypo->GetScoreBreakdown().GetScoresForProducer(scorer)[0] // full score
            - prevState->GetPrefixScore();                              // - prefix score

          // copy language model state
          delete lmState;
          lmState = NewState( prevState->GetRightContext() );

          // push its suffix
          size_t remainingWords = subPhraseLength - (GetNGramOrder()-1);
          if (remainingWords > GetNGramOrder()-1) {
            // only what is needed for the history window
            remainingWords = GetNGramOrder()-1;
          }
          for(size_t suffixPos = prevState->GetSuffix().GetSize() - remainingWords;
              suffixPos < prevState->GetSuffix().GetSize();
              suffixPos++) {
            const Word &word = prevState->GetSuffix().GetWord(suffixPos);
            ShiftOrPush(contextFactor, word);
          }
          wordPos += subPhraseLength;
        }
      }
    }
  }

  // assign combined score to score breakdown
  out->Assign(scorer, prefixScore + finalizedScore);

  ret->Set(prefixScore, lmState);
  return ret;
}

void LanguageModelImplementation::updateChartScore(float *prefixScore, float *finalizedScore, float score, size_t wordPos) const {
  if (wordPos < GetNGramOrder()) {
    *prefixScore += score;
  }
  else {
    *finalizedScore += score;
  }
}

//Evaluate language model for MBOT target phrases
//Fabienne Braune : we only use prefix information and rescore the whole window each time, this is simpler to understand
FFState* LanguageModelImplementation::EvaluateMBOT(const ChartHypothesisMBOT& hypo, int featureID, ScoreComponentCollection* out, const LanguageModel *scorer) const {

    LanguageModelMBOTState *ret = new LanguageModelMBOTState(hypo, featureID, GetNGramOrder());

  // initialize language model context state
  FFState *lmState = NewState( GetNullContextState() );

  // initial language model scores
  float prefixScore = 0.0;    // not yet final for initial words (lack context)
  float finalizedScore = 0.0; // finalized, has sufficient context

  //each MBOT target phrase has own score
  float allPrefixes = 0.0;
  float totalScore = 0.0;

  //maps each word inside of an MBOT target phrase to an integer to indicate which non-terminals have already been processed
  map<Word,unsigned> lhsCurrMap;

  //maps for constructing non contiguous suffixes
  //maps each target lhs in the previous hypothesis to an integer to indicate which target lhs non-terminals of the previous
  //hypothesis have already been processed
   map<Word,unsigned> lhsPrevMap;

  //maps for constructing non contiguous prefixes
  map<Word,unsigned> lhsPrefixMap;

    size_t currentlyProcessed = 0;

    //Get size here because hypo is modified inside the loops (?)
    size_t sizeOfMbot = hypo.GetCurrTargetPhraseMBOT().GetMBOTPhrases().size();

    while(currentlyProcessed < sizeOfMbot)
    {
        //in case we have already found the same non terminal in another mbot
        map<Word,unsigned> :: iterator itr_lhs_map_curr = lhsCurrMap.find(hypo.GetTargetLHSMBOT()[currentlyProcessed]);

        const std::vector<const AlignmentInfoMBOT*> *alignedTargets = hypo.GetCurrTargetPhraseMBOT().GetMBOTAlignments();

        //Get size here because hypo is modified inside the loops (?)
        size_t sizeOfPhrase = hypo.GetCurrTargetPhraseMBOT().GetMBOTPhrases()[currentlyProcessed].GetSize();

        ret->GetContextFactor()->AddPhrase(hypo.GetCurrTargetPhraseMBOT().GetMBOTPhrase(currentlyProcessed));

    	 // loop over rule
        for (size_t phrasePos = 0, wordPos = 0;
        		phrasePos < sizeOfPhrase;
        		phrasePos++)
			{
			// consult rule for either word or non-terminal
			const Word &word = ret->GetContextFactor()->GetPhrase()->GetWord(phrasePos);

			//Map the position of each word in a rule to the word itself
			//Set source map outside of while
			if(itr_lhs_map_curr!=lhsCurrMap.end())
			{
				lhsCurrMap.insert(make_pair(word,itr_lhs_map_curr->second++));
			}
			else
			{
				lhsCurrMap.insert(make_pair(word,0));
			}

			// regular word
			if (!word.IsNonTerminal())
			{
			  ret->GetContextFactor()->ShiftOrPushFromCurrent(phrasePos);
			  
				if (word == GetSentenceStartArray())
				{
					CHECK(phrasePos == 0);
					delete lmState;
					lmState = NewState( GetBeginSentenceState() );
				}
				// score a regular word added by the rule
				else
				{
					//IF nothing in context factors then there is nothing to retrieve
					updateChartScore( &prefixScore, &finalizedScore, UntransformLMScore(GetValueGivenState(ret->GetContextFactor()->GetContextFactor(), *lmState).score), ++wordPos );
				}
			}
		// non-terminal, add phrase from underlying hypothesis
		else
		{

			const AlignmentInfoMBOT::NonTermIndexMapPointer nonTermIndexMap =
				alignedTargets->at(currentlyProcessed)->GetNonTermIndexMap();

			size_t nonTermIndex = nonTermIndexMap->at(phrasePos);
			const ChartHypothesisMBOT *prevHypo = hypo.GetPrevHypoMBOT(nonTermIndex);

			//check if previous hypo has already been used (MBOT phrase)
			//-->returns the number of times this hypo id has already been used as previous hypo
			//this is important to know how many times we can use the suffix
			int numberOfTimesSameHypoHasBeenUsed = ret->GetContextFactor()->IsSameHypoId(prevHypo->GetId());
			ret->GetContextFactor()->SetHypoId(prevHypo->GetId());

			//Clear previous maps
			lhsPrevMap.clear();
			lhsPrefixMap.clear();

			const LanguageModelMBOTState* prevState =
			static_cast<const LanguageModelMBOTState*>(prevHypo->GetFFState(featureID));
			size_t sizeOfPreviousMbot = prevHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases().size();
			bool previousFound = false;

				if (phrasePos == 0 || phrasePos == numberOfTimesSameHypoHasBeenUsed) {

					//previous processed : which target lhs of the previous hypo has been processed
					ret->GetContextFactor()->ResetPreviousPosition();

					//if we see the same hypo then we have to word on the i-th mbot component of this hypo
					//underlying assumption : the number of times the same hypo is used again corresponds to the
					//number of mbot components it has
					for(int i=0; i<numberOfTimesSameHypoHasBeenUsed;i++)
					{
						ret->GetContextFactor()->IncrementPreviousPosition();
					}

					//process mbot phrases in previous hypotheses....
					while(ret->GetContextFactor()->GetPreviousPosition() < sizeOfPreviousMbot && !(previousFound))
					{
						size_t previousProcessed = ret->GetContextFactor()->GetPreviousPosition();

						//Fabienne Braune : we don't copy anything from the previous state and recompute everyting
						prefixScore = 0;
						finalizedScore = 0;

						// get language model state
						delete lmState;
						lmState = NewState( prevState->GetRightContext() );

						//map each non-terminal in the target lhs of the previous hypo to its position in the previous MBOT hypo
						map<Word,unsigned> :: iterator itr_lhs_map_prev = lhsPrevMap.find(prevHypo->GetTargetLHSMBOT()[previousProcessed]);

						if(itr_lhs_map_prev !=lhsPrevMap.end())
						{
							lhsPrevMap.insert(make_pair(prevHypo->GetTargetLHSMBOT()[previousProcessed],itr_lhs_map_prev->second++));
						}
						else
						{
							lhsPrevMap.insert(make_pair(prevHypo->GetTargetLHSMBOT()[previousProcessed],0));
						}

						if( (prevHypo->GetTargetLHSMBOT()[previousProcessed] == word)
						   && (!(lhsPrevMap.find(prevHypo->GetTargetLHSMBOT()[previousProcessed])->second > currentlyProcessed ))
						   && (!(lhsCurrMap.find(word)->second > previousProcessed )
								   )
						   )
						{
							//put suffixes in context factors
							ret->GetContextFactor()->SetPrefix(&(prevState->GetPrefixesMBOT()[previousProcessed]));

							for(size_t prefixPos = 0; prefixPos < ret->GetContextFactor()->GetPrefixSize(); prefixPos++)
							{
								//Fabienne Braune : begin and end of sentence symbols are ignored when pushing for context factor
								if(ret->GetContextFactor()->GetPrefixPhrase()->GetWord(prefixPos) != GetSentenceStartArray())
								{
									ret->GetContextFactor()->ShiftOrPushFromPrefix(prefixPos);
									updateChartScore( &prefixScore, &finalizedScore, UntransformLMScore(GetValueGivenState(ret->GetContextFactor()->GetContextFactor(), *lmState).score), ++wordPos );
								}
							}
							previousFound = true;

						}
						else{
							//std::cout << "FOUND : SKIP : " << prevHypo->GetTargetLHSMBOT()[previousProcessed] << std::endl;
						}
						ret->GetContextFactor()->IncrementPreviousPosition();
					}
				}
				// internal non-terminal : we have a contiguous phrase : compute prefix
				else
				{

					//reset previous position
					ret->GetContextFactor()->ResetPreviousPosition();

					//if we see the same hypo then we are querying the i-th mbot component of this hypo
					//underlying assumption : the number of times the same hypo is used again corresponds to the
					//number of mbot components it has
					for(int i=0; i<numberOfTimesSameHypoHasBeenUsed;i++)
					{
					    ret->GetContextFactor()->IncrementPreviousPosition();
					}

						 while(ret->GetContextFactor()->GetPreviousPosition() < sizeOfPreviousMbot && !(previousFound))
						 {

							 size_t previousProcessed = ret->GetContextFactor()->GetPreviousPosition();

							//2. try to match a previous mbot

							//Length of MBOT phrase in previous state
							size_t subPhraseLength = prevState->GetNumTargetTerminalsMBOT()[previousProcessed];

							//Add prefix in contextFactor
							ret->GetContextFactor()->SetPrefix(&prevState->GetPrefixesMBOT()[previousProcessed]);
							map<Word,unsigned> :: iterator itr_lhs_map_pref = lhsPrefixMap.find(prevHypo->GetTargetLHSMBOT()[previousProcessed]);
							if(itr_lhs_map_pref !=lhsPrefixMap.end())
							{
							   lhsPrefixMap.insert(make_pair(prevState->GetTargetLHSMBOT()[previousProcessed],itr_lhs_map_pref->second++));
							}
							else
							{
							  lhsPrefixMap.insert(make_pair(prevState->GetTargetLHSMBOT()[previousProcessed],0));
							}

							//match previous and current
							//skip if TLHS is found
							if(
									(prevState->GetTargetLHSMBOT()[previousProcessed] == word)
									&& (!(lhsPrefixMap.find(prevState->GetTargetLHSMBOT()[previousProcessed])->second > currentlyProcessed ))
									&& (!(lhsCurrMap.find(word)->second > previousProcessed ))
									)
							{

								//score prefix of matched non-terminal
								// score its prefix

								//when we plug in the prefix of an internal non-terminal we have to remember which position we have
								//to increment it by the number of the hypothesis
								for(size_t prefixPos = 0;
									prefixPos < GetNGramOrder()-1 // up to LM order window
									  && prefixPos < subPhraseLength; // up to length
									prefixPos++)
								{
								  //Update what corresponds to the same mbot phrase
								  if(ret->GetContextFactor()->GetPrefixPhrase()->GetWord(prefixPos) != GetSentenceStartArray())
								  {
									  ret->GetContextFactor()->ShiftOrPushFromPrefix(prefixPos);
									  updateChartScore( &prefixScore, &finalizedScore, UntransformLMScore(GetValueGivenState(ret->GetContextFactor()->GetContextFactor(), *lmState).score), ++wordPos );
								  }
								 }

								// check if we are dealing with a large sub-phrase
								if (subPhraseLength > GetNGramOrder() - 1)
								{

									//Fabienen Braune : we don't add anything here and recompute the whole LM score

									// copy language model state
									delete lmState;
									lmState = NewState( prevState->GetRightContext() );


									for(size_t prefixPos = GetNGramOrder() -1; prefixPos < ret->GetContextFactor()->GetPrefixSize(); prefixPos++)
									{
										if(ret->GetContextFactor()->GetPrefixPhrase()->GetWord(prefixPos) != GetSentenceStartArray())
										{
											wordPos = prefixPos;
											ret->GetContextFactor()->ShiftOrPushFromPrefix(prefixPos);
											updateChartScore( &prefixScore, &finalizedScore, UntransformLMScore(GetValueGivenState(ret->GetContextFactor()->GetContextFactor(), *lmState).score), ++wordPos );
										}
									  }
									}
								}
							ret->GetContextFactor()->IncrementPreviousPosition();
					}

				}//else
			}//else
		}//end of for
   //process next mbot prhase
   currentlyProcessed++;
   ret->GetContextFactor()->IncrementMbotPosition();

  //for each MBOT target phrase, we add (log mult) the obtained score
  totalScore += finalizedScore;
  allPrefixes += prefixScore;
  finalizedScore = 0;
  prefixScore = 0;
  lhsCurrMap.clear();
  ret->GetContextFactor()->Clear();

  }//end of while

  // assign combined score tbto score breakdown
  out->Assign(scorer, allPrefixes + totalScore);
  ret->Set(allPrefixes, lmState);

  return ret;
}

}//namespace
