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

#ifndef moses_LanguageModel_h
#define moses_LanguageModel_h

#include <string>
#include <cstddef>

#include "moses/FF/StatefulFeatureFunction.h"

namespace Moses
{

namespace Incremental
{
class Manager;
}

class FactorCollection;
class Factor;
class Phrase;

//! Abstract base class which represent a language model on a contiguous phrase
class LanguageModel : public StatefulFeatureFunction
{
protected:
  LanguageModel(const std::string &line);

  // This can't be in the constructor for virual function dispatch reasons

  bool m_enableOOVFeature;

public:
  static const LanguageModel &GetFirstLM();

  virtual ~LanguageModel();

  bool OOVFeatureEnabled() const {
    return m_enableOOVFeature;
  }

  float GetWeight() const;
  float GetOOVWeight() const;


  virtual const FFState* EmptyHypothesisState(const InputType &input) const = 0;

  /* calc total unweighted LM score of this phrase and return score via arguments.
   * Return scores should always be in natural log, regardless of representation with LM implementation.
   * Uses GetValue() of inherited class.
   * \param fullScore scores of all unigram, bigram... of contiguous n-gram of the phrase
   * \param ngramScore score of only n-gram of order m_nGramOrder
   * \param oovCount number of LM OOVs
   */
  virtual void CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, std::size_t &oovCount) const = 0;
  virtual void CalcScoreFromCache(const Phrase &phrase, float &fullScore, float &ngramScore, std::size_t &oovCount) const {
  }

  virtual void IssueRequestsFor(Hypothesis& hypo,
                                const FFState* input_state) {
  }
  virtual void sync() {
  }
  virtual void SetFFStateIdx(int state_idx) {
  }

  // KenLM only (others throw an exception): call incremental search with the model and mapping.
  virtual void IncrementalCallback(Incremental::Manager &manager) const;
  virtual void ReportHistoryOrder(std::ostream &out,const Phrase &phrase) const;

  virtual void EvaluateInIsolation(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const;

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const {
  }

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }

};

}

#endif
