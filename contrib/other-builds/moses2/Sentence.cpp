/*
 * Sentence.cpp
 *
 *  Created on: 14 Dec 2015
 *      Author: hieu
 */

#include "Sentence.h"
#include "MemPool.h"
#include "legacy/Util2.h"

using namespace std;

namespace Moses2
{

Sentence::Sentence(long translationId, MemPool &pool, size_t size) :
    InputType(translationId), PhraseImpl(pool, size)
{
  // TODO Auto-generated constructor stub

}

Sentence::~Sentence()
{
  // TODO Auto-generated destructor stub
}

Sentence *Sentence::CreateFromString(MemPool &pool, FactorCollection &vocab,
    const System &system, const std::string &str, long translationId,
    bool addBOSEOS)
{
  vector<string> toks = Tokenize(str);
  size_t size = toks.size();

  /*
   if (addBOSEOS) {
   size += 2;
   }
   */

  Sentence *ret;

  ret = new (pool.Allocate<Sentence>()) Sentence(translationId, pool, size);

  ret->PhraseImplTemplate<Word>::CreateFromString(vocab, system, toks,
      addBOSEOS);

  return ret;
}

} /* namespace Moses2 */

