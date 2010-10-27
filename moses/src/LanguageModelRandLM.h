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

#ifndef moses_LanguageModelRandLM_h
#define moses_LanguageModelRandLM_h

#include <string>
#include <vector>
#include "Factor.h"
#include "Util.h"
#include "LanguageModelSingleFactor.h"
#include "RandLM.h"

class randlm::RandLM;

namespace Moses
{
class Factor;
class Phrase;

// RandLM wrapper (single factor LM)

class LanguageModelRandLM : public LanguageModelPointerState {
public:
  LanguageModelRandLM(bool registerScore, ScoreIndexManager &scoreIndexManager)
    : LanguageModelSingleFactor(registerScore, scoreIndexManager), m_lm(0) {}
  bool Load(const std::string &filePath, FactorType factorType, size_t nGramOrder);
  virtual float GetValue(const std::vector<const Word*> &contextFactor, State* finalState = NULL, unsigned int* len=0) const;
  ~LanguageModelRandLM() {
    delete m_lm;
  }
  void CleanUpAfterSentenceProcessing() {
    m_lm->clearCaches(); // clear caches
  }
  void InitializeBeforeSentenceProcessing() {} // nothing to do
 protected:
  std::vector<randlm::WordID> m_randlm_ids_vec;
  randlm::RandLM* m_lm;
  randlm::WordID m_oov_id;
  void CreateFactors(FactorCollection &factorCollection);
  randlm::WordID GetLmID( const std::string &str ) const;
  randlm::WordID GetLmID( const Factor *factor ) const{
    size_t factorId = factor->GetId();
    return ( factorId >= m_randlm_ids_vec.size()) ? m_oov_id : m_randlm_ids_vec[factorId];
  };

};

}

#endif
