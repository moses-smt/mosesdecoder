#include "TargetConstituentAdjacencyFeature.h"
#include "moses/PP/TargetConstituentBoundariesRightAdjacentPhraseProperty.h"
#include "moses/PP/TargetConstituentBoundariesLeftPhraseProperty.h"
#include "moses/StaticData.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"
#include "moses/FactorCollection.h"
#include "moses/TreeInput.h"
#include <algorithm>


using namespace std;

namespace Moses
{

size_t TargetConstituentAdjacencyFeatureState::hash() const
{
  if (m_recombine) {
    return 0;
  }
  size_t ret = 0;
  boost::hash_combine(ret, m_collection.size());
  for (std::map<const Factor*, float>::const_iterator it=m_collection.begin();
       it!=m_collection.end(); ++it) {
    boost::hash_combine(ret, it->first);
  }
  return ret;
};

bool TargetConstituentAdjacencyFeatureState::operator==(const FFState& other) const
{
  if (m_recombine) {
    return true;
  }

  if (this == &other) {
    return true;
  }

  const TargetConstituentAdjacencyFeatureState* otherState =
    dynamic_cast<const TargetConstituentAdjacencyFeatureState*>(&other);
  UTIL_THROW_IF2(otherState == NULL, "Wrong state type");

  if (m_collection.size() != (otherState->m_collection).size()) {
    return false;
  }
  std::map<const Factor*, float>::const_iterator thisIt, otherIt;
  for (thisIt=m_collection.begin(), otherIt=(otherState->m_collection).begin();
       thisIt!=m_collection.end(); ++thisIt, ++otherIt) {
    if (thisIt->first != otherIt->first) {
      return false;
    }
  }
  return true;
};


TargetConstituentAdjacencyFeature::TargetConstituentAdjacencyFeature(const std::string &line)
  : StatefulFeatureFunction(2, line)
  , m_featureVariant(0)
  , m_recombine(false)
{
  VERBOSE(1, "Initializing feature " << GetScoreProducerDescription() << " ...");
  ReadParameters();
  VERBOSE(1, " Done." << std::endl);
  VERBOSE(1, " Feature variant: " << m_featureVariant << "." << std::endl);
}


void TargetConstituentAdjacencyFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "variant") {
    m_featureVariant = Scan<size_t>(value);
  } else if (key == "recombine") {
    m_recombine = Scan<bool>(value);
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}


FFState* TargetConstituentAdjacencyFeature::EvaluateWhenApplied(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  // dense scores
  std::vector<float> newScores(m_numScoreComponents,0); // m_numScoreComponents == 2

  // state
  const TargetConstituentAdjacencyFeatureState *prevState = static_cast<const TargetConstituentAdjacencyFeatureState*>(prev_state);

  // read TargetConstituentAdjacency property
  const TargetPhrase &currTarPhr = cur_hypo.GetCurrTargetPhrase();
  FEATUREVERBOSE(2, "Phrase: " << currTarPhr << std::endl);

  if (const PhraseProperty *property = currTarPhr.GetProperty("TargetConstituentBoundariesLeft")) {

    const TargetConstituentBoundariesLeftPhraseProperty *targetConstituentBoundariesLeftPhraseProperty =
      static_cast<const TargetConstituentBoundariesLeftPhraseProperty*>(property);
    const TargetConstituentBoundariesLeftCollection& leftConstituentCollection =
      targetConstituentBoundariesLeftPhraseProperty->GetCollection();
    float prob = 0;
    size_t numMatch = 0;
    size_t numOverall = 0;

    if ( !cur_hypo.GetPrevHypo()->GetPrevHypo() ) {
      // previous hypothesis is initial, i.e. target sentence starts here

      ++numOverall;
      FactorCollection &factorCollection = FactorCollection::Instance();
      const Factor* bosFactor = factorCollection.AddFactor("BOS_",false);
      TargetConstituentBoundariesLeftCollection::const_iterator found =
        leftConstituentCollection.find(bosFactor);
      if ( found != leftConstituentCollection.end() ) {
        ++numMatch;
        prob += found->second;
      }

    } else {

      const std::map<const Factor*, float>& hypConstituentCollection = prevState->m_collection;
      std::map<const Factor*, float>::const_iterator iter1 = hypConstituentCollection.begin();
      std::map<const Factor*, float>::const_iterator iter2 = leftConstituentCollection.begin();
      while ( iter1 != hypConstituentCollection.end() && iter2 != leftConstituentCollection.end() ) {
        ++numOverall;
        if ( iter1->first < iter2->first ) {
          ++iter1;
        } else if ( iter2->first < iter1->first ) {
          ++iter2;
        } else {
          ++numMatch;
          float currProb = iter1->second * iter2->second;
          if (currProb > prob)
            prob = currProb;
          ++iter1;
          ++iter2;
        }
      }
    }

    if ( (numMatch == 0) || (prob == 0) ) {
      ++newScores[1];
    } else {
      if ( m_featureVariant == 1 ) {
        newScores[0] += TransformScore(prob);
      } else {
        newScores[0] += TransformScore( (float)numMatch/numOverall );
      }
    }

  } else {

    // abort with error message if the phrase does not translate an unknown word
    UTIL_THROW_IF2(!currTarPhr.GetWord(0).IsOOV(), GetScoreProducerDescription()
                   << ": Missing TargetConstituentBoundariesLeft property.");

    ++newScores[1];

  }

  TargetConstituentAdjacencyFeatureState *newState = new TargetConstituentAdjacencyFeatureState(m_recombine);

  if (const PhraseProperty *property = currTarPhr.GetProperty("TargetConstituentBoundariesRightAdjacent")) {

    const TargetConstituentBoundariesRightAdjacentPhraseProperty *targetConstituentBoundariesRightAdjacentPhraseProperty =
      static_cast<const TargetConstituentBoundariesRightAdjacentPhraseProperty*>(property);
    const TargetConstituentBoundariesLeftCollection& rightAdjacentConstituentCollection = targetConstituentBoundariesRightAdjacentPhraseProperty->GetCollection();

    std::copy(rightAdjacentConstituentCollection.begin(), rightAdjacentConstituentCollection.end(),
              std::inserter(newState->m_collection, newState->m_collection.begin()));

  } else {

    // abort with error message if the phrase does not translate an unknown word
    UTIL_THROW_IF2(!currTarPhr.GetWord(0).IsOOV(), GetScoreProducerDescription()
                   << ": Missing TargetConstituentBoundariesRightAdjacent property.");

  }

  // add scores
  accumulator->PlusEquals(this, newScores);

  return newState;
}

}

