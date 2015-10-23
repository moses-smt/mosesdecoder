/*
 * Phrase.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <vector>
#include "Phrase.h"
#include "Word.h"
#include "moses/Util.h"

using namespace std;

Phrase *Phrase::CreateFromString(const std::string &str)
{
	vector<string> toks = Moses::Tokenize(str);
	size_t size = toks.size();
	Phrase *ret = new Phrase(size);

	for (size_t i = 0; i < size; ++i) {
		Word &word = (*ret)[i];
	}

	return ret;
}

Phrase::Phrase(size_t size)
{
  m_words = new Word[size];

}

Phrase::~Phrase() {
	delete m_words;
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

