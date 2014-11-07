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

#ifndef moses_LanguageModelKen_h
#define moses_LanguageModelKen_h

#include <string>
#include <boost/shared_ptr.hpp>

#include "lm/word_index.hh"

#include "moses/LM/Base.h"
#include "moses/Hypothesis.h"
#include "moses/TypeDef.h"
#include "moses/Word.h"

namespace Moses
{

//class LanguageModel;
class FFState;

LanguageModel *ConstructKenLM(const std::string &line);

//! This will also load. Returns a templated KenLM class
LanguageModel *ConstructKenLM(const std::string &line, const std::string &file, FactorType factorType, bool lazy);

/*
 * An implementation of single factor LM using Kenneth's code.
 */
template <class Model> class LanguageModelKen : public LanguageModel
{
public:
  LanguageModelKen(const std::string &line, const std::string &file, FactorType factorType, bool lazy);

  virtual const FFState *EmptyHypothesisState(const InputType &/*input*/) const;

  virtual void CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, size_t &oovCount) const;

  virtual FFState *EvaluateWhenApplied(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const;

  virtual FFState *EvaluateWhenApplied(const ChartHypothesis& cur_hypo, int featureID, ScoreComponentCollection *accumulator) const;

  virtual FFState *EvaluateWhenApplied(const Syntax::SHyperedge& hyperedge, int featureID, ScoreComponentCollection *accumulator) const;

  virtual void IncrementalCallback(Incremental::Manager &manager) const;
  virtual void ReportHistoryOrder(std::ostream &out,const Phrase &phrase) const;

  virtual bool IsUseable(const FactorMask &mask) const;

protected:
  boost::shared_ptr<Model> m_ngram;

  const Factor *m_beginSentenceFactor;

  FactorType m_factorType;

  lm::WordIndex TranslateID(const Word &word) const {
    std::size_t factor = word.GetFactor(m_factorType)->GetId();
    return (factor >= m_lmIdLookup.size() ? 0 : m_lmIdLookup[factor]);
  }

private:
  LanguageModelKen(const LanguageModelKen<Model> &copy_from);

  // Convert last words of hypothesis into vocab ids, returning an end pointer.
  lm::WordIndex *LastIDs(const Hypothesis &hypo, lm::WordIndex *indices) const {
    lm::WordIndex *index = indices;
    lm::WordIndex *end = indices + m_ngram->Order() - 1;
    int position = hypo.GetCurrTargetWordsRange().GetEndPos();
    for (; ; ++index, --position) {
      if (index == end) return index;
      if (position == -1) {
        *index = m_ngram->GetVocabulary().BeginSentence();
        return index + 1;
      }
      *index = TranslateID(hypo.GetWord(position));
    }
  }

  std::vector<lm::WordIndex> m_lmIdLookup;

};

} // namespace Moses

#endif
