// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
// $Id$
// vim:tabstop=2

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

#include <string>
#include <boost/algorithm/string/predicate.hpp>

#include "moses/FF/Factory.h"
#include "TypeDef.h"
#include "moses/FF/WordPenaltyProducer.h"
#include "moses/FF/UnknownWordPenaltyProducer.h"
#include "moses/FF/InputFeature.h"
#include "moses/FF/DynamicCacheBasedLanguageModel.h"
#include "moses/TranslationModel/PhraseDictionaryDynamicCacheBased.h"

#include "DecodeStepTranslation.h"
#include "DecodeStepGeneration.h"
#include "GenerationDictionary.h"
#include "StaticData.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Timer.h"
#include "TranslationOption.h"
#include "DecodeGraph.h"
#include "InputFileStream.h"
#include "ScoreComponentCollection.h"
#include "DecodeGraph.h"
#include "TranslationModel/PhraseDictionary.h"
#include "TranslationModel/PhraseDictionaryTreeAdaptor.h"

#ifdef WITH_THREADS
#include <boost/thread.hpp>
#endif
#ifdef HAVE_CMPH
#include "moses/TranslationModel/CompactPT/PhraseDictionaryCompact.h"
#endif
#if defined HAVE_CMPH
#include "moses/TranslationModel/CompactPT/LexicalReorderingTableCompact.h"
#endif

using namespace std;
using namespace boost::algorithm;

namespace Moses
{
StaticData StaticData::s_instance;

StaticData::StaticData()
  : m_options(new AllOptions)
  , m_requireSortingAfterSourceContext(false)
  , m_currentWeightSetting("default")
  , m_treeStructure(NULL)
  , m_coordSpaceNextID(1)
{
  Phrase::InitializeMemPool();
}

StaticData::~StaticData()
{
  RemoveAllInColl(m_decodeGraphs);
  Phrase::FinalizeMemPool();
}

bool StaticData::LoadDataStatic(Parameter *parameter, const std::string &execPath)
{
  s_instance.SetExecPath(execPath);
  return s_instance.LoadData(parameter);
}

void
StaticData
::initialize_features()
{
  std::map<std::string, std::string> featureNameOverride = OverrideFeatureNames();
  // all features
  map<string, int> featureIndexMap;

  const PARAM_VEC* params = m_parameter->GetParam("feature");
  for (size_t i = 0; params && i < params->size(); ++i) {
    const string &line = Trim(params->at(i));
    VERBOSE(1,"line=" << line << endl);
    if (line.empty())
      continue;

    vector<string> toks = Tokenize(line);

    string &feature = toks[0];
    std::map<std::string, std::string>::const_iterator iter
    = featureNameOverride.find(feature);
    if (iter == featureNameOverride.end()) {
      // feature name not override
      m_registry.Construct(feature, line);
    } else {
      // replace feature name with new name
      string newName = iter->second;
      feature = newName;
      string newLine = Join(" ", toks);
      m_registry.Construct(newName, newLine);
    }
  }

  NoCache();
  OverrideFeatures();

}

bool
StaticData
::ini_output_options()
{
  // verbose level
  m_parameter->SetParameter(m_verboseLevel, "verbose", (size_t) 1);
  m_parameter->SetParameter<string>(m_outputUnknownsFile,
                                    "output-unknowns", "");
  return true;
}

// threads, timeouts, etc.
bool
StaticData
::ini_performance_options()
{
  const PARAM_VEC *params;

  m_threadCount = 1;
  params = m_parameter->GetParam("threads");
  if (params && params->size()) {
    if (params->at(0) == "all") {
#ifdef WITH_THREADS
      m_threadCount = boost::thread::hardware_concurrency();
      if (!m_threadCount) {
        std::cerr << "-threads all specified but Boost doesn't know how many cores there are";
        return false;
      }
#else
      std::cerr << "-threads all specified but moses not built with thread support";
      return false;
#endif
    } else {
      m_threadCount = Scan<int>(params->at(0));
      if (m_threadCount < 1) {
        std::cerr << "Specify at least one thread.";
        return false;
      }
#ifndef WITH_THREADS
      if (m_threadCount > 1) {
        std::cerr << "Error: Thread count of " << params->at(0)
                  << " but moses not built with thread support";
        return false;
      }
#endif
    }
  }
  return true;
}

bool StaticData::LoadData(Parameter *parameter)
{
  m_parameter = parameter;

  const PARAM_VEC *params;

  m_options->init(*parameter);
  if (is_syntax(m_options->search.algo))
    m_options->syntax.LoadNonTerminals(*parameter, FactorCollection::Instance());

  if (is_syntax(m_options->search.algo))
    LoadChartDecodingParameters();

  // ORDER HERE MATTERS, SO DON'T CHANGE IT UNLESS YOU KNOW WHAT YOU ARE DOING!
  // input, output

  m_parameter->SetParameter<string>(m_factorDelimiter, "factor-delimiter", "|");
  m_parameter->SetParameter<size_t>(m_lmcache_cleanup_threshold, "clean-lm-cache", 1);

  m_bookkeeping_options.init(*parameter);
  if (!ini_output_options()) return false;

  // threading etc.
  if (!ini_performance_options()) return false;

  // FEATURE FUNCTION INITIALIZATION HAPPENS HERE ===============================

  // set class-specific default parameters
#if defined HAVE_CMPH
  LexicalReorderingTableCompact::SetStaticDefaultParameters(*parameter);
  PhraseDictionaryCompact::SetStaticDefaultParameters(*parameter);
#endif

  initialize_features();

  if (m_parameter->GetParam("show-weights") == NULL)
    LoadFeatureFunctions();

  LoadDecodeGraphs();

  // sanity check that there are no weights without an associated FF
  if (!CheckWeights()) return false;

  //Load extra feature weights
  string weightFile;
  m_parameter->SetParameter<string>(weightFile, "weight-file", "");
  if (!weightFile.empty()) {
    ScoreComponentCollection extraWeights;
    if (!extraWeights.Load(weightFile)) {
      std::cerr << "Unable to load weights from " << weightFile;
      return false;
    }
    m_allWeights.PlusEquals(extraWeights);
  }

  //Load sparse features from config (overrules weight file)
  LoadSparseWeightsFromConfig();

  // load alternate weight settings
  //
  // When and where are these used??? [UG]
  //
  // Update: Just checked the manual. The config file is NOT the right
  // place to do this. [UG]
  //
  // <TODO>
  // * Eliminate alternate-weight-setting. Alternate weight settings should
  //   be provided with the input, not in the config file.
  // </TODO>
  params = m_parameter->GetParam("alternate-weight-setting");
  if (params && params->size() && !LoadAlternateWeightSettings())
    return false;

  return true;
}

void StaticData::SetWeight(const FeatureFunction* sp, float weight)
{
  m_allWeights.Resize();
  m_allWeights.Assign(sp,weight);
}

void StaticData::SetWeights(const FeatureFunction* sp,
                            const std::vector<float>& weights)
{
  m_allWeights.Resize();
  m_allWeights.Assign(sp,weights);
}

void StaticData::LoadNonTerminals()
{
  string defaultNonTerminals;
  m_parameter->SetParameter<string>(defaultNonTerminals, "non-terminals", "X");

  FactorCollection &factorCollection = FactorCollection::Instance();

  m_inputDefaultNonTerminal.SetIsNonTerminal(true);
  const Factor *sourceFactor = factorCollection.AddFactor(Input, 0, defaultNonTerminals, true);
  m_inputDefaultNonTerminal.SetFactor(0, sourceFactor);

  m_outputDefaultNonTerminal.SetIsNonTerminal(true);
  const Factor *targetFactor = factorCollection.AddFactor(Output, 0, defaultNonTerminals, true);
  m_outputDefaultNonTerminal.SetFactor(0, targetFactor);

  // for unknown words
  const PARAM_VEC *params = m_parameter->GetParam("unknown-lhs");
  if (params == NULL || params->size() == 0) {
    UnknownLHSEntry entry(defaultNonTerminals, 0.0f);
    m_unknownLHS.push_back(entry);
  } else {
    const string &filePath = params->at(0);

    InputFileStream inStream(filePath);
    string line;
    while(getline(inStream, line)) {
      vector<string> tokens = Tokenize(line);
      UTIL_THROW_IF2(tokens.size() != 2,
                     "Incorrect unknown LHS format: " << line);
      UnknownLHSEntry entry(tokens[0], Scan<float>(tokens[1]));
      m_unknownLHS.push_back(entry);
      // const Factor *targetFactor =
      factorCollection.AddFactor(Output, 0, tokens[0], true);
    }

  }

}

void StaticData::LoadChartDecodingParameters()
{
  LoadNonTerminals();

  // source label overlap
  m_parameter->SetParameter(m_sourceLabelOverlap, "source-label-overlap",
                            SourceLabelOverlapAdd);

}

void StaticData::LoadDecodeGraphs()
{
  vector<string> mappingVector;
  vector<size_t> maxChartSpans;

  const PARAM_VEC *params;

  params = m_parameter->GetParam("mapping");
  if (params && params->size()) {
    mappingVector = *params;
  } else {
    mappingVector.assign(1,"0 T 0");
  }

  params = m_parameter->GetParam("max-chart-span");
  if (params && params->size()) {
    maxChartSpans = Scan<size_t>(*params);
  }

  vector<string> toks = Tokenize(mappingVector[0]);
  if (toks.size() == 3) {
    // eg 0 T 0
    LoadDecodeGraphsOld(mappingVector, maxChartSpans);
  } else if (toks.size() == 2) {
    if (toks[0] == "T" || toks[0] == "G") {
      // eg. T 0
      LoadDecodeGraphsOld(mappingVector, maxChartSpans);
    } else {
      // eg. 0 TM1
      LoadDecodeGraphsNew(mappingVector, maxChartSpans);
    }
  } else {
    UTIL_THROW(util::Exception, "Malformed mapping");
  }
}

void
StaticData::
LoadDecodeGraphsOld(const vector<string> &mappingVector,
                    const vector<size_t> &maxChartSpans)
{
  const vector<PhraseDictionary*>& pts = PhraseDictionary::GetColl();
  const vector<GenerationDictionary*>& gens = GenerationDictionary::GetColl();

  const std::vector<FeatureFunction*> *featuresRemaining
  = &FeatureFunction::GetFeatureFunctions();
  DecodeStep *prev = 0;
  size_t prevDecodeGraphInd = 0;

  for(size_t i=0; i<mappingVector.size(); i++) {
    vector<string>	token		= Tokenize(mappingVector[i]);
    size_t decodeGraphInd;
    DecodeType decodeType;
    size_t index;
    if (token.size() == 2) {
      // eg. T 0
      decodeGraphInd = 0;
      decodeType = token[0] == "T" ? Translate : Generate;
      index = Scan<size_t>(token[1]);
    } else if (token.size() == 3) {
      // eg. 0 T 0
      // For specifying multiple translation model
      decodeGraphInd = Scan<size_t>(token[0]);
      //the vectorList index can only increment by one
      UTIL_THROW_IF2(decodeGraphInd != prevDecodeGraphInd
                     && decodeGraphInd != prevDecodeGraphInd + 1,
                     "Malformed mapping");
      if (decodeGraphInd > prevDecodeGraphInd) {
        prev = NULL;
      }

      if (prevDecodeGraphInd < decodeGraphInd) {
        featuresRemaining = &FeatureFunction::GetFeatureFunctions();
      }

      decodeType = token[1] == "T" ? Translate : Generate;
      index = Scan<size_t>(token[2]);
    } else {
      UTIL_THROW(util::Exception, "Malformed mapping");
    }

    DecodeStep* decodeStep = NULL;
    switch (decodeType) {
    case Translate:
      if(index>=pts.size()) {
        util::StringStream strme;
        strme << "No phrase dictionary with index "
              << index << " available!";
        UTIL_THROW(util::Exception, strme.str());
      }
      decodeStep = new DecodeStepTranslation(pts[index], prev, *featuresRemaining);
      break;
    case Generate:
      if(index>=gens.size()) {
        util::StringStream strme;
        strme << "No generation dictionary with index "
              << index << " available!";
        UTIL_THROW(util::Exception, strme.str());
      }
      decodeStep = new DecodeStepGeneration(gens[index], prev, *featuresRemaining);
      break;
    default:
      UTIL_THROW(util::Exception, "Unknown decode step");
      break;
    }

    featuresRemaining = &decodeStep->GetFeaturesRemaining();

    UTIL_THROW_IF2(decodeStep == NULL, "Null decode step");
    if (m_decodeGraphs.size() < decodeGraphInd + 1) {
      DecodeGraph *decodeGraph;
      if (is_syntax(m_options->search.algo)) {
        size_t maxChartSpan;
        if (decodeGraphInd < maxChartSpans.size()) {
          maxChartSpan = maxChartSpans[decodeGraphInd];
          VERBOSE(1,"max-chart-span: " << maxChartSpans[decodeGraphInd] << endl);
        } else {
          maxChartSpan = DEFAULT_MAX_CHART_SPAN;
        }
        decodeGraph = new DecodeGraph(m_decodeGraphs.size(), maxChartSpan);
      } else {
        decodeGraph = new DecodeGraph(m_decodeGraphs.size());
      }

      m_decodeGraphs.push_back(decodeGraph); // TODO max chart span
    }

    m_decodeGraphs[decodeGraphInd]->Add(decodeStep);
    prev = decodeStep;
    prevDecodeGraphInd = decodeGraphInd;
  }

  // set maximum n-gram size for backoff approach to decoding paths
  // default is always use subsequent paths (value = 0)
  // if specified, record maxmimum unseen n-gram size
  const vector<string> *backoffVector = m_parameter->GetParam("decoding-graph-backoff");
  for(size_t i=0; i<m_decodeGraphs.size() && backoffVector && i<backoffVector->size(); i++) {
    DecodeGraph &decodeGraph = *m_decodeGraphs[i];

    if (i < backoffVector->size()) {
      decodeGraph.SetBackoff(Scan<size_t>(backoffVector->at(i)));
    }
  }
}

void StaticData::LoadDecodeGraphsNew(const std::vector<std::string> &mappingVector, const std::vector<size_t> &maxChartSpans)
{
  const std::vector<FeatureFunction*> *featuresRemaining = &FeatureFunction::GetFeatureFunctions();
  DecodeStep *prev = 0;
  size_t prevDecodeGraphInd = 0;

  for(size_t i=0; i<mappingVector.size(); i++) {
    vector<string>	token		= Tokenize(mappingVector[i]);
    size_t decodeGraphInd;

    decodeGraphInd = Scan<size_t>(token[0]);
    //the vectorList index can only increment by one
    UTIL_THROW_IF2(decodeGraphInd != prevDecodeGraphInd
                   && decodeGraphInd != prevDecodeGraphInd + 1,
                   "Malformed mapping");
    if (decodeGraphInd > prevDecodeGraphInd) {
      prev = NULL;
    }

    if (prevDecodeGraphInd < decodeGraphInd) {
      featuresRemaining = &FeatureFunction::GetFeatureFunctions();
    }

    FeatureFunction &ff = FeatureFunction::FindFeatureFunction(token[1]);

    DecodeStep* decodeStep = NULL;
    if (typeid(ff) == typeid(PhraseDictionary)) {
      decodeStep = new DecodeStepTranslation(&static_cast<PhraseDictionary&>(ff), prev, *featuresRemaining);
    } else if (typeid(ff) == typeid(GenerationDictionary)) {
      decodeStep = new DecodeStepGeneration(&static_cast<GenerationDictionary&>(ff), prev, *featuresRemaining);
    } else {
      UTIL_THROW(util::Exception, "Unknown decode step");
    }

    featuresRemaining = &decodeStep->GetFeaturesRemaining();

    UTIL_THROW_IF2(decodeStep == NULL, "Null decode step");
    if (m_decodeGraphs.size() < decodeGraphInd + 1) {
      DecodeGraph *decodeGraph;
      if (is_syntax(m_options->search.algo)) {
        size_t maxChartSpan = (decodeGraphInd < maxChartSpans.size()) ? maxChartSpans[decodeGraphInd] : DEFAULT_MAX_CHART_SPAN;
        VERBOSE(1,"max-chart-span: " << maxChartSpans[decodeGraphInd] << endl);
        decodeGraph = new DecodeGraph(m_decodeGraphs.size(), maxChartSpan);
      } else {
        decodeGraph = new DecodeGraph(m_decodeGraphs.size());
      }

      m_decodeGraphs.push_back(decodeGraph); // TODO max chart span
    }

    m_decodeGraphs[decodeGraphInd]->Add(decodeStep);
    prev = decodeStep;
    prevDecodeGraphInd = decodeGraphInd;
  }

  // set maximum n-gram size for backoff approach to decoding paths
  // default is always use subsequent paths (value = 0)
  // if specified, record maxmimum unseen n-gram size
  const vector<string> *backoffVector = m_parameter->GetParam("decoding-graph-backoff");
  for(size_t i=0; i<m_decodeGraphs.size() && backoffVector && i<backoffVector->size(); i++) {
    DecodeGraph &decodeGraph = *m_decodeGraphs[i];

    if (i < backoffVector->size()) {
      decodeGraph.SetBackoff(Scan<size_t>(backoffVector->at(i)));
    }
  }

}

void StaticData::ReLoadBleuScoreFeatureParameter(float weight)
{
  //loop over ScoreProducers to update weights of BleuScoreFeature
  const std::vector<FeatureFunction*> &producers = FeatureFunction::GetFeatureFunctions();
  for(size_t i=0; i<producers.size(); ++i) {
    FeatureFunction *ff = producers[i];
    std::string ffName = ff->GetScoreProducerDescription();

    if (ffName == "BleuScoreFeature") {
      SetWeight(ff, weight);
      break;
    }
  }
}

// ScoreComponentCollection StaticData::GetAllWeightsScoreComponentCollection() const {}
// in ScoreComponentCollection.h

void StaticData::SetExecPath(const std::string &path)
{
  // NOT TESTED
  size_t pos = path.rfind("/");
  if (pos !=  string::npos) {
    m_binPath = path.substr(0, pos);
  }
  VERBOSE(1,m_binPath << endl);
}

const string &StaticData::GetBinDirectory() const
{
  return m_binPath;
}

float StaticData::GetWeightWordPenalty() const
{
  float weightWP = GetWeight(&WordPenaltyProducer::Instance());
  return weightWP;
}

void
StaticData::
InitializeForInput(ttasksptr const& ttask) const
{
  const std::vector<FeatureFunction*> &producers
  = FeatureFunction::GetFeatureFunctions();
  for(size_t i=0; i<producers.size(); ++i) {
    FeatureFunction &ff = *producers[i];
    if (! IsFeatureFunctionIgnored(ff)) {
      Timer iTime;
      iTime.start();
      ff.InitializeForInput(ttask);
      VERBOSE(3,"InitializeForInput( " << ff.GetScoreProducerDescription()
              << " )" << "= " << iTime << endl);
    }
  }
}

void
StaticData::
CleanUpAfterSentenceProcessing(ttasksptr const& ttask) const
{
  const std::vector<FeatureFunction*> &producers
  = FeatureFunction::GetFeatureFunctions();
  for(size_t i=0; i<producers.size(); ++i) {
    FeatureFunction &ff = *producers[i];
    if (! IsFeatureFunctionIgnored(ff)) {
      ff.CleanUpAfterSentenceProcessing(ttask);
    }
  }
}

void StaticData::LoadFeatureFunctions()
{
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  std::vector<FeatureFunction*>::const_iterator iter;
  for (iter = ffs.begin(); iter != ffs.end(); ++iter) {
    FeatureFunction *ff = *iter;
    bool doLoad = true;

    if (ff->RequireSortingAfterSourceContext()) {
      m_requireSortingAfterSourceContext = true;
    }

    if (dynamic_cast<PhraseDictionary*>(ff)) {
      doLoad = false;
    }

    if (doLoad) {
      VERBOSE(1, "Loading " << ff->GetScoreProducerDescription() << endl);
      ff->Load(options());
    }
  }

  const std::vector<PhraseDictionary*> &pts = PhraseDictionary::GetColl();
  for (size_t i = 0; i < pts.size(); ++i) {
    PhraseDictionary *pt = pts[i];
    VERBOSE(1, "Loading " << pt->GetScoreProducerDescription() << endl);
    pt->Load(options());
  }

  CheckLEGACYPT();
}

bool StaticData::CheckWeights() const
{
  set<string> weightNames = m_parameter->GetWeightNames();
  set<string> featureNames;

  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  for (size_t i = 0; i < ffs.size(); ++i) {
    const FeatureFunction &ff = *ffs[i];
    const string &descr = ff.GetScoreProducerDescription();
    featureNames.insert(descr);

    set<string>::iterator iter = weightNames.find(descr);
    if (iter == weightNames.end()) {
      cerr << "Can't find weights for feature function " << descr << endl;
    } else {
      weightNames.erase(iter);
    }
  }

  //sparse features
  if (!weightNames.empty()) {
    set<string>::iterator iter;
    for (iter = weightNames.begin(); iter != weightNames.end(); ) {
      string fname = (*iter).substr(0, (*iter).find("_"));
      VERBOSE(1,fname << "\n");
      if (featureNames.find(fname) != featureNames.end()) {
        weightNames.erase(iter++);
      } else {
        ++iter;
      }
    }
  }

  if (!weightNames.empty()) {
    cerr << "The following weights have no feature function. "
         << "Maybe incorrectly spelt weights: ";
    set<string>::iterator iter;
    for (iter = weightNames.begin(); iter != weightNames.end(); ++iter) {
      cerr << *iter << ",";
    }
    return false;
  }

  return true;
}


void StaticData::LoadSparseWeightsFromConfig()
{
  set<string> featureNames;
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  for (size_t i = 0; i < ffs.size(); ++i) {
    const FeatureFunction &ff = *ffs[i];
    const string &descr = ff.GetScoreProducerDescription();
    featureNames.insert(descr);
  }

  const std::map<std::string, std::vector<float> > &weights = m_parameter->GetAllWeights();
  std::map<std::string, std::vector<float> >::const_iterator iter;
  for (iter = weights.begin(); iter != weights.end(); ++iter) {
    // this indicates that it is sparse feature
    if (featureNames.find(iter->first) == featureNames.end()) {
      UTIL_THROW_IF2(iter->second.size() != 1, "ERROR: only one weight per sparse feature allowed: " << iter->first);
      m_allWeights.Assign(iter->first, iter->second[0]);
    }
  }

}


/**! Read in settings for alternative weights */
bool StaticData::LoadAlternateWeightSettings()
{
  if (m_threadCount > 1) {
    cerr << "ERROR: alternative weight settings currently not supported with multi-threading.";
    return false;
  }

  vector<string> weightSpecification;
  const PARAM_VEC *params = m_parameter->GetParam("alternate-weight-setting");
  if (params && params->size()) {
    weightSpecification = *params;
  }

  // get mapping from feature names to feature functions
  map<string,FeatureFunction*> nameToFF;
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  for (size_t i = 0; i < ffs.size(); ++i) {
    nameToFF[ ffs[i]->GetScoreProducerDescription() ] = ffs[i];
  }

  // copy main weight setting as default
  m_weightSetting["default"] = new ScoreComponentCollection( m_allWeights );

  // go through specification in config file
  string currentId = "";
  bool hasErrors = false;
  for (size_t i=0; i<weightSpecification.size(); ++i) {

    // identifier line (with optional additional specifications)
    if (weightSpecification[i].find("id=") == 0) {
      vector<string> tokens = Tokenize(weightSpecification[i]);
      vector<string> args = Tokenize(tokens[0], "=");
      currentId = args[1];
      VERBOSE(1,"alternate weight setting " << currentId << endl);
      UTIL_THROW_IF2(m_weightSetting.find(currentId) != m_weightSetting.end(),
                     "Duplicate alternate weight id: " << currentId);
      m_weightSetting[ currentId ] = new ScoreComponentCollection;

      // other specifications
      for(size_t j=1; j<tokens.size(); j++) {
        vector<string> args = Tokenize(tokens[j], "=");
        // sparse weights
        if (args[0] == "weight-file") {
          if (args.size() != 2) {
            std::cerr << "One argument should be supplied for weight-file";
            return false;
          }
          ScoreComponentCollection extraWeights;
          if (!extraWeights.Load(args[1])) {
            std::cerr << "Unable to load weights from " << args[1];
            return false;
          }
          m_weightSetting[ currentId ]->PlusEquals(extraWeights);
        }
        // ignore feature functions
        else if (args[0] == "ignore-ff") {
          set< string > *ffNameSet = new set< string >;
          m_weightSettingIgnoreFF[ currentId ] = *ffNameSet;
          vector<string> featureFunctionName = Tokenize(args[1], ",");
          for(size_t k=0; k<featureFunctionName.size(); k++) {
            // check if a valid nane
            map<string,FeatureFunction*>::iterator ffLookUp = nameToFF.find(featureFunctionName[k]);
            if (ffLookUp == nameToFF.end()) {
              cerr << "ERROR: alternate weight setting " << currentId
                   << " specifies to ignore feature function " << featureFunctionName[k]
                   << " but there is no such feature function" << endl;
              hasErrors = true;
            } else {
              m_weightSettingIgnoreFF[ currentId ].insert( featureFunctionName[k] );
            }
          }
        }
      }
    }

    // weight lines
    else {
      UTIL_THROW_IF2(currentId.empty(), "No alternative weights specified");
      vector<string> tokens = Tokenize(weightSpecification[i]);
      UTIL_THROW_IF2(tokens.size() < 2
                     , "Incorrect format for alternate weights: " << weightSpecification[i]);

      // get name and weight values
      string name = tokens[0];
      name = name.substr(0, name.size() - 1); // remove trailing "="
      vector<float> weights(tokens.size() - 1);
      for (size_t i = 1; i < tokens.size(); ++i) {
        float weight = Scan<float>(tokens[i]);
        weights[i - 1] = weight;
      }

      // check if a valid nane
      map<string,FeatureFunction*>::iterator ffLookUp = nameToFF.find(name);
      if (ffLookUp == nameToFF.end()) {
        cerr << "ERROR: alternate weight setting " << currentId
             << " specifies weight(s) for " << name
             << " but there is no such feature function" << endl;
        hasErrors = true;
      } else {
        m_weightSetting[ currentId ]->Assign( nameToFF[name], weights);
      }
    }
  }
  UTIL_THROW_IF2(hasErrors, "Errors loading alternate weights");
  return true;
}

void StaticData::NoCache()
{
  bool noCache;
  m_parameter->SetParameter(noCache, "no-cache", false );

  if (noCache) {
    const std::vector<PhraseDictionary*> &pts = PhraseDictionary::GetColl();
    for (size_t i = 0; i < pts.size(); ++i) {
      PhraseDictionary &pt = *pts[i];
      pt.SetParameter("cache-size", "0");
    }
  }
}

std::map<std::string, std::string>
StaticData
::OverrideFeatureNames()
{
  std::map<std::string, std::string> ret;

  const PARAM_VEC *params = m_parameter->GetParam("feature-name-overwrite");
  if (params && params->size()) {
    UTIL_THROW_IF2(params->size() != 1, "Only provide 1 line in the section [feature-name-overwrite]");
    vector<string> toks = Tokenize(params->at(0));
    UTIL_THROW_IF2(toks.size() % 2 != 0, "Format of -feature-name-overwrite must be [old-name new-name]*");

    for (size_t i = 0; i < toks.size(); i += 2) {
      const string &oldName = toks[i];
      const string &newName = toks[i+1];
      ret[oldName] = newName;
    }
  }

  // FIXME Does this make sense for F2S?  Perhaps it should be changed once
  // FIXME the pipeline uses RuleTable consistently.
  SearchAlgorithm algo = m_options->search.algo;
  if (algo == SyntaxS2T || algo == SyntaxT2S ||
      algo == SyntaxT2S_SCFG || algo == SyntaxF2S) {
    // Automatically override PhraseDictionary{Memory,Scope3}.  This will
    // have to change if the FF parameters diverge too much in the future,
    // but for now it makes switching between the old and new decoders much
    // more convenient.
    ret["PhraseDictionaryMemory"] = "RuleTable";
    ret["PhraseDictionaryScope3"] = "RuleTable";
  }

  return ret;
}

void StaticData::OverrideFeatures()
{
  const PARAM_VEC *params = m_parameter->GetParam("feature-overwrite");
  for (size_t i = 0; params && i < params->size(); ++i) {
    const string &str = params->at(i);
    vector<string> toks = Tokenize(str);
    UTIL_THROW_IF2(toks.size() <= 1, "Incorrect format for feature override: " << str);

    FeatureFunction &ff = FeatureFunction::FindFeatureFunction(toks[0]);

    for (size_t j = 1; j < toks.size(); ++j) {
      const string &keyValStr = toks[j];
      vector<string> keyVal = Tokenize(keyValStr, "=");
      UTIL_THROW_IF2(keyVal.size() != 2, "Incorrect format for parameter override: " << keyValStr);

      VERBOSE(1, "Override " << ff.GetScoreProducerDescription() << " "
              << keyVal[0] << "=" << keyVal[1] << endl);

      ff.SetParameter(keyVal[0], keyVal[1]);

    }
  }

}

void StaticData::CheckLEGACYPT()
{
  const std::vector<PhraseDictionary*> &pts = PhraseDictionary::GetColl();
  for (size_t i = 0; i < pts.size(); ++i) {
    const PhraseDictionary *phraseDictionary = pts[i];
    if (dynamic_cast<const PhraseDictionaryTreeAdaptor*>(phraseDictionary) != NULL) {
      m_useLegacyPT = true;
      return;
    }
  }

  m_useLegacyPT = false;
}


void StaticData::ResetWeights(const std::string &denseWeights, const std::string &sparseFile)
{
  m_allWeights = ScoreComponentCollection();

  // dense weights
  string name("");
  vector<float> weights;
  vector<string> toks = Tokenize(denseWeights);
  for (size_t i = 0; i < toks.size(); ++i) {
    const string &tok = toks[i];

    if (ends_with(tok, "=")) {
      // start of new feature

      if (name != "") {
        // save previous ff
        const FeatureFunction &ff = FeatureFunction::FindFeatureFunction(name);
        m_allWeights.Assign(&ff, weights);
        weights.clear();
      }

      name = tok.substr(0, tok.size() - 1);
    } else {
      // a weight for curr ff
      float weight = Scan<float>(toks[i]);
      weights.push_back(weight);
    }
  }

  const FeatureFunction &ff = FeatureFunction::FindFeatureFunction(name);
  m_allWeights.Assign(&ff, weights);

  // sparse weights
  InputFileStream sparseStrme(sparseFile);
  string line;
  while (getline(sparseStrme, line)) {
    vector<string> toks = Tokenize(line);
    UTIL_THROW_IF2(toks.size() != 2, "Incorrect sparse weight format. Should be FFName_spareseName weight");

    vector<string> names = Tokenize(toks[0], "_");
    UTIL_THROW_IF2(names.size() != 2, "Incorrect sparse weight name. Should be FFName_spareseName");

    const FeatureFunction &ff = FeatureFunction::FindFeatureFunction(names[0]);
    m_allWeights.Assign(&ff, names[1], Scan<float>(toks[1]));
  }
}

size_t StaticData::GetCoordSpace(string space) const
{
  map<string const, size_t>::const_iterator m = m_coordSpaceMap.find(space);
  if(m == m_coordSpaceMap.end()) {
    return 0;
  }
  return m->second;
}

size_t StaticData::MapCoordSpace(string space)
{
  map<string const, size_t>::const_iterator m = m_coordSpaceMap.find(space);
  if (m != m_coordSpaceMap.end()) {
    return m->second;
  }
  size_t id = m_coordSpaceNextID;
  m_coordSpaceNextID += 1;
  m_coordSpaceMap[space] = id;
  return id;
}

} // namespace
