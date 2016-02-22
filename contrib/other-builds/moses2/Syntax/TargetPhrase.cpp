/*
 * TargetPhrase.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "TargetPhrase.h"
#include "../legacy/FactorCollection.h"
#include "../legacy/Util2.h"
#include "../System.h"

using namespace std;

namespace Moses2
{
namespace Syntax
{
TargetPhrase *TargetPhrase::CreateFromString(MemPool &pool, const PhraseTable &pt, const System &system, const std::string &str)
{
	FactorCollection &vocab = system.GetVocab();

	vector<string> toks = Tokenize(str);
	size_t size = toks.size();
	TargetPhrase *ret = new (pool.Allocate<TargetPhrase>()) TargetPhrase(pool, pt, system, size);
	//ret->PhraseImpl::CreateFromString(vocab, system, toks);

	return ret;
}


}
}
