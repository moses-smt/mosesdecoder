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

#ifndef moses_TargetPhrase_h
#define moses_TargetPhrase_h

#include <vector>
#include "TypeDef.h"
#include "Phrase.h"
#include "ScoreComponentCollection.h"
#include "AlignmentInfo.h"

#include "util/string_piece.hh"

#ifdef HAVE_PROTOBUF
#include "rule.pb.h"
#endif

namespace Moses
{

class FeatureFunction;

/** represents an entry on the target side of a phrase table (scores, translation, alignment)
 */
class TargetPhrase: public Phrase
{
  friend std::ostream& operator<<(std::ostream&, const TargetPhrase&);
protected:
  float m_fullScore;
  ScoreComponentCollection m_scoreBreakdown;

  // in case of confusion net, ptr to source phrase
  Phrase m_sourcePhrase;
  const AlignmentInfo* m_alignTerm, *m_alignNonTerm;
  const Word *m_lhsTarget;

public:
  TargetPhrase();
  TargetPhrase(const TargetPhrase &copy);
  explicit TargetPhrase(std::string out_string);
  explicit TargetPhrase(const Phrase &targetPhrase);
  ~TargetPhrase();

  void Evaluate(const Phrase &source);
  void Evaluate(const Phrase &source, const std::vector<FeatureFunction*> &ffs);

  void Evaluate(const InputType &input);

  void SetSparseScore(const FeatureFunction* translationScoreProducer, const StringPiece &sparseString);

  // used to set translation or gen score
  void SetXMLScore(float score);
  void SetInputScore(const Scores &scoreVector);

  TargetPhrase *MergeNext(const TargetPhrase &targetPhrase) const;
  // used for translation step

#ifdef HAVE_PROTOBUF
  void WriteToRulePB(hgmert::Rule* pb) const;
#endif

  /***
   * return the estimated score resulting from our being added to a sentence
   * (it's an estimate because we don't have full n-gram info for the language model
   *  without using the (unknown) full sentence)
   *
   */
  inline float GetFutureScore() const {
    return m_fullScore;
  }

  inline const ScoreComponentCollection &GetScoreBreakdown() const {
    return m_scoreBreakdown;
  }
  inline ScoreComponentCollection &GetScoreBreakdown() {
    return m_scoreBreakdown;
  }

  //TODO: Probably shouldn't copy this, but otherwise ownership is unclear
  void SetSourcePhrase(const Phrase&  p) {
    m_sourcePhrase=p;
  }
  const Phrase& GetSourcePhrase() const {
    return m_sourcePhrase;
  }

  void SetTargetLHS(const Word *lhs) {
    m_lhsTarget = lhs;
  }
  const Word &GetTargetLHS() const {
    return *m_lhsTarget;
  }

  void SetAlignmentInfo(const StringPiece &alignString);
  void SetAlignTerm(const AlignmentInfo *alignTerm) {
    m_alignTerm = alignTerm;
  }
  void SetAlignNonTerm(const AlignmentInfo *alignNonTerm) {
    m_alignNonTerm = alignNonTerm;
  }

  void SetAlignTerm(const AlignmentInfo::CollType &coll);
  void SetAlignNonTerm(const AlignmentInfo::CollType &coll);

  const AlignmentInfo &GetAlignTerm() const {
    return *m_alignTerm;
  }
  const AlignmentInfo &GetAlignNonTerm() const {
    return *m_alignNonTerm;
  }


  TO_STRING();
};

std::ostream& operator<<(std::ostream&, const TargetPhrase&);

/**
 * Hasher that looks at source and target phrase.
 **/
struct TargetPhraseHasher {
  inline size_t operator()(const TargetPhrase& targetPhrase) const {
    size_t seed = 0;
    boost::hash_combine(seed, targetPhrase);
    boost::hash_combine(seed, targetPhrase.GetSourcePhrase());
    boost::hash_combine(seed, targetPhrase.GetAlignTerm());
    boost::hash_combine(seed, targetPhrase.GetAlignNonTerm());

    return seed;
  }
};

struct TargetPhraseComparator {
  inline bool operator()(const TargetPhrase& lhs, const TargetPhrase& rhs) const {
    return lhs.Compare(rhs) == 0 &&
           lhs.GetSourcePhrase().Compare(rhs.GetSourcePhrase()) == 0 &&
           lhs.GetAlignTerm() == rhs.GetAlignTerm() &&
           lhs.GetAlignNonTerm() == rhs.GetAlignNonTerm();
  }

};

}

#endif
