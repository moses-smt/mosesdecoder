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

#include <limits>
#include <iostream>
#include <fstream>

#include "SRI.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/FactorCollection.h"
#include "moses/Phrase.h"
#include "moses/StaticData.h"

// By default, SRILM defines a function called zopen.
//
// However, on Mac OS X (and possibly other BSDs),
// <stdio.h> already defines a zopen function.
//
// To resolve this conflict, SRILM checks to see if HAVE_ZOPEN is defined.
// If it is, SRILM will rename its zopen function as my_zopen.
//
// So, before importing any SRILM headers,
// it is important to define HAVE_ZOPEN if we are on an Apple OS:
//
#ifdef __APPLE__
#define HAVE_ZOPEN
#endif

#include "Vocab.h"
#include "Ngram.h"

using namespace std;

namespace Moses
{
LanguageModelSRI::LanguageModelSRI(const std::string &line)
  :LanguageModelSingleFactor(line)
  ,m_srilmVocab(0)
  ,m_srilmModel(0)
{
  ReadParameters();
}

LanguageModelSRI::~LanguageModelSRI()
{
  delete m_srilmModel;
  delete m_srilmVocab;
}

void LanguageModelSRI::Load(AllOptions::ptr const& opts)
{
  m_srilmVocab  = new ::Vocab();
  m_srilmModel	= new Ngram(*m_srilmVocab, m_nGramOrder);

  m_srilmModel->skipOOVs() = false;

  File file( m_filePath.c_str(), "r" );
  m_srilmModel->read(file);

  // LM can be ok, just outputs warnings
  CreateFactors();
  m_unknownId = m_srilmVocab->unkIndex();
}

void LanguageModelSRI::CreateFactors()
{
  // add factors which have srilm id
  FactorCollection &factorCollection = FactorCollection::Instance();

  std::map<size_t, VocabIndex> lmIdMap;
  size_t maxFactorId = 0; // to create lookup vector later on

  VocabString str;
  VocabIter iter(*m_srilmVocab);
  while ( (str = iter.next()) != NULL) {
    VocabIndex lmId = GetLmID(str);
    size_t factorId = factorCollection.AddFactor(Output, m_factorType, str)->GetId();
    lmIdMap[factorId] = lmId;
    maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  }

  size_t factorId;

  m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
  factorId = m_sentenceStart->GetId();
  lmIdMap[factorId] = GetLmID(BOS_);
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  m_sentenceStartWord[m_factorType] = m_sentenceStart;

  m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
  factorId = m_sentenceEnd->GetId();
  lmIdMap[factorId] = GetLmID(EOS_);
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  m_sentenceEndWord[m_factorType] = m_sentenceEnd;

  // add to lookup vector in object
  m_lmIdLookup.resize(maxFactorId+1);

  fill(m_lmIdLookup.begin(), m_lmIdLookup.end(), m_unknownId);

  map<size_t, VocabIndex>::iterator iterMap;
  for (iterMap = lmIdMap.begin() ; iterMap != lmIdMap.end() ; ++iterMap) {
    m_lmIdLookup[iterMap->first] = iterMap->second;
  }
}

VocabIndex LanguageModelSRI::GetLmID( const std::string &str ) const
{
  return m_srilmVocab->getIndex( str.c_str(), m_unknownId );
}
VocabIndex LanguageModelSRI::GetLmID( const Factor *factor ) const
{
  size_t factorId = factor->GetId();
  return ( factorId >= m_lmIdLookup.size()) ? m_unknownId : m_lmIdLookup[factorId];
}

LMResult LanguageModelSRI::GetValue(VocabIndex wordId, VocabIndex *context) const
{
  LMResult ret;
  ret.score = FloorScore(TransformLMScore(m_srilmModel->wordProb( wordId, context)));
  ret.unknown = (wordId == m_unknownId);
  return ret;
}

LMResult LanguageModelSRI::GetValue(const vector<const Word*> &contextFactor, State* finalState) const
{
  LMResult ret;
  FactorType	factorType = GetFactorType();
  size_t count = contextFactor.size();
  if (count <= 0) {
    if(finalState)
      *finalState = NULL;
    ret.score = 0.0;
    ret.unknown = false;
    return ret;
  }

  // set up context
  VocabIndex ngram[count + 1];
  for (size_t i = 0 ; i < count - 1 ; i++) {
    ngram[i+1] =  GetLmID((*contextFactor[count-2-i])[factorType]);
  }
  ngram[count] = Vocab_None;

  UTIL_THROW_IF2((*contextFactor[count-1])[factorType] == NULL,
                 "No factor " << factorType << " at position " << (count-1));
  // call sri lm fn
  VocabIndex lmId = GetLmID((*contextFactor[count-1])[factorType]);
  ret = GetValue(lmId, ngram+1);

  if (finalState) {
    ngram[0] = lmId;
    unsigned int dummy;
    *finalState = m_srilmModel->contextID(ngram, dummy);
  }
  return ret;
}

}



