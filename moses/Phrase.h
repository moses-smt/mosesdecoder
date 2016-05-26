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

#ifndef moses_Phrase_h
#define moses_Phrase_h

#include <iostream>
#include <vector>
#include <list>
#include <string>

#include <boost/functional/hash.hpp>

#include "Word.h"
#include "Util.h"

#include "util/string_piece.hh"
#include "util/exception.hh"
#include "parameters/AllOptions.h"

namespace Moses
{
class FactorMask;
class Range;
class ContextScope;

/** Representation of a phrase, ie. a contiguous number of words.
 *  Wrapper for vector of words
 */
class Phrase
{
  friend std::ostream& operator<<(std::ostream&, const Phrase&);
  // private:
protected:
  std::vector<Word>			m_words;

public:

  virtual bool HasScope() const {
    return false;
  }

  virtual SPTR<ContextScope> GetScope() const {
    return SPTR<ContextScope>();
  }


  /** No longer does anything as not using mem pool for Phrase class anymore */
  static void InitializeMemPool();
  static void FinalizeMemPool();

  /** create empty phrase
  */
  Phrase();
  explicit Phrase(size_t reserveSize);
  /** create phrase from vectors of words	*/
  explicit Phrase(const std::vector< const Word* > &mergeWords);

  /* This isn't a swap function because classes inherit from Phrase and might
   * not override swap, which would be bad.
   */
  void SwapWords(Phrase &other) {
    swap(m_words, other.m_words);
  }

  /** destructor */
  virtual ~Phrase();

  /**
   * Fills phrase with words from format string, typically from phrase table or sentence input
   *
   * \param factorOrder  factor types of each element in 2D string vector
   * \param phraseString formatted input string to parse
   * \param lhs          returns the non-terminal Word for the left-hand side of an SCFG rule, may be NULL for phrase-based
  */
  void CreateFromString(FactorDirection direction,
                        const std::vector<FactorType> &factorOrder,
                        const StringPiece &phraseString,
                        Word **lhs);

  /**	copy factors from the other phrase to this phrase.
  	IsCompatible() must be run beforehand to ensure incompatible factors aren't overwritten
  */
  void MergeFactors(const Phrase &copy);
  //! copy a single factor (specified by factorType)
  void MergeFactors(const Phrase &copy, FactorType factorType);
  //! copy all factors specified in factorVec and none others
  void MergeFactors(const Phrase &copy, const std::vector<FactorType>& factorVec);

  /** compare 2 phrases to ensure no factors are lost if the phrases are merged
  *	must run IsCompatible() to ensure incompatible factors aren't being overwritten
  */
  bool IsCompatible(const Phrase &inputPhrase) const;
  bool IsCompatible(const Phrase &inputPhrase, FactorType factorType) const;
  bool IsCompatible(const Phrase &inputPhrase, const std::vector<FactorType>& factorVec) const;

  //! number of words
  inline size_t GetSize() const {
    return m_words.size();
  }

  //! word at a particular position
  inline const Word &GetWord(size_t pos) const {
    return m_words[pos];
  }
  inline Word &GetWord(size_t pos) {
    return m_words[pos];
  }

  inline Word &Front() {
    return m_words[0];
  }

  inline Word &Back() {
    return m_words[GetSize() - 1];
  }

  inline const Word &Front() const {
    return m_words[0];
  }

  inline const Word &Back() const {
    return m_words[GetSize() - 1];
  }

  //! particular factor at a particular position
  inline const Factor *GetFactor(size_t pos, FactorType factorType) const {
    const Word &ptr = m_words[pos];
    return ptr[factorType];
  }
  inline void SetFactor(size_t pos, FactorType factorType, const Factor *factor) {
    Word &ptr = m_words[pos];
    ptr[factorType] = factor;
  }

  size_t GetNumTerminals() const;
  size_t GetNumNonTerminals() const {
    return GetSize() - GetNumTerminals();
  }

  //! whether the 2D vector is a substring of this phrase
  bool Contains(const std::vector< std::vector<std::string> > &subPhraseVector
                , const std::vector<FactorType> &inputFactor) const;

  size_t Find(const Phrase &sought, int maxUnknown) const;

  //! create an empty word at the end of the phrase
  Word &AddWord();
  //! create copy of input word at the end of the phrase
  void AddWord(const Word &newWord) {
    AddWord() = newWord;
  }

  /** appends a phrase at the end of current phrase **/
  void Append(const Phrase &endPhrase);
  void PrependWord(const Word &newWord);

  void Clear() {
    m_words.clear();
  }

  void RemoveWord(size_t pos) {
    UTIL_THROW_IF2(pos >= m_words.size(),
                   "Referencing position " << pos << " out of bound");
    m_words.erase(m_words.begin() + pos);
  }

  void InitStartEndWord();

  //! create new phrase class that is a substring of this phrase
  Phrase GetSubString(const Range &range) const;
  Phrase GetSubString(const Range &range, FactorType factorType) const;

  //! return a string rep of the phrase;
  // w/ factors delimited by FactorDelimiter
  std::string
  GetStringRep(std::vector<FactorType> const& factorsToPrint,
               AllOptions const* opts=NULL) const;

  TO_STRING();


  int Compare(const Phrase &other) const;

  /** transitive comparison between 2 phrases
   *		used to insert & find phrase in dictionary
   */
  bool operator< (const Phrase &compare) const {
    return Compare(compare) < 0;
  }

  size_t hash() const;

  bool operator==(const Phrase &compare) const;
  bool operator!=(const Phrase &compare) const {
    return ! (*this == compare);
  }

  void OnlyTheseFactors(const FactorMask &factors);

};

inline size_t hash_value(const Phrase& phrase)
{
  return phrase.hash();
}

struct PhrasePtrComparator {
  inline bool operator()(const Phrase* lhs, const Phrase* rhs) const {
    return *lhs == *rhs;
  }
};

struct PhrasePtrHasher {
  inline size_t operator()(const Phrase* phrase) const {
    size_t seed = 0;
    boost::hash_combine(seed,*phrase);
    return seed;
  }

};

}

#endif
