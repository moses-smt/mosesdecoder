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

#include <algorithm>
#include <stdlib.h>
#include "util/check.hh"
#include "util/exception.hh"
#include "util/tokenize_piece.hh"

#include "TargetPhrase.h"
#include "GenerationDictionary.h"
#include "LM/Base.h"
#include "StaticData.h"
#include "ScoreComponentCollection.h"
#include "Util.h"
#include "AlignmentInfoCollection.h"

using namespace std;

namespace Moses
{
TargetPhrase::TargetPhrase( std::string out_string)
  :Phrase(0)
  , m_fullScore(0.0)
  , m_futureScore(0.0)
  , m_sourcePhrase(0)
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
{

  //ACAT
  const StaticData &staticData = StaticData::Instance();
  CreateFromString(Output, staticData.GetInputFactorOrder(), out_string, staticData.GetFactorDelimiter(), NULL);
}

TargetPhrase::TargetPhrase()
  :Phrase()
  , m_fullScore(0.0)
  , m_futureScore(0.0)
  ,m_sourcePhrase()
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
{
}

TargetPhrase::TargetPhrase(const Phrase &phrase)
  : Phrase(phrase)
  , m_fullScore(0.0)
  , m_futureScore(0.0)
  , m_sourcePhrase()
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
{
}

TargetPhrase::TargetPhrase(const TargetPhrase &copy)
  : Phrase(copy)
  , m_fullScore(copy.m_fullScore)
  , m_futureScore(copy.m_futureScore)
  , m_sourcePhrase(copy.m_sourcePhrase)
  , m_alignTerm(copy.m_alignTerm)
  , m_alignNonTerm(copy.m_alignNonTerm)
  , m_scoreBreakdown(copy.m_scoreBreakdown)
{
  if (copy.m_lhsTarget) {
    m_lhsTarget = new Word(copy.m_lhsTarget);
  } else {
    m_lhsTarget = NULL;
  }

}

TargetPhrase::~TargetPhrase()
{
  //cerr << "m_lhsTarget=" << m_lhsTarget << endl;

  delete m_lhsTarget;
}

#ifdef HAVE_PROTOBUF
void TargetPhrase::WriteToRulePB(hgmert::Rule* pb) const
{
  pb->add_trg_words("[X,1]");
  for (size_t pos = 0 ; pos < GetSize() ; pos++)
    pb->add_trg_words(GetWord(pos)[0]->GetString());
}
#endif

void TargetPhrase::Evaluate(const Phrase &source)
{
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  Evaluate(source, ffs);
}

void TargetPhrase::Evaluate(const Phrase &source, const std::vector<FeatureFunction*> &ffs)
{
  if (ffs.size()) {
    ScoreComponentCollection futureScoreBreakdown;
    for (size_t i = 0; i < ffs.size(); ++i) {
      const FeatureFunction &ff = *ffs[i];
      ff.Evaluate(source, *this, m_scoreBreakdown, futureScoreBreakdown);
    }

    float weightedScore = m_scoreBreakdown.GetWeightedScore();
    m_futureScore += futureScoreBreakdown.GetWeightedScore();
    m_fullScore = weightedScore + m_futureScore;

  }
}

void TargetPhrase::Evaluate(const InputType &input)
{
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();

  for (size_t i = 0; i < ffs.size(); ++i) {
    const FeatureFunction &ff = *ffs[i];
    ff.Evaluate(input, m_scoreBreakdown);
  }
}

void TargetPhrase::SetXMLScore(float score)
{
  const StaticData &staticData = StaticData::Instance();
  const FeatureFunction* prod = staticData.GetPhraseDictionaries()[0];
  size_t numScores = prod->GetNumScoreComponents();
  vector <float> scoreVector(numScores,score/numScores);

  m_scoreBreakdown.Assign(prod, scoreVector);
}

void TargetPhrase::SetInputScore(const Scores &scoreVector)
{
  //we use an existing score producer to figure out information for score setting (number of scores and weights)
  const StaticData &staticData = StaticData::Instance();
  const FeatureFunction* prod = staticData.GetPhraseDictionaries()[0];

  //expand the input weight vector
  CHECK(scoreVector.size() <= prod->GetNumScoreComponents());
  Scores sizedScoreVector = scoreVector;
  sizedScoreVector.resize(prod->GetNumScoreComponents(),0.0f);

  m_scoreBreakdown.Assign(prod, sizedScoreVector);
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
    UTIL_THROW_IF(++dash, util::Exception, "Extra gunk in alignment " << *token);


    if (GetWord(targetPos).IsNonTerminal()) {
      alignNonTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    } else {
      alignTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    }
  }
  SetAlignTerm(alignTerm);
  SetAlignNonTerm(alignNonTerm);

}

void TargetPhrase::SetAlignTerm(const AlignmentInfo::CollType &coll)
{
  const AlignmentInfo *alignmentInfo = AlignmentInfoCollection::Instance().Add(coll);
  m_alignTerm = alignmentInfo;

}

void TargetPhrase::SetAlignNonTerm(const AlignmentInfo::CollType &coll)
{
  const AlignmentInfo *alignmentInfo = AlignmentInfoCollection::Instance().Add(coll);
  m_alignNonTerm = alignmentInfo;
}

void TargetPhrase::SetSparseScore(const FeatureFunction* translationScoreProducer, const StringPiece &sparseString)
{
  m_scoreBreakdown.Assign(translationScoreProducer, sparseString.as_string());
}

void TargetPhrase::Merge(const TargetPhrase &copy, const std::vector<FactorType>& factorVec)
{
  Phrase::MergeFactors(copy, factorVec);
  m_scoreBreakdown.Merge(copy.GetScoreBreakdown());
  m_futureScore += copy.m_futureScore;
  m_fullScore += copy.m_fullScore;
}

TO_STRING_BODY(TargetPhrase);

std::ostream& operator<<(std::ostream& os, const TargetPhrase& tp)
{
  os << static_cast<const Phrase&>(tp) << ":" << flush;
  os << tp.GetAlignNonTerm() << flush;
  os << ": c=" << tp.m_fullScore << flush;
  os << " " << tp.m_scoreBreakdown << flush;

  return os;
}

}

