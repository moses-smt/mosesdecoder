/*
 * PhraseImpl.cpp
 *
 *  Created on: 19 Feb 2016
 *      Author: hieu
 */
#include <vector>
#include "PhraseImpl.h"
#include "SubPhrase.h"
#include "legacy/Util2.h"

using namespace std;

namespace Moses2
{

PhraseImpl *PhraseImpl::CreateFromString(MemPool &pool, FactorCollection &vocab, const System &system, const std::string &str)
{
	vector<string> toks = Tokenize(str);
	size_t size = toks.size();
	PhraseImpl *ret;

	ret = new (pool.Allocate<PhraseImpl>()) PhraseImpl(pool, size);

	ret->CreateFromString(vocab, system, toks);
	return ret;
}

void PhraseImpl::CreateFromString(FactorCollection &vocab, const System &system, const std::vector<std::string> &toks)
{
	for (size_t i = 0; i < m_size; ++i) {
		Word &word = (*this)[i];
		word.CreateFromString(vocab, system, toks[i]);
	}
}

PhraseImpl::PhraseImpl(MemPool &pool, size_t size)
:m_size(size)
{
  m_words = new (pool.Allocate<Word>(size)) Word[size];

}

PhraseImpl::PhraseImpl(MemPool &pool, const PhraseImpl &copy)
:m_size(copy.GetSize())
{
  m_words = new (pool.Allocate<Word>(m_size)) Word[m_size];
	for (size_t i = 0; i < m_size; ++i) {
		const Word &word = copy[i];
		(*this)[i] = word;
	}
}

PhraseImpl::~PhraseImpl() {

}

SubPhrase PhraseImpl::GetSubPhrase(size_t start, size_t end) const
{
	SubPhrase ret(*this, start, end);
	return ret;
}

std::ostream& operator<<(std::ostream &out, const Phrase &obj)
{
	if (obj.GetSize()) {
		out << obj[0];
		for (size_t i = 1; i < obj.GetSize(); ++i) {
			const Word &word = obj[i];
			out << " " << word;
		}
	}
	return out;
}

void PhraseImpl::Prefetch() const
{
	for (size_t i = 0; i < m_size; ++i) {
		const Word &word = m_words[i];
		const Factor *factor = word[0];
		 __builtin_prefetch(factor);
	}
}

}

