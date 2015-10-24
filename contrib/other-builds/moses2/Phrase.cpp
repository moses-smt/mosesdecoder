/*
 * Phrase.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <vector>
#include "Phrase.h"
#include "Word.h"
#include "Vocab.h"
#include "moses/Util.h"
#include "util/pool.hh"

using namespace std;

Phrase *Phrase::CreateFromString(util::Pool *pool, const std::string &str)
{
	vector<string> toks = Moses::Tokenize(str);
	size_t size = toks.size();
	Phrase *ret;

	if (pool) {
		ret = new (pool->Allocate<Phrase>()) Phrase(pool, size);
	}
	else {
		ret = new Phrase(pool, size);
	}

	ret->CreateFromString(toks);
	return ret;
}

void Phrase::CreateFromString(const std::vector<std::string> &toks)
{
	for (size_t i = 0; i < m_size; ++i) {
		Word &word = (*this)[i];
	}
}

Phrase::Phrase(util::Pool *pool, size_t size)
:m_size(size)
{
  if (pool) {
	  m_words = new (pool->Allocate<Word>(size)) Word[size];
  }
  else {
	  m_words = new Word[size];
  }

}

Phrase::~Phrase() {
	delete[] m_words;
}

SubPhrase Phrase::GetSubPhrase(size_t start, size_t end) const
{
	SubPhrase ret(*this, start, end);
	return ret;
}

////////////////////////////////////////////////////////////
SubPhrase::SubPhrase(const Phrase &origPhrase, size_t start, size_t end)
:m_origPhrase(origPhrase)
,m_start(start)
,m_end(end)
{

}

