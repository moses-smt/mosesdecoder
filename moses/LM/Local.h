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

#ifndef moses_LanguageModelLocal_h
#define moses_LanguageModelLocal_h

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include "moses/Factor.h"
#include "moses/TypeDef.h"
#include "SingleFactor.h"
#include "MultiFactor.h"

class Factor;
class Phrase;
class Vocab;
class Ngram;

namespace Moses
{

/** Local language models (Monz 2011)
 */
class LanguageModelLocal : public LanguageModelMultiFactor, public LanguageModelPointerState
{
protected:
  boost::unordered_map<size_t, unsigned int> m_lmIdLookup;
  ::Vocab *m_srilmVocab;
  Ngram   *m_srilmModel;
  unsigned int  m_unknownId;
  const Factor *m_factorHead;

  void GetValue(unsigned int wordId, unsigned int *context, LMResult &ret) const;
  void CreateFactors();
  unsigned int GetLmID( const std::string &str ) const;
  unsigned int GetLmID( const Factor *form, const Factor *tag ) const;

  // Cantor's pairing function
  size_t PairNumbers(size_t a, size_t b) const
  {
    return (a + b) * (a + b + 1) / 2 + b;
  }

public:
  LanguageModelLocal();
  ~LanguageModelLocal();
  bool Load(const std::string &filePath, const std::vector<FactorType> &factors, size_t nGramOrder);
  inline bool IsOrdinaryWord(const Word *word) const {
    return (*word)[m_factorTypes[0]]->GetString() != BOS_
      && (*word)[m_factorTypes[0]]->GetString() != EOS_;
  }

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactors, State* finalState = 0) const;
};


}
#endif
