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

#include "TypeDef.h"
#include "moses/FF/WordPenaltyProducer.h"
#include "moses/FF/UnknownWordPenaltyProducer.h"
#include "moses/FF/InputFeature.h"

#include "DecodeStepTranslation.h"
#include "DecodeStepGeneration.h"
#include "GenerationDictionary.h"
#include "StaticData.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Timer.h"
#include "UserMessage.h"
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

using namespace std;

namespace Moses
{

StaticData StaticData::s_instance;

StaticData::StaticData()
  :m_sourceStartPosMattersForRecombination(false)
  ,m_inputType(SentenceInput)
  ,m_wpProducer(NULL)
  ,m_unknownWordPenaltyProducer(NULL)
  ,m_inputFeature(NULL)
  ,m_detailedTranslationReportingFilePath()
  ,m_detailedTreeFragmentsTranslationReportingFilePath()
  ,m_onlyDistinctNBest(false)
  ,m_needAlignmentInfo(false)
  ,m_factorDelimiter("|") // default delimiter between factors
  ,m_lmEnableOOVFeature(false)
  ,m_isAlwaysCreateDirectTranslationOption(false)
  ,m_currentWeightSetting("default")
{
  m_xmlBrackets.first="<";
  m_xmlBrackets.second=">";

  // memory pools
  Phrase::InitializeMemPool();
}

StaticData::~StaticData()
{
  RemoveAllInColl(m_decodeGraphs);

  /*
  const std::vector<FeatureFunction*> &producers = FeatureFunction::GetFeatureFunctions();
  for(size_t i=0;i<producers.size();++i) {
  FeatureFunction *ff = producers[i];
    delete ff;
  }
  */

  // memory pools
  Phrase::FinalizeMemPool();
}

bool StaticData::LoadDataStatic(Parameter *parameter, const std::string &execPath)
{
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

  // to cube or not to cube
  m_searchAlgorithm = (m_parameter->GetParam("search-algorithm").size() > 0) ?
                      (SearchAlgorithm) Scan<size_t>(m_parameter->GetParam("search-algorithm")[0]) : Normal;

  if (IsChart())
    LoadChartDecodingParameters();

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

  // factor delimiter
  if (m_parameter->GetParam("factor-delimiter").size() > 0) {
    m_factorDelimiter = m_parameter->GetParam("factor-delimiter")[0];
  }

  SetBooleanParameter( &m_continuePartialTranslation, "continue-partial-translation", false );
  SetBooleanParameter( &m_outputHypoScore, "output-hypo-score", false );

  //word-to-word alignment
  // alignments
  SetBooleanParameter( &m_PrintAlignmentInfo, "print-alignment-info", false );
  if (m_PrintAlignmentInfo) {
    m_needAlignmentInfo = true;
  }

  if(m_parameter->GetParam("sort-word-alignment").size()) {
    m_wordAlignmentSort = (WordAlignmentSort) Scan<size_t>(m_parameter->GetParam("sort-word-alignment")[0]);
  }

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
    m_onlyDistinctNBest=(m_parameter->GetParam("n-best-list").size()>2
                         && m_parameter->GetParam("n-best-list")[2]=="distinct");
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
  } else {
    m_outputSearchGraph = false;
  }
  if (m_parameter->GetParam("output-search-graph-slf").size() > 0) {
    m_outputSearchGraphSLF = true;
  } else {
    m_outputSearchGraphSLF = false;
  }
  if (m_parameter->GetParam("output-search-graph-hypergraph").size() > 0) {
    m_outputSearchGraphHypergraph = true;
  } else {
    m_outputSearchGraphHypergraph = false;
  }
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
  SetBooleanParameter( &m_reportSegmentationEnriched, "report-segmentation-enriched", false );

  // print all factors of output translations
  SetBooleanParameter( &m_reportAllFactors, "report-all-factors", false );

  // print all factors of output translations
  SetBooleanParameter( &m_reportAllFactorsNBest, "report-all-factors-in-n-best", false );

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
  if (m_parameter->isParamSpecified("tree-translation-details")) {
    const vector<string> &args = m_parameter->GetParam("tree-translation-details");
    if (args.size() == 1) {
      m_detailedTreeFragmentsTranslationReportingFilePath = args[0];
    } else {
      UserMessage::Add(string("the tree-translation-details option requires exactly one filename argument"));
      return false;
    }
  }

  //DIMw
  if (m_parameter->isParamSpecified("translation-all-details")) {
    const vector<string> &args = m_parameter->GetParam("translation-all-details");
    if (args.size() == 1) {
      m_detailedAllTranslationReportingFilePath = args[0];
    } else {
      UserMessage::Add(string("the translation-all-details option requires exactly one filename argument"));
      return false;
    }
  }

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
  SetBooleanParameter( &m_markUnknown, "mark-unknown", false );

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

  // lattice MBR
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

  // use of xml in input
  if (m_parameter->GetParam("xml-input").size() == 0) m_xmlInputType = XmlPassThrough;
  else if (m_parameter->GetParam("xml-input")[0]=="exclusive") m_xmlInputType = XmlExclusive;
  else if (m_parameter->GetParam("xml-input")[0]=="inclusive") m_xmlInputType = XmlInclusive;
  else if (m_parameter->GetParam("xml-input")[0]=="constraint") m_xmlInputType = XmlConstraint;
  else if (m_parameter->GetParam("xml-input")[0]=="ignore") m_xmlInputType = XmlIgnore;
  else if (m_parameter->GetParam("xml-input")[0]=="pass-through") m_xmlInputType = XmlPassThrough;
  else {
    UserMessage::Add("invalid xml-input value, must be pass-through, exclusive, inclusive, constraint, or ignore");
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

  if (m_parameter->GetParam("placeholder-factor").size() > 0) {
    m_placeHolderFactor = Scan<FactorType>(m_parameter->GetParam("placeholder-factor")[0]);
  } else {
    m_placeHolderFactor = NOT_FOUND;
  }

  std::map<std::string, std::string> featureNameOverride = OverrideFeatureNames();

  // all features
  map<string, int> featureIndexMap;

  const vector<string> &features = m_parameter->GetParam("feature");
  for (size_t i = 0; i < features.size(); ++i) {
    const string &line = Trim(features[i]);
    cerr << "line=" << line << endl;
    if (line.empty())
      continue;

    vector<string> toks = Tokenize(line);

    string &feature = toks[0];
    std::map<std::string, std::string>::const_iterator iter = featureNameOverride.find(feature);
    if (iter == featureNameOverride.end()) {
    	// feature name not override
    	m_registry.Construct(feature, line);
    }
    else {
    	// replace feature name with new name
    	string newName = iter->second;
    	feature = newName;
    	string newLine = Join(" ", toks);
    	m_registry.Construct(newName, newLine);
    }

  }

  NoCache();
  OverrideFeatures();

  LoadFeatureFunctions();

  if (!LoadDecodeGraphs()) return false;

  if (!CheckWeights()) {
    return false;
  }

  //Add any other features here.

  //Load extra feature weights
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

  // alternate weight settings
  if (m_parameter->GetParam("alternate-weight-setting").size() > 0) {
    if (!LoadAlternateWeightSettings()) {
      return false;
    }
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

void StaticData::SetWeight(const FeatureFunction* sp, float weight)
{
  m_allWeights.Resize();
  m_allWeights.Assign(sp,weight);
}

void StaticData::SetWeights(const FeatureFunction* sp, const std::vector<float>& weights)
{
  m_allWeights.Resize();
  m_allWeights.Assign(sp,weights);
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
      UTIL_THROW_IF2(tokens.size() != 2,
    		  "Incorrect unknown LHS format: " << line);
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

bool StaticData::LoadDecodeGraphs()
{
  const vector<string> &mappingVector = m_parameter->GetParam("mapping");
  const vector<size_t> &maxChartSpans = Scan<size_t>(m_parameter->GetParam("max-chart-span"));
  const vector<PhraseDictionary*>& pts = PhraseDictionary::GetColl();
  const vector<GenerationDictionary*>& gens = GenerationDictionary::GetColl();

  const std::vector<FeatureFunction*> *featuresRemaining = &FeatureFunction::GetFeatureFunctions();
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
      UTIL_THROW_IF2(decodeGraphInd != prevDecodeGraphInd && decodeGraphInd != prevDecodeGraphInd + 1,
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
        stringstream strme;
        strme << "No phrase dictionary with index "
              << index << " available!";
        UTIL_THROW(util::Exception, strme.str());
      }
      decodeStep = new DecodeStepTranslation(pts[index], prev, *featuresRemaining);
      break;
    case Generate:
      if(index>=gens.size()) {
        stringstream strme;
        strme << "No generation dictionary with index "
              << index << " available!";
        UTIL_THROW(util::Exception, strme.str());
      }
      decodeStep = new DecodeStepGeneration(gens[index], prev, *featuresRemaining);
      break;
    case InsertNullFertilityWord:
      UTIL_THROW(util::Exception, "Please implement NullFertilityInsertion.");
      break;
    }

    featuresRemaining = &decodeStep->GetFeaturesRemaining();

    UTIL_THROW_IF2(decodeStep == NULL, "Null decode step");
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

void StaticData::ReLoadParameter()
{
  UTIL_THROW(util::Exception, "completely redo. Too many hardcoded ff"); // TODO completely redo. Too many hardcoded ff
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
  if (pos !=  string::npos) {
    m_binPath = path.substr(0, pos);
  }
  cerr << m_binPath << endl;
}

const string &StaticData::GetBinDirectory() const
{
  return m_binPath;
}

float StaticData::GetWeightWordPenalty() const
{
  float weightWP = GetWeight(m_wpProducer);
  //VERBOSE(1, "Read weightWP from translation sytem: " << weightWP << std::endl);
  return weightWP;
}

float StaticData::GetWeightUnknownWordPenalty() const
{
  return GetWeight(m_unknownWordPenaltyProducer);
}

void StaticData::InitializeForInput(const InputType& source) const
{
  const std::vector<FeatureFunction*> &producers = FeatureFunction::GetFeatureFunctions();
  for(size_t i=0; i<producers.size(); ++i) {
    FeatureFunction &ff = *producers[i];
    ff.InitializeForInput(source);
  }
}

void StaticData::CleanUpAfterSentenceProcessing(const InputType& source) const
{
  const std::vector<FeatureFunction*> &producers = FeatureFunction::GetFeatureFunctions();
  for(size_t i=0; i<producers.size(); ++i) {
    FeatureFunction &ff = *producers[i];
    ff.CleanUpAfterSentenceProcessing(source);
  }
}

void StaticData::LoadFeatureFunctions()
{
  const std::vector<FeatureFunction*> &ffs
  = FeatureFunction::GetFeatureFunctions();
  std::vector<FeatureFunction*>::const_iterator iter;
  for (iter = ffs.begin(); iter != ffs.end(); ++iter) {
    FeatureFunction *ff = *iter;
    bool doLoad = true;

    if (PhraseDictionary *ffCast = dynamic_cast<PhraseDictionary*>(ff)) {
      doLoad = false;
    } else if (const GenerationDictionary *ffCast
               = dynamic_cast<const GenerationDictionary*>(ff)) {
    	// do nothing
    } else if (WordPenaltyProducer *ffCast
               = dynamic_cast<WordPenaltyProducer*>(ff)) {
      UTIL_THROW_IF2(m_wpProducer, "Only 1 word penalty allowed"); // max 1 feature;
      m_wpProducer = ffCast;
    } else if (UnknownWordPenaltyProducer *ffCast
               = dynamic_cast<UnknownWordPenaltyProducer*>(ff)) {
      UTIL_THROW_IF2(m_unknownWordPenaltyProducer, "Only 1 unknown word penalty allowed"); // max 1 feature;
      m_unknownWordPenaltyProducer = ffCast;
    } else if (const InputFeature *ffCast = dynamic_cast<const InputFeature*>(ff)) {
      UTIL_THROW_IF2(m_inputFeature, "Only 1 input feature allowed"); // max 1 input feature;
      m_inputFeature = ffCast;
    }

    if (doLoad) {
      VERBOSE(1, "Loading " << ff->GetScoreProducerDescription() << endl);
      ff->Load();
    }
  }

  const std::vector<PhraseDictionary*> &pts = PhraseDictionary::GetColl();
  for (size_t i = 0; i < pts.size(); ++i) {
    PhraseDictionary *pt = pts[i];
    VERBOSE(1, "Loading " << pt->GetScoreProducerDescription() << endl);
    pt->Load();
  }

  CheckLEGACYPT();
}

bool StaticData::CheckWeights() const
{
  set<string> weightNames = m_parameter->GetWeightNames();

  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  for (size_t i = 0; i < ffs.size(); ++i) {
    const FeatureFunction &ff = *ffs[i];
    const string &descr = ff.GetScoreProducerDescription();

    set<string>::iterator iter = weightNames.find(descr);
    if (iter == weightNames.end()) {
      cerr << "Can't find weights for feature function " << descr << endl;
    } else {
      weightNames.erase(iter);
    }
  }

  if (!weightNames.empty()) {
    cerr << "The following weights have no feature function. Maybe incorrectly spelt weights: ";
    set<string>::iterator iter;
    for (iter = weightNames.begin(); iter != weightNames.end(); ++iter) {
      cerr << *iter << ",";
    }
    return false;
  }

  return true;
}

/**! Read in settings for alternative weights */
bool StaticData::LoadAlternateWeightSettings()
{
  if (m_threadCount > 1) {
    cerr << "ERROR: alternative weight settings currently not supported with multi-threading.";
    return false;
  }

  const vector<string> &weightSpecification = m_parameter->GetParam("alternate-weight-setting");

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
      cerr << "alternate weight setting " << currentId << endl;
      UTIL_THROW_IF2(m_weightSetting.find(currentId) != m_weightSetting.end(),
    		  "Duplicate alternate weight id: " << currentId);
      m_weightSetting[ currentId ] = new ScoreComponentCollection;

      // other specifications
      for(size_t j=1; j<tokens.size(); j++) {
        vector<string> args = Tokenize(tokens[j], "=");
        // TODO: support for sparse weights
        if (args[0] == "weight-file") {
          cerr << "ERROR: sparse weight files currently not supported";
        }
        // ignore feature functions
        else if (args[0] == "ignore-ff") {
          set< string > *ffNameSet = new set< string >;
          m_weightSettingIgnoreFF[ currentId ] = *ffNameSet;
          vector<string> featureFunctionName = Tokenize(args[1], ",");
          for(size_t k=0; k<featureFunctionName.size(); k++) {
            // check if a valid nane
            map<string,FeatureFunction*>::iterator ffLookUp
            = nameToFF.find(featureFunctionName[k]);
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
	SetBooleanParameter( &noCache, "no-cache", false );

	if (noCache) {
		const std::vector<PhraseDictionary*> &pts = PhraseDictionary::GetColl();
		for (size_t i = 0; i < pts.size(); ++i) {
			PhraseDictionary &pt = *pts[i];
			pt.SetParameter("cache-size", "0");
		}
	}
}

std::map<std::string, std::string> StaticData::OverrideFeatureNames()
{
	std::map<std::string, std::string> ret;

	const PARAM_VEC &params = m_parameter->GetParam("feature-name-overwrite");
	if (params.size()) {
		UTIL_THROW_IF2(params.size() != 1, "Only provide 1 line in the section [feature-name-overwrite]");
		vector<string> toks = Tokenize(params[0]);
		UTIL_THROW_IF2(toks.size() % 2 == 0, "Format of -feature-name-overwrite must be [old-name new-name]*");

		for (size_t i = 0; i < toks.size(); i += 2) {
			const string &oldName = toks[i];
			const string &newName = toks[i+1];
			ret[oldName] = newName;
		}
	}

	return ret;
}

void StaticData::OverrideFeatures()
{
  const PARAM_VEC &params = m_parameter->GetParam("feature-overwrite");
  for (size_t i = 0; i < params.size(); ++i) {
    const string &str = params[i];
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


} // namespace

