/*
 * System.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <string>
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include "System.h"
#include "FF/FeatureFunction.h"
#include "TranslationModel/UnknownWordPenalty.h"
#include "legacy/Util2.h"
#include "util/exception.hh"

using namespace std;

namespace Moses2
{

thread_local MemPool System::m_managerPool;
thread_local MemPool System::m_systemPool;
thread_local Recycler<HypothesisBase*> System::m_hypoRecycler;

System::System(const Parameter &paramsArg) :
  params(paramsArg), featureFunctions(*this)
{
  options.init(paramsArg);
  IsPb();

  bestCollector.reset(new OutputCollector());

  params.SetParameter(cpuAffinityOffset, "cpu-affinity-offset", -1);
  params.SetParameter(cpuAffinityOffsetIncr, "cpu-affinity-increment", 1);

  const PARAM_VEC *section;

  // output collectors
  if (options.nbest.nbest_size) {
    nbestCollector.reset(new OutputCollector(options.nbest.output_file_path));
  }

  if (!options.output.detailed_transrep_filepath.empty()) {
    detailedTranslationCollector.reset(new OutputCollector(options.output.detailed_transrep_filepath));
  }

  featureFunctions.Create();
  LoadWeights();

  if (params.GetParam("show-weights")) {
    cerr << "Showing weights then exit" << endl;
    featureFunctions.ShowWeights(weights);
    //return;
  }

  cerr << "START featureFunctions.Load()" << endl;
  featureFunctions.Load();
  cerr << "START LoadMappings()" << endl;
  LoadMappings();
  cerr << "END LoadMappings()" << endl;
  LoadDecodeGraphBackoff();
  cerr << "END LoadDecodeGraphBackoff()" << endl;

  UTIL_THROW_IF2(options.input.xml_policy == XmlConstraint, "XmlConstraint not supported");

  // max spans for scfg decoding
  if (!isPb) {
    section = params.GetParam("max-chart-span");
    if (section && section->size()) {
      maxChartSpans = Scan<size_t>(*section);
      maxChartSpans.resize(mappings.size(), DEFAULT_MAX_CHART_SPAN);

      /*
      cerr << "maxChartSpans=" << maxChartSpans.size();
      for (size_t i = 0; i < maxChartSpans.size(); ++i) {
          cerr << " " << mappings[i]->GetName() << "=" << maxChartSpans[i];
      }
      cerr << endl;
      */
    }
  }

}

System::~System()
{
}

void System::LoadWeights()
{
  weights.Init(featureFunctions);

  //cerr << "Weights:" << endl;
  typedef std::map<std::string, std::vector<float> > WeightMap;
  const WeightMap &allWeights = params.GetAllWeights();

  // check all weights are there for all FF
  const std::vector<const FeatureFunction*> &ffs = featureFunctions.GetFeatureFunctions();
  BOOST_FOREACH(const FeatureFunction *ff, ffs) {
    if (ff->IsTuneable()) {
      const std::string &ffName = ff->GetName();
      WeightMap::const_iterator iterWeight = allWeights.find(ffName);
      UTIL_THROW_IF2(iterWeight == allWeights.end(), "Must specify weight for " << ffName);
    }
  }


  // set weight
  BOOST_FOREACH(const WeightMap::value_type &valPair, allWeights) {
    const string &ffName = valPair.first;
    const std::vector<float> &ffWeights = valPair.second;
    /*
    cerr << ffName << "=";
    for (size_t i = 0; i < ffWeights.size(); ++i) {
      cerr << ffWeights[i] << " ";
    }
    cerr << endl;
    */
    weights.SetWeights(featureFunctions, ffName, ffWeights);
  }
}

void System::LoadMappings()
{
  const PARAM_VEC *vec = params.GetParam("mapping");
  UTIL_THROW_IF2(vec == NULL, "Must have [mapping] section");

  BOOST_FOREACH(const std::string &line, *vec) {
    vector<string> toks = Tokenize(line);
    assert( (toks.size() == 2 && toks[0] == "T") || (toks.size() == 3 && toks[1] == "T") );

    size_t ptInd;
    if (toks.size() == 2) {
      ptInd = Scan<size_t>(toks[1]);
    } else {
      ptInd = Scan<size_t>(toks[2]);
    }
    const PhraseTable *pt = featureFunctions.GetPhraseTableExcludeUnknownWordPenalty(ptInd);
    mappings.push_back(pt);
  }

// unk pt
  const UnknownWordPenalty *unkWP = featureFunctions.GetUnknownWordPenalty();
  if (unkWP) {
    mappings.push_back(unkWP);
  }
}

void System::LoadDecodeGraphBackoff()
{
  const PARAM_VEC *vec = params.GetParam("decoding-graph-backoff");

  for (size_t i = 0; i < mappings.size(); ++i) {
    PhraseTable *pt = const_cast<PhraseTable*>(mappings[i]);

    if (vec && vec->size() < i) {
      pt->decodeGraphBackoff = Scan<int>((*vec)[i]);
    } else if (pt == featureFunctions.GetUnknownWordPenalty()) {
      pt->decodeGraphBackoff = 1;
    } else {
      pt->decodeGraphBackoff = 0;
    }
  }
}

MemPool &System::GetSystemPool() const
{
  return m_systemPool;
}

MemPool &System::GetManagerPool() const
{
  return m_managerPool;
}

FactorCollection &System::GetVocab() const
{
  return m_vocab;
}

Recycler<HypothesisBase*> &System::GetHypoRecycler() const
{
  return m_hypoRecycler;
}

Batch &System::GetBatch(MemPool &pool) const
{
  Batch *obj;
  obj = m_batch.get();
  if (obj == NULL) {
    obj = new Batch(pool);
    m_batch.reset(obj);
  }
  assert(obj);
  return *obj;
}

void System::IsPb()
{
  switch (options.search.algo) {
  case Normal:
  case NormalBatch:
  case CubePruning:
  case CubePruningPerMiniStack:
  case CubePruningPerBitmap:
  case CubePruningCardinalStack:
  case CubePruningBitmapStack:
  case CubePruningMiniStack:
    isPb = true;
    break;
  case CYKPlus:
    isPb = false;
    break;
  default:
    abort();
    break;
  }
}


}

