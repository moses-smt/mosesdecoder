#ifndef moses_LM_Rand_h
#define moses_LM_Rand_h
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
#include <vector>
#include <string>
#include <stdint.h>
#include "SingleFactor.h"
#include "moses/TypeDef.h"
#include "moses/Word.h"
//#include "RandLM.h"

namespace randlm
{
class RandLM;
}

namespace Moses
{
class LanguageModelRandLM : public LanguageModelSingleFactor
{
public:
  LanguageModelRandLM(const std::string &line);
  ~LanguageModelRandLM();

  void Load();
  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = NULL) const;
  void InitializeForInput(InputType const& source);
  void CleanUpAfterSentenceProcessing(const InputType& source);

protected:
  //std::vector<randlm::WordID> m_randlm_ids_vec;
  std::vector<uint32_t> m_randlm_ids_vec; // Ken made me do this

  randlm::RandLM* m_lm;
  uint32_t m_oov_id;
  void CreateFactors(FactorCollection &factorCollection);
  uint32_t GetLmID( const std::string &str ) const;
  uint32_t GetLmID( const Factor *factor ) const;

};

}

#endif
