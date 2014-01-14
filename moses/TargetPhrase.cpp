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
#include "InputPath.h"

using namespace std;

namespace Moses
{
TargetPhrase::TargetPhrase( std::string out_string)
  :Phrase(0)
  , m_fullScore(0.0)
  , m_futureScore(0.0)
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
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
{
}

TargetPhrase::TargetPhrase(const Phrase &phrase)
  : Phrase(phrase)
  , m_fullScore(0.0)
  , m_futureScore(0.0)
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
{
}

TargetPhrase::TargetPhrase(const TargetPhrase &copy)
  : Phrase(copy)
  , m_fullScore(copy.m_fullScore)
  , m_futureScore(copy.m_futureScore)
  , m_scoreBreakdown(copy.m_scoreBreakdown)
  , m_alignTerm(copy.m_alignTerm)
  , m_alignNonTerm(copy.m_alignNonTerm)
{
  if (copy.m_lhsTarget) {
    m_lhsTarget = new Word(*copy.m_lhsTarget);
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
  cerr << "TargetPhrase::Evaluate(const Phrase &source) START" << endl;
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  cerr << "there are ffs.size():|" << ffs.size() << "|" << endl;
  Evaluate(source, ffs);
  cerr << "TargetPhrase::Evaluate(const Phrase &source) END" << endl;
}

void TargetPhrase::Evaluate(const Phrase &source, const std::vector<FeatureFunction*> &ffs)
{
  cerr << "TargetPhrase::Evaluate(const Phrase &source, const std::vector<FeatureFunction*> &ffs) START" << endl;
  cerr << "ffs.size():|" << ffs.size() << "|" << endl;
  cerr << "source:|" << source << "|" << endl;
  cerr << "target:|" << *this << "|" << endl;
  if (ffs.size()) {
    const StaticData &staticData = StaticData::Instance();
    ScoreComponentCollection futureScoreBreakdown;
    for (size_t i = 0; i < ffs.size(); ++i) {
      cerr << "ffs index:|" << i << "|" << endl;
      const FeatureFunction &ff = *ffs[i];
      cerr << "ffs description:|" << ff.GetScoreProducerDescription() << "|" << endl;
      if (! staticData.IsFeatureFunctionIgnored( ff )) {
        cerr << "feature is not ignored" << endl;
        ff.Evaluate(source, *this, m_scoreBreakdown, futureScoreBreakdown);
      }
      else{
        cerr << "feature is ignored" << endl;
      }
    }

    float weightedScore = m_scoreBreakdown.GetWeightedScore();
    m_futureScore += futureScoreBreakdown.GetWeightedScore();
    m_fullScore = weightedScore + m_futureScore;
    cerr << "m_scoreBreakdown:|" << m_scoreBreakdown << "|" << endl;
  }
  cerr << "TargetPhrase::Evaluate(const Phrase &source, const std::vector<FeatureFunction*> &ffs) END" << endl;
}

void TargetPhrase::Evaluate(const InputType &input, const InputPath &inputPath)
{
  cerr << "TargetPhrase::Evaluate(const InputType &input) START" << endl;
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  cerr << "input:|" << input << "|" << endl;
  cerr << "target:|" << *this << "|" << endl;
  cerr << "ffs.size():|" << ffs.size() << "|" << endl;

  for (size_t i = 0; i < ffs.size(); ++i) {
    cerr << "ffs index:|" << i << "|" << endl;
    const FeatureFunction &ff = *ffs[i];
    ff.Evaluate(input, inputPath, m_scoreBreakdown);
  }
  cerr << "TargetPhrase::Evaluate(const InputType &input) END" << endl;
}

void TargetPhrase::SetXMLScore(float score)
{
  const StaticData &staticData = StaticData::Instance();
  const FeatureFunction* prod = staticData.GetPhraseDictionaries()[0];
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
	  CHECK(keyValue.size() == 2);
	  SetProperty(keyValue[0], keyValue[1]);
  }
}

void TargetPhrase::GetProperty(const std::string &key, std::string &value, bool &found) const
{
  std::map<std::string, std::string>::const_iterator iter;
  iter = m_properties.find(key);
  if (iter == m_properties.end()) {
    found = false;
  } else {
    found = true;
    value = iter->second;
  }
}

void swap(TargetPhrase &first, TargetPhrase &second)
{
  first.SwapWords(second);
  std::swap(first.m_fullScore, second.m_fullScore);
  std::swap(first.m_futureScore, second.m_futureScore);
  swap(first.m_scoreBreakdown, second.m_scoreBreakdown);
  std::swap(first.m_alignTerm, second.m_alignTerm);
  std::swap(first.m_alignNonTerm, second.m_alignNonTerm);
  std::swap(first.m_lhsTarget, second.m_lhsTarget);
}

TO_STRING_BODY(TargetPhrase);

std::ostream& operator<<(std::ostream& os, const TargetPhrase& tp)
{
  if (tp.m_lhsTarget) {
    os << *tp.m_lhsTarget<< " -> ";
  }
  os << static_cast<const Phrase&>(tp) << ":" << flush;
  os << tp.GetAlignNonTerm() << flush;
  os << ": c=" << tp.m_fullScore << flush;
  os << " " << tp.m_scoreBreakdown << flush;

  return os;
}

}

