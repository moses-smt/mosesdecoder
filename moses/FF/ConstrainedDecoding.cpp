#include "ConstrainedDecoding.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
ConstrainedDecodingState::ConstrainedDecodingState(const Hypothesis &hypo)
{
  hypo.GetOutputPhrase(m_outputPhrase);
}

ConstrainedDecodingState::ConstrainedDecodingState(const ChartHypothesis &hypo)
{
  hypo.GetOutputPhrase(m_outputPhrase);
}

int ConstrainedDecodingState::Compare(const FFState& other) const
{
  const ConstrainedDecodingState &otherFF = static_cast<const ConstrainedDecodingState&>(other);
  bool ret = 	m_outputPhrase.Compare(otherFF.m_outputPhrase);
  return ret;
}

//////////////////////////////////////////////////////////////////
void ConstrainedDecoding::Load()
{
  const StaticData &staticData = StaticData::Instance();
  bool addBeginEndWord = (staticData.GetSearchAlgorithm() == ChartDecoding) || (staticData.GetSearchAlgorithm() == ChartIncremental);

  InputFileStream constraintFile(m_path);
  std::string line;
  long sentenceID = staticData.GetStartTranslationId() - 1;
  while (getline(constraintFile, line)) {
    vector<string> vecStr = Tokenize(line, "\t");

    Phrase phrase(0);
    if (vecStr.size() == 1) {
      sentenceID++;
      phrase.CreateFromString(Output, staticData.GetOutputFactorOrder(), vecStr[0], staticData.GetFactorDelimiter(), NULL);
    } else if (vecStr.size() == 2) {
      sentenceID = Scan<long>(vecStr[0]);
      phrase.CreateFromString(Output, staticData.GetOutputFactorOrder(), vecStr[1], staticData.GetFactorDelimiter(), NULL);
    } else {
      UTIL_THROW(util::Exception, "Reference file not loaded");
    }

    if (addBeginEndWord) {
      phrase.InitStartEndWord();
    }
    m_constraints.insert(make_pair(sentenceID,phrase));

  }
}

std::vector<float> ConstrainedDecoding::DefaultWeights() const
{
  UTIL_THROW_IF2(m_numScoreComponents != 1,
		  "ConstrainedDecoding must only have 1 score");
  vector<float> ret(1, 1);
  return ret;
}

template <class H, class M>
const Phrase *GetConstraint(const std::map<long,Phrase> &constraints, const H &hypo)
{
  const M &mgr = hypo.GetManager();
  const InputType &input = mgr.GetSource();
  long id = input.GetTranslationId();

  map<long,Phrase>::const_iterator iter;
  iter = constraints.find(id);

  if (iter == constraints.end()) {
    UTIL_THROW(util::Exception, "Couldn't find reference " << id);

    return NULL;
  } else {
    return &iter->second;
  }
}

FFState* ConstrainedDecoding::Evaluate(
  const Hypothesis& hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  const Phrase *ref = GetConstraint<Hypothesis, Manager>(m_constraints, hypo);
  assert(ref);

  ConstrainedDecodingState *ret = new ConstrainedDecodingState(hypo);
  const Phrase &outputPhrase = ret->GetPhrase();

  size_t searchPos = ref->Find(outputPhrase, m_maxUnknowns);

  float score;
  if (hypo.IsSourceCompleted()) {
    // translated entire sentence.
    score = (searchPos == 0) && (ref->GetSize() == outputPhrase.GetSize())
            ? 0 : - std::numeric_limits<float>::infinity();
  } else {
    score = (searchPos != NOT_FOUND) ? 0 : - std::numeric_limits<float>::infinity();
  }

  accumulator->PlusEquals(this, score);

  return ret;
}

FFState* ConstrainedDecoding::EvaluateChart(
  const ChartHypothesis &hypo,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  const Phrase *ref = GetConstraint<ChartHypothesis, ChartManager>(m_constraints, hypo);
  assert(ref);

  const ChartManager &mgr = hypo.GetManager();
  const Sentence &source = static_cast<const Sentence&>(mgr.GetSource());

  ConstrainedDecodingState *ret = new ConstrainedDecodingState(hypo);
  const Phrase &outputPhrase = ret->GetPhrase();
  size_t searchPos = ref->Find(outputPhrase, m_maxUnknowns);

  float score;
  if (hypo.GetCurrSourceRange().GetStartPos() == 0 &&
      hypo.GetCurrSourceRange().GetEndPos() == source.GetSize() - 1) {
    // translated entire sentence.
    score = (searchPos == 0) && (ref->GetSize() == outputPhrase.GetSize())
            ? 0 : - std::numeric_limits<float>::infinity();
  } else {
    score = (searchPos != NOT_FOUND) ? 0 : - std::numeric_limits<float>::infinity();
  }

  accumulator->PlusEquals(this, score);

  return ret;
}

void ConstrainedDecoding::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_path = value;
  } else if (key == "max-unknowns") {
    m_maxUnknowns = Scan<int>(value);
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

}

