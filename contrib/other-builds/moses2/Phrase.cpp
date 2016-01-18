/*
 * PhraseImpl.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/functional/hash.hpp>
#include <vector>
#include "Phrase.h"
#include "Word.h"
#include "legacy/Util2.h"
#include "MemPool.h"

using namespace std;

namespace Moses2
{

size_t Phrase::hash() const
{
  size_t  seed = 0;

  for (size_t i = 0; i < GetSize(); ++i) {
	  const Word &word = (*this)[i];
	  size_t wordHash = word.hash();
	  boost::hash_combine(seed, wordHash);
  }

  return seed;
}

bool Phrase::operator==(const Phrase &compare) const
{
  if (GetSize() != compare.GetSize()) {
	  return false;
  }

  for (size_t i = 0; i < GetSize(); ++i) {
	  const Word &word = (*this)[i];
	  const Word &otherWord = compare[i];
	  if (word != otherWord) {
		  return false;
	  }
  }

  return true;
}

std::string Phrase::GetString(const FactorList &factorTypes) const
{
	if (GetSize() == 0) {
		return "";
	}

	std::stringstream ret;

	const Word &word = (*this)[0];
	ret << word.GetString(factorTypes);
	for (size_t i = 1; i < GetSize(); ++i) {
		const Word &word = (*this)[i];
		ret << " " << word.GetString(factorTypes);
	}
	return ret.str();

}

////////////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////////////
SubPhrase::SubPhrase(const PhraseImpl &origPhrase, size_t start, size_t end)
:m_origPhrase(&origPhrase)
,m_start(start)
,m_end(end)
{

}

std::ostream& operator<<(std::ostream &out, const SubPhrase &obj)
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

SubPhrase SubPhrase::GetSubPhrase(size_t start, size_t end) const
{
	SubPhrase ret(*m_origPhrase, m_start + start, m_start + end);
	return ret;
}

}

