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

#include "moses/TranslationModel/PhraseDictionaryGroup.h"

#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>

#include "util/exception.hh"

using namespace std;
using namespace boost;

namespace Moses
{

PhraseDictionaryGroup::PhraseDictionaryGroup(const string &line)
    : PhraseDictionary(line, true),
      m_numModels(0),
      m_restrict(false)
{
  ReadParameters();
}

void PhraseDictionaryGroup::SetParameter(const string& key, const string& value)
{
  if (key == "members") {
    m_memberPDStrs = Tokenize(value, ",");
    m_numModels = m_memberPDStrs.size();
  } else if (key == "restrict") {
    m_restrict = Scan<bool>(value);
  } else {
    PhraseDictionary::SetParameter(key, value);
  }
}

void PhraseDictionaryGroup::Load()
{
  SetFeaturesToApply();
  m_pdFeature.push_back(const_cast<PhraseDictionaryGroup*>(this));

  // Locate/check component phrase tables
  size_t componentWeights = 0;
  BOOST_FOREACH(const string& pdName, m_memberPDStrs) {
    bool pdFound = false;
    BOOST_FOREACH(PhraseDictionary* pd, PhraseDictionary::GetColl()) {
      if (pd->GetScoreProducerDescription() == pdName) {
        pdFound = true;
        m_memberPDs.push_back(pd);
        componentWeights += pd->GetNumScoreComponents();
      }
    }
    UTIL_THROW_IF2(!pdFound,
        "Could not find component phrase table " << pdName);
  }
  UTIL_THROW_IF2(componentWeights != m_numScoreComponents,
      "Total number of component model scores is unequal to specified number of scores");
}

void PhraseDictionaryGroup::GetTargetPhraseCollectionBatch(
    const ttasksptr& ttask, const InputPathList& inputPathQueue) const
{
  // Some implementations (mmsapt) do work in PrefixExists
  BOOST_FOREACH(const InputPath* inputPath, inputPathQueue) {
    const Phrase& phrase = inputPath->GetPhrase();
    BOOST_FOREACH(const PhraseDictionary* pd, m_memberPDs) {
      pd->PrefixExists(ttask, phrase);
    }
  }
  // Look up each input in each model
  BOOST_FOREACH(InputPath* inputPath, inputPathQueue) {
    const Phrase &phrase = inputPath->GetPhrase();
    const TargetPhraseCollection* targetPhrases =
        this->GetTargetPhraseCollectionLEGACY(ttask, phrase);
    inputPath->SetTargetPhrases(*this, targetPhrases, NULL);
  }
}

const TargetPhraseCollection* PhraseDictionaryGroup::GetTargetPhraseCollectionLEGACY(
    const Phrase& src) const
{
  UTIL_THROW2("Don't call me without the translation task.");
}

const TargetPhraseCollection* PhraseDictionaryGroup::GetTargetPhraseCollectionLEGACY(
    const ttasksptr& ttask, const Phrase& src) const
{
  TargetPhraseCollection* ret = CreateTargetPhraseCollection(ttask, src);
  ret->NthElement(m_tableLimit); // sort the phrases for pruning later
  const_cast<PhraseDictionaryGroup*>(this)->CacheForCleanup(ret);
  return ret;
}

TargetPhraseCollection* PhraseDictionaryGroup::CreateTargetPhraseCollection(
    const ttasksptr& ttask, const Phrase& src) const
{
  // Aggregation of phrases and the scores that will be applied to them
  vector<TargetPhrase*> allPhrases;
  unordered_map<const TargetPhrase*, vector<float>, PhrasePtrHasher,
      PhrasePtrComparator> allScores;

  // For each model
  size_t offset = 0;
  for (size_t i = 0; i < m_numModels; ++i) {

    // Collect phrases from this table
    const PhraseDictionary& pd = *m_memberPDs[i];
    const TargetPhraseCollection* ret_raw = pd.GetTargetPhraseCollectionLEGACY(
        ttask, src);

    if (ret_raw != NULL) {
      // Process each phrase from table
      BOOST_FOREACH(const TargetPhrase* targetPhrase, *ret_raw) {
        vector<float> raw_scores =
            targetPhrase->GetScoreBreakdown().GetScoresForProducer(&pd);

        // Phrase not in collection -> add if unrestricted or first model
        if (allScores.find(targetPhrase) == allScores.end()) {
          if (m_restrict && i > 0) {
            continue;
          }

          // Copy phrase to avoid disrupting base model
          TargetPhrase* phrase = new TargetPhrase(*targetPhrase);
          // Correct future cost estimates and total score
          phrase->GetScoreBreakdown().InvertDenseFeatures(&pd);
          vector<FeatureFunction*> pd_feature;
          pd_feature.push_back(m_memberPDs[i]);
          const vector<FeatureFunction*> pd_feature_const(pd_feature);
          phrase->EvaluateInIsolation(src, pd_feature_const);
          // Zero out scores from original phrase table
          phrase->GetScoreBreakdown().ZeroDenseFeatures(&pd);
          // Add phrase entry
          allPhrases.push_back(phrase);
          allScores[targetPhrase] = vector<float>(m_numScoreComponents, 0);
        }
        vector<float>& scores = allScores.find(targetPhrase)->second;

        // Copy scores from this model
        for (size_t j = 0; j < pd.GetNumScoreComponents(); ++j) {
          scores[offset + j] = raw_scores[j];
        }
      }
    }
    offset += pd.GetNumScoreComponents();
  }

  // Apply scores to phrases and add them to return collection
  TargetPhraseCollection* ret = new TargetPhraseCollection();
  const vector<FeatureFunction*> pd_feature_const(m_pdFeature);
  BOOST_FOREACH(TargetPhrase* phrase, allPhrases) {
    phrase->GetScoreBreakdown().Assign(this, allScores.find(phrase)->second);
    // Correct future cost estimates and total score
    phrase->EvaluateInIsolation(src, pd_feature_const);
    ret->Add(phrase);
  }

  return ret;
}

ChartRuleLookupManager *PhraseDictionaryGroup::CreateRuleLookupManager(
    const ChartParser &, const ChartCellCollectionBase&, size_t)
{
  UTIL_THROW(util::Exception, "Phrase table used in chart decoder");
}

//copied from PhraseDictionaryCompact; free memory allocated to TargetPhraseCollection (and each TargetPhrase) at end of sentence
void PhraseDictionaryGroup::CacheForCleanup(TargetPhraseCollection* tpc)
{
  PhraseCache &ref = GetPhraseCache();
  ref.push_back(tpc);
}

void PhraseDictionaryGroup::CleanUpAfterSentenceProcessing(
    const InputType &source)
{
  PhraseCache &ref = GetPhraseCache();
  for (PhraseCache::iterator it = ref.begin(); it != ref.end(); it++) {
    delete *it;
  }

  PhraseCache temp;
  temp.swap(ref);

  CleanUpComponentModels(source);
}

void PhraseDictionaryGroup::CleanUpComponentModels(const InputType &source)
{
  for (size_t i = 0; i < m_numModels; ++i) {
    m_memberPDs[i]->CleanUpAfterSentenceProcessing(source);
  }
}

} //namespace
