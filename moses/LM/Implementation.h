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
#include "moses/Word.h"
#include "Base.h"

#include <boost/shared_ptr.hpp>

namespace Moses
{

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
class LanguageModelImplementation : public LanguageModel
{
  // default constructor is ok

  void ShiftOrPush(std::vector<const Word*> &contextFactor, const Word &word) const;

protected:
  std::string	m_filePath;
  size_t			m_nGramOrder; //! max n-gram length contained in this LM
  Word m_sentenceStartWord, m_sentenceEndWord; //! Contains factors which represents the beging and end words for this LM.
  //! Usually <s> and </s>

  LanguageModelImplementation(const std::string &line);

public:

  virtual ~LanguageModelImplementation() {}

  void SetParameter(const std::string& key, const std::string& value);

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

  FFState *Evaluate(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const;

  FFState* EvaluateChart(const ChartHypothesis& cur_hypo, int featureID, ScoreComponentCollection* accumulator) const;

  void updateChartScore(float *prefixScore, float *finalScore, float score, size_t wordPos) const;

  //! max n-gram order of LM
  size_t GetNGramOrder() const {
    return m_nGramOrder;
  }

  //! Contains factors which represents the beging and end words for this LM. Usually <s> and </s>
  const Word &GetSentenceStartWord() const {
    return m_sentenceStartWord;
  }
  const Word &GetSentenceEndWord() const {
    return m_sentenceEndWord;
  }

  const FFState* EmptyHypothesisState(const InputType &/*input*/) const {
    return NewState(GetBeginSentenceState());
  }

};

}

#endif
