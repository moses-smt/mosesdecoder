/*
 * Sentence.cpp
 *
 *  Created on: 14 Dec 2015
 *      Author: hieu
 */

#include "Sentence.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{
Sentence *Sentence::CreateFromString(MemPool &pool, FactorCollection &vocab,
    const System &system, const std::string &str, long translationId)
{
  //cerr << "SCFG Sentence" << endl;

  std::vector<std::string> toks = Tokenize(str);
  size_t size = toks.size();
  size += 2;

  Sentence *ret;

  ret = new (pool.Allocate<Sentence>()) Sentence(pool, size);

  ret->PhraseImplTemplate<Word>::CreateFromString(vocab, system, toks, true);

  return ret;
}

}
} /* namespace Moses2 */

