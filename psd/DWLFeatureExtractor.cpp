#include "DWLFeatureExtractor.h"
#include "Util.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <set>

using namespace std;
using namespace boost::bimaps;
using namespace boost::property_tree;
using namespace Moses;

namespace PSD
{

DWLFeatureExtractor::DWLFeatureExtractor(const IndexType &targetIndex, const ExtractorConfig &config, bool train)
  : m_targetIndex(targetIndex), m_config(config), m_train(train)
{  
  if (! m_config.IsLoaded())
    throw logic_error("configuration file not loaded");
}

// map<string, float> DWLFeatureExtractor::GetMaxProb(const vector<CeptTranslation> &translations)
// {
//   map<string, float> maxProbs;
//   vector<CeptTranslation>::const_iterator it;
//   vector<TTableEntry>::const_iterator tableIt;
//   for (it = translations.begin(); it != translations.end(); it++) {
//     for (tableIt = it->m_ttableScores.begin(); tableIt != it->m_ttableScores.end(); tableIt++) {
//       if (tableIt->m_exists) {
//         maxProbs[tableIt->m_id] = max(tableIt->m_scores[P_E_F_INDEX], maxProbs[tableIt->m_id]);
//       }
//     }
//   }
//   return maxProbs;
// }

void DWLFeatureExtractor::GenerateFeatures(FeatureConsumer *fc,
  const ContextType &context,
  const vector<pair<int, int> > &sourceSpanList,
  const vector<CeptTranslation> &translations,
  vector<float> &losses)
{  
  fc->SetNamespace('s', true);

  int globalStart = sourceSpanList.front().first;
  int globalEnd   = sourceSpanList.back().second;
  if (m_config.GetSourceExternal()) GenerateContextFeatures(context, globalStart, globalEnd, fc);

  // get words (surface forms) in source phrase
  vector<string> sourceForms;
  vector<pair<int, int> >::const_iterator spanIt;
  for (spanIt = sourceSpanList.begin(); spanIt != sourceSpanList.end(); spanIt++) {
    int spanStart = spanIt->first;
    int spanEnd = spanIt->second;
    for (size_t i = spanStart; i < spanEnd; i++)
      sourceForms.push_back(context[i][FACTOR_FORM]);
  }
  cerr << "[DWL] Generating features for source: " + Join("_", sourceForms) + "\n";
  
  map<string, float> maxProbs;
//  if (m_config.GetMostFrequent()) maxProbs = GetMaxProb(translations);

  if (m_config.GetSourceInternal()) GenerateInternalFeatures(sourceForms, fc);
  if (m_config.GetPhraseFactor()) GeneratePhraseFactorFeatures(context, sourceSpanList, fc);
  if (m_config.GetBagOfWords()) GenerateBagOfWordsFeatures(context, globalStart, globalEnd, FACTOR_FORM, fc);
  if (m_config.GetSourceIndicator()) GenerateIndicatorFeature(sourceForms, fc); 


  vector<CeptTranslation>::const_iterator transIt = translations.begin();
  vector<float>::iterator lossIt = losses.begin();
  for (; transIt != translations.end(); transIt++, lossIt++) {
    assert(lossIt != losses.end());
    fc->SetNamespace('t', false);

    // get words in target phrase
    vector<string> targetForms = Tokenize(m_targetIndex.right.find(transIt->m_index)->second, " ");
    // cerr << "Predicting score for phrase " << Join(" ", targetForms) << endl;

    if (m_config.GetTargetInternal()) GenerateInternalFeatures(targetForms, fc);
    if (m_config.GetTargetPhraseFactor()) GenerateTargetFactorFeatures(targetForms, fc);
    if (m_config.GetPaired()) GeneratePairedFeatures(sourceForms, targetForms, fc); // everything is aligned to everything

//  TODO no scores for now
//  if (m_config.GetMostFrequent()) GenerateMostFrequentFeature(transIt->m_ttableScores, maxProbs, fc);

//    TODO no scores for now
//    if (m_config.GetBinnedScores()) GenerateScoreFeatures(transIt->m_ttableScores, fc);

    // later, for DA;  "NOT_IN_" features
//    if (m_config.GetBinnedScores() || m_config.GetMostFrequent()) GenerateTTableEntryFeatures(transIt->m_ttableScores, fc);

    if (m_config.GetTargetIndicator()) GenerateIndicatorFeature(targetForms, fc); 

    if (m_config.GetSourceTargetIndicator()) GenerateConcatIndicatorFeature(sourceForms, targetForms, fc); 

    if (m_train) {
      fc->Train(SPrint(transIt->m_index), *lossIt);
    } else {
      *lossIt = fc->Predict(SPrint(transIt->m_index));
    }
  }
  fc->FinishExample();
}


//
// private methods
//

string DWLFeatureExtractor::BuildContextFeature(size_t factor, int index, const string &value)
{
  return "c^" + SPrint(factor) + "_" + SPrint(index) + "_" + value;
}

void DWLFeatureExtractor::GenerateContextFeatures(const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  FeatureConsumer *fc)
{
  vector<size_t>::const_iterator factIt;
  for (factIt = m_config.GetFactors().begin(); factIt != m_config.GetFactors().end(); factIt++) {
    for (size_t i = 1; i <= m_config.GetWindowSize(); i++) {
      string left = "<s>";
      string right = "</s>"; 
      if (spanStart >= i) {
        CHECK(context[spanStart - i].size() > *factIt);
        left = context[spanStart - i][*factIt];
      }
      fc->AddFeature(BuildContextFeature(*factIt, -i, left));
      // spanEnd points beyond the last element
      if (spanEnd - 1 + i < context.size()) {
        CHECK(context[spanEnd - 1 + i].size() > *factIt);
        right = context[spanEnd - 1 + i][*factIt];
      }
      fc->AddFeature(BuildContextFeature(*factIt, i, right));
    }
  }
}

void DWLFeatureExtractor::GenerateIndicatorFeature(const vector<string> &span, FeatureConsumer *fc)
{
  fc->AddFeature("p^" + (span.empty() ? "__NULL__" : Join("_", span)));
}

void DWLFeatureExtractor::GenerateConcatIndicatorFeature(const vector<string> &span1, const vector<string> &span2, FeatureConsumer *fc)
{
  fc->AddFeature("p^" + Join("_", span1) + "^" + Join("_", span2));
}

//New feature to decompose target factor
//
void DWLFeatureExtractor::GenerateInternalFeatures(const vector<string> &span, FeatureConsumer *fc)
{
  vector<string>::const_iterator it;
  for (it = span.begin(); it != span.end(); it++) {
    fc->AddFeature("w^" + *it);
  }
}

void DWLFeatureExtractor::GenerateBagOfWordsFeatures(const ContextType &context, int spanStart, int spanEnd, size_t factorID, FeatureConsumer *fc)
{
  for (size_t i = 0; i < spanStart; i++)
    fc->AddFeature("bow^" + context[i][factorID]);
  for (size_t i = spanEnd; i < context.size(); i++)
    fc->AddFeature("bow^" + context[i][factorID]);
}

void DWLFeatureExtractor::GeneratePhraseFactorFeatures(const ContextType &context, const vector<pair<int, int> > &sourceSpanList, FeatureConsumer *fc)
{
  vector<pair<int, int> >::const_iterator spanIt;
  for (spanIt = sourceSpanList.begin(); spanIt != sourceSpanList.end(); spanIt++) {
    int spanStart = spanIt->first;
    int spanEnd = spanIt->second;
    for (size_t i = spanStart; i < spanEnd; i++) {
      vector<size_t>::const_iterator factIt;
      for (factIt = m_config.GetFactors().begin(); factIt != m_config.GetFactors().end(); factIt++) {
        fc->AddFeature("ibow^" + SPrint(*factIt) + "_" + context[i][*factIt]);
      }
    }
  }
}

void DWLFeatureExtractor::GenerateTargetFactorFeatures(vector<string> targetForms, FeatureConsumer *fc)
{
  vector<string>::iterator itr_target;
  for(itr_target = targetForms.begin(); itr_target != targetForms.end(); itr_target++)
  {
	  string currentWord = *itr_target;
	  vector<string> targetToken = Tokenize(*itr_target,"|");
	  vector<string>::iterator itr_target_token;
	  CHECK(targetToken.size() == 3);
	  fc->AddFeature("trich^" + targetToken[0]);
	  fc->AddFeature("trich^" + targetToken[1]);

	  //In case we want to learn that these features appear together
	  fc->AddFeature("trich^" + targetToken[2]);

	  string morphFeatures = targetToken[2];
	  string::iterator itr_morph;
	  int counter = 0;

	  for(itr_morph = morphFeatures.begin(); itr_morph != morphFeatures.end(); itr_morph++)
	  {
		  string c;
		  c += *itr_morph;
		  if(c == "-")
		  {
			  fc->AddFeature("trich^"+SPrint(counter)+"^UNDEFINED");
		  }
		  else
		  {
			  fc->AddFeature("trich^"+SPrint(counter)+"^"+c);
		  }
	  }
  }
}

// XXX NULL feature!!!

void DWLFeatureExtractor::GenerateGapFeatures(const ContextType &context, const vector<pair<int, int> > &sourceSpanList, FeatureConsumer *fc) {
  for (size_t i = 0; i < sourceSpanList.size(); i++) fc->AddFeature("gap^" + SPrint<int>(i));
}

void DWLFeatureExtractor::GeneratePairedFeatures(const vector<string> &srcPhrase, const vector<string> &tgtPhrase, 
    FeatureConsumer *fc)
{
  if (srcPhrase.empty()) {
    for (size_t i = 0; i < tgtPhrase.size(); i++) {
      fc->AddFeature("pair^__NULL__^" + tgtPhrase[i]);
    }
  } else {
    for (size_t i = 0; i < srcPhrase.size(); i++) {
      for (size_t j = 0; j < tgtPhrase.size(); j++) {
        fc->AddFeature("pair^" + srcPhrase[i] + "^" + tgtPhrase[j]);
      }
    }
  }
}

// void DWLFeatureExtractor::GenerateScoreFeatures(const std::vector<TTableEntry> &ttableScores, FeatureConsumer *fc)
// {
//   vector<size_t>::const_iterator scoreIt;
//   vector<float>::const_iterator binIt;
//   vector<TTableEntry>::const_iterator tableIt;
//   const vector<size_t> &scoreIDs = m_config.GetScoreIndexes();
//   const vector<float> &bins = m_config.GetScoreBins();
// 
//   for (tableIt = ttableScores.begin(); tableIt != ttableScores.end(); tableIt++) {
//     if (! tableIt->m_exists)
//       continue;
//     string prefix = ttableScores.size() == 1 ? "" : tableIt->m_id + "_";
//     for (scoreIt = scoreIDs.begin(); scoreIt != scoreIDs.end(); scoreIt++) {
//       for (binIt = bins.begin(); binIt != bins.end(); binIt++) {
//         float logScore = log(tableIt->m_scores[*scoreIt]);
//         if (logScore < *binIt || Equals(logScore, *binIt)) {
//           fc->AddFeature(prefix + "sc^" + SPrint<size_t>(*scoreIt) + "_" + SPrint(*binIt));
//         }
//       }
//     }
//   }
// }
// 
// void DWLFeatureExtractor::GenerateMostFrequentFeature(const std::vector<TTableEntry> &ttableScores, const map<string, float> &maxProbs, FeatureConsumer *fc)
// {
//   vector<TTableEntry>::const_iterator it;
//   for (it = ttableScores.begin(); it != ttableScores.end(); it++) {
//     if (it->m_exists && Equals(it->m_scores[P_E_F_INDEX], maxProbs.find(it->m_id)->second)) {
//       string prefix = ttableScores.size() == 1 ? "" : it->m_id + "_";
//       fc->AddFeature(prefix + "MOST_FREQUENT");
//     }
//   }
// }

// once we do DA, for now we don't have the scores
// void DWLFeatureExtractor::GenerateTTableEntryFeatures(const std::vector<TTableEntry> &ttableScores, FeatureConsumer *fc)
// {
//   vector<TTableEntry>::const_iterator it;
//   for (it = ttableScores.begin(); it != ttableScores.end(); it++) {
//     if (! it->m_exists)
//       fc->AddFeature("NOT_IN_" + it->m_id);
//   }
// }

} // namespace PSD
