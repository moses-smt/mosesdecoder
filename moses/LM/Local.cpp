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
  m_factorTypesOrdered = factors;
  m_nGramOrder  = nGramOrder;
  m_filePath    = filePath;

  if (factors.size() != 2) {
    cerr << "LocalLM needs exactly two factors tag|form" << endl;
    abort();
  }

  m_srilmModel->skipOOVs() = false;

  File file( filePath.c_str(), "r" );
  m_srilmModel->read(file);

  // LM can be ok, just outputs warnings
  m_factorHead = NULL;
  CreateFactors();
  if (m_factorHead == NULL) {
    cerr << "Language model does not contain the 'HEAD' token. Is this a local LM?" << endl;
    abort();
  }
  m_unknownId = m_srilmVocab->unkIndex();

  return true;
}

void LanguageModelLocal::CreateFactors()
{
  // add factors which have srilm id
  FactorCollection &factorCollection = FactorCollection::Instance();

  VocabString str;
  VocabIter iter(*m_srilmVocab);
  FactorType formFactor = m_factorTypesOrdered[0];
  FactorType tagFactor  = m_factorTypesOrdered[1];
  while ( (str = iter.next()) != NULL) {
    vector<string> factors = Tokenize(str, "|");
    if (factors.size() != 2) {
      cerr << "Warning: skipping word '" << str << "'" << endl;
      continue;
    }
    VocabIndex lmId = GetLmID(str);
    const Factor *tag = factorCollection.AddFactor(Output, tagFactor, factors[0]);
    if (factors[0] == "HEAD")
      m_factorHead = tag;
    size_t tagId = tag->GetId();
    size_t formId = factorCollection.AddFactor(Output, formFactor, factors[1])->GetId();
    m_lmIdLookup[PairNumbers(tagId, formId)] = lmId;
  }

  // sentence markers
  for (size_t index = 0 ; index < m_factorTypesOrdered.size() ; ++index) {
    FactorType factorType = m_factorTypesOrdered[index];
    m_sentenceStartArray[factorType] = factorCollection.AddFactor(Output, factorType, BOS_);
    m_sentenceEndArray[factorType]   = factorCollection.AddFactor(Output, factorType, EOS_);
  }
  m_lmIdLookup[PairNumbers(m_sentenceStartArray[tagFactor]->GetId(),
                           m_sentenceStartArray[formFactor]->GetId())] = GetLmID(BOS_);
  m_lmIdLookup[PairNumbers(m_sentenceEndArray[tagFactor]->GetId(),
                           m_sentenceEndArray[formFactor]->GetId())] = GetLmID(EOS_);
}

VocabIndex LanguageModelLocal::GetLmID( const std::string &str ) const
{
  return m_srilmVocab->getIndex( str.c_str(), m_unknownId );
}

VocabIndex LanguageModelLocal::GetLmID( const Factor *tag, const Factor *form ) const
{
  boost::unordered_map<size_t, unsigned int>::const_iterator it;
  it = m_lmIdLookup.find(PairNumbers(tag->GetId(), form->GetId()));
  return (it == m_lmIdLookup.end()) ? m_unknownId : it->second;
}

void LanguageModelLocal::GetValue(VocabIndex wordId, VocabIndex *context, LMResult &ret) const
{
  if (wordId != m_unknownId) {
    ret.score += FloorScore(TransformLMScore(m_srilmModel->wordProb( wordId, context)));
  }
  ret.unknown = (ret.unknown || (wordId == m_unknownId));
}

LMResult LanguageModelLocal::GetValue(const vector<const Word*> &contextFactors, State* finalState) const
{
  LMResult ret = { 0.0, false };

  size_t count = contextFactors.size();
  if (count <= 0) {
    if(finalState)
      *finalState = NULL;
    return ret;
  }

  FactorType formFactor = m_factorTypesOrdered[0];
  FactorType tagFactor  = m_factorTypesOrdered[1];

  VocabIndex lmId;
  VocabIndex ngram[count + 1];
  for (size_t head = 0; head < count; head++) {
    if (! IsOrdinaryWord(contextFactors[head]))
      continue; // head cannot be <s>,</s>

    for (size_t i = 1 ; i < count ; i++) {
      size_t contextPosition = count - i - 1;
      if (contextPosition == head) {
        ngram[i] = GetLmID(m_factorHead, (*contextFactors[head])[formFactor]);
      } else {
        if (IsOrdinaryWord(contextFactors[contextPosition])) {
          ngram[i] = GetLmID((*contextFactors[contextPosition])[tagFactor],
              (*contextFactors[head])[formFactor]);
        } else {
          ngram[i] = GetLmID((*contextFactors[contextPosition])[tagFactor],
              (*contextFactors[contextPosition])[formFactor]);
        }
      }
    } 
    ngram[count] = Vocab_None;

    CHECK((*contextFactors[count-1])[tagFactor] != NULL);
    if (head == count - 1) {
      lmId = GetLmID(m_factorHead, (*contextFactors[head])[formFactor]);
    } else {
      if (IsOrdinaryWord(contextFactors[count - 1])) {
        lmId = GetLmID((*contextFactors[count - 1])[tagFactor],
            (*contextFactors[head])[formFactor]);
      } else {
        lmId = GetLmID((*contextFactors[count - 1])[tagFactor],
            (*contextFactors[count - 1])[formFactor]);
      }
    }
    // call SRILM
    GetValue(lmId, ngram+1, /* out */ ret);
//    if (! ret.unknown) {
//      cerr << "LocalLM adding score of LM " << (*contextFactors[head])[formFactor]->GetString()
//        << "\t" << ret.score << "\n";
//    }
  }

  if (finalState) {
    ngram[0] = lmId;
    unsigned int dummy;
    *finalState = m_srilmModel->contextID(ngram, dummy);
  }
  return ret;
}

}
