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
namespace SCFG
{
PhraseImpl *PhraseImpl::CreateFromString(MemPool &pool, FactorCollection &vocab,
    const System &system, const std::string &str, bool skipLastWord)
{
  std::vector<std::string> toks = Moses2::Tokenize(str);
  size_t size = toks.size();
  if (skipLastWord) {
    --size;
  }
  PhraseImpl *ret;

  ret = new (pool.Allocate<PhraseImpl>()) PhraseImpl(pool, size);

  for (size_t i = 0; i < size; ++i) {
    SCFG::Word &word = (*ret)[i];
    word.CreateFromString(vocab, system, toks[i]);
  }

  return ret;
}

}
}

