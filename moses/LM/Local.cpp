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

#include "util/check.hh"
#include <limits>
#include <iostream>
#include <fstream>

#include "Local.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/FactorCollection.h"
#include "moses/Phrase.h"
#include "moses/StaticData.h"

#include "Vocab.h"
#include "Ngram.h"

using namespace std;

namespace Moses
{
LanguageModelLocal::LanguageModelLocal()
  : m_srilmVocab(0)
  , m_srilmModel(0)
{
}

LanguageModelLocal::~LanguageModelLocal()
{
  delete m_srilmModel;
  delete m_srilmVocab;
}

bool LanguageModelLocal::Load(const std::string &filePath, const std::vector<FactorType> &factors,
                              size_t nGramOrder)
{
  m_srilmVocab  = new ::Vocab();
  m_srilmModel  = new Ngram(*m_srilmVocab, nGramOrder);
  m_factorTypes = FactorMask(factors);
  m_nGramOrder  = nGramOrder;
  m_filePath    = filePath;

  if (factors.size() != 2) {
    cerr << "LocalLM needs exactly two factors form|tag" << endl;
    abort();
  }

  m_srilmModel->skipOOVs() = false;

  File file( filePath.c_str(), "r" );
  m_srilmModel->read(file);

  // LM can be ok, just outputs warnings
  CreateFactors();
  m_unknownId = m_srilmVocab->unkIndex();

  return true;
}

void LanguageModelLocal::CreateFactors()
{
  // add factors which have srilm id
  FactorCollection &factorCollection = FactorCollection::Instance();

  VocabString str;
  VocabIter iter(*m_srilmVocab);
  FactorType formFactor = m_factorTypes[0];
  FactorType tagFactor  = m_factorTypes[1];
  while ( (str = iter.next()) != NULL) {
    vector<string> factors = Tokenize(str, "|");
    if (factors.size() != 2) {
      cerr << "Incorrect format for LocalLM, expected 2 factors in word: " << str << endl;
      abort();
    }
    VocabIndex lmId = GetLmID(str);
    size_t formId = factorCollection.AddFactor(Output, formFactor, factors[0])->GetId();
    size_t tagId  = factorCollection.AddFactor(Output, tagFactor, factors[1])->GetId();
    m_lmIdLookup[PairNumbers(formId, tagId)] = lmId;
  }

  // sentence markers
  for (size_t index = 0 ; index < m_factorTypes.size() ; ++index) {
    FactorType factorType = m_factorTypes[index];
    m_sentenceStartArray[factorType] = factorCollection.AddFactor(Output, factorType, BOS_);
    m_sentenceEndArray[factorType]   = factorCollection.AddFactor(Output, factorType, EOS_);
  }
  m_lmIdLookup[PairNumbers(m_sentenceStartArray[formFactor]->GetId(),
                           m_sentenceStartArray[tagFactor]->GetId())] = GetLmID(BOS_);
  m_lmIdLookup[PairNumbers(m_sentenceEndArray[formFactor]->GetId(),
                           m_sentenceEndArray[tagFactor]->GetId())] = GetLmID(EOS_);
}

VocabIndex LanguageModelLocal::GetLmID( const std::string &str ) const
{
  return m_srilmVocab->getIndex( str.c_str(), m_unknownId );
}

VocabIndex LanguageModelLocal::GetLmID( const Factor *form, const Factor *tag ) const
{
  boost::unordered_map<size_t, unsigned int>::const_iterator it;
  it = m_lmIdLookup.find(PairNumbers(form->GetId(), tag->GetId()));
  return (it == m_lmIdLookup.end()) ? m_unknownId : it->second;
}

LMResult LanguageModelLocal::GetValue(VocabIndex wordId, VocabIndex *context) const
{
  LMResult ret;
  ret.score = FloorScore(TransformLMScore(m_srilmModel->wordProb( wordId, context)));
  ret.unknown = (wordId == m_unknownId);
  return ret;
}

LMResult LanguageModelLocal::GetValue(const vector<const Word*> &contextFactor, State* finalState) const
{
  LMResult ret;
  FactorType factorType = 0; // XXX
  size_t count = contextFactor.size();
  if (count <= 0) {
    if(finalState)
      *finalState = NULL;
    ret.score = 0.0;
    ret.unknown = false;
    return ret;
  }

  // set up context
  // 
  // TODO
  // for each head word (i.e. word W in contextFactor, ask about this n-gram:
  //   contextFactor[0].tag + W.form, ..., "HEAD" + W.form, ..., contextFactor[last].tag + W.form
  VocabIndex ngram[count + 1];
  for (size_t i = 0 ; i < count - 1 ; i++) {
    ngram[i+1] =  GetLmID((*contextFactor[count-2-i])[factorType], 0); // XXX
  } 
  ngram[count] = Vocab_None;

  CHECK((*contextFactor[count-1])[factorType] != NULL);
  // call sri lm fn
  VocabIndex lmId = GetLmID((*contextFactor[count-1])[factorType], 0); // XXX
  ret = GetValue(lmId, ngram+1);

  if (finalState) {
    ngram[0] = lmId;
    unsigned int dummy;
    *finalState = m_srilmModel->contextID(ngram, dummy);
  }
  return ret;
}

}
