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

#include "lm/word_index.hh"

#include "moses/Word.h"
#include "moses/LM/Base.h"

namespace Moses {

  class FFState;

//! This will also load. Returns a templated KenLM class
LanguageModel *ConstructKenLM(const std::string &file, FactorType factorType, bool lazy);

/*
 * An implementation of single factor LM using Kenneth's code.
 */
template <class Model> class LanguageModelKen : public LanguageModel {
  public:
    LanguageModelKen(const std::string &file, FactorType factorType, bool lazy);

    virtual LanguageModel *Duplicate() const;

    virtual bool Useable(const Phrase &phrase) const;

    std::string GetScoreProducerDescription(unsigned) const;
							    
    virtual const FFState *EmptyHypothesisState(const InputType &/*input*/) const;

    virtual void CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, size_t &oovCount) const;

    virtual FFState *Evaluate(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const;

    virtual FFState *EvaluateChart(const ChartHypothesis& cur_hypo, int featureID, ScoreComponentCollection *accumulator) const;

    virtual void IncrementalCallback(Incremental::Manager &manager) const;

  protected:

    boost::shared_ptr<Model> m_ngram;

  private:
    LanguageModelKen(const LanguageModelKen<Model> &copy_from);

    lm::WordIndex TranslateID(const Word &word) const;

    // Convert last words of hypothesis into vocab ids, returning an end pointer.  
    lm::WordIndex *LastIDs(const Hypothesis &hypo, lm::WordIndex *indices) const;
    
    std::vector<lm::WordIndex> m_lmIdLookup;

    FactorType m_factorType;

    const Factor *m_beginSentenceFactor;
};

} // namespace Moses

#endif
