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

#include "ParallelBackoff.h"

#include <vector>
#include <string>
#include <sstream>
#include <fstream>

#include "MultiFactor.h"
#include "moses/Word.h"
#include "moses/Factor.h"
#include "moses/FactorTypeSet.h"
#include "moses/FactorCollection.h"
#include "moses/Phrase.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"

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

#include "FNgramSpecs.h"
#include "FNgramStats.h"
#include "FactoredVocab.h"
#include "FNgram.h"
#include "wmatrix.h"
#include "Vocab.h"
#include "File.h"

using namespace std;

namespace Moses
{

namespace
{
class LanguageModelParallelBackoff : public LanguageModelMultiFactor
{
private:
  std::vector<FactorType> m_factorTypesOrdered;

  FactoredVocab   *m_srilmVocab;
  FNgram          *m_srilmModel;
  VocabIndex  m_unknownId;
  VocabIndex  m_wtid;
  VocabIndex  m_wtbid;
  VocabIndex  m_wteid;
  FNgramSpecs<FNgramCount>* fnSpecs;
  //std::vector<VocabIndex> m_lmIdLookup;
  std::map<size_t, VocabIndex>* lmIdMap;
  std::fstream* debugStream;

  WidMatrix *widMatrix;

public:
  LanguageModelParallelBackoff(const std::string &line)
    :LanguageModelMultiFactor(line) {
  }

  ~LanguageModelParallelBackoff();

  bool Load(const std::string &filePath, const std::vector<FactorType> &factorTypes, size_t nGramOrder);

  VocabIndex GetLmID( const std::string &str ) const;

  VocabIndex GetLmID( const Factor *factor, FactorType ft ) const;

  void CreateFactors();

  LMResult GetValueForgotState(const std::vector<const Word*> &contextFactor, FFState &outState) const;
  const FFState *GetNullContextState() const;
  const FFState *GetBeginSentenceState() const;
  FFState *NewState(const FFState *from) const;
};

LanguageModelParallelBackoff::~LanguageModelParallelBackoff()
{
  ///
}


bool LanguageModelParallelBackoff::Load(const std::string &filePath, const std::vector<FactorType> &factorTypes, size_t nGramOrder)
{

  cerr << "Loading Language Model Parallel Backoff!!!\n";
  widMatrix = new ::WidMatrix();
  m_factorTypes	= FactorMask(factorTypes);
  m_srilmVocab = new ::FactoredVocab();
  //assert(m_srilmVocab != 0);

  fnSpecs = 0;
  File f(filePath.c_str(),"r");
  fnSpecs = new ::FNgramSpecs<FNgramCount>(f,*m_srilmVocab, 0/*debug*/);

  cerr << "Loaded fnSpecs!\n";

  m_srilmVocab->unkIsWord() = true;
  m_srilmVocab->nullIsWord() = true;
  m_srilmVocab->toLower() = false;

  FNgramStats *factoredStats = new FNgramStats(*m_srilmVocab, *fnSpecs);

  factoredStats->debugme(2);

  cerr << "Factored stats\n";

  FNgram* fngramLM = new FNgram(*m_srilmVocab,*fnSpecs);

  cerr << "FNgram object created\n";

  fngramLM->skipOOVs = false;

  if (!factoredStats->read()) {
    cerr << "error reading in counts in factor file\n";
    exit(1);
  }

  cerr << "Factored stats read!\n";

  factoredStats->estimateDiscounts();
  factoredStats->computeCardinalityFunctions();
  factoredStats->sumCounts();

  cerr << "Another three operations made!\n";

  if (!fngramLM->read()) {
    cerr << "format error in lm file\n";
    exit(1);
  }

  cerr << "fngramLM reads!\n";

  m_filePath = filePath;
  m_nGramOrder= nGramOrder;

  m_factorTypesOrdered= factorTypes;

  m_unknownId = m_srilmVocab->unkIndex();

  cerr << "m_unknowdId = " << m_unknownId << endl;

  m_srilmModel = fngramLM;

  cerr << "Create factors...\n";

  CreateFactors();

  cerr << "Factors created! \n";
  //FactorCollection &factorCollection = FactorCollection::Instance();

  /*for (size_t index = 0 ; index < m_factorTypesOrdered.size() ; ++index)
  {
  	FactorType factorType = m_factorTypesOrdered[index];
  	m_sentenceStartArray[factorType] 	= factorCollection.AddFactor(Output, factorType, BOS_);


  	m_sentenceEndArray[factorType] 		= factorCollection.AddFactor(Output, factorType, EOS_);

    //factorIdStart = m_sentenceStartArray[factorType]->GetId();
    //factorIdEnd = m_sentenceEndArray[factorType]->GetId();

    for (size_t i = 0; i < 10; i++)
    {
      lmIdMap[factorIdStart * 10 + i] = GetLmID(BOS_);
  		lmIdMap[factorIdEnd * 10 + i] = GetLmID(EOS_);
    }

  	//(*lmIdMap)[factorIdStart * 10 + index] = GetLmID(BOS_);
  	//(*lmIdMap)[factorIdEnd * 10 + index] = GetLmID(EOS_);

  }*/
  return true;
}

VocabIndex LanguageModelParallelBackoff::GetLmID( const std::string &str ) const
{
  return m_srilmVocab->getIndex( str.c_str(), m_unknownId );
}

VocabIndex LanguageModelParallelBackoff::GetLmID( const Factor *factor, size_t ft ) const
{

  size_t factorId = factor->GetId();
  if ( lmIdMap->find( factorId * 10 + ft ) != lmIdMap->end() ) {
    return lmIdMap->find( factorId * 10 + ft )->second;
  } else {
    return m_unknownId;
  }

}

void LanguageModelParallelBackoff::CreateFactors()
{

  // add factors which have srilm id
  FactorCollection &factorCollection = FactorCollection::Instance();

  lmIdMap = new std::map<size_t, VocabIndex>();


  VocabString str;
  VocabIter iter(*m_srilmVocab);

  iter.init();

  size_t pomFactorTypeNum = 0;


  while ( (str = iter.next()) != NULL) {

    if ((str[0] < 'a' || str[0] > 'k') && str[0] != 'W') {
      continue;
    }
    VocabIndex lmId = GetLmID(str);
    pomFactorTypeNum = str[0] - 'a';

    size_t factorId = factorCollection.AddFactor(Output, m_factorTypesOrdered[pomFactorTypeNum], &(str[2]) )->GetId();
    (*lmIdMap)[factorId * 10 + pomFactorTypeNum] = lmId;
  }

  size_t factorIdStart;
  size_t factorIdEnd;

  // sentence markers
  for (size_t index = 0 ; index < m_factorTypesOrdered.size() ; ++index) {
    FactorType factorType = m_factorTypesOrdered[index];
    m_sentenceStartWord[index] 	= factorCollection.AddFactor(Output, factorType, BOS_);


    m_sentenceEndWord[index] 		= factorCollection.AddFactor(Output, factorType, EOS_);

    factorIdStart = m_sentenceStartWord[index]->GetId();
    factorIdEnd = m_sentenceEndWord[index]->GetId();

    /*for (size_t i = 0; i < 10; i++)
    {
      lmIdMap[factorIdStart * 10 + i] = GetLmID(BOS_);
    	lmIdMap[factorIdEnd * 10 + i] = GetLmID(EOS_);
    }*/

    (*lmIdMap)[factorIdStart * 10 + index] = GetLmID(BOS_);
    (*lmIdMap)[factorIdEnd * 10 + index] = GetLmID(EOS_);

    cerr << "BOS_:" << GetLmID(BOS_) << ", EOS_:" << GetLmID(EOS_) << endl;

  }

  m_wtid = GetLmID("W-<unk>");
  m_wtbid = GetLmID("W-<s>");
  m_wteid = GetLmID("W-</s>");

  cerr << "W-<unk> index: " << m_wtid << endl;
  cerr << "W-<s> index: " << m_wtbid << endl;
  cerr << "W-</s> index: " << m_wteid << endl;


}

LMResult LanguageModelParallelBackoff::GetValueForgotState(const std::vector<const Word*> &contextFactor, FFState & /*outState */) const
{

  static WidMatrix widMatrix;

  for (int i=0; i<contextFactor.size(); i++)
    ::memset(widMatrix[i],0,(m_factorTypesOrdered.size() + 1)*sizeof(VocabIndex));


  for (size_t i = 0; i < contextFactor.size(); i++) {
    const Word &word = *contextFactor[i];

    for (size_t j = 0; j < m_factorTypesOrdered.size(); j++) {
      const Factor *factor = word[ m_factorTypesOrdered[j] ];

      if (factor == NULL)
        widMatrix[i][j + 1] = 0;
      else
        widMatrix[i][j + 1] = GetLmID(factor, j);
    }

    if (widMatrix[i][1] == GetLmID(m_sentenceStartWord[0], 0) ) {
      widMatrix[i][0] = m_wtbid;
    } else if (widMatrix[i][1] == GetLmID(m_sentenceEndWord[0], 0 )) {
      widMatrix[i][0] = m_wteid;
    } else {
      widMatrix[i][0] = m_wtid;
    }
  }


  LMResult ret;
  ret.score = m_srilmModel->wordProb( widMatrix, contextFactor.size() - 1, contextFactor.size() );
  ret.score = FloorScore(TransformLMScore(ret.score));
  ret.unknown = !contextFactor.empty() && (widMatrix[contextFactor.size() - 1][0] == m_unknownId);
  return ret;

  /*if (contextFactor.size() == 0)
  {
  	return 0;
  }

  for (size_t currPos = 0 ; currPos < m_nGramOrder ; ++currPos )
  {
  	const Word &word = *contextFactor[currPos];

  	for (size_t index = 0 ; index < m_factorTypesOrdered.size() ; ++index)
  	{
  		FactorType factorType = m_factorTypesOrdered[index];
  		const Factor *factor = word[factorType];

  		(*widMatrix)[currPos][index] = GetLmID(factor, index);

  	}

  }

  float p = m_srilmModel->wordProb( (*widMatrix), m_nGramOrder - 1, m_nGramOrder );
  return FloorScore(TransformLMScore(p)); */
}

// The old version did not initialize finalState like it should.  Technically that makes the behavior undefined, so it's not clear what else to do here.
FFState *LanguageModelParallelBackoff::NewState(const FFState * /*from*/) const
{
  return NULL;
}

const FFState *LanguageModelParallelBackoff::GetNullContextState() const
{
  return NULL;
}

const FFState *LanguageModelParallelBackoff::GetBeginSentenceState() const
{
  return NULL;
}

}


}

