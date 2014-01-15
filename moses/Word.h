// $Id$

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

#ifndef moses_Word_h
#define moses_Word_h

#include <cstring>
#include <iostream>
#include <vector>
#include <list>

#include "util/murmur_hash.hh"

#include "TypeDef.h"
#include "Util.h"
#include "util/string_piece.hh"

namespace Moses
{
class Factor;
class FactorMask;

/** Represent a word (terminal or non-term)
 * Wrapper around hold a set of factors for a single word
 */
class Word
{
  friend std::ostream& operator<<(std::ostream&, const Word&);

protected:

  typedef const Factor * FactorArray[MAX_NUM_FACTORS];

  FactorArray m_factorArray; /**< set of factors */
  bool m_isNonTerminal;
  bool m_isOOV;

public:
  /** deep copy */
  Word(const Word &copy)
    :m_isNonTerminal(copy.m_isNonTerminal)
    ,m_isOOV(copy.m_isOOV) {
    std::memcpy(m_factorArray, copy.m_factorArray, sizeof(FactorArray));
  }

  /** empty word */
  explicit Word(bool isNonTerminal = false) {
    std::memset(m_factorArray, 0, sizeof(FactorArray));
    m_isNonTerminal = isNonTerminal;
    m_isOOV = false;
  }

  ~Word() {}

  //! returns Factor pointer for particular FactorType
  const Factor*& operator[](FactorType index) {
    return m_factorArray[index];
  }

  const Factor * const & operator[](FactorType index) const {
    return m_factorArray[index];
  }

  //! Deprecated. should use operator[]
  inline const Factor* GetFactor(FactorType factorType) const {
    return m_factorArray[factorType];
  }
  inline void SetFactor(FactorType factorType, const Factor *factor) {
    m_factorArray[factorType] = factor;
  }

  inline bool IsNonTerminal() const {
    return m_isNonTerminal;
  }
  inline void SetIsNonTerminal(bool val) {
    m_isNonTerminal = val;
  }

  inline bool IsOOV() const {
    return m_isOOV;
  }
  inline void SetIsOOV(bool val) {
    m_isOOV = val;
  }

  bool IsEpsilon() const;

  /** add the factors from sourceWord into this representation,
   * NULL elements in sourceWord will be skipped */
  void Merge(const Word &sourceWord);

  /** get string representation of list of factors. Used by PDTimp so supposed
  * to be invariant to changes in format of debuggin output, therefore, doesn't
  * use streaming output or ToString() from any class so not dependant on
  * these debugging functions.
  */
  std::string GetString(const std::vector<FactorType> factorType,bool endWithBlank) const;
  StringPiece  GetString(FactorType factorType) const;
  TO_STRING();

  //! transitive comparison of Word objects
  inline bool operator< (const Word &compare) const {
    // needed to store word in GenerationDictionary map
    // uses comparison of FactorKey
    // 'proper' comparison, not address/id comparison
    return Compare(*this, compare) < 0;
  }

  inline bool operator== (const Word &compare) const {
    // needed to store word in GenerationDictionary map
    // uses comparison of FactorKey
    // 'proper' comparison, not address/id comparison
    return Compare(*this, compare) == 0;
  }

  inline bool operator!= (const Word &compare) const {
    return Compare(*this, compare) != 0;
  }

  int Compare(const Word &other) const {
    return Compare(*this, other);
  }


  /* static functions */

  /** transitive comparison of 2 word objects. Used by operator<.
  *	Only compare the co-joined factors, ie. where factor exists for both words.
  *	Should make it non-static
  */
  static int Compare(const Word &targetWord, const Word &sourceWord);

  void CreateFromString(FactorDirection direction
                        , const std::vector<FactorType> &factorOrder
                        , const StringPiece &str
                        , bool isNonTerminal);

  void CreateUnknownWord(const Word &sourceWord);

  void OnlyTheseFactors(const FactorMask &factors);

  inline size_t hash() const {
    return util::MurmurHashNative(m_factorArray, MAX_NUM_FACTORS*sizeof(Factor*), m_isNonTerminal);
  }
};

struct WordComparer {
  //! returns true if hypoA can be recombined with hypoB
  bool operator()(const Word *a, const Word *b) const {
    return *a < *b;
  }
};


inline size_t hash_value(const Word& word)
{
  return word.hash();
}

}

#endif
