// $Id$
// vim::tabstop=2

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

#include <sstream>
#include "memory.h"
#include "Word.h"
#include "TypeDef.h"
#include "StaticData.h"  // needed to determine the FactorDelimiter

using namespace std;

namespace Moses
{

// static
int Word::Compare(const Word &targetWord, const Word &sourceWord)
{
  if (targetWord.IsNonTerminal() != sourceWord.IsNonTerminal()) {
    return targetWord.IsNonTerminal() ? -1 : 1;
  }

  for (size_t factorType = 0 ; factorType < MAX_NUM_FACTORS ; factorType++) {
    const Factor *targetFactor		= targetWord[factorType]
                                    ,*sourceFactor	= sourceWord[factorType];

    if (targetFactor == NULL || sourceFactor == NULL)
      continue;
    if (targetFactor == sourceFactor)
      continue;

    return (targetFactor<sourceFactor) ? -1 : +1;
  }
  return 0;

}

void Word::Merge(const Word &sourceWord)
{
  for (unsigned int currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; currFactor++) {
    const Factor *sourcefactor		= sourceWord.m_factorArray[currFactor]
                                    ,*targetFactor		= this     ->m_factorArray[currFactor];
    if (targetFactor == NULL && sourcefactor != NULL) {
      m_factorArray[currFactor] = sourcefactor;
    }
  }
}

std::string Word::GetString(const vector<FactorType> factorType,bool endWithBlank) const
{
  stringstream strme;
  CHECK(factorType.size() <= MAX_NUM_FACTORS);
  const std::string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();
  bool firstPass = true;
  for (unsigned int i = 0 ; i < factorType.size() ; i++) {
    const Factor *factor = m_factorArray[factorType[i]];
    if (factor != NULL) {
      if (firstPass) {
        firstPass = false;
      } else {
        strme << factorDelimiter;
      }
      strme << factor->GetString();
    }
  }
  if(endWithBlank) strme << " ";
  return strme.str();
}

void Word::CreateFromString(FactorDirection direction
                            , const std::vector<FactorType> &factorOrder
                            , const std::string &str
                            , bool isNonTerminal)
{
  FactorCollection &factorCollection = FactorCollection::Instance();

  vector<string> wordVec;
  Tokenize(wordVec, str, "|");
  CHECK(wordVec.size() <= factorOrder.size());

  const Factor *factor;
  for (size_t ind = 0; ind < wordVec.size(); ++ind) {
    FactorType factorType = factorOrder[ind];
    factor = factorCollection.AddFactor(direction, factorType, wordVec[ind]);
    m_factorArray[factorType] = factor;
  }

  // assume term/non-term same for all factors
  m_isNonTerminal = isNonTerminal;
}

void Word::CreateUnknownWord(const Word &sourceWord)
{
  FactorCollection &factorCollection = FactorCollection::Instance();

  for (unsigned int currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; currFactor++) {
    FactorType factorType = static_cast<FactorType>(currFactor);

    const Factor *sourceFactor = sourceWord[currFactor];
    if (sourceFactor == NULL)
      SetFactor(factorType, factorCollection.AddFactor(Output, factorType, UNKNOWN_FACTOR));
    else
      SetFactor(factorType, factorCollection.AddFactor(Output, factorType, sourceFactor->GetString()));
  }
  m_isNonTerminal = sourceWord.IsNonTerminal();
}

TO_STRING_BODY(Word);

// friend
ostream& operator<<(ostream& out, const Word& word)
{
  stringstream strme;

  const std::string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();
  bool firstPass = true;
  for (unsigned int currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; currFactor++) {
    FactorType factorType = static_cast<FactorType>(currFactor);
    const Factor *factor = word.GetFactor(factorType);
    if (factor != NULL) {
      if (firstPass) {
        firstPass = false;
      } else {
        strme << factorDelimiter;
      }
      strme << *factor;
    }
  }
  out << strme.str() << " ";
  return out;
}

}

