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
#include "util/exception.hh"
#include "util/string_stream.hh"

#include "moses/TranslationModel/PhraseDictionaryMultiModel.h"

using namespace std;

namespace Moses

{

PhraseDictionaryMultiModel::
PhraseDictionaryMultiModel(const std::string &line)
  : PhraseDictionary(line, true)
{
  ReadParameters();

  if (m_mode == "interpolate") {
    size_t numWeights = m_numScoreComponents;
    UTIL_THROW_IF2(m_pdStr.size() != m_multimodelweights.size() &&
                   m_pdStr.size()*numWeights != m_multimodelweights.size(),
                   "Number of scores and weights are not equal");
  } else if (m_mode == "all" || m_mode == "all-restrict") {
    UTIL_THROW2("Implementation has moved: use PhraseDictionaryGroup with restrict=true/false");
  } else {
    util::StringStream msg;
    msg << "combination mode unknown: " << m_mode;
    throw runtime_error(msg.str());
  }
}

PhraseDictionaryMultiModel::
PhraseDictionaryMultiModel(int type, const std::string &line)
  :PhraseDictionary(line, true)
{
  if (type == 1) {
    // PhraseDictionaryMultiModelCounts
    UTIL_THROW_IF2(m_pdStr.size() != m_multimodelweights.size() &&
                   m_pdStr.size()*4 != m_multimodelweights.size(),
                   "Number of scores and weights are not equal");
  }
}

void
PhraseDictionaryMultiModel::
SetParameter(const std::string& key, const std::string& value)
{
  if (key == "mode") {
    m_mode = value;
  } else if (key == "components") {
    m_pdStr = Tokenize(value, ",");
    m_numModels = m_pdStr.size();
  } else if (key == "lambda") {
    m_multimodelweights = Tokenize<float>(value, ",");
  } else {
    PhraseDictionary::SetParameter(key, value);
  }
}

PhraseDictionaryMultiModel::
~PhraseDictionaryMultiModel()
{ }

void PhraseDictionaryMultiModel::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  SetFeaturesToApply();

  for(size_t i = 0; i < m_numModels; ++i) {
    const string &ptName = m_pdStr[i];

    PhraseDictionary *pt = FindPhraseDictionary(ptName);
    UTIL_THROW_IF2(pt == NULL,
                   "Could not find component phrase table " << ptName);
    m_pd.push_back(pt);
  }
}

TargetPhraseCollection::shared_ptr
PhraseDictionaryMultiModel::
GetTargetPhraseCollectionLEGACY(const Phrase& src) const
{

  std::vector<std::vector<float> > multimodelweights;
  multimodelweights = getWeights(m_numScoreComponents, true);
  TargetPhraseCollection::shared_ptr ret;

  std::map<std::string, multiModelStats*>* allStats;
  allStats = new(std::map<std::string,multiModelStats*>);
  CollectSufficientStatistics(src, allStats);
  ret = CreateTargetPhraseCollectionLinearInterpolation(src, allStats, multimodelweights);
  RemoveAllInMap(*allStats);
  delete allStats; // ??? Why the detour through malloc? UG

  ret->NthElement(m_tableLimit); // sort the phrases for pruning later
  const_cast<PhraseDictionaryMultiModel*>(this)->CacheForCleanup(ret);

  return ret;
}

void
PhraseDictionaryMultiModel::
CollectSufficientStatistics
(const Phrase& src, std::map<std::string, multiModelStats*>* allStats) const
{
  for(size_t i = 0; i < m_numModels; ++i) {
    const PhraseDictionary &pd = *m_pd[i];

    TargetPhraseCollection::shared_ptr ret_raw;
    ret_raw = pd.GetTargetPhraseCollectionLEGACY(src);
    if (ret_raw != NULL) {

      TargetPhraseCollection::const_iterator iterTargetPhrase, iterLast;
      if (m_tableLimit != 0 && ret_raw->GetSize() > m_tableLimit) {
        iterLast = ret_raw->begin() + m_tableLimit;
      } else {
        iterLast = ret_raw->end();
      }

      for (iterTargetPhrase = ret_raw->begin(); iterTargetPhrase != iterLast;  ++iterTargetPhrase) {
        const TargetPhrase * targetPhrase = *iterTargetPhrase;
        std::vector<float> raw_scores = targetPhrase->GetScoreBreakdown().GetScoresForProducer(&pd);

        std::string targetString = targetPhrase->GetStringRep(m_output);
        if (allStats->find(targetString) == allStats->end()) {

          multiModelStats * statistics = new multiModelStats;
          statistics->targetPhrase = new TargetPhrase(*targetPhrase); //make a copy so that we don't overwrite the original phrase table info
          statistics->p.resize(m_numScoreComponents);
          for(size_t j = 0; j < m_numScoreComponents; ++j) {
            statistics->p[j].resize(m_numModels);
          }

          //correct future cost estimates and total score
          statistics->targetPhrase->GetScoreBreakdown().InvertDenseFeatures(&pd);
          vector<FeatureFunction*> pd_feature;
          pd_feature.push_back(m_pd[i]);
          const vector<FeatureFunction*> pd_feature_const(pd_feature);
          statistics->targetPhrase->EvaluateInIsolation(src, pd_feature_const);
          // zero out scores from original phrase table
          statistics->targetPhrase->GetScoreBreakdown().ZeroDenseFeatures(&pd);

          (*allStats)[targetString] = statistics;

        }
        multiModelStats * statistics = (*allStats)[targetString];

        for(size_t j = 0; j < m_numScoreComponents; ++j) {
          statistics->p[j][i] = UntransformScore(raw_scores[j]);
        }

        (*allStats)[targetString] = statistics;
      }
    }
  }
}

TargetPhraseCollection::shared_ptr
PhraseDictionaryMultiModel::
CreateTargetPhraseCollectionLinearInterpolation
( const Phrase& src,
  std::map<std::string,multiModelStats*>* allStats,
  std::vector<std::vector<float> > &multimodelweights) const
{
  TargetPhraseCollection::shared_ptr ret(new TargetPhraseCollection);
  for ( std::map< std::string, multiModelStats*>::const_iterator iter = allStats->begin(); iter != allStats->end(); ++iter ) {

    multiModelStats * statistics = iter->second;

    Scores scoreVector(m_numScoreComponents);

    for(size_t i = 0; i < m_numScoreComponents; ++i) {
      scoreVector[i] = TransformScore(std::inner_product(statistics->p[i].begin(), statistics->p[i].end(), multimodelweights[i].begin(), 0.0));
    }

    statistics->targetPhrase->GetScoreBreakdown().Assign(this, scoreVector);

    //correct future cost estimates and total score
    vector<FeatureFunction*> pd_feature;
    pd_feature.push_back(const_cast<PhraseDictionaryMultiModel*>(this));
    const vector<FeatureFunction*> pd_feature_const(pd_feature);
    statistics->targetPhrase->EvaluateInIsolation(src, pd_feature_const);

    ret->Add(new TargetPhrase(*statistics->targetPhrase));
  }
  return ret;
}

//TODO: is it worth caching the results as long as weights don't change?
std::vector<std::vector<float> >
PhraseDictionaryMultiModel::
getWeights(size_t numWeights, bool normalize) const
{
  const std::vector<float>* weights_ptr;
  std::vector<float> raw_weights;

  weights_ptr = GetTemporaryMultiModelWeightsVector();

  // HIEU - uninitialised variable.
  //checking weights passed to mosesserver; only valid for this sentence; *don't* raise exception if client weights are malformed
  if (weights_ptr == NULL || weights_ptr->size() == 0) {
    weights_ptr = &m_multimodelweights; //fall back to weights defined in config
  } else if(weights_ptr->size() != m_numModels && weights_ptr->size() != m_numModels * numWeights) {
    //TODO: can we pass error message to client if weights are malformed?
    std::cerr << "Must have either one multimodel weight per model (" << m_numModels << "), or one per weighted feature and model (" << numWeights << "*" << m_numModels << "). You have " << weights_ptr->size() << ". Reverting to weights in config";
    weights_ptr = &m_multimodelweights; //fall back to weights defined in config
  }

  //checking weights defined in config; only valid for this sentence; raise exception if config weights are malformed
  if (weights_ptr == NULL || weights_ptr->size() == 0) {
    for (size_t i=0; i < m_numModels; i++) {
      raw_weights.push_back(1.0/m_numModels); //uniform weights created online
    }
  } else if(weights_ptr->size() != m_numModels && weights_ptr->size() != m_numModels * numWeights) {
    util::StringStream strme;
    strme << "Must have either one multimodel weight per model (" << m_numModels << "), or one per weighted feature and model (" << numWeights << "*" << m_numModels << "). You have " << weights_ptr->size() << ".";
    UTIL_THROW(util::Exception, strme.str());
  } else {
    raw_weights = *weights_ptr;
  }

  std::vector<std::vector<float> > multimodelweights (numWeights);

  for (size_t i=0; i < numWeights; i++) {
    std::vector<float> weights_onefeature (m_numModels);
    if(raw_weights.size() == m_numModels) {
      weights_onefeature = raw_weights;
    } else {
      copy ( raw_weights.begin()+i*m_numModels, raw_weights.begin()+(i+1)*m_numModels, weights_onefeature.begin() );
    }
    if(normalize) {
      multimodelweights[i] = normalizeWeights(weights_onefeature);
    } else {
      multimodelweights[i] = weights_onefeature;
    }
  }

  return multimodelweights;
}

std::vector<float>
PhraseDictionaryMultiModel::
normalizeWeights(std::vector<float> &weights) const
{
  std::vector<float> ret (m_numModels);
  float total = std::accumulate(weights.begin(),weights.end(),0.0);
  for (size_t i=0; i < weights.size(); i++) {
    ret[i] = weights[i]/total;
  }
  return ret;
}


ChartRuleLookupManager *
PhraseDictionaryMultiModel::
CreateRuleLookupManager(const ChartParser &, const ChartCellCollectionBase&,
                        std::size_t)
{
  UTIL_THROW(util::Exception, "Phrase table used in chart decoder");
}


//copied from PhraseDictionaryCompact; free memory allocated to TargetPhraseCollection (and each TargetPhrase) at end of sentence
void
PhraseDictionaryMultiModel::
CacheForCleanup(TargetPhraseCollection::shared_ptr tpc)
{
  GetPhraseCache().push_back(tpc);
}


void
PhraseDictionaryMultiModel::
CleanUpAfterSentenceProcessing(const InputType &source)
{
  // PhraseCache &ref = GetPhraseCache();
  // for(PhraseCache::iterator it = ref.begin(); it != ref.end(); it++) {
  //   it->reset();
  // }

  // PhraseCache temp;
  // temp.swap(ref);
  GetPhraseCache().clear();

  CleanUpComponentModels(source);

  std::vector<float> empty_vector;
  SetTemporaryMultiModelWeightsVector(empty_vector);
}


void
PhraseDictionaryMultiModel::
CleanUpComponentModels(const InputType &source)
{
  for(size_t i = 0; i < m_numModels; ++i) {
    m_pd[i]->CleanUpAfterSentenceProcessing(source);
  }
}

const std::vector<float>*
PhraseDictionaryMultiModel::
GetTemporaryMultiModelWeightsVector() const
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_lock_weights);
  if (m_multimodelweights_tmp.find(boost::this_thread::get_id()) != m_multimodelweights_tmp.end()) {
    return &m_multimodelweights_tmp.find(boost::this_thread::get_id())->second;
  } else {
    return NULL;
  }
#else
  return &m_multimodelweights_tmp;
#endif
}

void
PhraseDictionaryMultiModel::
SetTemporaryMultiModelWeightsVector(std::vector<float> weights)
{
#ifdef WITH_THREADS
  boost::unique_lock<boost::shared_mutex> lock(m_lock_weights);
  m_multimodelweights_tmp[boost::this_thread::get_id()] = weights;
#else
  m_multimodelweights_tmp = weights;
#endif
}

#ifdef WITH_DLIB
vector<float>
PhraseDictionaryMultiModel::
MinimizePerplexity(vector<pair<string, string> > &phrase_pair_vector)
{

  map<pair<string, string>, size_t> phrase_pair_map;

  for ( vector<pair<string, string> >::const_iterator iter = phrase_pair_vector.begin(); iter != phrase_pair_vector.end(); ++iter ) {
    phrase_pair_map[*iter] += 1;
  }

  vector<multiModelStatsOptimization*> optimizerStats;

  for ( map<pair<string, string>, size_t>::iterator iter = phrase_pair_map.begin(); iter != phrase_pair_map.end(); ++iter ) {

    pair<string, string> phrase_pair = iter->first;
    string source_string = phrase_pair.first;
    string target_string = phrase_pair.second;

    vector<float> fs(m_numModels);
    map<string,multiModelStats*>* allStats = new(map<string,multiModelStats*>);

    Phrase sourcePhrase(0);
    sourcePhrase.CreateFromString(Input, m_input, source_string, NULL);

    CollectSufficientStatistics(sourcePhrase, allStats); //optimization potential: only call this once per source phrase

    //phrase pair not found; leave cache empty
    if (allStats->find(target_string) == allStats->end()) {
      RemoveAllInMap(*allStats);
      delete allStats;
      continue;
    }

    multiModelStatsOptimization* targetStatistics = new multiModelStatsOptimization();
    targetStatistics->targetPhrase = new TargetPhrase(*(*allStats)[target_string]->targetPhrase);
    targetStatistics->p = (*allStats)[target_string]->p;
    targetStatistics->f = iter->second;
    optimizerStats.push_back(targetStatistics);

    RemoveAllInMap(*allStats);
    delete allStats;
  }

  Sentence sentence;
  CleanUpAfterSentenceProcessing(sentence); // free memory used by compact phrase tables

  size_t numWeights = m_numScoreComponents;

  vector<float> ret (m_numModels*numWeights);
  for (size_t iFeature=0; iFeature < numWeights; iFeature++) {

    CrossEntropy * ObjectiveFunction = new CrossEntropy(optimizerStats, this, iFeature);

    vector<float> weight_vector = Optimize(ObjectiveFunction, m_numModels);

    if (m_mode == "interpolate") {
      weight_vector = normalizeWeights(weight_vector);
    }

    cerr << "Weight vector for feature " << iFeature << ": ";
    for (size_t i=0; i < m_numModels; i++) {
      ret[(iFeature*m_numModels)+i] = weight_vector[i];
      cerr << weight_vector[i] << " ";
    }
    cerr << endl;
    delete ObjectiveFunction;
  }

  RemoveAllInColl(optimizerStats);
  return ret;

}

vector<float>
PhraseDictionaryMultiModel::
Optimize(OptimizationObjective *ObjectiveFunction, size_t numModels)
{

  dlib::matrix<double,0,1> starting_point;
  starting_point.set_size(numModels);
  starting_point = 1.0;

  try {
    dlib::find_min_bobyqa(*ObjectiveFunction,
                          starting_point,
                          2*numModels+1,    // number of interpolation points
                          dlib::uniform_matrix<double>(numModels,1, 1e-09),  // lower bound constraint
                          dlib::uniform_matrix<double>(numModels,1, 1e100),   // upper bound constraint
                          1.0,    // initial trust region radius
                          1e-5,  // stopping trust region radius
                          10000    // max number of objective function evaluations
                         );
  } catch (dlib::bobyqa_failure& e) {
    cerr << e.what() << endl;
  }

  vector<float> weight_vector (numModels);

  for (int i=0; i < starting_point.nr(); i++) {
    weight_vector[i] = starting_point(i);
  }

  cerr << "Cross-entropy: " << (*ObjectiveFunction)(starting_point) << endl;
  return weight_vector;
}


double CrossEntropy::operator() ( const dlib::matrix<double,0,1>& arg) const
{
  double total = 0.0;
  double n = 0.0;
  std::vector<float> weight_vector (m_model->m_numModels);

  for (int i=0; i < arg.nr(); i++) {
    weight_vector[i] = arg(i);
  }
  if (m_model->m_mode == "interpolate") {
    weight_vector = m_model->normalizeWeights(weight_vector);
  }

  for ( std::vector<multiModelStatsOptimization*>::const_iterator iter = m_optimizerStats.begin(); iter != m_optimizerStats.end(); ++iter ) {
    multiModelStatsOptimization* statistics = *iter;
    size_t f = statistics->f;

    double score;
    score = std::inner_product(statistics->p[m_iFeature].begin(), statistics->p[m_iFeature].end(), weight_vector.begin(), 0.0);

    total -= (FloorScore(TransformScore(score))/TransformScore(2))*f;
    n += f;
  }
  return total/n;
}

#endif

PhraseDictionary *FindPhraseDictionary(const string &ptName)
{
  const std::vector<PhraseDictionary*> &pts = PhraseDictionary::GetColl();

  PhraseDictionary *pt = NULL;
  std::vector<PhraseDictionary*>::const_iterator iter;
  for (iter = pts.begin(); iter != pts.end(); ++iter) {
    PhraseDictionary *currPt = *iter;
    if (currPt->GetScoreProducerDescription() == ptName) {
      pt = currPt;
      break;
    }
  }

  return pt;
}

} //namespace
