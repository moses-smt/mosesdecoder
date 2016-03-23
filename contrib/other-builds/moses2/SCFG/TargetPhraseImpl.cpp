/*
 * TargetPhraseImpl.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <stdlib.h>
#include "TargetPhraseImpl.h"
#include "../Scores.h"
#include "../System.h"
#include "../MemPool.h"
#include "../PhraseBased/Manager.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

TargetPhraseImpl *TargetPhraseImpl::CreateFromString(MemPool &pool, const PhraseTable &pt, const System &system, const std::string &str)
{
	//cerr << "str=" << str << endl;
	FactorCollection &vocab = system.GetVocab();

	vector<string> toks = Tokenize(str);
	size_t size = toks.size() - 1;
	TargetPhraseImpl *ret = new (pool.Allocate<TargetPhraseImpl>()) TargetPhraseImpl(pool, pt, system, size);

	for (size_t i = 0; i < size; ++i) {
		SCFG::Word &word = (*ret)[i];
		word.CreateFromString(vocab, system, toks[i], true);
	}

	// lhs
	ret->lhs.CreateFromString(vocab, system, toks.back(), false);
	//cerr << "ret=" << *ret << endl;
	return ret;
}

TargetPhraseImpl::TargetPhraseImpl(MemPool &pool, const PhraseTable &pt, const System &system, size_t size)
:TargetPhrase(pool, pt, system)
,PhraseImplTemplate<SCFG::Word>(pool, size)
{
	m_scores = new (pool.Allocate<Scores>()) Scores(system, pool, system.featureFunctions.GetNumScores());

}

TargetPhraseImpl::~TargetPhraseImpl() {
	// TODO Auto-generated destructor stub
}

std::ostream& operator<<(std::ostream &out, const TargetPhraseImpl &obj)
{
	out << (const Phrase&) obj << " SCORES:" << obj.GetScores();
	return out;
}

}
}
