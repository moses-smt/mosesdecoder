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
#include "../TargetPhrase.h"
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

class TargetPhraseImpl: public Moses2::TargetPhrase<SCFG::Word>
{
public:
  SCFG::Word lhs;
  const AlignmentInfo* m_alignTerm, *m_alignNonTerm;
  SCORE sortScore;

  static TargetPhraseImpl *CreateFromString(MemPool &pool,
      const PhraseTable &pt, const System &system, const std::string &str);

  TargetPhraseImpl(MemPool &pool, const PhraseTable &pt, const System &system,
      size_t size);
  //TargetPhraseImpl(MemPool &pool, const System &system, const TargetPhraseImpl &copy);

  virtual ~TargetPhraseImpl();

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

  virtual SCORE GetScoreForPruning() const
  { return sortScore; }

  std::string Debug(const System &system) const;

  //mutable void *chartState;
protected:
};


}
}

