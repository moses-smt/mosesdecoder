/*
 * TargetPhraseImpl.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <sstream>
#include <stdlib.h>
#include "TargetPhraseImpl.h"
#include "../Scores.h"
#include "../System.h"
#include "../MemPool.h"
#include "Manager.h"

using namespace std;

namespace Moses2
{

TargetPhraseImpl *TargetPhraseImpl::CreateFromString(MemPool &pool,
    const PhraseTable &pt, const System &system, const std::string &str)
{
  FactorCollection &vocab = system.GetVocab();

  vector<string> toks = Tokenize(str);
  size_t size = toks.size();
  TargetPhraseImpl *ret =
    new (pool.Allocate<TargetPhraseImpl>()) TargetPhraseImpl(pool, pt, system,
        size);
  ret->PhraseImplTemplate<Word>::CreateFromString(vocab, system, toks);

  return ret;
}

TargetPhraseImpl::TargetPhraseImpl(MemPool &pool, const PhraseTable &pt,
                                   const System &system, size_t size)
  :Moses2::TargetPhrase<Moses2::Word>(pool, pt, system, size)
{
  m_scores = new (pool.Allocate<Scores>()) Scores(system, pool,
      system.featureFunctions.GetNumScores());

  size_t numWithPtData = system.featureFunctions.GetWithPhraseTableInd().size();
  ffData = new (pool.Allocate<void *>(numWithPtData)) void *[numWithPtData];
}

TargetPhraseImpl::~TargetPhraseImpl()
{
  // TODO Auto-generated destructor stub
}

}
