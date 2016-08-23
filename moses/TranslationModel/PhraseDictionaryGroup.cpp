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
    m_totalModelScores(0),
    m_phraseCounts(false),
    m_wordCounts(false),
    m_modelBitmapCounts(false),
    m_restrict(false),
    m_haveDefaultScores(false),
    m_defaultAverageOthers(false),
    m_scoresPerModel(0),
    m_haveMmsaptLrFunc(false)
{
  ReadParameters();
}

void PhraseDictionaryGroup::SetParameter(const string& key, const string& value)
{
  if (key == "members") {
    m_memberPDStrs = Tokenize(value, ",");
    m_numModels = m_memberPDStrs.size();
    m_seenByAll = dynamic_bitset<>(m_numModels);
    m_seenByAll.set();
  } else if (key == "restrict") {
    m_restrict = Scan<bool>(value);
  } else if (key == "phrase-counts") {
    m_phraseCounts = Scan<bool>(value);
  } else if (key == "word-counts") {
    m_wordCounts = Scan<bool>(value);
  } else if (key == "model-bitmap-counts") {
    m_modelBitmapCounts = Scan<bool>(value);
  } else if (key =="default-scores") {
    m_haveDefaultScores = true;
    m_defaultScores = Scan<float>(Tokenize(value, ","));
  } else if (key =="default-average-others") {
    m_defaultAverageOthers = Scan<bool>(value);
  } else if (key =="mmsapt-lr-func") {
    m_haveMmsaptLrFunc = true;
  } else {
    PhraseDictionary::SetParameter(key, value);
  }
}

void PhraseDictionaryGroup::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  SetFeaturesToApply();
  m_pdFeature.push_back(const_cast<PhraseDictionaryGroup*>(this));
  size_t numScoreComponents = 0;

  // Locate/check component phrase tables
  BOOST_FOREACH(const string& pdName, m_memberPDStrs) {
    bool pdFound = false;
    BOOST_FOREACH(PhraseDictionary* pd, PhraseDictionary::GetColl()) {
      if (pd->GetScoreProducerDescription() == pdName) {
        pdFound = true;
        m_memberPDs.push_back(pd);
        size_t nScores = pd->GetNumScoreComponents();
        numScoreComponents += nScores;
        if (m_scoresPerModel == 0) {
          m_scoresPerModel = nScores;
        } else if (m_defaultAverageOthers) {
          UTIL_THROW_IF2(nScores != m_scoresPerModel,
                         m_description << ": member models must have the same number of scores when using default-average-others");
        }
      }
    }
    UTIL_THROW_IF2(!pdFound,
                   m_description << ": could not find member phrase table " << pdName);
  }
  m_totalModelScores = numScoreComponents;

  // Check feature total
  if (m_phraseCounts) {
    numScoreComponents += m_numModels;
  }
  if (m_wordCounts) {
    numScoreComponents += m_numModels;
  }
  if (m_modelBitmapCounts) {
    numScoreComponents += (pow(2, m_numModels) - 1);
  }
  UTIL_THROW_IF2(numScoreComponents != m_numScoreComponents,
                 m_description << ": feature count mismatch: specify \"num-features=" << numScoreComponents << "\" and supply " << numScoreComponents << " weights");

#ifdef PT_UG
  // Locate mmsapt lexical reordering functions if specified
  if (m_haveMmsaptLrFunc) {
    BOOST_FOREACH(PhraseDictionary* pd, m_memberPDs) {
      // pointer to pointer, all start as NULL and some may be populated prior
      // to translation
      m_mmsaptLrFuncs.push_back(&(static_cast<Mmsapt*>(pd)->m_lr_func));
    }
  }
#endif

  // Determine "zero" scores for features
  if (m_haveDefaultScores) {
    UTIL_THROW_IF2(m_defaultScores.size() != m_numScoreComponents,
                   m_description << ": number of specified default scores is unequal to number of member model scores");
  } else {
    // Default is all 0 (as opposed to e.g. -99 or similar to approximate log(0)
    // or a smoothed "not in model" score)
    m_defaultScores = vector<float>(m_numScoreComponents, 0);
  }
}

void PhraseDictionaryGroup::InitializeForInput(const ttasksptr& ttask)
{
  // Member models are registered as FFs and should already be initialized
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
    TargetPhraseCollection::shared_ptr  targetPhrases =
      this->GetTargetPhraseCollectionLEGACY(ttask, phrase);
    inputPath->SetTargetPhrases(*this, targetPhrases, NULL);
  }
}

TargetPhraseCollection::shared_ptr  PhraseDictionaryGroup::GetTargetPhraseCollectionLEGACY(
  const Phrase& src) const
{
  UTIL_THROW2("Don't call me without the translation task.");
}

TargetPhraseCollection::shared_ptr
PhraseDictionaryGroup::
GetTargetPhraseCollectionLEGACY(const ttasksptr& ttask, const Phrase& src) const
{
  TargetPhraseCollection::shared_ptr ret
  = CreateTargetPhraseCollection(ttask, src);
  ret->NthElement(m_tableLimit); // sort the phrases for pruning later
  const_cast<PhraseDictionaryGroup*>(this)->CacheForCleanup(ret);
  return ret;
}

TargetPhraseCollection::shared_ptr
PhraseDictionaryGroup::
CreateTargetPhraseCollection(const ttasksptr& ttask, const Phrase& src) const
{
  // Aggregation of phrases and corresponding statistics (scores, models seen by)
  vector<TargetPhrase*> phraseList;
  typedef unordered_map<const TargetPhrase*, PDGroupPhrase, UnorderedComparer<Phrase>, UnorderedComparer<Phrase> > PhraseMap;
  PhraseMap phraseMap;

  // For each model
  size_t offset = 0;
  for (size_t i = 0; i < m_numModels; ++i) {

    // Collect phrases from this table
    const PhraseDictionary& pd = *m_memberPDs[i];
    TargetPhraseCollection::shared_ptr
    ret_raw = pd.GetTargetPhraseCollectionLEGACY(ttask, src);

    if (ret_raw != NULL) {
      // Process each phrase from table
      BOOST_FOREACH(const TargetPhrase* targetPhrase, *ret_raw) {
        vector<float> raw_scores =
          targetPhrase->GetScoreBreakdown().GetScoresForProducer(&pd);

        // Phrase not in collection -> add if unrestricted or first model
        PhraseMap::iterator iter = phraseMap.find(targetPhrase);
        if (iter == phraseMap.end()) {
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
          phraseList.push_back(phrase);
          phraseMap[targetPhrase] = PDGroupPhrase(phrase, m_defaultScores, m_numModels);
        } else {
          // For existing phrases: merge extra scores (such as lr-func scores for mmsapt)
          TargetPhrase* phrase = iter->second.m_targetPhrase;
          BOOST_FOREACH(const TargetPhrase::ScoreCache_t::value_type pair, targetPhrase->GetExtraScores()) {
            phrase->SetExtraScores(pair.first, pair.second);
          }
        }
        // Don't repeat lookup if phrase already found
        PDGroupPhrase& pdgPhrase = (iter == phraseMap.end()) ? phraseMap.find(targetPhrase)->second : iter->second;

        // Copy scores from this model
        for (size_t j = 0; j < pd.GetNumScoreComponents(); ++j) {
          pdgPhrase.m_scores[offset + j] = raw_scores[j];
        }

        // Phrase seen by this model
        pdgPhrase.m_seenBy[i] = true;
      }
    }
    offset += pd.GetNumScoreComponents();
  }

  // Compute additional scores as phrases are added to return collection
  TargetPhraseCollection::shared_ptr ret(new TargetPhraseCollection);
  const vector<FeatureFunction*> pd_feature_const(m_pdFeature);
  BOOST_FOREACH(TargetPhrase* phrase, phraseList) {
    PDGroupPhrase& pdgPhrase = phraseMap.find(phrase)->second;

    // Score order (example with 2 models)
    // member1_scores member2_scores [m1_pc m2_pc] [m1_wc m2_wc]

    // Extra scores added after member model scores
    size_t offset = m_totalModelScores;
    // Phrase count (per member model)
    if (m_phraseCounts) {
      for (size_t i = 0; i < m_numModels; ++i) {
        if (pdgPhrase.m_seenBy[i]) {
          pdgPhrase.m_scores[offset + i] = 1;
        }
      }
      offset += m_numModels;
    }
    // Word count (per member model)
    if (m_wordCounts) {
      size_t wc = pdgPhrase.m_targetPhrase->GetSize();
      for (size_t i = 0; i < m_numModels; ++i) {
        if (pdgPhrase.m_seenBy[i]) {
          pdgPhrase.m_scores[offset + i] = wc;
        }
      }
      offset += m_numModels;
    }

    // Model bitmap features (one feature per possible bitmap)
    // e.g. seen by models 1 and 3 but not 2 -> "101" fires
    if (m_modelBitmapCounts) {
      // Throws exception if someone tries to combine more than 64 models
      pdgPhrase.m_scores[offset + (pdgPhrase.m_seenBy.to_ulong() - 1)] = 1;
      offset += m_seenByAll.to_ulong();
    }

    // Average other-model scores to fill in defaults when models have not seen
    // this phrase
    if (m_defaultAverageOthers) {
      // Average seen scores
      if (pdgPhrase.m_seenBy != m_seenByAll) {
        vector<float> avgScores(m_scoresPerModel, 0);
        size_t seenBy = 0;
        offset = 0;
        // sum
        for (size_t i = 0; i < m_numModels; ++i) {
          if (pdgPhrase.m_seenBy[i]) {
            for (size_t j = 0; j < m_scoresPerModel; ++j) {
              avgScores[j] += pdgPhrase.m_scores[offset + j];
            }
            seenBy += 1;
          }
          offset += m_scoresPerModel;
        }
        // divide
        for (size_t j = 0; j < m_scoresPerModel; ++j) {
          avgScores[j] /= seenBy;
        }
        // copy
        offset = 0;
        for (size_t i = 0; i < m_numModels; ++i) {
          if (!pdgPhrase.m_seenBy[i]) {
            for (size_t j = 0; j < m_scoresPerModel; ++j) {
              pdgPhrase.m_scores[offset + j] = avgScores[j];
            }
          }
          offset += m_scoresPerModel;
        }
#ifdef PT_UG
        // Also average LexicalReordering scores if specified
        // We don't necessarily have a lr-func for each model
        if (m_haveMmsaptLrFunc) {
          SPTR<Scores> avgLRScores;
          size_t seenBy = 0;
          // For each model
          for (size_t i = 0; i < m_numModels; ++i) {
            const LexicalReordering* lrFunc = *m_mmsaptLrFuncs[i];
            // Add if phrase seen and model has lr-func
            if (pdgPhrase.m_seenBy[i] && lrFunc != NULL) {
              const Scores* scores = pdgPhrase.m_targetPhrase->GetExtraScores(lrFunc);
              if (!avgLRScores) {
                avgLRScores.reset(new Scores(*scores));
              } else {
                for (size_t j = 0; j < scores->size(); ++j) {
                  (*avgLRScores)[j] += (*scores)[j];
                }
              }
              seenBy += 1;
            }
          }
          // Make sure we have at least one lr-func
          if (avgLRScores) {
            // divide
            for (size_t j = 0; j < avgLRScores->size(); ++j) {
              (*avgLRScores)[j] /= seenBy;
            }
            // set
            for (size_t i = 0; i < m_numModels; ++i) {
              const LexicalReordering* lrFunc = *m_mmsaptLrFuncs[i];
              if (!pdgPhrase.m_seenBy[i] && lrFunc != NULL) {
                pdgPhrase.m_targetPhrase->SetExtraScores(lrFunc, avgLRScores);
              }
            }
          }
        }
#endif
      }
    }

    // Assign scores
    phrase->GetScoreBreakdown().Assign(this, pdgPhrase.m_scores);
    // Correct future cost estimates and total score
    phrase->EvaluateInIsolation(src, pd_feature_const);
    ret->Add(phrase);
  }

  return ret;
}

ChartRuleLookupManager*
PhraseDictionaryGroup::
CreateRuleLookupManager(const ChartParser &,
                        const ChartCellCollectionBase&, size_t)
{
  UTIL_THROW(util::Exception, "Phrase table used in chart decoder");
}

//copied from PhraseDictionaryCompact; free memory allocated to TargetPhraseCollection (and each TargetPhrase) at end of sentence
void PhraseDictionaryGroup::CacheForCleanup(TargetPhraseCollection::shared_ptr  tpc)
{
  PhraseCache &ref = GetPhraseCache();
  ref.push_back(tpc);
}

void
PhraseDictionaryGroup::
CleanUpAfterSentenceProcessing(const InputType &source)
{
  GetPhraseCache().clear();
  CleanUpComponentModels(source);
}

void PhraseDictionaryGroup::CleanUpComponentModels(const InputType &source)
{
  for (size_t i = 0; i < m_numModels; ++i) {
    m_memberPDs[i]->CleanUpAfterSentenceProcessing(source);
  }
}

} //namespace
