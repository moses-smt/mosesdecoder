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
#include "util/string_stream.hh"
#include "util/tokenize_piece.hh"

using namespace std;

namespace Moses
{

// utility function for factorless decoding
size_t
max_fax()
{
  if (StaticData::Instance().GetFactorDelimiter().size())
    return MAX_NUM_FACTORS;
  return 1;
}

// static
int Word::Compare(const Word &targetWord, const Word &sourceWord)
{
  if (targetWord.IsNonTerminal() != sourceWord.IsNonTerminal()) {
    return targetWord.IsNonTerminal() ? -1 : 1;
  }

  for (size_t factorType = 0 ; factorType < MAX_NUM_FACTORS ; factorType++) {
    const Factor *targetFactor = targetWord[factorType];
    const Factor *sourceFactor = sourceWord[factorType];

    if (targetFactor == NULL || sourceFactor == NULL)
      continue;
    if (targetFactor == sourceFactor)
      continue;

    return (targetFactor<sourceFactor) ? -1 : +1;
  }
  return 0;
}

bool Word::operator==(const Word &compare) const
{
  if (IsNonTerminal() != compare.IsNonTerminal()) {
    return false;
  }

  for (size_t factorType = 0 ; factorType < MAX_NUM_FACTORS ; factorType++) {
    const Factor *thisFactor = GetFactor(factorType);
    const Factor *otherFactor = compare.GetFactor(factorType);

    if (thisFactor != otherFactor) {
      return false;
    }
  }
  return true;
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
  util::StringStream strme;
  const std::string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();
  bool firstPass = true;
  unsigned int stop = min(max_fax(),factorType.size());
  for (unsigned int i = 0 ; i < stop ; i++) {
    UTIL_THROW_IF2(factorType[i] >= MAX_NUM_FACTORS,
                   "Trying to reference factor " << factorType[i]
                   << ". Max factor is " << MAX_NUM_FACTORS);

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

void
Word::
CreateFromString(FactorDirection direction
                 , const std::vector<FactorType> &factorOrder
                 , const StringPiece &str
                 , bool isNonTerminal
                 , bool strict)
{
  FactorCollection &factorCollection = FactorCollection::Instance();
  vector<StringPiece> bits(MAX_NUM_FACTORS);
  string factorDelimiter = StaticData::Instance().GetFactorDelimiter();
  if (factorDelimiter.size()) {
    util::TokenIter<util::MultiCharacter> fit(str, factorDelimiter);
    size_t i = 0;
    for (; i < MAX_NUM_FACTORS && fit; ++i,++fit)
      bits[i] = *fit;
    if (i == MAX_NUM_FACTORS)
      UTIL_THROW_IF(fit, StrayFactorException,
                    "The hard limit for factors is " << MAX_NUM_FACTORS
                    << ". The word " << str << " contains factor delimiter "
                    << StaticData::Instance().GetFactorDelimiter()
                    << " too many times.");
    if (strict)
      UTIL_THROW_IF(fit, StrayFactorException,
                    "You have configured " << factorOrder.size()
                    << " factors but the word " << str
                    << " contains factor delimiter "
                    << StaticData::Instance().GetFactorDelimiter()
                    << " too many times.");
    UTIL_THROW_IF(!isNonTerminal && i < factorOrder.size(),util::Exception,
                  "Too few factors in string '" << str << "'.");
  } else {
    bits[0] = str;
  }
  for (size_t k = 0; k < factorOrder.size(); ++k) {
    UTIL_THROW_IF(factorOrder[k] >= MAX_NUM_FACTORS, util::Exception,
                  "Factor order out of bounds.");
    m_factorArray[factorOrder[k]] = factorCollection.AddFactor(bits[k], isNonTerminal);
  }
  // assume term/non-term same for all factors
  m_isNonTerminal = isNonTerminal;
}

void Word::CreateUnknownWord(const Word &sourceWord)
{
  FactorCollection &factorCollection = FactorCollection::Instance();

  m_isNonTerminal = sourceWord.IsNonTerminal();

  // const std::string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();
  unsigned int stop = max_fax();
  for (unsigned int currFactor = 0 ; currFactor < stop; currFactor++) {
    FactorType factorType = static_cast<FactorType>(currFactor);

    const Factor *sourceFactor = sourceWord[currFactor];
    if (sourceFactor == NULL)
      SetFactor(factorType, factorCollection.AddFactor(Output, factorType, UNKNOWN_FACTOR, m_isNonTerminal));
    else
      SetFactor(factorType, factorCollection.AddFactor(Output, factorType, sourceFactor->GetString(), m_isNonTerminal));
  }

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
  util::StringStream strme;
  const std::string& factorDelimiter
  = StaticData::Instance().options()->output.factor_delimiter;
  bool firstPass = true;
  unsigned int stop = max_fax();
  for (unsigned int currFactor = 0 ; currFactor < stop; currFactor++) {
    FactorType factorType = static_cast<FactorType>(currFactor);
    const Factor *factor = word.GetFactor(factorType);
    if (factor != NULL) {
      if (firstPass) {
        firstPass = false;
      } else {
        strme << factorDelimiter;
      }
      strme << factor->GetString();
    }
  }
  out << strme.str() << " ";
  return out;
}

}

