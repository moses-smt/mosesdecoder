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

#ifndef moses_LanguageModelImplementation_h
#define moses_LanguageModelImplementation_h

#include <string>
#include <vector>
#include "moses/Factor.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/FeatureFunction.h"
#include "moses/Word.h"
#include "Base.h"

#include <boost/shared_ptr.hpp>

namespace Moses
{

class LanguageModel;

class FactorCollection;
class Factor;
class Phrase;

//! to be returned from LM functions
struct LMResult {
  // log probability
  float score;
  // Is the word unknown?  
  bool unknown;
};

//! Abstract base class which represent a language model on a contiguous phrase
class LanguageModelImplementation
{
  // default constructor is ok

  void ShiftOrPush(std::vector<const Word*> &contextFactor, const Word &word) const;

protected:
  std::string	m_filePath; //! for debugging purposes
  size_t			m_nGramOrder; //! max n-gram length contained in this LM
  Word m_sentenceStartArray, m_sentenceEndArray; //! Contains factors which represents the beging and end words for this LM.
  //! Usually <s> and </s>

public:
  virtual ~LanguageModelImplementation() {}

  //! Single or multi-factor
  virtual LMType GetLMType() const = 0;

  /* whether this LM can be used on a particular phrase.
   * Should return false if phrase size = 0 or factor types required don't exists
   */
  virtual bool Useable(const Phrase &phrase) const = 0;

  /* get score of n-gram. n-gram should not be bigger than m_nGramOrder
   * Specific implementation can return State and len data to be used in hypothesis pruning
   * \param contextFactor n-gram to be scored
   * \param state LM state.  Input and output.  state must be initialized.  If state isn't initialized, you want GetValueWithoutState.
   */
  virtual LMResult GetValueGivenState(const std::vector<const Word*> &contextFactor, FFState &state) const;

  // Like GetValueGivenState but state may not be initialized (however it is non-NULL).
  // For example, state just came from NewState(NULL).
  virtual LMResult GetValueForgotState(const std::vector<const Word*> &contextFactor, FFState &outState) const = 0;

  //! get State for a particular n-gram.  We don't care what the score is.
  // This is here so models can implement a shortcut to GetValueAndState.
  virtual void GetState(const std::vector<const Word*> &contextFactor, FFState &outState) const;

  virtual const FFState *GetNullContextState() const = 0;
  virtual const FFState *GetBeginSentenceState() const = 0;
  virtual FFState *NewState(const FFState *from = NULL) const = 0;

  void CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, size_t &oovCount) const;

  FFState *Evaluate(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out, const LanguageModel *feature) const;

  FFState* EvaluateChart(const ChartHypothesis& cur_hypo, int featureID, ScoreComponentCollection* accumulator, const LanguageModel *feature) const;

  void updateChartScore(float *prefixScore, float *finalScore, float score, size_t wordPos) const;

  //! max n-gram order of LM
  size_t GetNGramOrder() const {
    return m_nGramOrder;
  }

  //! Contains factors which represents the beging and end words for this LM. Usually <s> and </s>
  const Word &GetSentenceStartArray() const {
    return m_sentenceStartArray;
  }
  const Word &GetSentenceEndArray() const {
    return m_sentenceEndArray;
  }


  std::string GetScoreProducerWeightShortName(unsigned) const {
    return "lm";
  }

  //! overrideable funtions for IRST LM to cleanup. Maybe something to do with on demand/cache loading/unloading
  virtual void InitializeBeforeSentenceProcessing() {};
  virtual void CleanUpAfterSentenceProcessing(const InputType& source) {};
};

class LMRefCount : public LanguageModel {
  public:
    LMRefCount(LanguageModelImplementation *impl) : m_impl(impl) {}

    LanguageModel *Duplicate() const {
      return new LMRefCount(*this);
    }

    void InitializeBeforeSentenceProcessing() {
      m_impl->InitializeBeforeSentenceProcessing();
    }

    void CleanUpAfterSentenceProcessing(const InputType& source) {
      m_impl->CleanUpAfterSentenceProcessing(source);
    }

    const FFState* EmptyHypothesisState(const InputType &/*input*/) const {
      return m_impl->NewState(m_impl->GetBeginSentenceState());
    }

    bool Useable(const Phrase &phrase) const {
      return m_impl->Useable(phrase);
    }

    void CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, size_t &oovCount) const {
      return m_impl->CalcScore(phrase, fullScore, ngramScore, oovCount);
    }

    FFState* Evaluate(const Hypothesis& cur_hypo, const FFState* prev_state, ScoreComponentCollection* accumulator) const {
      return m_impl->Evaluate(cur_hypo, prev_state, accumulator, this);
    }

    FFState* EvaluateChart(const ChartHypothesis& cur_hypo, int featureID, ScoreComponentCollection* accumulator) const {
      return m_impl->EvaluateChart(cur_hypo, featureID, accumulator, this);
    }


    LanguageModelImplementation *MosesServerCppShouldNotHaveLMCode() { return m_impl.get(); }

  private:
    LMRefCount(const LMRefCount &copy_from) : m_impl(copy_from.m_impl) {}

    boost::shared_ptr<LanguageModelImplementation> m_impl;
};

}

#endif
