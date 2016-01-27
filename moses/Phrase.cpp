// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <algorithm>
#include <sstream>
#include <string>
#include "memory.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "StaticData.h"  // GetMaxNumFactors

#include "util/string_piece.hh"
#include "util/string_stream.hh"
#include "util/tokenize_piece.hh"

using namespace std;

namespace Moses
{

Phrase::Phrase() {}

Phrase::Phrase(size_t reserveSize)
{
  m_words.reserve(reserveSize);
}

Phrase::Phrase(const vector< const Word* > &mergeWords)
{
  m_words.reserve(mergeWords.size());
  for (size_t currPos = 0 ; currPos < mergeWords.size() ; currPos++) {
    AddWord(*mergeWords[currPos]);
  }
}

Phrase::~Phrase()
{
}

void Phrase::MergeFactors(const Phrase &copy)
{
  UTIL_THROW_IF2(GetSize() != copy.GetSize(), "Both phrases need to be the same size to merge");
  size_t size = GetSize();
  const size_t maxNumFactors = MAX_NUM_FACTORS;
  for (size_t currPos = 0 ; currPos < size ; currPos++) {
    for (unsigned int currFactor = 0 ; currFactor < maxNumFactors ; currFactor++) {
      FactorType factorType = static_cast<FactorType>(currFactor);
      const Factor *factor = copy.GetFactor(currPos, factorType);
      if (factor != NULL)
        SetFactor(currPos, factorType, factor);
    }
  }
}

void Phrase::MergeFactors(const Phrase &copy, FactorType factorType)
{
  UTIL_THROW_IF2(GetSize() != copy.GetSize(), "Both phrases need to be the same size to merge");
  for (size_t currPos = 0 ; currPos < GetSize() ; currPos++)
    SetFactor(currPos, factorType, copy.GetFactor(currPos, factorType));
}

void Phrase::MergeFactors(const Phrase &copy, const std::vector<FactorType>& factorVec)
{
  UTIL_THROW_IF2(GetSize() != copy.GetSize(), "Both phrases need to be the same size to merge");
  for (size_t currPos = 0 ; currPos < GetSize() ; currPos++)
    for (std::vector<FactorType>::const_iterator i = factorVec.begin();
         i != factorVec.end(); ++i) {
      SetFactor(currPos, *i, copy.GetFactor(currPos, *i));
    }
}


Phrase Phrase::GetSubString(const Range &range) const
{
  Phrase retPhrase(range.GetNumWordsCovered());

  for (size_t currPos = range.GetStartPos() ; currPos <= range.GetEndPos() ; currPos++) {
    Word &word = retPhrase.AddWord();
    word = GetWord(currPos);
  }

  return retPhrase;
}

Phrase Phrase::GetSubString(const Range &range, FactorType factorType) const
{
  Phrase retPhrase(range.GetNumWordsCovered());

  for (size_t currPos = range.GetStartPos() ; currPos <= range.GetEndPos() ; currPos++) {
    const Factor* f = GetFactor(currPos, factorType);
    Word &word = retPhrase.AddWord();
    word.SetFactor(factorType, f);
  }

  return retPhrase;
}

std::string
Phrase::
GetStringRep(vector<FactorType> const& factorsToPrint,
             AllOptions const* opts) const
{
  if (!opts) opts = StaticData::Instance().options().get();
  bool markUnk = opts->unk.mark;
  util::StringStream strme;
  for (size_t pos = 0 ; pos < GetSize() ; pos++) {
    if (markUnk && GetWord(pos).IsOOV()) {
      strme << opts->unk.prefix;
    }
    strme << GetWord(pos).GetString(factorsToPrint, (pos != GetSize()-1));
    if (markUnk && GetWord(pos).IsOOV()) {
      strme << opts->unk.suffix;
    }
  }
  return strme.str();
}

Word &Phrase::AddWord()
{
  m_words.push_back(Word());
  return m_words.back();
}

void Phrase::Append(const Phrase &endPhrase)
{

  for (size_t i = 0; i < endPhrase.GetSize(); i++) {
    AddWord(endPhrase.GetWord(i));
  }
}

void Phrase::PrependWord(const Word &newWord)
{
  AddWord();

  // shift
  for (size_t pos = GetSize() - 1; pos >= 1; --pos) {
    const Word &word = m_words[pos - 1];
    m_words[pos] = word;
  }

  m_words[0] = newWord;
}

void Phrase::CreateFromString(FactorDirection direction,
                              const std::vector<FactorType> &factorOrder,
                              const StringPiece &phraseString,
                              Word **lhs)
{
  // parse
  vector<StringPiece> annotatedWordVector;
  for (util::TokenIter<util::AnyCharacter, true> it(phraseString, "\t "); it; ++it) {
    annotatedWordVector.push_back(*it);
  }

  if (annotatedWordVector.size() == 0) {
    if (lhs) {
      (*lhs) = NULL;
    }
    return;
  }

  // KOMMA|none ART|Def.Z NN|Neut.NotGen.Sg VVFIN|none
  //    to
  // "KOMMA|none" "ART|Def.Z" "NN|Neut.NotGen.Sg" "VVFIN|none"

  size_t numWords;
  const StringPiece &annotatedWord = annotatedWordVector.back();
  if (annotatedWord.size() >= 2
      && *annotatedWord.data() == '['
      && annotatedWord.data()[annotatedWord.size() - 1] == ']') {
    // hiero/syntax rule
    numWords = annotatedWordVector.size()-1;

    // lhs
    assert(lhs);
    (*lhs) = new Word(true);
    (*lhs)->CreateFromString(direction, factorOrder, annotatedWord.substr(1, annotatedWord.size() - 2), true);
    assert((*lhs)->IsNonTerminal());
  } else {
    numWords = annotatedWordVector.size();
    if (lhs) {
      (*lhs) = NULL;
    }
  }

  // parse each word
  m_words.reserve(numWords);

  for (size_t phrasePos = 0 ; phrasePos < numWords; phrasePos++) {
    StringPiece &annotatedWord = annotatedWordVector[phrasePos];
    bool isNonTerminal;
    if (annotatedWord.size() >= 2 && *annotatedWord.data() == '[' && annotatedWord.data()[annotatedWord.size() - 1] == ']') {
      // non-term
      isNonTerminal = true;

      size_t nextPos = annotatedWord.find('[', 1);
      UTIL_THROW_IF2(nextPos == string::npos,
                     "Incorrect formatting of non-terminal. Should have 2 non-terms, eg. [X][X]. "
                     << "Current string: " << annotatedWord);

      if (direction == Input)
        annotatedWord = annotatedWord.substr(1, nextPos - 2);
      else
        annotatedWord = annotatedWord.substr(nextPos + 1, annotatedWord.size() - nextPos - 2);
    } else {
      isNonTerminal = false;
    }

    Word &word = AddWord();
    word.CreateFromString(direction, factorOrder, annotatedWord, isNonTerminal);

  }
}

int Phrase::Compare(const Phrase &other) const
{
#ifdef min
#undef min
#endif
  size_t thisSize			= GetSize()
                        ,compareSize	= other.GetSize();
  if (thisSize != compareSize) {
    return (thisSize < compareSize) ? -1 : 1;
  }

  for (size_t pos = 0 ; pos < thisSize ; pos++) {
    const Word &thisWord	= GetWord(pos)
                            ,&otherWord	= other.GetWord(pos);
    int ret = Word::Compare(thisWord, otherWord);

    if (ret != 0)
      return ret;
  }

  return 0;
}

size_t Phrase::hash() const
{
  size_t  seed = 0;
  for (size_t i = 0; i < GetSize(); ++i) {
    boost::hash_combine(seed, GetWord(i));
  }
  return seed;
}

bool Phrase::operator== (const Phrase &other) const
{
  size_t thisSize = GetSize()
                    ,compareSize = other.GetSize();
  if (thisSize != compareSize) {
    return false;
  }

  for (size_t pos = 0 ; pos < thisSize ; pos++) {
    const Word &thisWord	= GetWord(pos)
                            ,&otherWord	= other.GetWord(pos);
    bool ret = thisWord == otherWord;
    if (!ret) {
      return false;
    }
  }

  return true;
}


bool Phrase::Contains(const vector< vector<string> > &subPhraseVector
                      , const vector<FactorType> &inputFactor) const
{
  const size_t subSize = subPhraseVector.size()
                         ,thisSize= GetSize();
  if (subSize > thisSize)
    return false;

  // try to match word-for-word
  for (size_t currStartPos = 0 ; currStartPos < (thisSize - subSize + 1) ; currStartPos++) {
    bool match = true;

    for (size_t currFactorIndex = 0 ; currFactorIndex < inputFactor.size() ; currFactorIndex++) {
      FactorType factorType = inputFactor[currFactorIndex];
      for (size_t currSubPos = 0 ; currSubPos < subSize ; currSubPos++) {
        size_t currThisPos = currSubPos + currStartPos;
        const string &subStr	= subPhraseVector[currSubPos][currFactorIndex];
        StringPiece thisStr	= GetFactor(currThisPos, factorType)->GetString();
        if (subStr != thisStr) {
          match = false;
          break;
        }
      }
      if (!match)
        break;
    }

    if (match)
      return true;
  }
  return false;
}

bool Phrase::IsCompatible(const Phrase &inputPhrase) const
{
  if (inputPhrase.GetSize() != GetSize()) {
    return false;
  }

  const size_t size = GetSize();

  const size_t maxNumFactors = MAX_NUM_FACTORS;
  for (size_t currPos = 0 ; currPos < size ; currPos++) {
    for (unsigned int currFactor = 0 ; currFactor < maxNumFactors ; currFactor++) {
      FactorType factorType = static_cast<FactorType>(currFactor);
      const Factor *thisFactor 		= GetFactor(currPos, factorType)
                                    ,*inputFactor	= inputPhrase.GetFactor(currPos, factorType);
      if (thisFactor != NULL && inputFactor != NULL && thisFactor != inputFactor)
        return false;
    }
  }
  return true;

}

bool Phrase::IsCompatible(const Phrase &inputPhrase, FactorType factorType) const
{
  if (inputPhrase.GetSize() != GetSize())	{
    return false;
  }
  for (size_t currPos = 0 ; currPos < GetSize() ; currPos++) {
    if (GetFactor(currPos, factorType) != inputPhrase.GetFactor(currPos, factorType))
      return false;
  }
  return true;
}

bool Phrase::IsCompatible(const Phrase &inputPhrase, const std::vector<FactorType>& factorVec) const
{
  if (inputPhrase.GetSize() != GetSize())	{
    return false;
  }
  for (size_t currPos = 0 ; currPos < GetSize() ; currPos++) {
    for (std::vector<FactorType>::const_iterator i = factorVec.begin();
         i != factorVec.end(); ++i) {
      if (GetFactor(currPos, *i) != inputPhrase.GetFactor(currPos, *i))
        return false;
    }
  }
  return true;
}

size_t Phrase::GetNumTerminals() const
{
  size_t ret = 0;

  for (size_t pos = 0; pos < GetSize(); ++pos) {
    if (!GetWord(pos).IsNonTerminal())
      ret++;
  }
  return ret;
}

void Phrase::InitializeMemPool()
{
}

void Phrase::FinalizeMemPool()
{
}

void Phrase::OnlyTheseFactors(const FactorMask &factors)
{
  for (unsigned int currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; currFactor++) {
    if (!factors[currFactor]) {
      for (size_t pos = 0; pos < GetSize(); ++pos) {
        SetFactor(pos, currFactor, NULL);
      }
    }
  }
}

void Phrase::InitStartEndWord()
{
  FactorCollection &factorCollection = FactorCollection::Instance();

  Word startWord(Input);
  const Factor *factor = factorCollection.AddFactor(Input, 0, BOS_); // TODO - non-factored
  startWord.SetFactor(0, factor);
  PrependWord(startWord);

  Word endWord(Input);
  factor = factorCollection.AddFactor(Input, 0, EOS_); // TODO - non-factored
  endWord.SetFactor(0, factor);
  AddWord(endWord);
}

size_t Phrase::Find(const Phrase &sought, int maxUnknown) const
{
  if (GetSize() < sought.GetSize()) {
    // sought phrase too big
    return NOT_FOUND;
  }

  size_t maxStartPos = GetSize() - sought.GetSize();
  for (size_t startThisPos = 0; startThisPos <= maxStartPos; ++startThisPos) {
    size_t thisPos = startThisPos;
    int currUnknowns = 0;
    size_t soughtPos;
    for (soughtPos = 0; soughtPos < sought.GetSize(); ++soughtPos) {
      const Word &soughtWord = sought.GetWord(soughtPos);
      const Word &thisWord = GetWord(thisPos);

      if (soughtWord == thisWord) {
        ++thisPos;
      } else if (soughtWord.IsOOV() && (maxUnknown < 0 || currUnknowns < maxUnknown)) {
        // the output has an OOV word. Allow a certain number of OOVs
        ++currUnknowns;
        ++thisPos;
      } else {
        break;
      }
    }

    if (soughtPos == sought.GetSize()) {
      return startThisPos;
    }
  }

  return NOT_FOUND;
}

TO_STRING_BODY(Phrase);

// friend
ostream& operator<<(ostream& out, const Phrase& phrase)
{
//	out << "(size " << phrase.GetSize() << ") ";
  for (size_t pos = 0 ; pos < phrase.GetSize() ; pos++) {
    const Word &word = phrase.GetWord(pos);
    out << word;
  }
  return out;
}

}


