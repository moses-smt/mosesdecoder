#include <boost/functional/hash.hpp>
#include <vector>
#include <algorithm>
#include <iterator>
#include <boost/foreach.hpp>
#include "CoveredReferenceFeature.h"
#include "moses/ScoreComponentCollection.h"
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

size_t CoveredReferenceState::hash() const
{
  UTIL_THROW2("TODO:Haven't figure this out yet");
}

bool CoveredReferenceState::operator==(const FFState& other) const
{
  UTIL_THROW2("TODO:Haven't figure this out yet");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CoveredReferenceFeature::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedScores) const
{
  long id = input.GetTranslationId();
  boost::unordered_map<long, std::multiset<string> >::const_iterator refIt = m_refs.find(id);
  multiset<string> wordsInPhrase = GetWordsInPhrase(targetPhrase);
  multiset<string> covered;
  set_intersection(wordsInPhrase.begin(), wordsInPhrase.end(),
                   refIt->second.begin(), refIt->second.end(),
                   inserter(covered, covered.begin()));
  vector<float> scores;
  scores.push_back(covered.size());

  scoreBreakdown.Assign(this, scores);
  estimatedScores->Assign(this, scores);
}

void CoveredReferenceFeature::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  InputFileStream refFile(m_path);
  std::string line;
  const StaticData &staticData = StaticData::Instance();
  long sentenceID = opts->output.start_translation_id;
  while (getline(refFile, line)) {
    vector<string> words = Tokenize(line, " ");
    multiset<string> wordSet;
    // TODO make Tokenize work with other containers than vector
    copy(words.begin(), words.end(), inserter(wordSet, wordSet.begin()));
    m_refs.insert(make_pair(sentenceID++, wordSet));
  }
}

void CoveredReferenceFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_path = value;
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

FFState* CoveredReferenceFeature::EvaluateWhenApplied(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  const CoveredReferenceState &prev = static_cast<const CoveredReferenceState&>(*prev_state);
  CoveredReferenceState *ret = new CoveredReferenceState(prev);

  const Manager &mgr = cur_hypo.GetManager();
  const InputType &input = mgr.GetSource();
  long id = input.GetTranslationId();

  // which words from the reference remain uncovered
  multiset<string> remaining;
  boost::unordered_map<long, std::multiset<string> >::const_iterator refIt = m_refs.find(id);
  if (refIt == m_refs.end()) UTIL_THROW(util::Exception, "Sentence id out of range: " + SPrint<long>(id));
  set_difference(refIt->second.begin(), refIt->second.end(),
                 ret->m_coveredRef.begin(), ret->m_coveredRef.end(),
                 inserter(remaining, remaining.begin()));

  // which of the remaining words are present in the current phrase
  multiset<string> wordsInPhrase = GetWordsInPhrase(cur_hypo.GetCurrTargetPhrase());
  multiset<string> newCovered;
  set_intersection(wordsInPhrase.begin(), wordsInPhrase.end(),
                   remaining.begin(), remaining.end(),
                   inserter(newCovered, newCovered.begin()));

  vector<float> estimateScore =
    cur_hypo.GetCurrTargetPhrase().GetScoreBreakdown().GetScoresForProducer(this);
  vector<float> scores;
  scores.push_back(newCovered.size() - estimateScore[0]);
  accumulator->PlusEquals(this, scores);

  // update feature state
  multiset<string>::const_iterator newCoveredIt;
  for (newCoveredIt = newCovered.begin(); newCoveredIt != newCovered.end(); newCoveredIt++) {
    ret->m_coveredRef.insert(*newCoveredIt);
  }
  return ret;
}

FFState* CoveredReferenceFeature::EvaluateWhenApplied(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  UTIL_THROW(util::Exception, "Not implemented");
}

}
