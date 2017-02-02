/*
 * TargetPhrase.h
 *
 *  Created on: 26 Apr 2016
 *      Author: hieu
 */

#pragma once
#include <sstream>
#include "PhraseImplTemplate.h"
#include "System.h"
#include "Scores.h"
#include "AlignmentInfoCollection.h"
#include "TranslationModel/PhraseTable.h"

namespace Moses2
{
class AlignmentInfo;

template<typename WORD>
class TargetPhrase: public PhraseImplTemplate<WORD>
{
public:
  typedef PhraseImplTemplate<WORD> Parent;
  const PhraseTable &pt;
  mutable void **ffData;
  SCORE *scoreProperties;

  TargetPhrase(MemPool &pool, const PhraseTable &pt, const System &system, size_t size)
    : PhraseImplTemplate<WORD>(pool, size)
    , pt(pt)
    , scoreProperties(NULL)
    , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo()) {
    m_scores = new (pool.Allocate<Scores>()) Scores(system, pool,
        system.featureFunctions.GetNumScores());
  }

  Scores &GetScores() {
    return *m_scores;
  }

  const Scores &GetScores() const {
    return *m_scores;
  }

  virtual SCORE GetScoreForPruning() const = 0;

  SCORE *GetScoresProperty(int propertyInd) const {
    return scoreProperties ? scoreProperties + propertyInd : NULL;
  }

  const AlignmentInfo &GetAlignTerm() const {
    return *m_alignTerm;
  }

  void SetAlignTerm(const AlignmentInfo &alignInfo) {
    m_alignTerm = &alignInfo;
  }

  // ALNREP = alignment representation,
  // see AlignmentInfo constructors for supported representations
  template<typename ALNREP>
  void
  SetAlignTerm(const ALNREP &coll) {
    m_alignTerm = AlignmentInfoCollection::Instance().Add(coll);
  }

  virtual void SetAlignmentInfo(const std::string &alignString) {
    AlignmentInfo::CollType alignTerm;

    std::vector<std::string> toks = Tokenize(alignString);
    for (size_t i = 0; i < toks.size(); ++i) {
      std::vector<size_t> alignPair = Tokenize<size_t>(toks[i], "-");
      UTIL_THROW_IF2(alignPair.size() != 2, "Wrong alignment format");

      size_t sourcePos = alignPair[0];
      size_t targetPos = alignPair[1];

      alignTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    }

    SetAlignTerm(alignTerm);
    //    cerr << "TargetPhrase::SetAlignmentInfo(const StringPiece &alignString) this:|" << *this << "|\n";

    //cerr << "alignTerm=" << alignTerm.size() << endl;
    //cerr << "alignNonTerm=" << alignNonTerm.size() << endl;

  }

  void OutputToStream(const System &system, const Phrase<WORD> &inputPhrase, std::ostream &out) const {
    // get placeholders
    FactorType placeholderFactor = system.options.input.placeholder_factor;
    std::map<size_t, const Factor*> placeholders;
    if (placeholderFactor != NOT_FOUND) {
      // creates map of target position -> factor for placeholders
      placeholders = GetPlaceholders(system, inputPhrase);
    }

    size_t size = PhraseImplTemplate<WORD>::GetSize();
    for (size_t i = 0; i < size; ++i) {
      // output placeholder, if any
      std::map<size_t, const Factor*>::const_iterator iter = placeholders.find(i);
      if (iter == placeholders.end()) {
        const WORD &word = (*this)[i];
        word.OutputToStream(system, out);
      } else {
        const Factor *factor = iter->second;
        out << *factor;
      }

      out << " ";
    }
  }

  std::map<size_t, const Factor*> GetPlaceholders(const System &system, const Phrase<WORD> &inputPhrase) const {
    FactorType placeholderFactor = system.options.input.placeholder_factor;
    std::map<size_t, const Factor*> ret;
    //std::cerr << "inputPhrase=" << inputPhrase.Debug(system) << std::endl;

    for (size_t sourcePos = 0; sourcePos < inputPhrase.GetSize(); ++sourcePos) {
      const Factor *factor = inputPhrase[sourcePos][placeholderFactor];
      if (factor) {
        //std::cerr << "factor=" << *factor << std::endl;
        //std::cerr << "tp=" << Debug(system) << std::endl;
        std::set<size_t> targetPos = GetAlignTerm().GetAlignmentsForSource(sourcePos);
        UTIL_THROW_IF2(targetPos.size() != 1,
                       "Placeholder should be aligned to 1, and only 1, word:" << targetPos.size() << "!=1");
        ret[*targetPos.begin()] = factor;
      }
    }

    return ret;
  }

  virtual std::string Debug(const System &system) const {
    std::stringstream out;
    out << Phrase<WORD>::Debug(system);
    out << " pt=" << pt.GetName() << " ";
    out << " SCORES:" << GetScores().Debug(system);
    out << " ALIGN-T:";
    out << GetAlignTerm().Debug(system);

    return out.str();
  }

protected:
  Scores *m_scores;
  const AlignmentInfo *m_alignTerm;
};

///////////////////////////////////////////////////////////////////////
template<typename TP>
struct CompareScoreForPruning {
  bool operator()(const TP *a, const TP *b) const {
    return a->GetScoreForPruning() > b->GetScoreForPruning();
  }

  bool operator()(const TP &a, const TP &b) const {
    return a.GetScoreForPruning() > b.GetScoreForPruning();
  }
};

} /* namespace Moses2a */

