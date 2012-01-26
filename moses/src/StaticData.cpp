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
#include "PhraseDictionaryMemory.h"
#include "DecodeStepTranslation.h"
#include "DecodeStepGeneration.h"
#include "GenerationDictionary.h"
#include "DummyScoreProducers.h"
#include "StaticData.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Timer.h"
#include "LM/Factory.h"
#include "LexicalReordering.h"
#include "GlobalLexicalModel.h"
#include "SentenceStats.h"
#include "PhraseDictionary.h"
#include "UserMessage.h"
#include "TranslationOption.h"
#include "DecodeGraph.h"
#include "InputFileStream.h"

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

StaticData StaticData::s_instance;

StaticData::StaticData()
  :m_numLinkParams(1)
  ,m_fLMsLoaded(false)
  ,m_sourceStartPosMattersForRecombination(false)
  ,m_inputType(SentenceInput)
  ,m_numInputScores(0)
  ,m_detailedTranslationReportingFilePath()
  ,m_onlyDistinctNBest(false)
  ,m_factorDelimiter("|") // default delimiter between factors
  ,m_lmEnableOOVFeature(false)
  ,m_isAlwaysCreateDirectTranslationOption(false)
{
  m_maxFactorIdx[0] = 0;  // source side
  m_maxFactorIdx[1] = 0;  // target side

  m_xmlBrackets.first="<";
  m_xmlBrackets.second=">";

  // memory pools
  Phrase::InitializeMemPool();
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

  if (m_searchAlgorithm == ChartDecoding)
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

  //word-to-word alignment
  SetBooleanParameter( &m_UseAlignmentInfo, "use-alignment-info", false );
  SetBooleanParameter( &m_PrintAlignmentInfo, "print-alignment-info", false );
  SetBooleanParameter( &m_PrintAlignmentInfoNbest, "print-alignment-info-in-n-best", false );

  SetBooleanParameter( &m_outputHypoScore, "output-hypo-score", false );

  if (!m_UseAlignmentInfo && m_PrintAlignmentInfo) {
    TRACE_ERR("--print-alignment-info should only be used together with \"--use-alignment-info true\". Continue forcing to false.\n");
    m_PrintAlignmentInfo=false;
  }
  if (!m_UseAlignmentInfo && m_PrintAlignmentInfoNbest) {
    TRACE_ERR("--print-alignment-info-in-n-best should only be used together with \"--use-alignment-info true\". Continue forcing to false.\n");
    m_PrintAlignmentInfoNbest=false;
  }

  if (m_parameter->GetParam("alignment-output-file").size() > 0) {
    m_alignmentOutputFile = Scan<std::string>(m_parameter->GetParam("alignment-output-file")[0]);
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
  SetBooleanParameter( &m_unprunedSearchGraph, "unpruned-search-graph", true );

  // include feature names in the n-best list
  SetBooleanParameter( &m_labeledNBestList, "labeled-n-best-list", true );

  // include word alignment in the n-best list
  SetBooleanParameter( &m_nBestIncludesAlignment, "include-alignment-in-n-best", false );

  // printing source phrase spans
  SetBooleanParameter( &m_reportSegmentation, "report-segmentation", false );

  // print all factors of output translations
  SetBooleanParameter( &m_reportAllFactors, "report-all-factors", false );

  // print all factors of output translations
  SetBooleanParameter( &m_reportAllFactorsNBest, "report-all-factors-in-n-best", false );

  //
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
  for (size_t i = 0; i < m_parameter->GetParam("weight-w").size(); ++i) {
    float weightWordPenalty       = Scan<float>( m_parameter->GetParam("weight-w")[i] );
    m_wordPenaltyProducers.push_back(new WordPenaltyProducer(m_scoreIndexManager));
    m_allWeights.push_back(weightWordPenalty);
  }


  float weightUnknownWord				= (m_parameter->GetParam("weight-u").size() > 0) ? Scan<float>(m_parameter->GetParam("weight-u")[0]) : 1;
  m_unknownWordPenaltyProducer = new UnknownWordPenaltyProducer(m_scoreIndexManager);
  m_allWeights.push_back(weightUnknownWord);

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

#ifdef HAVE_SYNLM
	if (m_parameter->GetParam("slmodel-file").size() > 0) {
	  if (!LoadSyntacticLanguageModel()) return false;
	}
#endif
	
  if (!LoadLexicalReorderingModel()) return false;
  if (!LoadLanguageModels()) return false;
  if (!LoadGenerationTables()) return false;
  if (!LoadPhraseTables()) return false;
  if (!LoadGlobalLexicalModel()) return false;
  if (!LoadDecodeGraphs()) return false;


  //configure the translation systems with these tables
  vector<string> tsConfig = m_parameter->GetParam("translation-systems");
  if (!tsConfig.size()) {
    //use all models in default system.
    tsConfig.push_back(TranslationSystem::DEFAULT + " R * D * L * G *");
  }

  if (m_wordPenaltyProducers.size() != tsConfig.size()) {
    UserMessage::Add(string("Mismatch between number of word penalties and number of translation systems"));
    return false;
  }

  if (m_searchAlgorithm == ChartDecoding) {
    //insert some null distortion score producers
    m_distortionScoreProducers.assign(tsConfig.size(), NULL);
  } else {
    if (m_distortionScoreProducers.size() != tsConfig.size()) {
      UserMessage::Add(string("Mismatch between number of distortion scores and number of translation systems. Or [search-algorithm] has been set to a phrase-based algorithm when it should be chart decoding"));
      return false;
    }
  }

  for (size_t i = 0; i < tsConfig.size(); ++i) {
    vector<string> config = Tokenize(tsConfig[i]);
    if (config.size() % 2 != 1) {
      UserMessage::Add(string("Incorrect number of fields in Translation System config. Should be an odd number"));
    }
    m_translationSystems.insert(pair<string, TranslationSystem>(config[0],
                                TranslationSystem(config[0],m_wordPenaltyProducers[i],m_unknownWordPenaltyProducer,m_distortionScoreProducers[i])));
    for (size_t j = 1; j < config.size(); j += 2) {
      const string& id = config[j];
      const string& tables = config[j+1];
      set<size_t> tableIds;
      if (tables != "*") {
        //selected tables
        vector<string> tableIdStrings = Tokenize(tables,",");
        vector<size_t> tableIdList;
        Scan<size_t>(tableIdList, tableIdStrings);
        copy(tableIdList.begin(), tableIdList.end(), inserter(tableIds,tableIds.end()));
      }
      if (id == "D") {
        for (size_t k = 0; k < m_decodeGraphs.size(); ++k) {
          if (!tableIds.size() || tableIds.find(k) != tableIds.end()) {
            VERBOSE(2,"Adding decoder graph " << k << " to translation system " << config[0] << endl);
            m_translationSystems.find(config[0])->second.AddDecodeGraph(m_decodeGraphs[k],m_decodeGraphBackoff[k]);
          }
        }
      } else if (id == "R") {
        for (size_t k = 0; k < m_reorderModels.size(); ++k) {
          if (!tableIds.size() || tableIds.find(k) != tableIds.end()) {
            m_translationSystems.find(config[0])->second.AddReorderModel(m_reorderModels[k]);
            VERBOSE(2,"Adding reorder table " << k << " to translation system " << config[0] << endl);
          }
        }
      } else if (id == "G") {
        for (size_t k = 0; k < m_globalLexicalModels.size(); ++k) {
          if (!tableIds.size() || tableIds.find(k) != tableIds.end()) {
            m_translationSystems.find(config[0])->second.AddGlobalLexicalModel(m_globalLexicalModels[k]);
            VERBOSE(2,"Adding global lexical model " << k << " to translation system " << config[0] << endl);
          }
        }
      } else if (id == "L") {
        size_t lmid = 0;
        for (LMList::const_iterator k = m_languageModel.begin(); k != m_languageModel.end(); ++k, ++lmid) {
          if (!tableIds.size() || tableIds.find(lmid) != tableIds.end()) {
            m_translationSystems.find(config[0])->second.AddLanguageModel(*k);
            VERBOSE(2,"Adding language model " << lmid << " to translation system " << config[0] << endl);
          }
        }
      } else {
        UserMessage::Add(string("Incorrect translation system identifier: ") + id);
        return false;
      }
    }
    //Instigate dictionary loading
    m_translationSystems.find(config[0])->second.ConfigDictionaries();



    //Add any other features here.
#ifdef HAVE_SYNLM
    if (m_syntacticLanguageModel != NULL) {
      m_translationSystems.find(config[0])->second.AddFeatureFunction(m_syntacticLanguageModel);
    }
#endif
  }


  m_scoreIndexManager.InitFeatureNames();

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

StaticData::~StaticData()
{
  RemoveAllInColl(m_phraseDictionary);
  RemoveAllInColl(m_generationDictionary);
  RemoveAllInColl(m_reorderModels);
  RemoveAllInColl(m_globalLexicalModels);
	
#ifdef HAVE_SYNLM
	delete m_syntacticLanguageModel;
#endif


  RemoveAllInColl(m_decodeGraphs);
  RemoveAllInColl(m_wordPenaltyProducers);
  RemoveAllInColl(m_distortionScoreProducers);
  m_languageModel.CleanUp();

  // delete trans opt
  ClearTransOptionCache();

  // small score producers
  delete m_unknownWordPenaltyProducer;

  //delete m_parameter;

  // memory pools
  Phrase::FinalizeMemPool();

}

#ifdef HAVE_SYNLM
  bool StaticData::LoadSyntacticLanguageModel() {
    cerr << "Loading syntactic language models..." << std::endl;
    
    const vector<float> weights = Scan<float>(m_parameter->GetParam("weight-slm"));
    const vector<string> files = m_parameter->GetParam("slmodel-file");
    
    const FactorType factorType = (m_parameter->GetParam("slmodel-factor").size() > 0) ?
      TransformScore(Scan<int>(m_parameter->GetParam("slmodel-factor")[0]))
      : 0;

    const size_t beamWidth = (m_parameter->GetParam("slmodel-beam").size() > 0) ?
      TransformScore(Scan<int>(m_parameter->GetParam("slmodel-beam")[0]))
      : 500;

    if (files.size() < 1) {
      cerr << "No syntactic language model files specified!" << std::endl;
      return false;
    }

    // check if feature is used
    if (weights.size() >= 1) {

      //cout.setf(ios::scientific,ios::floatfield);
      //cerr.setf(ios::scientific,ios::floatfield);
      
      // create the feature
      m_syntacticLanguageModel = new SyntacticLanguageModel(files,weights,factorType,beamWidth); 

      /* 
      /////////////////////////////////////////
      // BEGIN LANE's UNSTABLE EXPERIMENT :)
      //

      double ppl = m_syntacticLanguageModel->perplexity();
      cerr << "Probability is " << ppl << endl;


      //
      // END LANE's UNSTABLE EXPERIMENT
      /////////////////////////////////////////
      */


      if (m_syntacticLanguageModel==NULL) {
	return false;
      }

    }
    
    return true;

  }
#endif

bool StaticData::LoadLexicalReorderingModel()
{
  VERBOSE(1, "Loading lexical distortion models...");
  const vector<string> fileStr    = m_parameter->GetParam("distortion-file");
  bool hasWeightlr = (m_parameter->GetParam("weight-lr").size() != 0);
  vector<string> weightsStr;
  if (hasWeightlr) {
    weightsStr = m_parameter->GetParam("weight-lr");
  } else {
    weightsStr = m_parameter->GetParam("weight-d");
  }

  std::vector<float>   weights;
  size_t w = 1; //cur weight
  if (hasWeightlr) {
    w = 0; // if reading from weight-lr, don't have to count first as distortion penalty
  }
  size_t f = 0; //cur file
  //get weights values
  VERBOSE(1, "have " << fileStr.size() << " models" << std::endl);
  for(size_t j = 0; j < weightsStr.size(); ++j) {
    weights.push_back(Scan<float>(weightsStr[j]));
  }
  //load all models
  for(size_t i = 0; i < fileStr.size(); ++i) {
    vector<string> spec = Tokenize<string>(fileStr[f], " ");
    ++f; //mark file as consumed
    if(spec.size() != 4) {
      UserMessage::Add("Invalid Lexical Reordering Model Specification: " + fileStr[f]);
      return false;
    }

    // spec[0] = factor map
    // spec[1] = name
    // spec[2] = num weights
    // spec[3] = fileName

    // decode factor map

    vector<FactorType> input, output;
    vector<string> inputfactors = Tokenize(spec[0],"-");
    if(inputfactors.size() == 2) {
      input  = Tokenize<FactorType>(inputfactors[0],",");
      output = Tokenize<FactorType>(inputfactors[1],",");
    } else if(inputfactors.size() == 1) {
      //if there is only one side assume it is on e side... why?
      output = Tokenize<FactorType>(inputfactors[0],",");
    } else {
      //format error
      return false;
    }

    string modelType = spec[1];

    // decode num weights and fetch weights from array
    std::vector<float> mweights;
    size_t numWeights = atoi(spec[2].c_str());
    for(size_t k = 0; k < numWeights; ++k, ++w) {
      if(w >= weights.size()) {
        UserMessage::Add("Lexicalized distortion model: Not enough weights, add to [weight-d]");
        return false;
      } else
        mweights.push_back(weights[w]);
    }

    string filePath = spec[3];

    m_reorderModels.push_back(new LexicalReordering(input, output, modelType, filePath, mweights));
  }
  return true;
}

bool StaticData::LoadGlobalLexicalModel()
{
  const vector<float> &weight = Scan<float>(m_parameter->GetParam("weight-lex"));
  const vector<string> &file = m_parameter->GetParam("global-lexical-file");

  if (weight.size() != file.size()) {
    std::cerr << "number of weights and models for the global lexical model does not match ("
              << weight.size() << " != " << file.size() << ")" << std::endl;
    return false;
  }

  for (size_t i = 0; i < weight.size(); i++ ) {
    vector<string> spec = Tokenize<string>(file[i], " ");
    if ( spec.size() != 2 ) {
      std::cerr << "wrong global lexical model specification: " << file[i] << endl;
      return false;
    }
    vector< string > factors = Tokenize(spec[0],"-");
    if ( factors.size() != 2 ) {
      std::cerr << "wrong factor definition for global lexical model: " << spec[0] << endl;
      return false;
    }
    vector<FactorType> inputFactors = Tokenize<FactorType>(factors[0],",");
    vector<FactorType> outputFactors = Tokenize<FactorType>(factors[1],",");
    m_globalLexicalModels.push_back( new GlobalLexicalModel( spec[1], weight[i], inputFactors, outputFactors ) );
  }
  return true;
}

bool StaticData::LoadLanguageModels()
{
  if (m_parameter->GetParam("lmodel-file").size() > 0) {
    // weights
    vector<float> weightAll = Scan<float>(m_parameter->GetParam("weight-l"));

    for (size_t i = 0 ; i < weightAll.size() ; i++) {
      m_allWeights.push_back(weightAll[i]);
    }

    // dictionary upper-bounds fo all IRST LMs
    vector<int> LMdub = Scan<int>(m_parameter->GetParam("lmodel-dub"));
    if (m_parameter->GetParam("lmodel-dub").size() == 0) {
      for(size_t i=0; i<m_parameter->GetParam("lmodel-file").size(); i++)
        LMdub.push_back(0);
    }

    // initialize n-gram order for each factor. populated only by factored lm
    const vector<string> &lmVector = m_parameter->GetParam("lmodel-file");
    //prevent language models from being loaded twice
    map<string,LanguageModel*> languageModelsLoaded;

    for(size_t i=0; i<lmVector.size(); i++) {
      LanguageModel* lm = NULL;
      if (languageModelsLoaded.find(lmVector[i]) != languageModelsLoaded.end()) {
        lm = languageModelsLoaded[lmVector[i]]->Duplicate(m_scoreIndexManager); 
      } else {
        vector<string>	token		= Tokenize(lmVector[i]);
        if (token.size() != 4 && token.size() != 5 ) {
          UserMessage::Add("Expected format 'LM-TYPE FACTOR-TYPE NGRAM-ORDER filePath [mapFilePath (only for IRSTLM)]'");
          return false;
        }
        // type = implementation, SRI, IRST etc
        LMImplementation lmImplementation = static_cast<LMImplementation>(Scan<int>(token[0]));

        // factorType = 0 = Surface, 1 = POS, 2 = Stem, 3 = Morphology, etc
        vector<FactorType> 	factorTypes		= Tokenize<FactorType>(token[1], ",");

        // nGramOrder = 2 = bigram, 3 = trigram, etc
        size_t nGramOrder = Scan<int>(token[2]);

        string &languageModelFile = token[3];
        if (token.size() == 5) {
          if (lmImplementation==IRST)
            languageModelFile += " " + token[4];
          else {
            UserMessage::Add("Expected format 'LM-TYPE FACTOR-TYPE NGRAM-ORDER filePath [mapFilePath (only for IRSTLM)]'");
            return false;
          }
        }
        IFVERBOSE(1)
        PrintUserTime(string("Start loading LanguageModel ") + languageModelFile);

        lm = LanguageModelFactory::CreateLanguageModel(
               lmImplementation
               , factorTypes
               , nGramOrder
               , languageModelFile
               , m_scoreIndexManager
               , LMdub[i]);
        if (lm == NULL) {
          UserMessage::Add("no LM created. We probably don't have it compiled");
          return false;
        }
        languageModelsLoaded[lmVector[i]] = lm;
      }

      m_languageModel.Add(lm);
    }
  }
  // flag indicating that language models were loaded,
  // since phrase table loading requires their presence
  m_fLMsLoaded = true;
  IFVERBOSE(1)
  PrintUserTime("Finished loading LanguageModels");
  return true;
}

bool StaticData::LoadGenerationTables()
{
  if (m_parameter->GetParam("generation-file").size() > 0) {
    const vector<string> &generationVector = m_parameter->GetParam("generation-file");
    const vector<float> &weight = Scan<float>(m_parameter->GetParam("weight-generation"));

    IFVERBOSE(1) {
      TRACE_ERR( "weight-generation: ");
      for (size_t i = 0 ; i < weight.size() ; i++) {
        TRACE_ERR( weight[i] << "\t");
      }
      TRACE_ERR(endl);
    }
    size_t currWeightNum = 0;

    for(size_t currDict = 0 ; currDict < generationVector.size(); currDict++) {
      vector<string>			token		= Tokenize(generationVector[currDict]);
      vector<FactorType> 	input		= Tokenize<FactorType>(token[0], ",")
                                    ,output	= Tokenize<FactorType>(token[1], ",");
      m_maxFactorIdx[1] = CalcMax(m_maxFactorIdx[1], input, output);
      string							filePath;
      size_t							numFeatures;

      numFeatures = Scan<size_t>(token[2]);
      filePath = token[3];

      if (!FileExists(filePath) && FileExists(filePath + ".gz")) {
        filePath += ".gz";
      }

      VERBOSE(1, filePath << endl);

      m_generationDictionary.push_back(new GenerationDictionary(numFeatures, m_scoreIndexManager, input,output));
      CHECK(m_generationDictionary.back() && "could not create GenerationDictionary");
      if (!m_generationDictionary.back()->Load(filePath, Output)) {
        delete m_generationDictionary.back();
        return false;
      }
      for(size_t i = 0; i < numFeatures; i++) {
        CHECK(currWeightNum < weight.size());
        m_allWeights.push_back(weight[currWeightNum++]);
      }
    }
    if (currWeightNum != weight.size()) {
      TRACE_ERR( "  [WARNING] config file has " << weight.size() << " generation weights listed, but the configuration for generation files indicates there should be " << currWeightNum << "!\n");
    }
  }

  return true;
}

/* Doesn't load phrase tables any more. Just creates the features. */
bool StaticData::LoadPhraseTables()
{
  VERBOSE(2,"Creating phrase table features" << endl);

  // language models must be loaded prior to loading phrase tables
  CHECK(m_fLMsLoaded);
  // load phrase translation tables
  if (m_parameter->GetParam("ttable-file").size() > 0) {
    // weights
    vector<float> weightAll									= Scan<float>(m_parameter->GetParam("weight-t"));

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

    size_t index = 0;
    size_t weightAllOffset = 0;
    bool oldFileFormat = false;
    for(size_t currDict = 0 ; currDict < translationVector.size(); currDict++) {
      vector<string>                  token           = Tokenize(translationVector[currDict]);

      if(currDict == 0 && token.size() == 4) {
        VERBOSE(1, "Warning: Phrase table specification in old 4-field format. Assuming binary phrase tables (type 1)!" << endl);
        oldFileFormat = true;
      }

      if((!oldFileFormat && token.size() < 5) || (oldFileFormat && token.size() != 4)) {
        UserMessage::Add("invalid phrase table specification");
        return false;
      }

      PhraseTableImplementation implementation = (PhraseTableImplementation) Scan<int>(token[0]);
      if(oldFileFormat) {
        token.push_back(token[3]);
        token[3] = token[2];
        token[2] = token[1];
        token[1] = token[0];
        token[0] = "1";
        implementation = Binary;
      } else
        implementation = (PhraseTableImplementation) Scan<int>(token[0]);

      CHECK(token.size() >= 5);
      //characteristics of the phrase table

      vector<FactorType>  input		= Tokenize<FactorType>(token[1], ",")
                                    ,output = Tokenize<FactorType>(token[2], ",");
      m_maxFactorIdx[0] = CalcMax(m_maxFactorIdx[0], input);
      m_maxFactorIdx[1] = CalcMax(m_maxFactorIdx[1], output);
      m_maxNumFactors = std::max(m_maxFactorIdx[0], m_maxFactorIdx[1]) + 1;
      size_t numScoreComponent = Scan<size_t>(token[3]);
      string filePath= token[4];

      CHECK(weightAll.size() >= weightAllOffset + numScoreComponent);

      // weights for this phrase dictionary
      // first InputScores (if any), then translation scores
      vector<float> weight;

      if(currDict==0 && (m_inputType == ConfusionNetworkInput || m_inputType == WordLatticeInput)) {
        // TODO. find what the assumptions made by confusion network about phrase table output which makes
        // it only work with binrary file. This is a hack

        m_numInputScores=m_parameter->GetParam("weight-i").size();
        
        if (implementation == Binary)
        {
          for(unsigned k=0; k<m_numInputScores; ++k)
            weight.push_back(Scan<float>(m_parameter->GetParam("weight-i")[k]));
        }
        
        if(m_parameter->GetParam("link-param-count").size())
          m_numLinkParams = Scan<size_t>(m_parameter->GetParam("link-param-count")[0]);

        //print some info about this interaction:
        if (implementation == Binary) {
          if (m_numLinkParams == m_numInputScores) {
            VERBOSE(1,"specified equal numbers of link parameters and insertion weights, not using non-epsilon 'real' word link count.\n");
          } else if ((m_numLinkParams + 1) == m_numInputScores) {
            VERBOSE(1,"WARN: "<< m_numInputScores << " insertion weights found and only "<< m_numLinkParams << " link parameters specified, applying non-epsilon 'real' word link count for last feature weight.\n");
          } else {
            stringstream strme;
            strme << "You specified " << m_numInputScores
                  << " input weights (weight-i), but you specified " << m_numLinkParams << " link parameters (link-param-count)!";
            UserMessage::Add(strme.str());
            return false;
          }
        }
        
      }
      if (!m_inputType) {
        m_numInputScores=0;
      }
      //this number changes depending on what phrase table we're talking about: only 0 has the weights on it
      size_t tableInputScores = (currDict == 0 && implementation == Binary) ? m_numInputScores : 0;

      for (size_t currScore = 0 ; currScore < numScoreComponent; currScore++)
        weight.push_back(weightAll[weightAllOffset + currScore]);

      if(weight.size() - tableInputScores != numScoreComponent) {
        stringstream strme;
        strme << "Your phrase table has " << numScoreComponent
              << " scores, but you specified " << (weight.size() - tableInputScores) << " weights!";
        UserMessage::Add(strme.str());
        return false;
      }

      weightAllOffset += numScoreComponent;
      numScoreComponent += tableInputScores;

      string targetPath, alignmentsFile;
      if (implementation == SuffixArray) {
        targetPath		= token[5];
        alignmentsFile= token[6];
      }

      CHECK(numScoreComponent==weight.size());

      std::copy(weight.begin(),weight.end(),std::back_inserter(m_allWeights));

      //This is needed for regression testing, but the phrase table
      //might not really be loading here
      IFVERBOSE(1)
      PrintUserTime(string("Start loading PhraseTable ") + filePath);
      VERBOSE(1,"filePath: " << filePath <<endl);

      PhraseDictionaryFeature* pdf = new PhraseDictionaryFeature(
        implementation
        , numScoreComponent
        , (currDict==0 ? m_numInputScores : 0)
        , input
        , output
        , filePath
        , weight
        , maxTargetPhrase[index]
        , targetPath, alignmentsFile);

      m_phraseDictionary.push_back(pdf);





      index++;
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
  const vector<string> distortionWeights = m_parameter->GetParam("weight-d");
  size_t distortionWeightCount = distortionWeights.size();
  //if there's a lex-reordering model, and no separate weight set, then
  //take just one of these weights for linear distortion
  if (!m_parameter->GetParam("weight-lr").size() && m_parameter->GetParam("distortion-file").size()) {
    distortionWeightCount = 1;
  }
  for (size_t i = 0; i < distortionWeightCount; ++i) {
    float weightDistortion = Scan<float>(distortionWeights[i]);
    m_distortionScoreProducers.push_back(new DistortionScoreProducer(m_scoreIndexManager));
    m_allWeights.push_back(weightDistortion);
  }
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
      if (m_searchAlgorithm == ChartDecoding) {
        size_t maxChartSpan = (decodeGraphInd < maxChartSpans.size()) ? maxChartSpans[decodeGraphInd] : DEFAULT_MAX_CHART_SPAN;
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


void StaticData::SetWeightsForScoreProducer(const ScoreProducer* sp, const std::vector<float>& weights)
{
  const size_t id = sp->GetScoreBookkeepingID();
  const size_t begin = m_scoreIndexManager.GetBeginIndex(id);
  const size_t end = m_scoreIndexManager.GetEndIndex(id);
  CHECK(end - begin == weights.size());
  if (m_allWeights.size() < end)
    m_allWeights.resize(end);
  std::vector<float>::const_iterator weightIter = weights.begin();
  for (size_t i = begin; i < end; i++)
    m_allWeights[i] = *weightIter++;
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

}


