/*
 * PhraseImpl.cpp
 *
 *  Created on: 19 Feb 2016
 *      Author: hieu
 */
#include "PhraseImpl.h"

using namespace std;

namespace Moses2
{
PhraseImpl *PhraseImpl::CreateFromString(MemPool &pool, FactorCollection &vocab,
    const System &system, const std::string &str)
{
  std::vector<std::string> toks = Moses2::Tokenize(str);
  size_t size = toks.size();
  PhraseImpl *ret;

  ret = new (pool.Allocate<PhraseImpl>()) PhraseImpl(pool, size);

  ret->PhraseImplTemplate<Word>::CreateFromString(vocab, system, toks);
  return ret;
}

}

