/*
 * PhraseImpl.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/functional/hash.hpp>
#include "Phrase.h"
#include "Word.h"
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

}

