/*
 * TargetPhraseImpl.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <iostream>
#include "../Phrase.h"
#include "../PhraseImplTemplate.h"
#include "../MemPool.h"
#include "../SubPhrase.h"
#include "../AlignmentInfoCollection.h"
#include "Word.h"

namespace Moses2
{
class Scores;
class Manager;
class System;
class PhraseTable;
class AlignmentInfo;

namespace SCFG
{

class TargetPhraseImpl: public Moses2::TargetPhrase, public PhraseImplTemplate<SCFG::Word>
{
  friend std::ostream& operator<<(std::ostream &, const SCFG::TargetPhraseImpl &);
public:
  SCFG::Word lhs;
  const AlignmentInfo* m_alignTerm, *m_alignNonTerm;

  static TargetPhraseImpl *CreateFromString(MemPool &pool,
      const PhraseTable &pt, const System &system, const std::string &str);

  TargetPhraseImpl(MemPool &pool, const PhraseTable &pt, const System &system,
      size_t size);
  //TargetPhraseImpl(MemPool &pool, const System &system, const TargetPhraseImpl &copy);

  virtual ~TargetPhraseImpl();

  const Word& operator[](size_t pos) const
  {
    return m_words[pos];
  }

  Word& operator[](size_t pos)
  {
    return m_words[pos];
  }

  size_t GetSize() const
  {
    return m_size;
  }

  SubPhrase GetSubPhrase(size_t start, size_t size) const
  {
    SubPhrase ret(*this, start, size);
    return ret;
  }

  // ALNREP = alignment representation,
  // see AlignmentInfo constructors for supported representations
  template<typename ALNREP>
  void
  SetAlignTerm(const ALNREP &coll) {
    m_alignTerm = AlignmentInfoCollection::Instance().Add(coll);
  }

  const AlignmentInfo &GetAlignTerm() const {
    return *m_alignTerm;
  }
  const AlignmentInfo &GetAlignNonTerm() const {
    return *m_alignNonTerm;
  }

  // ALNREP = alignment representation,
  // see AlignmentInfo constructors for supported representations
  template<typename ALNREP>
  void
  SetAlignNonTerm(const ALNREP &coll) {
    m_alignNonTerm = AlignmentInfoCollection::Instance().Add(coll);
  }

  void SetAlignmentInfo(const std::string &alignString);

  //mutable void *chartState;
protected:
};

}

}

