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

#ifndef moses_LanguageModelReloading_h
#define moses_LanguageModelReloading_h

#include <string>

#include "moses/LM/Base.h"
#include "moses/LM/Ken.h"

#include <iostream>
namespace Moses
{

class FFState;

class ReloadingLanguageModel : public LanguageModel
{
public:

 ReloadingLanguageModel(const std::string &line) : LanguageModel(line), m_lm(ConstructKenLM(std::string(line).replace(0,11,"KENLM"))) {
    std::cout << "ReloadingLM constructor" << std::endl;
    std::cout << std::string(line).replace(0,11,"KENLM") << std::endl;
  }

  ~ReloadingLanguageModel() {
    delete m_lm;
  }

  virtual const FFState *EmptyHypothesisState(const InputType &input) const {
    return m_lm->EmptyHypothesisState(input);
  }

  virtual void CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, size_t &oovCount) const {
    m_lm->CalcScore(phrase, fullScore, ngramScore, oovCount);
  }

  virtual FFState *EvaluateWhenApplied(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const {
    return m_lm->EvaluateWhenApplied(hypo, ps, out);
  }

  virtual FFState *EvaluateWhenApplied(const ChartHypothesis& cur_hypo, int featureID, ScoreComponentCollection *accumulator) const {
    return m_lm->EvaluateWhenApplied(cur_hypo, featureID, accumulator);
  }

  virtual FFState *EvaluateWhenApplied(const Syntax::SHyperedge& hyperedge, int featureID, ScoreComponentCollection *accumulator) const {
    return m_lm->EvaluateWhenApplied(hyperedge, featureID, accumulator);
  }

  virtual void IncrementalCallback(Incremental::Manager &manager) const {
    m_lm->IncrementalCallback(manager);
  }

  virtual void ReportHistoryOrder(std::ostream &out,const Phrase &phrase) const {
    m_lm->ReportHistoryOrder(out, phrase);
  }

  virtual bool IsUseable(const FactorMask &mask) const {
    return m_lm->IsUseable(mask);
  }


private:

  LanguageModel *m_lm;

};

} // namespace Moses

#endif

