/*
 * PhraseImpl.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <cstddef>
#include <string>
#include <sstream>
#include <iostream>
#include "Word.h"
#include "MemPool.h"
#include "TypeDef.h"
#include "legacy/FactorCollection.h"
#include "SCFG/Word.h"

namespace Moses2
{

template<typename WORD>
class SubPhrase;

class Scores;
class PhraseTable;
class MemPool;
class System;

template<typename WORD>
class Phrase
{
public:
  virtual ~Phrase() {
  }
  virtual const WORD& operator[](size_t pos) const = 0;
  virtual size_t GetSize() const = 0;

  virtual const WORD& Back() const {
    return (*this)[GetSize() - 1];
  }

  virtual size_t hash() const {
    size_t seed = 0;

    for (size_t i = 0; i < GetSize(); ++i) {
      const WORD &word = (*this)[i];
      size_t wordHash = word.hash();
      boost::hash_combine(seed, wordHash);
    }

    return seed;
  }

  virtual bool operator==(const Phrase &compare) const {
    if (GetSize() != compare.GetSize()) {
      return false;
    }

    for (size_t i = 0; i < GetSize(); ++i) {
      const WORD &word = (*this)[i];
      const WORD &otherWord = compare[i];
      if (word != otherWord) {
        return false;
      }
    }

    return true;
  }

  virtual bool operator!=(const Phrase &compare) const {
    return !((*this) == compare);
  }

  virtual std::string GetString(const FactorList &factorTypes) const {
    if (GetSize() == 0) {
      return "";
    }

    std::stringstream ret;

    const WORD &word = (*this)[0];
    ret << word.GetString(factorTypes);
    for (size_t i = 1; i < GetSize(); ++i) {
      const WORD &word = (*this)[i];
      ret << " " << word.GetString(factorTypes);
    }
    return ret.str();
  }

  virtual SubPhrase<WORD> GetSubPhrase(size_t start, size_t size) const = 0;

  virtual std::string Debug(const System &system) const {
    std::stringstream out;
    size_t size = GetSize();
    if (size) {
      out << (*this)[0].Debug(system);
      for (size_t i = 1; i < size; ++i) {
        const WORD &word = (*this)[i];
        out << " " << word.Debug(system);
      }
    }

    return out.str();
  }

  virtual void OutputToStream(const System &system, std::ostream &out) const {
    size_t size = GetSize();
    if (size) {
      (*this)[0].OutputToStream(system, out);
      for (size_t i = 1; i < size; ++i) {
        const WORD &word = (*this)[i];
        out << " ";
        word.OutputToStream(system, out);
      }
    }
  }


};

////////////////////////////////////////////////////////////////////////
template<typename WORD>
class PhraseOrdererLexical
{
public:
  bool operator()(const Phrase<WORD> &a, const Phrase<WORD> &b) const {
    size_t minSize = std::min(a.GetSize(), b.GetSize());
    for (size_t i = 0; i < minSize; ++i) {
      const Word &aWord = a[i];
      const Word &bWord = b[i];
      int cmp = aWord.Compare(bWord);
      //std::cerr << "WORD: " << aWord << " ||| " << bWord << " ||| " << lessThan << std::endl;
      if (cmp) {
        return (cmp < 0);
      }
    }
    return a.GetSize() < b.GetSize();
  }
};

}

