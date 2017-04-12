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

#include <algorithm>
#include <cstdlib>
#include "util/exception.hh"
#include "util/tokenize_piece.hh"

#include "TargetPhrase.h"
#include "GenerationDictionary.h"
#include "LM/Base.h"
#include "StaticData.h"
#include "ScoreComponentCollection.h"
#include "Util.h"
#include "AlignmentInfoCollection.h"
#include "InputPath.h"
#include "TranslationTask.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include <boost/foreach.hpp>

using namespace std;

namespace Moses
{
TargetPhrase::TargetPhrase( std::string out_string, const PhraseDictionary *pt)
  :Phrase(0)
  , m_futureScore(0.0)
  , m_estimatedScore(0.0)
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
  , m_ruleSource(NULL)
  , m_container(pt)
{
  //ACAT
  const StaticData &staticData = StaticData::Instance();
  // XXX should this really be InputFactorOrder???
  CreateFromString(Output, staticData.options()->input.factor_order, out_string,
                   NULL);
}

TargetPhrase::TargetPhrase(ttasksptr& ttask, std::string out_string, const PhraseDictionary *pt)
  :Phrase(0)
  , m_futureScore(0.0)
  , m_estimatedScore(0.0)
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
  , m_ruleSource(NULL)
  , m_container(pt)
{
  if (ttask) m_scope = ttask->GetScope();

  // XXX should this really be InputFactorOrder???
  CreateFromString(Output, ttask->options()->input.factor_order, out_string,
                   NULL);
}

TargetPhrase::TargetPhrase(ttasksptr& ttask, const PhraseDictionary *pt)
  : Phrase()
  , m_futureScore(0.0)
  , m_estimatedScore(0.0)
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
  , m_ruleSource(NULL)
  , m_container(pt)
{
  if (ttask) m_scope = ttask->GetScope();
}

TargetPhrase::TargetPhrase(ttasksptr& ttask, const Phrase &phrase, const PhraseDictionary *pt)
  : Phrase(phrase)
  , m_futureScore(0.0)
  , m_estimatedScore(0.0)
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
  , m_ruleSource(NULL)
  , m_container(pt)
{
  if (ttask) m_scope = ttask->GetScope();
}

TargetPhrase::TargetPhrase(const PhraseDictionary *pt)
  :Phrase()
  , m_futureScore(0.0)
  , m_estimatedScore(0.0)
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
  , m_ruleSource(NULL)
  , m_container(pt)
{
}

TargetPhrase::TargetPhrase(const Phrase &phrase, const PhraseDictionary *pt)
  : Phrase(phrase)
  , m_futureScore(0.0)
  , m_estimatedScore(0.0)
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
  , m_ruleSource(NULL)
  , m_container(pt)
{
}

TargetPhrase::TargetPhrase(const TargetPhrase &copy)
  : Phrase(copy)
  , m_cached_coord(copy.m_cached_coord)
  , m_cached_scores(copy.m_cached_scores)
  , m_scope(copy.m_scope)
  , m_futureScore(copy.m_futureScore)
  , m_estimatedScore(copy.m_estimatedScore)
  , m_scoreBreakdown(copy.m_scoreBreakdown)
  , m_alignTerm(copy.m_alignTerm)
  , m_alignNonTerm(copy.m_alignNonTerm)
  , m_properties(copy.m_properties)
  , m_container(copy.m_container)
{
  if (copy.m_lhsTarget) {
    m_lhsTarget = new Word(*copy.m_lhsTarget);
  } else {
    m_lhsTarget = NULL;
  }

  if (copy.m_ruleSource) {
    m_ruleSource = new Phrase(*copy.m_ruleSource);
  } else {
    m_ruleSource = NULL;
  }
}

TargetPhrase::~TargetPhrase()
{
  //cerr << "m_lhsTarget=" << m_lhsTarget << endl;

  delete m_lhsTarget;
  delete m_ruleSource;
}

#ifdef HAVE_PROTOBUF
void TargetPhrase::WriteToRulePB(hgmert::Rule* pb) const
{
  pb->add_trg_words("[X,1]");
  for (size_t pos = 0 ; pos < GetSize() ; pos++)
    pb->add_trg_words(GetWord(pos)[0]->GetString());
}
#endif

bool TargetPhrase::HasScope() const
{
  return !m_scope.expired(); // should actually never happen
}

SPTR<ContextScope> TargetPhrase::GetScope() const
{
  return m_scope.lock();
}

void TargetPhrase::EvaluateInIsolation(const Phrase &source)
{
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  EvaluateInIsolation(source, ffs);
}

void TargetPhrase::EvaluateInIsolation(const Phrase &source, const std::vector<FeatureFunction*> &ffs)
{
  if (ffs.size()) {
    const StaticData &staticData = StaticData::Instance();
    ScoreComponentCollection estimatedScores;
    for (size_t i = 0; i < ffs.size(); ++i) {
      const FeatureFunction &ff = *ffs[i];
      if (! staticData.IsFeatureFunctionIgnored( ff )) {
        ff.EvaluateInIsolation(source, *this, m_scoreBreakdown, estimatedScores);
      }
    }

    float weightedScore = m_scoreBreakdown.GetWeightedScore();
    m_estimatedScore += estimatedScores.GetWeightedScore();
    m_futureScore = weightedScore + m_estimatedScore;
  }
}

void TargetPhrase::EvaluateWithSourceContext(const InputType &input, const InputPath &inputPath)
{
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  const StaticData &staticData = StaticData::Instance();
  ScoreComponentCollection futureScoreBreakdown;
  for (size_t i = 0; i < ffs.size(); ++i) {
    const FeatureFunction &ff = *ffs[i];
    if (! staticData.IsFeatureFunctionIgnored( ff )) {
      ff.EvaluateWithSourceContext(input, inputPath, *this, NULL, m_scoreBreakdown, &futureScoreBreakdown);
    }
  }
  float weightedScore = m_scoreBreakdown.GetWeightedScore();
  m_estimatedScore += futureScoreBreakdown.GetWeightedScore();
  m_futureScore = weightedScore + m_estimatedScore;
}

void TargetPhrase::UpdateScore(ScoreComponentCollection* futureScoreBreakdown)
{
  float weightedScore = m_scoreBreakdown.GetWeightedScore();
  if(futureScoreBreakdown)
    m_estimatedScore += futureScoreBreakdown->GetWeightedScore();
  m_futureScore = weightedScore + m_estimatedScore;
}

void TargetPhrase::SetXMLScore(float score)
{
  const FeatureFunction* prod = PhraseDictionary::GetColl()[0];
  size_t numScores = prod->GetNumScoreComponents();
  vector <float> scoreVector(numScores,score/numScores);

  m_scoreBreakdown.Assign(prod, scoreVector);
}

void TargetPhrase::SetAlignmentInfo(const StringPiece &alignString)
{
  AlignmentInfo::CollType alignTerm, alignNonTerm;
  for (util::TokenIter<util::AnyCharacter, true> token(alignString, util::AnyCharacter(" \t")); token; ++token) {
    util::TokenIter<util::SingleCharacter, false> dash(*token, util::SingleCharacter('-'));

    char *endptr;
    size_t sourcePos = strtoul(dash->data(), &endptr, 10);
    UTIL_THROW_IF(endptr != dash->data() + dash->size(), util::ErrnoException, "Error parsing alignment" << *dash);
    ++dash;
    size_t targetPos = strtoul(dash->data(), &endptr, 10);
    UTIL_THROW_IF(endptr != dash->data() + dash->size(), util::ErrnoException, "Error parsing alignment" << *dash);
    UTIL_THROW_IF2(++dash, "Extra gunk in alignment " << *token);

    if (GetWord(targetPos).IsNonTerminal()) {
      alignNonTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    } else {
      alignTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    }
  }
  SetAlignTerm(alignTerm);
  SetAlignNonTerm(alignNonTerm);
  //		cerr << "TargetPhrase::SetAlignmentInfo(const StringPiece &alignString) this:|" << *this << "|\n";
}

// void TargetPhrase::SetAlignTerm(const AlignmentInfo::CollType &coll)
// {
//   const AlignmentInfo *alignmentInfo = AlignmentInfoCollection::Instance().Add(coll);
//   m_alignTerm = alignmentInfo;

// }

// void TargetPhrase::SetAlignNonTerm(const AlignmentInfo::CollType &coll)
// {
//   const AlignmentInfo *alignmentInfo = AlignmentInfoCollection::Instance().Add(coll);
//   m_alignNonTerm = alignmentInfo;
// }

void TargetPhrase::SetSparseScore(const FeatureFunction* translationScoreProducer, const StringPiece &sparseString)
{
  m_scoreBreakdown.Assign(translationScoreProducer, sparseString.as_string());
}

boost::shared_ptr<Scores>
mergescores(boost::shared_ptr<Scores> const& a,
            boost::shared_ptr<Scores> const& b)
{
  boost::shared_ptr<Scores> ret;
  if (!a) return b ? b : ret;
  if (!b) return a;
  if (a->size() != b->size()) return ret;
  ret.reset(new Scores(*a));
  for (size_t i = 0; i < a->size(); ++i) {
    if ((*a)[i] == 0) (*a)[i] = (*b)[i];
    else if ((*b)[i]) {
      UTIL_THROW_IF2((*a)[i] != (*b)[i], "can't merge feature vectors");
    }
  }
  return ret;
}

void
TargetPhrase::
Merge(const TargetPhrase &copy, const std::vector<FactorType>& factorVec)
{
  Phrase::MergeFactors(copy, factorVec);
  m_scoreBreakdown.Merge(copy.GetScoreBreakdown());
  m_estimatedScore += copy.m_estimatedScore;
  m_futureScore += copy.m_futureScore;
  typedef ScoreCache_t::iterator iter;
  typedef ScoreCache_t::value_type item;
  BOOST_FOREACH(item const& s, copy.m_cached_scores) {
    pair<iter,bool> foo = m_cached_scores.insert(s);
    if (foo.second == false)
      foo.first->second = mergescores(foo.first->second, s.second);
  }
}

TargetPhrase::ScoreCache_t const&
TargetPhrase::
GetExtraScores() const
{
  return m_cached_scores;
}

Scores const*
TargetPhrase::
GetExtraScores(FeatureFunction const* ff) const
{
  ScoreCache_t::const_iterator m = m_cached_scores.find(ff);
  return m != m_cached_scores.end() ? m->second.get() : NULL;
}

void
TargetPhrase::
SetExtraScores(FeatureFunction const* ff,
               boost::shared_ptr<Scores> const& s)
{
  m_cached_scores[ff] = s;
}

vector<SPTR<vector<float> > > const*
TargetPhrase::
GetCoordList(size_t const spaceID) const
{
  if(!m_cached_coord) {
    return NULL;
  }
  CoordCache_t::const_iterator m = m_cached_coord->find(spaceID);
  if(m == m_cached_coord->end()) {
    return NULL;
  }
  return &m->second;
}

void
TargetPhrase::
PushCoord(size_t const spaceID,
          SPTR<vector<float> > const coord)
{
  if (!m_cached_coord) {
    m_cached_coord.reset(new CoordCache_t);
  }
  vector<SPTR<vector<float> > >& coordList = (*m_cached_coord)[spaceID];
  coordList.push_back(coord);
}

void TargetPhrase::SetProperties(const StringPiece &str)
{
  if (str.size() == 0) {
    return;
  }

  vector<string> toks;
  TokenizeMultiCharSeparator(toks, str.as_string(), "{{");
  for (size_t i = 0; i < toks.size(); ++i) {
    string &tok = toks[i];
    if (tok.empty()) {
      continue;
    }
    size_t endPos = tok.rfind("}");

    tok = tok.substr(0, endPos - 1);

    vector<string> keyValue = TokenizeFirstOnly(tok, " ");
    UTIL_THROW_IF2(keyValue.size() != 2,
                   "Incorrect format of property: " << str);
    SetProperty(keyValue[0], keyValue[1]);
  }
}

void TargetPhrase::SetProperty(const std::string &key, const std::string &value)
{
  const StaticData &staticData = StaticData::Instance();
  const PhrasePropertyFactory& phrasePropertyFactory = staticData.GetPhrasePropertyFactory();
  m_properties[key] = phrasePropertyFactory.ProduceProperty(key,value);
}

const PhraseProperty *TargetPhrase::GetProperty(const std::string &key) const
{
  std::map<std::string, boost::shared_ptr<PhraseProperty> >::const_iterator iter;
  iter = m_properties.find(key);
  if (iter != m_properties.end()) {
    const boost::shared_ptr<PhraseProperty> &pp = iter->second;
    return pp.get();
  }
  return NULL;
}

void TargetPhrase::SetRuleSource(const Phrase &ruleSource) const
{
  if (m_ruleSource == NULL) {
    m_ruleSource = new Phrase(ruleSource);
  }
}

void swap(TargetPhrase &first, TargetPhrase &second)
{
  first.SwapWords(second);
  std::swap(first.m_futureScore, second.m_futureScore);
  std::swap(first.m_estimatedScore, second.m_estimatedScore);
  swap(first.m_scoreBreakdown, second.m_scoreBreakdown);
  std::swap(first.m_alignTerm, second.m_alignTerm);
  std::swap(first.m_alignNonTerm, second.m_alignNonTerm);
  std::swap(first.m_lhsTarget, second.m_lhsTarget);
  std::swap(first.m_cached_scores, second.m_cached_scores);
}

TO_STRING_BODY(TargetPhrase);

std::ostream& operator<<(std::ostream& os, const TargetPhrase& tp)
{
  if (tp.m_lhsTarget) {
    os << *tp.m_lhsTarget<< " -> ";
  }

  os << static_cast<const Phrase&>(tp) << ":" << flush;
  os << tp.GetAlignNonTerm() << flush;
  os << ": term=" << tp.GetAlignTerm() << flush;
  os << ": nonterm=" << tp.GetAlignNonTerm() << flush;
  os << ": c=" << tp.m_futureScore << flush;
  os << " " << tp.m_scoreBreakdown << flush;

  const Phrase *sourcePhrase = tp.GetRuleSource();
  if (sourcePhrase) {
    os << " sourcePhrase=" << *sourcePhrase << flush;
  }

  if (tp.m_properties.size()) {
    os << " properties: " << flush;

    TargetPhrase::Properties::const_iterator iter;
    for (iter = tp.m_properties.begin(); iter != tp.m_properties.end(); ++iter) {
      const string &key = iter->first;
      const PhraseProperty *prop = iter->second.get();
      assert(prop);

      os << key << "=" << *prop << " ";
    }
  }

  return os;
}



}

