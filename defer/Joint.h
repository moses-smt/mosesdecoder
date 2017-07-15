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

#ifndef moses_LanguageModelJoint_h
#define moses_LanguageModelJoint_h

#include <vector>
#include <string>
#include <sstream>
#include "SingleFactor.h"
#include "MultiFactor.h"
#include "moses/Word.h"
#include "moses/FactorTypeSet.h"
#include "moses/FactorCollection.h"

namespace Moses
{

class Phrase;
class FactorCollection;

/** LM of multiple factors. A simple extension of single factor LM - factors backoff together.
 *	Rather slow as this uses string concatenation/split.
 *  Not used for a long time
 */
class LanguageModelJoint : public LanguageModelMultiFactor
{
protected:
  LanguageModelSingleFactor *m_lmImpl;
  std::vector<FactorType> m_factorTypesOrdered;

  size_t m_implFactor;
public:
  LanguageModelJoint(const std::string &line, LanguageModelSingleFactor *lmImpl)
    :LanguageModelMultiFactor(line) {
    m_lmImpl = lmImpl;
  }

  ~LanguageModelJoint() {
    delete m_lmImpl;
  }

  bool Load(AllOptions const& opts, const std::string &filePath
            , const std::vector<FactorType> &factorTypes
            , size_t nGramOrder) {
    m_factorTypes				= FactorMask(factorTypes);
    m_filePath 					= filePath;
    m_nGramOrder 				= nGramOrder;

    m_factorTypesOrdered= factorTypes;
    m_implFactor				= 0;

    FactorCollection &factorCollection = FactorCollection::Instance();

    // sentence markers
    for (size_t index = 0 ; index < factorTypes.size() ; ++index) {
      FactorType factorType = factorTypes[index];
      m_sentenceStartWord[factorType] 	= factorCollection.AddFactor(Output, factorType, BOS_);
      m_sentenceEndWord[factorType] 		= factorCollection.AddFactor(Output, factorType, EOS_);
    }

    m_lmImpl->Load(AllOptions const& opts);
  }

  LMResult GetValueForgotState(const std::vector<const Word*> &contextFactor, FFState &outState) const {
    if (contextFactor.size() == 0) {
      LMResult ret;
      ret.score = 0.0;
      ret.unknown = false;
      return ret;
    }

    // joint context for internal LM
    std::vector<const Word*> jointContext;

    for (size_t currPos = 0 ; currPos < m_nGramOrder ; ++currPos ) {
      const Word &word = *contextFactor[currPos];

      // add word to chunked context
      std::stringstream stream("");

      const Factor *factor = word[ m_factorTypesOrdered[0] ];
      stream << factor->GetString();

      for (size_t index = 1 ; index < m_factorTypesOrdered.size() ; ++index) {
        FactorType factorType = m_factorTypesOrdered[index];
        const Factor *factor = word[factorType];
        stream << "|" << factor->GetString();
      }

      factor = FactorCollection::Instance().AddFactor(Output, m_implFactor, stream.str());

      Word* jointWord = new Word;
      jointWord->SetFactor(m_implFactor, factor);
      jointContext.push_back(jointWord);
    }

    // calc score on chunked phrase
    LMResult ret = m_lmImpl->GetValueForgotState(jointContext, outState);

    RemoveAllInColl(jointContext);

    return ret;
  }

  const FFState *GetNullContextState() const {
    return m_lmImpl->GetNullContextState();
  }

  const FFState *GetBeginSentenceState() const {
    return m_lmImpl->GetBeginSentenceState();
  }

  FFState *NewState(const FFState *from) const {
    return m_lmImpl->NewState(from);
  }

};

}
#endif
