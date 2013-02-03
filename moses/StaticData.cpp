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
#include "util/check.hh"
#include "moses/TranslationModel/PhraseDictionaryMemory.h"
#include "DecodeStepTranslation.h"
#include "DecodeStepGeneration.h"
#include "GenerationDictionary.h"
#include "DummyScoreProducers.h"
#include "StaticData.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Timer.h"
#include "LexicalReordering.h"
#include "GlobalLexicalModel.h"
#include "GlobalLexicalModelUnlimited.h"
#include "SentenceStats.h"
#include "PhraseBoundaryFeature.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "SparsePhraseDictionaryFeature.h"
#include "PhrasePairFeature.h"
#include "PhraseLengthFeature.h"
#include "TargetWordInsertionFeature.h"
#include "SourceWordDeletionFeature.h"
#include "WordTranslationFeature.h"
#include "UserMessage.h"
#include "TranslationOption.h"
#include "TargetBigramFeature.h"
#include "TargetNgramFeature.h"
#include "DecodeGraph.h"
#include "InputFileStream.h"
#include "BleuScoreFeature.h"
#include "ScoreComponentCollection.h"
#include "LM/Ken.h"
#include "LM/IRST.h"

#ifdef HAVE_SYNLM
#include "SyntacticLanguageModel.h"
#endif

#ifdef WITH_THREADS
#include <boost/thread.hpp>
#endif

using namespace std;

namespace Moses
{
static size_t CalcMax(size_t x, const vector<size_t>& y)
{
  size_t max = x;
  for (vector<size_t>::const_iterator i=y.begin(); i != y.end(); ++i)
    if (*i > max) max = *i;
  return max;
}

static size_t CalcMax(size_t x, const vector<size_t>& y, const vector<size_t>& z)
{
  size_t max = x;
  for (vector<size_t>::const_iterator i=y.begin(); i != y.end(); ++i)
    if (*i > max) max = *i;
  for (vector<size_t>::const_iterator i=z.begin(); i != z.end(); ++i)
    if (*i > max) max = *i;
  return max;
}

int GetFeatureIndex(std::map<string, int> &map, const string &featureName)
{
  std::map<string, int>::iterator iter;
  iter = map.find(featureName);
  if (iter == map.end()) {
    map[featureName] = 0;
    return 0;
  }
  else {
    int &index = iter->second;
    ++index;
    return index;
  }
}

StaticData StaticData::s_instance;

StaticData::StaticData()
  :m_sourceStartPosMattersForRecombination(false)
  ,m_inputType(SentenceInput)
  ,m_detailedTranslationReportingFilePath()
  ,m_onlyDistinctNBest(false)
  ,m_factorDelimiter("|") // default delimiter between factors
  ,m_lmEnableOOVFeature(false)
  ,m_isAlwaysCreateDirectTranslationOption(false)
  ,m_needAlignmentInfo(false)
{
  m_maxFactorIdx[0] = 0;  // source side
  m_maxFactorIdx[1] = 0;  // target side

  m_xmlBrackets.first="<";
  m_xmlBrackets.second=">";

  // memory pools
  Phrase::InitializeMemPool();
}

bool StaticData::LoadDataStatic(Parameter *parameter, const std::string &execPath) {
  s_instance.SetExecPath(execPath);
  return s_instance.LoadData(parameter);
}

bool StaticData::LoadData(Parameter *parameter)
{
  ResetUserTime();
  m_parameter = parameter;

  // verbose level
  m_verboseLevel = 1;
  if (m_parameter->GetParam("verbose").size() == 1) {
    m_verboseLevel = Scan<size_t>( m_parameter->GetParam("verbose")[0]);
  }

  m_parsingAlgorithm = (m_parameter->GetParam("parsing-algorithm").size() > 0) ?
                      (ParsingAlgorithm) Scan<size_t>(m_parameter->GetParam("parsing-algorithm")[0]) : ParseCYKPlus;

  // to cube or not to cube
  m_searchAlgorithm = (m_parameter->GetParam("search-algorithm").size() > 0) ?
                      (SearchAlgorithm) Scan<size_t>(m_parameter->GetParam("search-algorithm")[0]) : Normal;

  if (IsChart())
    LoadChartDecodingParameters();
  else
    LoadPhraseBasedParameters();

  // input type has to be specified BEFORE loading the phrase tables!
  if(m_parameter->GetParam("inputtype").size())
    m_inputType= (InputTypeEnum) Scan<int>(m_parameter->GetParam("inputtype")[0]);
  std::string s_it = "text input";
  if (m_inputType == 1) {
    s_it = "confusion net";
  }
  if (m_inputType == 2) {
    s_it = "word lattice";
  }
  VERBOSE(2,"input type is: "<<s_it<<"\n");

  if(m_parameter->GetParam("recover-input-path").size()) {
    m_recoverPath = Scan<bool>(m_parameter->GetParam("recover-input-path")[0]);
    if (m_recoverPath && m_inputType == SentenceInput) {
      TRACE_ERR("--recover-input-path should only be used with confusion net or word lattice input!\n");
      m_recoverPath = false;
    }
  }

  if(m_parameter->GetParam("sort-word-alignment").size()) {
    m_wordAlignmentSort = (WordAlignmentSort) Scan<size_t>(m_parameter->GetParam("sort-word-alignment")[0]);
  }
  
  // factor delimiter
  if (m_parameter->GetParam("factor-delimiter").size() > 0) {
    m_factorDelimiter = m_parameter->GetParam("factor-delimiter")[0];
  }

  SetBooleanParameter( &m_continuePartialTranslation, "continue-partial-translation", false );
  SetBooleanParameter( &m_outputHypoScore, "output-hypo-score", false );

  //word-to-word alignment
  SetBooleanParameter( &m_PrintAlignmentInfoNbest, "print-alignment-info-in-n-best", false );
  if (m_PrintAlignmentInfoNbest) {
    m_needAlignmentInfo = true;
  }

  if (m_parameter->GetParam("alignment-output-file").size() > 0) {
    m_alignmentOutputFile = Scan<std::string>(m_parameter->GetParam("alignment-output-file")[0]);
    m_needAlignmentInfo = true;
  }

  // n-best
  if (m_parameter->GetParam("n-best-list").size() >= 2) {
    m_nBestFilePath = m_parameter->GetParam("n-best-list")[0];
    m_nBestSize = Scan<size_t>( m_parameter->GetParam("n-best-list")[1] );
    m_onlyDistinctNBest=(m_parameter->GetParam("n-best-list").size()>2 && m_parameter->GetParam("n-best-list")[2]=="distinct");
  } else if (m_parameter->GetParam("n-best-list").size() == 1) {
    UserMessage::Add(string("wrong format for switch -n-best-list file size"));
    return false;
  } else {
    m_nBestSize = 0;
  }
  if (m_parameter->GetParam("n-best-factor").size() > 0) {
    m_nBestFactor = Scan<size_t>( m_parameter->GetParam("n-best-factor")[0]);
  } else {
    m_nBestFactor = 20;
  }
  
  // explicit setting of distinct nbest
  SetBooleanParameter( &m_onlyDistinctNBest, "distinct-nbest", false);

  //lattice samples
  if (m_parameter->GetParam("lattice-samples").size() ==2 ) {
    m_latticeSamplesFilePath = m_parameter->GetParam("lattice-samples")[0];
    m_latticeSamplesSize = Scan<size_t>(m_parameter->GetParam("lattice-samples")[1]);
  } else if (m_parameter->GetParam("lattice-samples").size() != 0 ) {
    UserMessage::Add(string("wrong format for switch -lattice-samples file size"));
    return false;
  } else {
    m_latticeSamplesSize = 0;
  }

  // word graph
  if (m_parameter->GetParam("output-word-graph").size() == 2)
    m_outputWordGraph = true;
  else
    m_outputWordGraph = false;

  // search graph
  if (m_parameter->GetParam("output-search-graph").size() > 0) {
    if (m_parameter->GetParam("output-search-graph").size() != 1) {
      UserMessage::Add(string("ERROR: wrong format for switch -output-search-graph file"));
      return false;
    }
    m_outputSearchGraph = true;
  }
  // ... in extended format
  else if (m_parameter->GetParam("output-search-graph-extended").size() > 0) {
    if (m_parameter->GetParam("output-search-graph-extended").size() != 1) {
      UserMessage::Add(string("ERROR: wrong format for switch -output-search-graph-extended file"));
      return false;
    }
    m_outputSearchGraph = true;
    m_outputSearchGraphExtended = true;
  } else
    m_outputSearchGraph = false;
#ifdef HAVE_PROTOBUF
  if (m_parameter->GetParam("output-search-graph-pb").size() > 0) {
    if (m_parameter->GetParam("output-search-graph-pb").size() != 1) {
      UserMessage::Add(string("ERROR: wrong format for switch -output-search-graph-pb path"));
      return false;
    }
    m_outputSearchGraphPB = true;
  } else
    m_outputSearchGraphPB = false;
#endif
  SetBooleanParameter( &m_unprunedSearchGraph, "unpruned-search-graph", false );
  SetBooleanParameter( &m_includeLHSInSearchGraph, "include-lhs-in-search-graph", false );
  
  if (m_parameter->isParamSpecified("output-unknowns")) {

    if (m_parameter->GetParam("output-unknowns").size() == 1) {
      m_outputUnknownsFile =Scan<string>(m_parameter->GetParam("output-unknowns")[0]); 
    } else {
      UserMessage::Add(string("need to specify exactly one file name for unknowns"));
      return false;
    }
  }

  // include feature names in the n-best list
  SetBooleanParameter( &m_labeledNBestList, "labeled-n-best-list", true );

  // include word alignment in the n-best list
  SetBooleanParameter( &m_nBestIncludesSegmentation, "include-segmentation-in-n-best", false );

  // printing source phrase spans
  SetBooleanParameter( &m_reportSegmentation, "report-segmentation", false );

  // print all factors of output translations
  SetBooleanParameter( &m_reportAllFactors, "report-all-factors", false );

  // print all factors of output translations
  SetBooleanParameter( &m_reportAllFactorsNBest, "report-all-factors-in-n-best", false );

  // caching of translation options
  if (m_inputType == SentenceInput) {
    SetBooleanParameter( &m_useTransOptCache, "use-persistent-cache", true );
    m_transOptCacheMaxSize = (m_parameter->GetParam("persistent-cache-size").size() > 0)
                             ? Scan<size_t>(m_parameter->GetParam("persistent-cache-size")[0]) : DEFAULT_MAX_TRANS_OPT_CACHE_SIZE;
  } else {
    m_useTransOptCache = false;
  }

  //input factors
  const vector<string> &inputFactorVector = m_parameter->GetParam("input-factors");
  for(size_t i=0; i<inputFactorVector.size(); i++) {
    m_inputFactorOrder.push_back(Scan<FactorType>(inputFactorVector[i]));
  }
  if(m_inputFactorOrder.empty()) {
    UserMessage::Add(string("no input factor specified in config file"));
    return false;
  }

  //output factors
  const vector<string> &outputFactorVector = m_parameter->GetParam("output-factors");
  for(size_t i=0; i<outputFactorVector.size(); i++) {
    m_outputFactorOrder.push_back(Scan<FactorType>(outputFactorVector[i]));
  }
  if(m_outputFactorOrder.empty()) {
    // default. output factor 0
    m_outputFactorOrder.push_back(0);
  }

  //source word deletion
  SetBooleanParameter( &m_wordDeletionEnabled, "phrase-drop-allowed", false );

  //Disable discarding
  SetBooleanParameter(&m_disableDiscarding, "disable-discarding", false);

  //Print All Derivations
  SetBooleanParameter( &m_printAllDerivations , "print-all-derivations", false );

  // additional output
  if (m_parameter->isParamSpecified("translation-details")) {
    const vector<string> &args = m_parameter->GetParam("translation-details");
    if (args.size() == 1) {
      m_detailedTranslationReportingFilePath = args[0];
    } else {
      UserMessage::Add(string("the translation-details option requires exactly one filename argument"));
      return false;
    }
  }

  // word penalties
  CHECK(m_parameter->GetWeights("WordPenalty", 0).size() == 1);
  float weightWordPenalty       = m_parameter->GetWeights("WordPenalty", 0)[0];
  m_wpProducer = new WordPenaltyProducer("WordPenalty");

  SetWeight(m_wpProducer, weightWordPenalty);

  const vector<float> &weightsUnknownWord				= m_parameter->GetWeights("UnknownWordPenalty", 0);
  float weightUnknownWord = weightsUnknownWord.size() ? weightsUnknownWord[0] : 1.0;

  m_unknownWordPenaltyProducer = new UnknownWordPenaltyProducer("UnknownWordPenaltyProducer");

SetWeight(m_unknownWordPenaltyProducer, weightUnknownWord);

  // reordering constraints
  m_maxDistortion = (m_parameter->GetParam("distortion-limit").size() > 0) ?
                    Scan<int>(m_parameter->GetParam("distortion-limit")[0])
                    : -1;
  SetBooleanParameter( &m_reorderingConstraint, "monotone-at-punctuation", false );

  // settings for pruning
  m_maxHypoStackSize = (m_parameter->GetParam("stack").size() > 0)
                       ? Scan<size_t>(m_parameter->GetParam("stack")[0]) : DEFAULT_MAX_HYPOSTACK_SIZE;

  m_minHypoStackDiversity = 0;
  if (m_parameter->GetParam("stack-diversity").size() > 0) {
    if (m_maxDistortion > 15) {
      UserMessage::Add("stack diversity > 0 is not allowed for distortion limits larger than 15");
      return false;
    }
    if (m_inputType == WordLatticeInput) {
      UserMessage::Add("stack diversity > 0 is not allowed for lattice input");
      return false;
    }
    m_minHypoStackDiversity = Scan<size_t>(m_parameter->GetParam("stack-diversity")[0]);
  }

  m_beamWidth = (m_parameter->GetParam("beam-threshold").size() > 0) ?
                TransformScore(Scan<float>(m_parameter->GetParam("beam-threshold")[0]))
                : TransformScore(DEFAULT_BEAM_WIDTH);
  m_earlyDiscardingThreshold = (m_parameter->GetParam("early-discarding-threshold").size() > 0) ?
                               TransformScore(Scan<float>(m_parameter->GetParam("early-discarding-threshold")[0]))
                               : TransformScore(DEFAULT_EARLY_DISCARDING_THRESHOLD);
  m_translationOptionThreshold = (m_parameter->GetParam("translation-option-threshold").size() > 0) ?
                                 TransformScore(Scan<float>(m_parameter->GetParam("translation-option-threshold")[0]))
                                 : TransformScore(DEFAULT_TRANSLATION_OPTION_THRESHOLD);

  m_maxNoTransOptPerCoverage = (m_parameter->GetParam("max-trans-opt-per-coverage").size() > 0)
                               ? Scan<size_t>(m_parameter->GetParam("max-trans-opt-per-coverage")[0]) : DEFAULT_MAX_TRANS_OPT_SIZE;

  m_maxNoPartTransOpt = (m_parameter->GetParam("max-partial-trans-opt").size() > 0)
                        ? Scan<size_t>(m_parameter->GetParam("max-partial-trans-opt")[0]) : DEFAULT_MAX_PART_TRANS_OPT_SIZE;

  m_maxPhraseLength = (m_parameter->GetParam("max-phrase-length").size() > 0)
                      ? Scan<size_t>(m_parameter->GetParam("max-phrase-length")[0]) : DEFAULT_MAX_PHRASE_LENGTH;

  m_cubePruningPopLimit = (m_parameter->GetParam("cube-pruning-pop-limit").size() > 0)
                          ? Scan<size_t>(m_parameter->GetParam("cube-pruning-pop-limit")[0]) : DEFAULT_CUBE_PRUNING_POP_LIMIT;

  m_cubePruningDiversity = (m_parameter->GetParam("cube-pruning-diversity").size() > 0)
                           ? Scan<size_t>(m_parameter->GetParam("cube-pruning-diversity")[0]) : DEFAULT_CUBE_PRUNING_DIVERSITY;

  SetBooleanParameter(&m_cubePruningLazyScoring, "cube-pruning-lazy-scoring", false);

  // early distortion cost
  SetBooleanParameter( &m_useEarlyDistortionCost, "early-distortion-cost", false );

  // unknown word processing
  SetBooleanParameter( &m_dropUnknown, "drop-unknown", false );

  SetBooleanParameter( &m_lmEnableOOVFeature, "lmodel-oov-feature", false);

  // minimum Bayes risk decoding
  SetBooleanParameter( &m_mbr, "minimum-bayes-risk", false );
  m_mbrSize = (m_parameter->GetParam("mbr-size").size() > 0) ?
              Scan<size_t>(m_parameter->GetParam("mbr-size")[0]) : 200;
  m_mbrScale = (m_parameter->GetParam("mbr-scale").size() > 0) ?
               Scan<float>(m_parameter->GetParam("mbr-scale")[0]) : 1.0f;

  //lattice mbr
  SetBooleanParameter( &m_useLatticeMBR, "lminimum-bayes-risk", false );
  if (m_useLatticeMBR && m_mbr) {
    cerr << "Errror: Cannot use both n-best mbr and lattice mbr together" << endl;
    exit(1);
  }
  
  //mira training
  SetBooleanParameter( &m_mira, "mira", false );

  if (m_useLatticeMBR) m_mbr = true;

  m_lmbrPruning = (m_parameter->GetParam("lmbr-pruning-factor").size() > 0) ?
                  Scan<size_t>(m_parameter->GetParam("lmbr-pruning-factor")[0]) : 30;
  m_lmbrThetas = Scan<float>(m_parameter->GetParam("lmbr-thetas"));
  SetBooleanParameter( &m_useLatticeHypSetForLatticeMBR, "lattice-hypo-set", false );
  m_lmbrPrecision = (m_parameter->GetParam("lmbr-p").size() > 0) ?
                    Scan<float>(m_parameter->GetParam("lmbr-p")[0]) : 0.8f;
  m_lmbrPRatio = (m_parameter->GetParam("lmbr-r").size() > 0) ?
                 Scan<float>(m_parameter->GetParam("lmbr-r")[0]) : 0.6f;
  m_lmbrMapWeight = (m_parameter->GetParam("lmbr-map-weight").size() >0) ?
                    Scan<float>(m_parameter->GetParam("lmbr-map-weight")[0]) : 0.0f;

  //consensus decoding
  SetBooleanParameter( &m_useConsensusDecoding, "consensus-decoding", false );
  if (m_useConsensusDecoding && m_mbr) {
    cerr<< "Error: Cannot use consensus decoding together with mbr" << endl;
    exit(1);
  }
  if (m_useConsensusDecoding) m_mbr=true;
  
  // Compact phrase table and reordering model
  SetBooleanParameter( &m_minphrMemory, "minphr-memory", false );
  SetBooleanParameter( &m_minlexrMemory, "minlexr-memory", false );

  m_timeout_threshold = (m_parameter->GetParam("time-out").size() > 0) ?
                        Scan<size_t>(m_parameter->GetParam("time-out")[0]) : -1;
  m_timeout = (GetTimeoutThreshold() == (size_t)-1) ? false : true;


  m_lmcache_cleanup_threshold = (m_parameter->GetParam("clean-lm-cache").size() > 0) ?
                                Scan<size_t>(m_parameter->GetParam("clean-lm-cache")[0]) : 1;

  m_threadCount = 1;
  const std::vector<std::string> &threadInfo = m_parameter->GetParam("threads");
  if (!threadInfo.empty()) {
    if (threadInfo[0] == "all") {
#ifdef WITH_THREADS
      m_threadCount = boost::thread::hardware_concurrency();
      if (!m_threadCount) {
        UserMessage::Add("-threads all specified but Boost doesn't know how many cores there are");
        return false;
      }
#else
      UserMessage::Add("-threads all specified but moses not built with thread support");
      return false;
#endif
    } else {
      m_threadCount = Scan<int>(threadInfo[0]);
      if (m_threadCount < 1) {
        UserMessage::Add("Specify at least one thread.");
        return false;
      }
#ifndef WITH_THREADS
      if (m_threadCount > 1) {
        UserMessage::Add(std::string("Error: Thread count of ") + threadInfo[0] + " but moses not built with thread support");
        return false;
      }
#endif
    }
  }

  m_startTranslationId = (m_parameter->GetParam("start-translation-id").size() > 0) ?
          Scan<long>(m_parameter->GetParam("start-translation-id")[0]) : 0;

  // Read in constraint decoding file, if provided
  if(m_parameter->GetParam("constraint").size()) {
    if (m_parameter->GetParam("search-algorithm").size() > 0
        && Scan<size_t>(m_parameter->GetParam("search-algorithm")[0]) != 0) {
      cerr << "Can use -constraint only with stack-based search (-search-algorithm 0)" << endl;
      exit(1);
    }
    m_constraintFileName = m_parameter->GetParam("constraint")[0];

    InputFileStream constraintFile(m_constraintFileName);

    std::string line;
    
    long sentenceID = GetStartTranslationId() - 1;
    while (getline(constraintFile, line)) {
      vector<string> vecStr = Tokenize(line, "\t");

      if (vecStr.size() == 1) {
        sentenceID++;
        Phrase phrase(0);
        phrase.CreateFromString(GetOutputFactorOrder(), vecStr[0], GetFactorDelimiter());
        m_constraints.insert(make_pair(sentenceID,phrase));
      } else if (vecStr.size() == 2) {
        sentenceID = Scan<long>(vecStr[0]);
        Phrase phrase(0);
        phrase.CreateFromString(GetOutputFactorOrder(), vecStr[1], GetFactorDelimiter());
        m_constraints.insert(make_pair(sentenceID,phrase));
      } else {
        CHECK(false);
      }
    }
  }

  // use of xml in input
  if (m_parameter->GetParam("xml-input").size() == 0) m_xmlInputType = XmlPassThrough;
  else if (m_parameter->GetParam("xml-input")[0]=="exclusive") m_xmlInputType = XmlExclusive;
  else if (m_parameter->GetParam("xml-input")[0]=="inclusive") m_xmlInputType = XmlInclusive;
  else if (m_parameter->GetParam("xml-input")[0]=="ignore") m_xmlInputType = XmlIgnore;
  else if (m_parameter->GetParam("xml-input")[0]=="pass-through") m_xmlInputType = XmlPassThrough;
  else {
    UserMessage::Add("invalid xml-input value, must be pass-through, exclusive, inclusive, or ignore");
    return false;
  }

  // specify XML tags opening and closing brackets for XML option
  if (m_parameter->GetParam("xml-brackets").size() > 0) {
     std::vector<std::string> brackets = Tokenize(m_parameter->GetParam("xml-brackets")[0]);
     if(brackets.size()!=2) {
          cerr << "invalid xml-brackets value, must specify exactly 2 blank-delimited strings for XML tags opening and closing brackets" << endl;
          exit(1);
     }
     m_xmlBrackets.first= brackets[0];
     m_xmlBrackets.second=brackets[1];
     cerr << "XML tags opening and closing brackets for XML input are: " << m_xmlBrackets.first << " and " << m_xmlBrackets.second << endl;
  }

  // all features
  map<string, int> featureIndexMap;

  const vector<string> &features = m_parameter->GetParam("feature");
  for (size_t i = 0; i < features.size(); ++i) {
    const string &line = features[i];
    vector<string> toks = Tokenize(line);

    const string &feature = toks[0];
    int featureIndex = GetFeatureIndex(featureIndexMap, feature);

    if (feature == "GlobalLexicalModel") {
      GlobalLexicalModel *model = new GlobalLexicalModel(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      SetWeights(model, weights);
    }
    else if (feature == "glm") {
      GlobalLexicalModelUnlimited *model = NULL; //new GlobalLexicalModelUnlimited(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      SetWeights(model, weights);
    }
    else if (feature == "swd") {
      SourceWordDeletionFeature *model = new SourceWordDeletionFeature(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      //SetWeights(model, weights);
    }
    else if (feature == "twi") {
      TargetWordInsertionFeature *model = new TargetWordInsertionFeature(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      //SetWeights(model, weights);
    }
    else if (feature == "PhraseBoundaryFeature") {
      PhraseBoundaryFeature *model = new PhraseBoundaryFeature(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      //SetWeights(model, weights);
    }
    else if (feature == "pl") {
      PhraseLengthFeature *model = new PhraseLengthFeature(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      //SetWeights(model, weights);
    }
    else if (feature == "WordTranslationFeature") {
      WordTranslationFeature *model = new WordTranslationFeature(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      //SetWeights(model, weights);
    }
    else if (feature == "TargetBigramFeature") {
    	TargetBigramFeature *model = new TargetBigramFeature(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      //SetWeights(model, weights);
    }
    else if (feature == "TargetNgramFeature") {
    	TargetNgramFeature *model = new TargetNgramFeature(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      //SetWeights(model, weights);
    }
    else if (feature == "PhrasePairFeature") {
      PhrasePairFeature *model = new PhrasePairFeature(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      //SetWeights(model, weights);
    }
    else if (feature == "LexicalReordering") {
      LexicalReordering *model = new LexicalReordering(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      SetWeights(model, weights);
    }
    else if (feature == "KENLM") {
      LanguageModel *model = ConstructKenLM(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      SetWeights(model, weights);
    }
    else if (feature == "IRSTLM") {
      LanguageModelIRST *irstlm = new LanguageModelIRST(line);
      LanguageModel *model = new LMRefCount(irstlm, line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      SetWeights(model, weights);
    }
    else if (feature == "Generation") {
      GenerationDictionary *model = new GenerationDictionary(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      SetWeights(model, weights);
    }
    else if (feature == "BleuScoreFeature") {
      BleuScoreFeature *model = new BleuScoreFeature(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      SetWeights(model, weights);
    }
    else if (feature == "SparsePhraseDictionaryFeature") {
      SparsePhraseDictionaryFeature *model = new SparsePhraseDictionaryFeature(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      //SetWeights(model, weights);
      m_sparsePhraseDictionary.push_back(model);
    }

#ifdef HAVE_SYNLM
    else if (feature == "SyntacticLanguageModel") {
      SyntacticLanguageModel *model = new SyntacticLanguageModel(line);
      const vector<float> &weights = m_parameter->GetWeights(feature, featureIndex);
      SetWeights(model, weights);
    }
#endif
    else {
      UserMessage::Add("Unknown feature function");
      return false;
    }

  }

  CollectFeatureFunctions();

  if (!LoadPhraseTables()) return false;
  if (!LoadDecodeGraphs()) return false;

  // report individual sparse features in n-best list
  if (m_parameter->GetParam("report-sparse-features").size() > 0) {
    for(size_t i=0; i<m_parameter->GetParam("report-sparse-features").size(); i++) {
      const std::string &name = m_parameter->GetParam("report-sparse-features")[i];
      for (size_t j = 0; j < m_sparsePhraseDictionary.size(); ++j) {
        if (m_sparsePhraseDictionary[j] && name.compare(m_sparsePhraseDictionary[j]->GetScoreProducerDescription()) == 0) {
          m_sparsePhraseDictionary[j]->SetSparseFeatureReporting();
        }
      }
    } // for(size_t i=0; i<m_parameter->GetParam("report-sparse-features").
  }

  //Instigate dictionary loading
  ConfigDictionaries();

  for (int i = 0; i < m_phraseDictionary.size(); i++)
    cerr << m_phraseDictionary[i] << " ";
  cerr << endl;
  for (int i = 0; i < m_generationDictionary.size(); i++)
      cerr << m_generationDictionary[i] << " ";
    cerr << endl;

  //Add any other features here.

  //Load extra feature weights
  //NB: These are common to all translation systems (at the moment!)
  vector<string> extraWeightConfig = m_parameter->GetParam("weight-file");
  if (extraWeightConfig.size()) {
    if (extraWeightConfig.size() != 1) {
      UserMessage::Add("One argument should be supplied for weight-file");
      return false;
    }
    ScoreComponentCollection extraWeights;
    if (!extraWeights.Load(extraWeightConfig[0])) {
      UserMessage::Add("Unable to load weights from " + extraWeightConfig[0]);
      return false;
    }

    m_allWeights.PlusEquals(extraWeights);
  }

  return true;
}

void StaticData::SetBooleanParameter( bool *parameter, string parameterName, bool defaultValue )
{
  // default value if nothing is specified
  *parameter = defaultValue;
  if (! m_parameter->isParamSpecified( parameterName ) ) {
    return;
  }

  // if parameter is just specified as, e.g. "-parameter" set it true
  if (m_parameter->GetParam( parameterName ).size() == 0) {
    *parameter = true;
  }

  // if paramter is specified "-parameter true" or "-parameter false"
  else if (m_parameter->GetParam( parameterName ).size() == 1) {
    *parameter = Scan<bool>( m_parameter->GetParam( parameterName )[0]);
  }
}

void StaticData::SetWeight(const ScoreProducer* sp, float weight)
{
  m_allWeights.Resize();
  m_allWeights.Assign(sp,weight);
}

void StaticData::SetWeights(const ScoreProducer* sp, const std::vector<float>& weights)
{
  m_allWeights.Resize();
  m_allWeights.Assign(sp,weights);
}

StaticData::~StaticData()
{
  /*
  const std::vector<ScoreProducer*> &producers = FeatureFunction::GetFeatureFunctions();
  for(size_t i=0;i<producers.size();++i) {
    ScoreProducer *ff = producers[i];
    cerr << endl << "Destroying" << ff << endl;
    delete ff;
  }
  */

  // memory pools
  Phrase::FinalizeMemPool();
}

/* Doesn't load phrase tables any more. Just creates the features. */
bool StaticData::LoadPhraseTables()
{
  VERBOSE(2,"Creating phrase table features" << endl);

  // language models must be loaded prior to loading phrase tables

  // load phrase translation tables
  if (m_parameter->GetParam("ttable-file").size() > 0) {
    // weights
    const vector<string> &translationVector = m_parameter->GetParam("ttable-file");
    vector<size_t>	maxTargetPhrase					= Scan<size_t>(m_parameter->GetParam("ttable-limit"));

    if(maxTargetPhrase.size() == 1 && translationVector.size() > 1) {
      VERBOSE(1, "Using uniform ttable-limit of " << maxTargetPhrase[0] << " for all translation tables." << endl);
      for(size_t i = 1; i < translationVector.size(); i++)
        maxTargetPhrase.push_back(maxTargetPhrase[0]);
    } else if(maxTargetPhrase.size() != 1 && maxTargetPhrase.size() < translationVector.size()) {
      stringstream strme;
      strme << "You specified " << translationVector.size() << " translation tables, but only " << maxTargetPhrase.size() << " ttable-limits.";
      UserMessage::Add(strme.str());
      return false;
    }

    // MAIN LOOP
    for(size_t currDict = 0 ; currDict < translationVector.size(); currDict++) {
      stringstream ptLine("PhraseModel ");

      vector<string>                  token           = Tokenize(translationVector[currDict]);
      const vector<float> &weights  = m_parameter->GetWeights("PhraseModel", currDict);

      if(currDict == 0 && token.size() == 4) {
        UserMessage::Add("Phrase table specification in old 4-field format. No longer supported");
        return false;
      }
      CHECK(token.size() >= 5);

      PhraseTableImplementation implementation = (PhraseTableImplementation) Scan<int>(token[0]);
      ptLine << "implementation=" << implementation << " ";
      ptLine << "input-factor=" << token[1] << " ";
      ptLine << "output-factor=" << token[2] << " ";
      ptLine << "path=" << token[4] << " ";

      //characteristics of the phrase table

      vector<FactorType>  input		= Tokenize<FactorType>(token[1], ",")
                                    ,output = Tokenize<FactorType>(token[2], ",");
      m_maxFactorIdx[0] = CalcMax(m_maxFactorIdx[0], input);
      m_maxFactorIdx[1] = CalcMax(m_maxFactorIdx[1], output);
      m_maxNumFactors = std::max(m_maxFactorIdx[0], m_maxFactorIdx[1]) + 1;
      size_t numScoreComponent = Scan<size_t>(token[3]);
      string filePath= token[4];

      CHECK(weights.size() >= numScoreComponent);

      if(m_inputType == ConfusionNetworkInput || m_inputType == WordLatticeInput) {
        if (currDict==0) { // only the 1st pt. THis is shit
          // TODO. find what the assumptions made by confusion network about phrase table output which makes
          // it only work with binary file. This is a hack
          CHECK(implementation == Binary);

          if (m_parameter->GetParam("input-scores").size()) {
            m_numInputScores = Scan<size_t>(m_parameter->GetParam("input-scores")[0]);
          }
          else {
            m_numInputScores = 1;
          }
          numScoreComponent += m_numInputScores;

          if (m_parameter->GetParam("input-scores").size() > 1) {
            m_numRealWordsInInput = Scan<size_t>(m_parameter->GetParam("input-scores")[1]);
          }
          else {
            m_numRealWordsInInput = 0;
          }
          numScoreComponent += m_numRealWordsInInput;
        }
      }
      else { // not confusion network or lattice input
        m_numInputScores = 0;
        m_numRealWordsInInput = 0;
      }

      ptLine << "num-features=" << numScoreComponent << " ";
      ptLine << "num-input-features=" << (currDict==0 ? m_numInputScores + m_numRealWordsInInput : 0) << " ";
      ptLine << "table-limit=" << maxTargetPhrase[currDict] << " ";

      string targetPath, alignmentsFile;
      if (implementation == SuffixArray) {
        targetPath		= token[5];
        alignmentsFile= token[6];

        ptLine << "target-path=" << targetPath << " ";
        ptLine << "alignment-path=" << alignmentsFile << " ";
      }

      //This is needed for regression testing, but the phrase table
      //might not really be loading here
      IFVERBOSE(1)
      PrintUserTime(string("Start loading PhraseTable ") + filePath);
      VERBOSE(1,"filePath: " << filePath <<endl);

      PhraseDictionaryFeature* pdf = new PhraseDictionaryFeature(ptLine.str());
      /*
      PhraseDictionaryFeature* pdf = new PhraseDictionaryFeature(
        implementation
        , numScoreComponent
        , (currDict==0 ? m_numInputScores + m_numRealWordsInInput : 0)
        , input
        , output
        , filePath
        , maxTargetPhrase[currDict]
        , targetPath, alignmentsFile);
      */

      //optional create sparse phrase feature
      if (m_sparsePhraseDictionary.size() > currDict) {
        SparsePhraseDictionaryFeature* spdf = m_sparsePhraseDictionary[currDict];
        pdf->SetSparsePhraseDictionaryFeature(spdf);
      }

      m_phraseDictionary.push_back(pdf);

      SetWeights(m_phraseDictionary.back(),weights);

    }
  }

  IFVERBOSE(1)
  PrintUserTime("Finished loading phrase tables");
  return true;
}

void StaticData::LoadNonTerminals()
{
  string defaultNonTerminals;

  if (m_parameter->GetParam("non-terminals").size() == 0) {
    defaultNonTerminals = "X";
  } else {
    vector<std::string> tokens = Tokenize(m_parameter->GetParam("non-terminals")[0]);
    defaultNonTerminals = tokens[0];
  }

  FactorCollection &factorCollection = FactorCollection::Instance();

  m_inputDefaultNonTerminal.SetIsNonTerminal(true);
  const Factor *sourceFactor = factorCollection.AddFactor(Input, 0, defaultNonTerminals);
  m_inputDefaultNonTerminal.SetFactor(0, sourceFactor);

  m_outputDefaultNonTerminal.SetIsNonTerminal(true);
  const Factor *targetFactor = factorCollection.AddFactor(Output, 0, defaultNonTerminals);
  m_outputDefaultNonTerminal.SetFactor(0, targetFactor);

  // for unknwon words
  if (m_parameter->GetParam("unknown-lhs").size() == 0) {
    UnknownLHSEntry entry(defaultNonTerminals, 0.0f);
    m_unknownLHS.push_back(entry);
  } else {
    const string &filePath = m_parameter->GetParam("unknown-lhs")[0];

    InputFileStream inStream(filePath);
    string line;
    while(getline(inStream, line)) {
      vector<string> tokens = Tokenize(line);
      CHECK(tokens.size() == 2);
      UnknownLHSEntry entry(tokens[0], Scan<float>(tokens[1]));
      m_unknownLHS.push_back(entry);
    }

  }

}

void StaticData::LoadChartDecodingParameters()
{
  LoadNonTerminals();

  // source label overlap
  if (m_parameter->GetParam("source-label-overlap").size() > 0) {
    m_sourceLabelOverlap = (SourceLabelOverlap) Scan<int>(m_parameter->GetParam("source-label-overlap")[0]);
  } else {
    m_sourceLabelOverlap = SourceLabelOverlapAdd;
  }

  m_ruleLimit = (m_parameter->GetParam("rule-limit").size() > 0)
                ? Scan<size_t>(m_parameter->GetParam("rule-limit")[0]) : DEFAULT_MAX_TRANS_OPT_SIZE;
}

void StaticData::LoadPhraseBasedParameters()
{
  const vector<float> &distortionWeights = m_parameter->GetWeights("Distortion", 0);
  CHECK(distortionWeights.size() == 1);

  float weightDistortion = distortionWeights[0];
  m_distortionScoreProducer = new DistortionScoreProducer("DistortionScoreProducer ");

  SetWeight(m_distortionScoreProducer, weightDistortion);

}

bool StaticData::LoadDecodeGraphs()
{
  const vector<string> &mappingVector = m_parameter->GetParam("mapping");
  const vector<size_t> &maxChartSpans = Scan<size_t>(m_parameter->GetParam("max-chart-span"));

  DecodeStep *prev = 0;
  size_t prevDecodeGraphInd = 0;
  for(size_t i=0; i<mappingVector.size(); i++) {
    vector<string>	token		= Tokenize(mappingVector[i]);
    size_t decodeGraphInd;
    DecodeType decodeType;
    size_t index;
    if (token.size() == 2) {
      decodeGraphInd = 0;
      decodeType = token[0] == "T" ? Translate : Generate;
      index = Scan<size_t>(token[1]);
    } else if (token.size() == 3) {
      // For specifying multiple translation model
      decodeGraphInd = Scan<size_t>(token[0]);
      //the vectorList index can only increment by one
      CHECK(decodeGraphInd == prevDecodeGraphInd || decodeGraphInd == prevDecodeGraphInd + 1);
      if (decodeGraphInd > prevDecodeGraphInd) {
        prev = NULL;
      }
      decodeType = token[1] == "T" ? Translate : Generate;
      index = Scan<size_t>(token[2]);
    } else {
      UserMessage::Add("Malformed mapping!");
      CHECK(false);
    }

    DecodeStep* decodeStep = NULL;
    switch (decodeType) {
    case Translate:
      if(index>=m_phraseDictionary.size()) {
        stringstream strme;
        strme << "No phrase dictionary with index "
              << index << " available!";
        UserMessage::Add(strme.str());
        CHECK(false);
      }
      decodeStep = new DecodeStepTranslation(m_phraseDictionary[index], prev);
      break;
    case Generate:
      if(index>=m_generationDictionary.size()) {
        stringstream strme;
        strme << "No generation dictionary with index "
              << index << " available!";
        UserMessage::Add(strme.str());
        CHECK(false);
      }
      decodeStep = new DecodeStepGeneration(m_generationDictionary[index], prev);
      break;
    case InsertNullFertilityWord:
      CHECK(!"Please implement NullFertilityInsertion.");
      break;
    }

    CHECK(decodeStep);
    if (m_decodeGraphs.size() < decodeGraphInd + 1) {
      DecodeGraph *decodeGraph;
      if (IsChart()) {
        size_t maxChartSpan = (decodeGraphInd < maxChartSpans.size()) ? maxChartSpans[decodeGraphInd] : DEFAULT_MAX_CHART_SPAN;
	cerr << "max-chart-span: " << maxChartSpans[decodeGraphInd] << endl;
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
  for(size_t i=0; i<m_decodeGraphs.size(); i++) {
    m_decodeGraphBackoff.push_back( 0 );
  }
  // if specified, record maxmimum unseen n-gram size
  const vector<string> &backoffVector = m_parameter->GetParam("decoding-graph-backoff");
  for(size_t i=0; i<m_decodeGraphs.size() && i<backoffVector.size(); i++) {
    m_decodeGraphBackoff[i] = Scan<size_t>(backoffVector[i]);
  }

  return true;
}

const TranslationOptionList* StaticData::FindTransOptListInCache(const DecodeGraph &decodeGraph, const Phrase &sourcePhrase) const
{
  std::pair<size_t, Phrase> key(decodeGraph.GetPosition(), sourcePhrase);
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_transOptCacheMutex);
#endif
  std::map<std::pair<size_t, Phrase>, std::pair<TranslationOptionList*,clock_t> >::iterator iter
    = m_transOptCache.find(key);
  if (iter == m_transOptCache.end())
    return NULL;
  iter->second.second = clock(); // update last used time
  return iter->second.first;
}

void StaticData::ReduceTransOptCache() const
{
  if (m_transOptCache.size() <= m_transOptCacheMaxSize) return; // not full
  clock_t t = clock();

  // find cutoff for last used time
  priority_queue< clock_t > lastUsedTimes;
  std::map<std::pair<size_t, Phrase>, std::pair<TranslationOptionList*,clock_t> >::iterator iter;
  iter = m_transOptCache.begin();
  while( iter != m_transOptCache.end() ) {
    lastUsedTimes.push( iter->second.second );
    iter++;
  }
  for( size_t i=0; i < lastUsedTimes.size()-m_transOptCacheMaxSize/2; i++ )
    lastUsedTimes.pop();
  clock_t cutoffLastUsedTime = lastUsedTimes.top();

  // remove all old entries
  iter = m_transOptCache.begin();
  while( iter != m_transOptCache.end() ) {
    if (iter->second.second < cutoffLastUsedTime) {
      std::map<std::pair<size_t, Phrase>, std::pair<TranslationOptionList*,clock_t> >::iterator iterRemove = iter++;
      delete iterRemove->second.first;
      m_transOptCache.erase(iterRemove);
    } else iter++;
  }
  VERBOSE(2,"Reduced persistent translation option cache in " << ((clock()-t)/(float)CLOCKS_PER_SEC) << " seconds." << std::endl);
}

void StaticData::AddTransOptListToCache(const DecodeGraph &decodeGraph, const Phrase &sourcePhrase, const TranslationOptionList &transOptList) const
{
  if (m_transOptCacheMaxSize == 0) return;
  std::pair<size_t, Phrase> key(decodeGraph.GetPosition(), sourcePhrase);
  TranslationOptionList* storedTransOptList = new TranslationOptionList(transOptList);
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_transOptCacheMutex);
#endif
  m_transOptCache[key] = make_pair( storedTransOptList, clock() );
  ReduceTransOptCache();
}
void StaticData::ClearTransOptionCache() const {
  map<std::pair<size_t, Phrase>, std::pair< TranslationOptionList*, clock_t > >::iterator iterCache;
  for (iterCache = m_transOptCache.begin() ; iterCache != m_transOptCache.end() ; ++iterCache) {
    TranslationOptionList *transOptList = iterCache->second.first;
    delete transOptList;
  }
}

void StaticData::ReLoadParameter()
{
  assert(false); // TODO completely redo. Too many hardcoded ff
  /*
  m_verboseLevel = 1;
  if (m_parameter->GetParam("verbose").size() == 1) {
    m_verboseLevel = Scan<size_t>( m_parameter->GetParam("verbose")[0]);
  }

  // check whether "weight-u" is already set
  if (m_parameter->isParamShortNameSpecified("u")) {
    if (m_parameter->GetParamShortName("u").size() < 1 ) {
      PARAM_VEC w(1,"1.0");
      m_parameter->OverwriteParamShortName("u", w);
    }
  }

  //loop over all ScoreProducer to update weights
  const TranslationSystem &transSystem = GetTranslationSystem(TranslationSystem::DEFAULT);

  std::vector<const ScoreProducer*>::const_iterator iterSP;
  for (iterSP = transSystem.GetFeatureFunctions().begin() ; iterSP != transSystem.GetFeatureFunctions().end() ; ++iterSP) {
    std::string paramShortName = (*iterSP)->GetScoreProducerWeightShortName();
    vector<float> Weights = Scan<float>(m_parameter->GetParamShortName(paramShortName));

    if (paramShortName == "d") { //basic distortion model takes the first weight
      if ((*iterSP)->GetScoreProducerDescription() == "Distortion") {
        Weights.resize(1); //take only the first element
      } else { //lexicalized reordering model takes the other
        Weights.erase(Weights.begin()); //remove the first element
      }
      //			std::cerr << "this is the Distortion Score Producer -> " << (*iterSP)->GetScoreProducerDescription() << std::cerr;
      //			std::cerr << "this is the Distortion Score Producer; it has " << (*iterSP)->GetNumScoreComponents() << " weights"<< std::cerr;
      //  	std::cerr << Weights << std::endl;
    } else if (paramShortName == "tm") {
      continue;
    }
    SetWeights(*iterSP, Weights);
  }

  //	std::cerr << "There are " << m_phraseDictionary.size() << " m_phraseDictionaryfeatures" << std::endl;

  const vector<float> WeightsTM = Scan<float>(m_parameter->GetParamShortName("tm"));
  //  std::cerr << "WeightsTM: " << WeightsTM << std::endl;

  const vector<float> WeightsLM = Scan<float>(m_parameter->GetParamShortName("lm"));
  //  std::cerr << "WeightsLM: " << WeightsLM << std::endl;

  size_t index_WeightTM = 0;
  for(size_t i=0; i<transSystem.GetPhraseDictionaries().size(); ++i) {
    PhraseDictionaryFeature &phraseDictionaryFeature = *m_phraseDictionary[i];

    //		std::cerr << "phraseDictionaryFeature.GetNumScoreComponents():" << phraseDictionaryFeature.GetNumScoreComponents() << std::endl;
    //		std::cerr << "phraseDictionaryFeature.GetNumInputScores():" << phraseDictionaryFeature.GetNumInputScores() << std::endl;

    vector<float> tmp_weights;
    for(size_t j=0; j<phraseDictionaryFeature.GetNumScoreComponents(); ++j)
      tmp_weights.push_back(WeightsTM[index_WeightTM++]);

    //  std::cerr << tmp_weights << std::endl;

    SetWeights(&phraseDictionaryFeature, tmp_weights);
  }
  */
}

void StaticData::ReLoadBleuScoreFeatureParameter(float weight)
{
  assert(false);
  /*
  //loop over ScoreProducers to update weights of BleuScoreFeature
  const TranslationSystem &transSystem = GetTranslationSystem(TranslationSystem::DEFAULT);

  std::vector<const ScoreProducer*>::const_iterator iterSP;
  for (iterSP = transSystem.GetFeatureFunctions().begin() ; iterSP != transSystem.GetFeatureFunctions().end() ; ++iterSP) {
    std::string paramShortName = (*iterSP)->GetScoreProducerWeightShortName();
    if (paramShortName == "bl") {
      SetWeight(*iterSP, weight);
      break;
    }
  }
  */
}

// ScoreComponentCollection StaticData::GetAllWeightsScoreComponentCollection() const {}
// in ScoreComponentCollection.h

void StaticData::SetExecPath(const std::string &path)
{
  /*
   namespace fs = boost::filesystem;
   
   fs::path full_path( fs::initial_path<fs::path>() );
   
   full_path = fs::system_complete( fs::path( path ) );
   
   //Without file name
   m_binPath = full_path.parent_path().string();
   */
  
  // NOT TESTED
  size_t pos = path.rfind("/");
  if (pos !=  string::npos)
  {
    m_binPath = path.substr(0, pos); 
  }
  cerr << m_binPath << endl;
}

const string &StaticData::GetBinDirectory() const
{
  return m_binPath;
}

float StaticData::GetWeightWordPenalty() const {
  float weightWP = GetWeight(m_wpProducer);
  //VERBOSE(1, "Read weightWP from translation sytem: " << weightWP << std::endl);
  return weightWP;
}

float StaticData::GetWeightUnknownWordPenalty() const {
  return GetWeight(m_unknownWordPenaltyProducer);
}

float StaticData::GetWeightDistortion() const {
  CHECK(m_distortionScoreProducer);
  return StaticData::Instance().GetWeight(m_distortionScoreProducer);
}

void StaticData::ConfigDictionaries() {
  for (vector<DecodeGraph*>::const_iterator i = m_decodeGraphs.begin();
    i != m_decodeGraphs.end(); ++i) {
      for (DecodeGraph::const_iterator j = (*i)->begin(); j != (*i)->end(); ++j) {
        const DecodeStep* step = *j;
        PhraseDictionaryFeature* pdict = const_cast<PhraseDictionaryFeature*>(step->GetPhraseDictionaryFeature());
        if (pdict) {
          pdict->InitDictionary(NULL);
        }
        GenerationDictionary* gdict = const_cast<GenerationDictionary*>(step->GetGenerationDictionaryFeature());
        if (gdict) {
        }
      }
  }

}

void StaticData::InitializeForInput(const InputType& source) const {
  const std::vector<FeatureFunction*> &producers = FeatureFunction::GetFeatureFunctions();
  for(size_t i=0;i<producers.size();++i) {
    FeatureFunction &ff = *producers[i];
    ff.InitializeForInput(source);
  }
}

void StaticData::CleanUpAfterSentenceProcessing(const InputType& source) const {
  const std::vector<FeatureFunction*> &producers = FeatureFunction::GetFeatureFunctions();
  for(size_t i=0;i<producers.size();++i) {
    FeatureFunction &ff = *producers[i];
    ff.CleanUpAfterSentenceProcessing(source);
    cerr << endl << "Cleaning " << &ff << endl;
  }
}

void StaticData::CollectFeatureFunctions()
{
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  std::vector<FeatureFunction*>::const_iterator iter;
  for (iter = ffs.begin(); iter != ffs.end(); ++iter) {
    const FeatureFunction *ff = *iter;
    cerr << ff->GetScoreProducerDescription() << endl;

    const LanguageModel *lm = dynamic_cast<const LanguageModel*>(ff);
    if (lm) {
      LanguageModel *lmNonConst = const_cast<LanguageModel*>(lm);
      m_languageModel.Add(lmNonConst);
      continue;
    }

    const GenerationDictionary *generation = dynamic_cast<const GenerationDictionary*>(ff);
    if (generation) {
      m_generationDictionary.push_back(generation);
      continue;
    }

  }

}

} // namespace


