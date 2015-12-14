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

Sentence::Sentence(long translationId, MemPool &pool, size_t size)
:InputType(translationId)
,PhraseImpl(pool, size)
{
	// TODO Auto-generated constructor stub

}

Sentence::~Sentence() {
	// TODO Auto-generated destructor stub
}


Sentence *Sentence::CreateFromString(MemPool &pool,
		FactorCollection &vocab,
		const System &system,
		const std::string &str,
		long translationId)
{
	vector<string> toks = Tokenize(str);
	size_t size = toks.size();
	Sentence *ret;

	ret = new (pool.Allocate<Sentence>()) Sentence(translationId, pool, size);

	ret->PhraseImpl::CreateFromString(vocab, system, toks);


	return ret;
}


} /* namespace Moses2 */

