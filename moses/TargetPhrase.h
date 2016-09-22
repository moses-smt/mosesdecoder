// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
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

#include <algorithm>
#include <vector>
#include "TypeDef.h"
#include "Phrase.h"
#include "ScoreComponentCollection.h"
#include "AlignmentInfo.h"
#include "AlignmentInfoCollection.h"
#include "moses/PP/PhraseProperty.h"
#include "util/string_piece.hh"
//#include "moses/TranslationTask.h"

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#ifdef HAVE_PROTOBUF
#include "rule.pb.h"
#endif

namespace Moses
{
class FeatureFunction;
class InputPath;
class InputPath;
class PhraseDictionary;

/** represents an entry on the target side of a phrase table (scores, translation, alignment)
 */
class TargetPhrase: public Phrase
{
public:
  typedef std::map<FeatureFunction const*, boost::shared_ptr<Scores> > ScoreCache_t;
  ScoreCache_t const& GetExtraScores() const;
  Scores const* GetExtraScores(FeatureFunction const* ff) const;
  void SetExtraScores(FeatureFunction const* ff,boost::shared_ptr<Scores> const& scores);

  typedef std::map<size_t const, std::vector<SPTR<std::vector<float> > > > CoordCache_t;
  std::vector<SPTR<std::vector<float> > > const* GetCoordList(size_t const spaceID) const;
  void PushCoord(size_t const spaceID, SPTR<std::vector<float> > const coord);

private:
  ScoreCache_t m_cached_scores;
  SPTR<CoordCache_t> m_cached_coord;
  WPTR<ContextScope> m_scope;

private:
  friend std::ostream& operator<<(std::ostream&, const TargetPhrase&);
  friend void swap(TargetPhrase &first, TargetPhrase &second);

  float m_futureScore, m_estimatedScore;
  ScoreComponentCollection m_scoreBreakdown;

  const AlignmentInfo* m_alignTerm, *m_alignNonTerm;
  const Word *m_lhsTarget;
  mutable Phrase *m_ruleSource; // to be set by the feature function that needs it.

  typedef std::map<std::string, boost::shared_ptr<PhraseProperty> > Properties;
  Properties m_properties;

  const PhraseDictionary *m_container;

  mutable boost::unordered_map<const std::string, boost::shared_ptr<void> > m_data;

public:
  TargetPhrase(const PhraseDictionary *pt = NULL);
  TargetPhrase(std::string out_string, const PhraseDictionary *pt = NULL);
  TargetPhrase(const TargetPhrase &copy);
  explicit TargetPhrase(const Phrase &targetPhrase, const PhraseDictionary *pt);

  /*ttasksptr version*/
  TargetPhrase(ttasksptr &ttask, const PhraseDictionary *pt = NULL);
  TargetPhrase(ttasksptr &ttask, std::string out_string, const PhraseDictionary *pt = NULL);
  explicit TargetPhrase(ttasksptr &ttask, const Phrase &targetPhrase, const PhraseDictionary *pt);

  // ttasksptr GetTtask() const;
  // bool HasTtaskSPtr() const;

  bool HasScope() const;
  SPTR<ContextScope> GetScope() const;

  ~TargetPhrase();

  // 1st evaluate method. Called during loading of phrase table.
  void EvaluateInIsolation(const Phrase &source, const std::vector<FeatureFunction*> &ffs);

  // as above, score with ALL FFs
  // Used only for OOV processing. Doesn't have a phrase table connect with it
  void EvaluateInIsolation(const Phrase &source);

  // 'inputPath' is guaranteed to be the raw substring from the input. No factors were added or taken away
  void EvaluateWithSourceContext(const InputType &input, const InputPath &inputPath);

  void UpdateScore(ScoreComponentCollection *futureScoreBreakdown = NULL);

  void SetSparseScore(const FeatureFunction* translationScoreProducer, const StringPiece &sparseString);

  // used to set translation or gen score
  void SetXMLScore(float score);

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
    return m_futureScore;
  }

  inline const ScoreComponentCollection &GetScoreBreakdown() const {
    return m_scoreBreakdown;
  }
  inline ScoreComponentCollection &GetScoreBreakdown() {
    return m_scoreBreakdown;
  }

  /*
    //TODO: Probably shouldn't copy this, but otherwise ownership is unclear
    void SetSourcePhrase(const Phrase&  p) {
      m_sourcePhrase=p;
    }
    const Phrase& GetSourcePhrase() const {
      return m_sourcePhrase;
    }
  */
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

  // ALNREP = alignment representation,
  // see AlignmentInfo constructors for supported representations
  template<typename ALNREP>
  void
  SetAlignTerm(const ALNREP &coll) {
    m_alignTerm = AlignmentInfoCollection::Instance().Add(coll);
  }

  // ALNREP = alignment representation,
  // see AlignmentInfo constructors for supported representations
  template<typename ALNREP>
  void
  SetAlignNonTerm(const ALNREP &coll) {
    m_alignNonTerm = AlignmentInfoCollection::Instance().Add(coll);
  }


  const AlignmentInfo &GetAlignTerm() const {
    return *m_alignTerm;
  }
  const AlignmentInfo &GetAlignNonTerm() const {
    return *m_alignNonTerm;
  }

  const Phrase *GetRuleSource() const {
    return m_ruleSource;
  }

  const PhraseDictionary *GetContainer() const {
    return m_container;
  }

  bool SetData(const std::string& key, boost::shared_ptr<void> value) const {
    std::pair< boost::unordered_map<const std::string, boost::shared_ptr<void> >::iterator, bool > inserted =
      m_data.insert( std::pair<const std::string, boost::shared_ptr<void> >(key,value) );
    if (!inserted.second) {
      return false;
    }
    return true;
  }

  boost::shared_ptr<void> GetData(const std::string& key) const {
    boost::unordered_map<const std::string, boost::shared_ptr<void> >::const_iterator found = m_data.find(key);
    if (found == m_data.end()) {
      return boost::shared_ptr<void>();
    }
    return found->second;
  }



  // To be set by the FF that needs it, by default the rule source = NULL
  // make a copy of the source side of the rule
  void SetRuleSource(const Phrase &ruleSource) const;

  void SetProperties(const StringPiece &str);
  void SetProperty(const std::string &key, const std::string &value);
  const PhraseProperty *GetProperty(const std::string &key) const;

  void Merge(const TargetPhrase &copy, const std::vector<FactorType>& factorVec);

  bool operator< (const TargetPhrase &compare) const; // NOT IMPLEMENTED
  bool operator== (const TargetPhrase &compare) const; // NOT IMPLEMENTED

  TO_STRING();
};

void swap(TargetPhrase &first, TargetPhrase &second);

std::ostream& operator<<(std::ostream&, const TargetPhrase&);

}

#endif
