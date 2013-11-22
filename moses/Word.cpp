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
#include "FactorTypeSet.h"
#include "FactorCollection.h"
#include "StaticData.h"  // needed to determine the FactorDelimiter
#include "util/exception.hh"
#include "util/tokenize_piece.hh"

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
  const std::string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();
  bool firstPass = true;
  for (unsigned int i = 0 ; i < factorType.size() ; i++) {
	UTIL_THROW_IF2(factorType[i] >= MAX_NUM_FACTORS,
				"Trying to reference factor " << factorType[i] << ". Max factor is " << MAX_NUM_FACTORS);

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

StringPiece Word::GetString(FactorType factorType) const
{
  return m_factorArray[factorType]->GetString();
}

class StrayFactorException : public util::Exception {};

void Word::CreateFromString(FactorDirection direction
                            , const std::vector<FactorType> &factorOrder
                            , const StringPiece &str
                            , bool isNonTerminal)
{
  FactorCollection &factorCollection = FactorCollection::Instance();

  util::TokenIter<util::MultiCharacter> fit(str, StaticData::Instance().GetFactorDelimiter());
  for (size_t ind = 0; ind < factorOrder.size() && fit; ++ind, ++fit) {
    m_factorArray[factorOrder[ind]] = factorCollection.AddFactor(*fit);
  }
  UTIL_THROW_IF(fit, StrayFactorException, "You have configured " << factorOrder.size() << " factors but the word " << str << " contains factor delimiter " << StaticData::Instance().GetFactorDelimiter() << " too many times.");

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
  m_isOOV = true;
}

void Word::OnlyTheseFactors(const FactorMask &factors)
{
  for (unsigned int currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; currFactor++) {
    if (!factors[currFactor]) {
      SetFactor(currFactor, NULL);
    }
  }
}

bool Word::IsEpsilon() const
{
       const Factor *factor = m_factorArray[0];
       int compare = factor->GetString().compare(EPSILON);

       return compare == 0;
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

