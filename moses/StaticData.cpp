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
#include "LM/Factory.h"
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
  :m_targetBigramFeature(NULL)
  ,m_phraseBoundaryFeature(NULL)
  ,m_phraseLengthFeature(NULL)
  ,m_targetWordInsertionFeature(NULL)
  ,m_sourceWordDeletionFeature(NULL)
  ,m_numLinkParams(1)
  ,m_fLMsLoaded(false)
  ,m_sourceStartPosMattersForRecombination(false)
  ,m_inputType(SentenceInput)
  ,m_numInputScores(0)
  ,m_bleuScoreFeature(NULL)
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

  SetBooleanParameter( &m_PrintPassthroughInformation, "print-passthrough", false );
  SetBooleanParameter( &m_PrintPassthroughInformationInNBest, "print-passthrough-in-n-best", false );

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
  for (size_t i = 0; i < m_parameter->GetParam("weight-w").size(); ++i) {
    float weightWordPenalty       = Scan<float>( m_parameter->GetParam("weight-w")[i] );
    m_wordPenaltyProducers.push_back(new WordPenaltyProducer());
    SetWeight(m_wordPenaltyProducers.back(), weightWordPenalty);
  }

  float weightUnknownWord				= (m_parameter->GetParam("weight-u").size() > 0) ? Scan<float>(m_parameter->GetParam("weight-u")[0]) : 1;
  m_unknownWordPenaltyProducer = new UnknownWordPenaltyProducer();
  SetWeight(m_unknownWordPenaltyProducer, weightUnknownWord);

  m_multimodelweights = Scan<float>( m_parameter->GetParam("weight-t-multimodel") );

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
  if (!LoadGlobalLexicalModelUnlimited()) return false;
  if (!LoadDecodeGraphs()) return false;
  if (!LoadReferences()) return  false;
  if (!LoadDiscrimLMFeature()) return false;
  if (!LoadPhrasePairFeature()) return false;
  if (!LoadPhraseBoundaryFeature()) return false;
  if (!LoadPhraseLengthFeature()) return false;
  if (!LoadTargetWordInsertionFeature()) return false;
  if (!LoadSourceWordDeletionFeature()) return false;
  if (!LoadWordTranslationFeature()) return false;

  // report individual sparse features in n-best list
  if (m_parameter->GetParam("report-sparse-features").size() > 0) {
    for(size_t i=0; i<m_parameter->GetParam("report-sparse-features").size(); i++) {
      const std::string &name = m_parameter->GetParam("report-sparse-features")[i];
      if (m_targetBigramFeature && name.compare(m_targetBigramFeature->GetScoreProducerWeightShortName(0)) == 0)
        m_targetBigramFeature->SetSparseFeatureReporting();
      if (m_targetNgramFeatures.size() > 0)
      	for (size_t i=0; i < m_targetNgramFeatures.size(); ++i)
      		if (name.compare(m_targetNgramFeatures[i]->GetScoreProducerWeightShortName(0)) == 0)
      			m_targetNgramFeatures[i]->SetSparseFeatureReporting();
      if (m_phraseBoundaryFeature && name.compare(m_phraseBoundaryFeature->GetScoreProducerWeightShortName(0)) == 0)
        m_phraseBoundaryFeature->SetSparseFeatureReporting();
      if (m_phraseLengthFeature && name.compare(m_phraseLengthFeature->GetScoreProducerWeightShortName(0)) == 0)
        m_phraseLengthFeature->SetSparseFeatureReporting();
      if (m_targetWordInsertionFeature && name.compare(m_targetWordInsertionFeature->GetScoreProducerWeightShortName(0)) == 0)
        m_targetWordInsertionFeature->SetSparseFeatureReporting();
      if (m_sourceWordDeletionFeature && name.compare(m_sourceWordDeletionFeature->GetScoreProducerWeightShortName(0)) == 0)
        m_sourceWordDeletionFeature->SetSparseFeatureReporting();
      if (m_wordTranslationFeatures.size() > 0)
      	for (size_t i=0; i < m_wordTranslationFeatures.size(); ++i)
	  if (name.compare(m_wordTranslationFeatures[i]->GetScoreProducerWeightShortName(0)) == 0)
	    m_wordTranslationFeatures[i]->SetSparseFeatureReporting();
      if (m_phrasePairFeatures.size() > 0)
      	for (size_t i=0; i < m_phrasePairFeatures.size(); ++i)
	  if (name.compare(m_phrasePairFeatures[i]->GetScoreProducerWeightShortName(0)) == 0)
	    m_wordTranslationFeatures[i]->SetSparseFeatureReporting();
      for (size_t j = 0; j < m_sparsePhraseDictionary.size(); ++j) {
        if (m_sparsePhraseDictionary[j] && name.compare(m_sparsePhraseDictionary[j]->GetScoreProducerWeightShortName(0)) == 0) {
          m_sparsePhraseDictionary[j]->SetSparseFeatureReporting();          
        }
      }
    }
  }

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

  if (IsChart()) {
    //insert some null distortion score producers
    m_distortionScoreProducers.assign(tsConfig.size(), NULL);
  } else {
    if (m_distortionScoreProducers.size() != tsConfig.size()) {
      UserMessage::Add(string("Mismatch between number of distortion scores and number of translation systems. Or [search-algorithm] has been set to a phrase-based algorithm when it should be chart decoding"));
      return false;
    }
  }

  TranslationSystem* tmpTS;
  for (size_t i = 0; i < tsConfig.size(); ++i) {
    vector<string> config = Tokenize(tsConfig[i]);
    if (config.size() % 2 != 1) {
      UserMessage::Add(string("Incorrect number of fields in Translation System config. Should be an odd number"));
    }
    m_translationSystems.insert(pair<string, TranslationSystem>(config[0],
                                TranslationSystem(config[0],m_wordPenaltyProducers[i],m_unknownWordPenaltyProducer,m_distortionScoreProducers[i])));
    tmpTS = &(m_translationSystems.find(config[0])->second);
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
    if (m_bleuScoreFeature) {
      m_translationSystems.find(config[0])->second.AddFeatureFunction(m_bleuScoreFeature);
    }
    if (m_targetBigramFeature) {
      m_translationSystems.find(config[0])->second.AddFeatureFunction(m_targetBigramFeature);
    }
    if (m_targetNgramFeatures.size() > 0) {
      for (size_t i=0; i < m_targetNgramFeatures.size(); ++i)
    	m_translationSystems.find(config[0])->second.AddFeatureFunction(m_targetNgramFeatures[i]);
    }
    if (m_phraseBoundaryFeature) {
      m_translationSystems.find(config[0])->second.AddFeatureFunction(m_phraseBoundaryFeature);
    }
    if (m_phraseLengthFeature) {
      m_translationSystems.find(config[0])->second.AddFeatureFunction(m_phraseLengthFeature);
    }
    if (m_targetWordInsertionFeature) {
      m_translationSystems.find(config[0])->second.AddFeatureFunction(m_targetWordInsertionFeature);
    }
    if (m_sourceWordDeletionFeature) {
      m_translationSystems.find(config[0])->second.AddFeatureFunction(m_sourceWordDeletionFeature);
    }
    if (m_wordTranslationFeatures.size() > 0) {
      for (size_t i=0; i < m_wordTranslationFeatures.size(); ++i)
    	m_translationSystems.find(config[0])->second.AddFeatureFunction(m_wordTranslationFeatures[i]);
    }
    if (m_phrasePairFeatures.size() > 0) {
      for (size_t i=0; i < m_phrasePairFeatures.size(); ++i)
    	m_translationSystems.find(config[0])->second.AddFeatureFunction(m_phrasePairFeatures[i]);
    }
#ifdef HAVE_SYNLM
    if (m_syntacticLanguageModel != NULL) {
      m_translationSystems.find(config[0])->second.AddFeatureFunction(m_syntacticLanguageModel);
    }
#endif
    for (size_t i = 0; i < m_sparsePhraseDictionary.size(); ++i) {
      if (m_sparsePhraseDictionary[i]) {
        m_translationSystems.find(config[0])->second.AddFeatureFunction(m_sparsePhraseDictionary[i]);
      }
    }
    if (m_globalLexicalModelsUnlimited.size() > 0) {
      for (size_t i=0; i < m_globalLexicalModelsUnlimited.size(); ++i)
    	m_translationSystems.find(config[0])->second.AddFeatureFunction(m_globalLexicalModelsUnlimited[i]);
    }
  }

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

    
    // DLM: apply additional weight to sparse features if applicable
    for (size_t i = 0; i < m_targetNgramFeatures.size(); ++i) {
    	float weight = m_targetNgramFeatures[i]->GetSparseProducerWeight();
    	if (weight != 1) {
	  tmpTS->AddSparseProducer(m_targetNgramFeatures[i]);
	  cerr << "dlm sparse producer weight: " << weight << endl;
    	}
    }

    // GLM: apply additional weight to sparse features if applicable
    for (size_t i = 0; i < m_globalLexicalModelsUnlimited.size(); ++i) {
    	float weight = m_globalLexicalModelsUnlimited[i]->GetSparseProducerWeight();
    	if (weight != 1) {
	  tmpTS->AddSparseProducer(m_globalLexicalModelsUnlimited[i]);
	  cerr << "glm sparse producer weight: " << weight << endl;
    	}
    }

    // WT: apply additional weight to sparse features if applicable
    for (size_t i = 0; i < m_wordTranslationFeatures.size(); ++i) {
      float weight = m_wordTranslationFeatures[i]->GetSparseProducerWeight();
      if (weight != 1) {
	tmpTS->AddSparseProducer(m_wordTranslationFeatures[i]);
	cerr << "wt sparse producer weight: " << weight << endl;
	if (m_mira) 
	  m_metaFeatureProducer = new MetaFeatureProducer("wt");
      }
    }
    
    // PP: apply additional weight to sparse features if applicable
    for (size_t i = 0; i < m_phrasePairFeatures.size(); ++i) {
      float weight = m_phrasePairFeatures[i]->GetSparseProducerWeight();
      if (weight != 1) {
	tmpTS->AddSparseProducer(m_phrasePairFeatures[i]);
	cerr << "pp sparse producer weight: " << weight << endl;
	if (m_mira)
	  m_metaFeatureProducer = new MetaFeatureProducer("pp");
      }
    }
    
    // PB: apply additional weight to sparse features if applicable
    if (m_phraseBoundaryFeature) {
    	float weight = m_phraseBoundaryFeature->GetSparseProducerWeight();
    	if (weight != 1) {
	  tmpTS->AddSparseProducer(m_phraseBoundaryFeature);
	  cerr << "pb sparse producer weight: " << weight << endl;
    	}
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
  RemoveAllInColl(m_sparsePhraseDictionary);
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
  delete m_targetBigramFeature;
  for (size_t i=0; i < m_targetNgramFeatures.size(); ++i)
  	delete m_targetNgramFeatures[i];
  delete m_phraseBoundaryFeature;
  delete m_phraseLengthFeature;
  delete m_targetWordInsertionFeature;
  delete m_sourceWordDeletionFeature;
  for (size_t i=0; i < m_wordTranslationFeatures.size(); ++i)
    delete m_wordTranslationFeatures[i];
  for (size_t i=0; i < m_phrasePairFeatures.size(); ++i)
    delete m_phrasePairFeatures[i];
  for (size_t i=0; i < m_globalLexicalModelsUnlimited.size(); ++i)
  	delete m_globalLexicalModelsUnlimited[i];

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

    m_reorderModels.push_back(new LexicalReordering(input, output, LexicalReorderingConfiguration(modelType), filePath, mweights));
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
    m_globalLexicalModels.push_back( new GlobalLexicalModel( spec[1], inputFactors, outputFactors ) );
    SetWeight(m_globalLexicalModels.back(),weight[i]);
  }
  return true;
}

bool StaticData::LoadGlobalLexicalModelUnlimited()
{
  const vector<float> &weight = Scan<float>(m_parameter->GetParam("weight-glm"));
  const vector<string> &modelSpec = m_parameter->GetParam("glm-feature");

  if (weight.size() != 0 && weight.size() != modelSpec.size()) {
    std::cerr << "number of sparse producer weights and model specs for the global lexical model unlimited "
    		"does not match (" << weight.size() << " != " << modelSpec.size() << ")" << std::endl;
    return false;
  }

  for (size_t i = 0; i < modelSpec.size(); i++ ) {
    bool ignorePunctuation = true, biasFeature = false, restricted = false;
    size_t context = 0;
    string filenameSource, filenameTarget;
    vector< string > factors;
    vector< string > spec = Tokenize(modelSpec[i]," ");

    // read optional punctuation and bias specifications
    if (spec.size() > 0) {
      if (spec.size() != 2 && spec.size() != 3 && spec.size() != 4 && spec.size() != 6) {
      	UserMessage::Add("Format of glm feature is <factor-src>-<factor-tgt> [ignore-punct] [use-bias] "
      			"[context-type] [filename-src filename-tgt]");
				return false;
      }
      
      factors = Tokenize(spec[0],"-");
      if (spec.size() >= 2)
      	ignorePunctuation = Scan<size_t>(spec[1]);
      if (spec.size() >= 3)
      	biasFeature = Scan<size_t>(spec[2]);
      if (spec.size() >= 4)
      	context = Scan<size_t>(spec[3]);
      if (spec.size() == 6) {
      	filenameSource = spec[4];
      	filenameTarget = spec[5];
      	restricted = true;
      }
    }
    else
    	factors = Tokenize(modelSpec[i],"-");

    if ( factors.size() != 2 ) {
      UserMessage::Add("Wrong factor definition for global lexical model unlimited: " + modelSpec[i]);
    	return false;
    }

    const vector<FactorType> inputFactors = Tokenize<FactorType>(factors[0],",");
    const vector<FactorType> outputFactors = Tokenize<FactorType>(factors[1],",");
    throw runtime_error("GlobalLexicalModelUnlimited should be reimplemented as a stateful feature");
    GlobalLexicalModelUnlimited* glmu = NULL; // new GlobalLexicalModelUnlimited(inputFactors, outputFactors, biasFeature, ignorePunctuation, context);
    m_globalLexicalModelsUnlimited.push_back(glmu);
    if (restricted) {
      cerr << "loading word translation word lists from " << filenameSource << " and " << filenameTarget << endl;
      if (!glmu->Load(filenameSource, filenameTarget)) {
        UserMessage::Add("Unable to load word lists for word translation feature from files " + filenameSource + " and " + filenameTarget);
        return false;
      }
    }
    if (weight.size() > i)
      m_globalLexicalModelsUnlimited[i]->SetSparseProducerWeight(weight[i]);
  }
  return true;
}

bool StaticData::LoadLanguageModels()
{
  if (m_parameter->GetParam("lmodel-file").size() > 0) {
    // weights
    vector<float> weightAll = Scan<float>(m_parameter->GetParam("weight-l"));

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
        lm = languageModelsLoaded[lmVector[i]]->Duplicate(); 
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
               , LMdub[i]);
        if (lm == NULL) {
          UserMessage::Add("no LM created. We probably don't have it compiled");
          return false;
        }
        languageModelsLoaded[lmVector[i]] = lm;
      }

      m_languageModel.Add(lm);
      if (m_lmEnableOOVFeature) {
        vector<float> weights(2);
        weights[0] = weightAll.at(i*2);
        weights[1] = weightAll.at(i*2+1);
        SetWeights(lm,weights);
      } else {
        SetWeight(lm,weightAll[i]);
      }
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

      m_generationDictionary.push_back(new GenerationDictionary(numFeatures, input,output));
      CHECK(m_generationDictionary.back() && "could not create GenerationDictionary");
      if (!m_generationDictionary.back()->Load(filePath, Output)) {
        delete m_generationDictionary.back();
        return false;
      }
      vector<float> gdWeights;
      for(size_t i = 0; i < numFeatures; i++) {
        CHECK(currWeightNum < weight.size());
        gdWeights.push_back(weight[currWeightNum++]);
      }
      SetWeights(m_generationDictionary.back(), gdWeights);
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

      CHECK(numScoreComponent==weight.size());


      //This is needed for regression testing, but the phrase table
      //might not really be loading here
      IFVERBOSE(1)
      PrintUserTime(string("Start loading PhraseTable ") + filePath);
      VERBOSE(1,"filePath: " << filePath <<endl);

      //optional create sparse phrase feature
      SparsePhraseDictionaryFeature* spdf = NULL; 
      if (token.size() >= 6 && token[5] == "sparse") {
          spdf = new SparsePhraseDictionaryFeature();
      }
      m_sparsePhraseDictionary.push_back(spdf);


      PhraseDictionaryFeature* pdf = new PhraseDictionaryFeature(
        implementation
        , spdf
        , numScoreComponent
        , (currDict==0 ? m_numInputScores : 0)
        , input
        , output
        , filePath
        , weight
       	, currDict
        , maxTargetPhrase[index]
        , token);

      m_phraseDictionary.push_back(pdf);

      SetWeights(m_phraseDictionary.back(),weight);




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
    m_distortionScoreProducers.push_back(new DistortionScoreProducer());
    SetWeight(m_distortionScoreProducers.back(), weightDistortion);
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

bool StaticData::LoadReferences()
{
  vector<string> bleuWeightStr = m_parameter->GetParam("weight-bl");
  vector<string> referenceFiles = m_parameter->GetParam("references");
  if ((!referenceFiles.size() && bleuWeightStr.size()) || (referenceFiles.size() && !bleuWeightStr.size())) {
    UserMessage::Add("You cannot use the bleu feature without references, and vice-versa");
    return false;
  }
  if (!referenceFiles.size()) {
    return true;
  }
  if (bleuWeightStr.size() > 1) {
    UserMessage::Add("Can only specify one weight for the bleu feature");
    return false;
  }

  float bleuWeight = Scan<float>(bleuWeightStr[0]);
  m_bleuScoreFeature = new BleuScoreFeature();
  SetWeight(m_bleuScoreFeature, bleuWeight);

  cerr << "Loading reference file " << referenceFiles[0] << endl;
  vector<vector<string> > references(referenceFiles.size());
  for (size_t i =0; i < referenceFiles.size(); ++i) {
    ifstream in(referenceFiles[i].c_str());
    if (!in) {
      stringstream strme;
      strme << "Unable to load references from " << referenceFiles[i];
      UserMessage::Add(strme.str());
      return false;
    }
    string line;
    while (getline(in,line)) {
/*      if (GetSearchAlgorithm() == ChartDecoding) {
    	stringstream tmp;
    	tmp << "<s> " << line << " </s>";
    	line = tmp.str();
      }*/
      references[i].push_back(line);
    }
    if (i > 0) {
      if (references[i].size() != references[i-1].size()) {
        UserMessage::Add("Reference files are of different lengths");
        return false;
      }
    }
    in.close();
  }
  //Set the references in the bleu feature
  m_bleuScoreFeature->LoadReferences(references);
  return true;
}

bool StaticData::LoadDiscrimLMFeature()
{
	// only load if specified
  const vector<string> &wordFile = m_parameter->GetParam("dlm-model");
  if (wordFile.empty()) {
    return true;
  }
  cerr << "Loading " << wordFile.size() << " discriminative language model(s).." << endl;

  // if this weight is specified, the sparse DLM weights will be scaled with an additional weight
  vector<string> dlmWeightStr = m_parameter->GetParam("weight-dlm");
  vector<float> dlmWeights;
  for (size_t i=0; i<dlmWeightStr.size(); ++i)
  	dlmWeights.push_back(Scan<float>(dlmWeightStr[i]));

  for (size_t i = 0; i < wordFile.size(); ++i) {
  	vector<string> tokens = Tokenize(wordFile[i]);
  	if (tokens.size() != 4) {
  		UserMessage::Add("Format of discriminative language model parameter is <order> <factor> <include-lower-ngrams> <filename>");
  		return false;
  	}

  	size_t order = Scan<size_t>(tokens[0]);
  	FactorType factorId = Scan<size_t>(tokens[1]);
  	bool include_lower_ngrams = Scan<bool>(tokens[2]);
  	string filename = tokens[3];

  	if (order == 2 && !include_lower_ngrams) { // TODO: remove TargetBigramFeature ?
  		m_targetBigramFeature = new TargetBigramFeature(factorId);
  		cerr << "loading vocab from " << filename << endl;
  		if (!m_targetBigramFeature->Load(filename)) {
  			UserMessage::Add("Unable to load word list from file " + filename);
  			return false;
  		}
  	}
  	else {
  		if (m_searchAlgorithm == ChartDecoding && !include_lower_ngrams) {
  			UserMessage::Add("Excluding lower order DLM ngrams is currently not supported for chart decoding.");
  			return false;
  		}

  		m_targetNgramFeatures.push_back(new TargetNgramFeature(factorId, order, include_lower_ngrams));
  		if (i < dlmWeights.size())
  			m_targetNgramFeatures[i]->SetSparseProducerWeight(dlmWeights[i]);
  		cerr << "loading vocab from " << filename << endl;
  		if (!m_targetNgramFeatures[i]->Load(filename)) {
  			UserMessage::Add("Unable to load word list from file " + filename);
  			return false;
  		}
  	}
  }

  return true;
}

bool StaticData::LoadPhraseBoundaryFeature()
{
  const vector<float> &weight = Scan<float>(m_parameter->GetParam("weight-pb"));
  if (weight.size() > 1) {
	std::cerr << "Only one sparse producer weight allowed for the phrase boundary feature" << std::endl;
    return false;
  }

  const vector<string> &phraseBoundarySourceFactors =
    m_parameter->GetParam("phrase-boundary-source-feature");
  const vector<string> &phraseBoundaryTargetFactors =
    m_parameter->GetParam("phrase-boundary-target-feature");
  if (phraseBoundarySourceFactors.size() == 0 && phraseBoundaryTargetFactors.size() == 0) {
    return true;
  }
  if (phraseBoundarySourceFactors.size() > 1) {
    UserMessage::Add("Need to specify comma separated list of source factors for phrase boundary");
    return false;
  }
  if (phraseBoundaryTargetFactors.size() > 1) {
    UserMessage::Add("Need to specify comma separated list of target factors for phrase boundary");
    return false;
  }
  FactorList sourceFactors;
  FactorList targetFactors;
  if (phraseBoundarySourceFactors.size()) {
    sourceFactors = Tokenize<FactorType>(phraseBoundarySourceFactors[0],",");
  }
  if (phraseBoundaryTargetFactors.size()) {
    targetFactors = Tokenize<FactorType>(phraseBoundaryTargetFactors[0],",");
  }
  //cerr << "source "; for (size_t i = 0; i < sourceFactors.size(); ++i) cerr << sourceFactors[i] << " "; cerr << endl;
  //cerr << "target "; for (size_t i = 0; i < targetFactors.size(); ++i) cerr << targetFactors[i] << " "; cerr << endl;
  m_phraseBoundaryFeature = new PhraseBoundaryFeature(sourceFactors,targetFactors);
  if (weight.size() > 0)
    m_phraseBoundaryFeature->SetSparseProducerWeight(weight[0]);
  return true;
}

bool StaticData::LoadPhrasePairFeature()
{
  const vector<float> &weight = Scan<float>(m_parameter->GetParam("weight-pp"));
  if (weight.size() > 1) {
	std::cerr << "Only one sparse producer weight allowed for the phrase pair feature" << std::endl;
	return false;
  }

  const vector<string> &parameters = m_parameter->GetParam("phrase-pair-feature");
  if (parameters.size() == 0) return true;
 
  for (size_t i=0; i<parameters.size(); ++i) {
    vector<string> tokens = Tokenize(parameters[i]);
    if (! (tokens.size() >= 1  && tokens.size() <= 6)) {
      UserMessage::Add("Format for phrase pair feature: --phrase-pair-feature <factor-src>-<factor-tgt> "
		       "[simple source-trigger] [ignore-punctuation] [domain-trigger] [filename-src]");
      return false;
    }
  
    vector <string> factors;
    if (tokens.size() == 2)
      factors = Tokenize(tokens[0]," ");  
    else 
      factors = Tokenize(tokens[0],"-");
    
    size_t sourceFactorId = Scan<size_t>(factors[0]);
    size_t targetFactorId = Scan<size_t>(factors[1]);
    bool simple = true, sourceContext = false, ignorePunctuation = false, domainTrigger = false;
    if (tokens.size() >= 3) {
      simple = Scan<size_t>(tokens[1]);
      sourceContext = Scan<size_t>(tokens[2]);
    }
    if (tokens.size() >= 4) 
      ignorePunctuation = Scan<size_t>(tokens[3]);
    if (tokens.size() >= 5)
      domainTrigger = Scan<size_t>(tokens[4]);
    
    m_phrasePairFeatures.push_back(new PhrasePairFeature(sourceFactorId, targetFactorId, simple, sourceContext, 
							 ignorePunctuation, domainTrigger));
    if (weight.size() > i)
      m_phrasePairFeatures[i]->SetSparseProducerWeight(weight[i]);
    
    // load word list 
    if (tokens.size() == 6) {
      string filenameSource = tokens[5];
      if (domainTrigger) {
	const vector<string> &texttype = m_parameter->GetParam("text-type");
	if (texttype.size() != 1) {
	  UserMessage::Add("Need texttype to load dictionary for domain triggers.");
	  return false;
	}
	stringstream filename(filenameSource + "." + texttype[0]);
	filenameSource = filename.str();
	cerr << "loading word translation term list from " << filenameSource << endl;
      }
      else {
	cerr << "loading word translation word list from " << filenameSource << endl;
      }
      if (!m_phrasePairFeatures[i]->Load(filenameSource)) {
	UserMessage::Add("Unable to load word lists for word translation feature from files " + filenameSource);
	return false;
      }
    }
  }
  return true;
}

bool StaticData::LoadPhraseLengthFeature()
{
  if (m_parameter->isParamSpecified("phrase-length-feature")) {
    m_phraseLengthFeature = new PhraseLengthFeature();
  }
  return true;
}

bool StaticData::LoadTargetWordInsertionFeature()
{
  const vector<string> &parameters = m_parameter->GetParam("target-word-insertion-feature");
  if (parameters.empty())
    return true;

  if (parameters.size() != 1) {
    UserMessage::Add("Can only have one target-word-insertion-feature");
    return false;
  }

  vector<string> tokens = Tokenize(parameters[0]);
  if (tokens.size() != 1 && tokens.size() != 2) {
    UserMessage::Add("Format of target word insertion feature parameter is: --target-word-insertion-feature <factor> [filename]");
    return false;
  }

  m_needAlignmentInfo = true;

  // set factor
  FactorType factorId = Scan<size_t>(tokens[0]);
  m_targetWordInsertionFeature = new TargetWordInsertionFeature(factorId);

  // load word list for restricted feature set
  if (tokens.size() == 2) {
    string filename = tokens[1];
    cerr << "loading target word insertion word list from " << filename << endl;
    if (!m_targetWordInsertionFeature->Load(filename)) {
      UserMessage::Add("Unable to load word list for target word insertion feature from file " + filename);
      return false;
    }
  }

  return true;
}

bool StaticData::LoadSourceWordDeletionFeature()
{
  const vector<string> &parameters = m_parameter->GetParam("source-word-deletion-feature");
  if (parameters.empty())
    return true;

  if (parameters.size() != 1) {
    UserMessage::Add("Can only have one source-word-deletion-feature");
    return false;
  }

  vector<string> tokens = Tokenize(parameters[0]);
  if (tokens.size() != 1 && tokens.size() != 2) {
    UserMessage::Add("Format of source word deletion feature parameter is: --source-word-deletion-feature <factor> [filename]");
    return false;
  }

  m_needAlignmentInfo = true;

  // set factor
  FactorType factorId = Scan<size_t>(tokens[0]);
  m_sourceWordDeletionFeature = new SourceWordDeletionFeature(factorId);

  // load word list for restricted feature set
  if (tokens.size() == 2) {
    string filename = tokens[1];
    cerr << "loading source word deletion word list from " << filename << endl;
    if (!m_sourceWordDeletionFeature->Load(filename)) {
      UserMessage::Add("Unable to load word list for source word deletion feature from file " + filename);
      return false;
    }
  }

  return true;
}

bool StaticData::LoadWordTranslationFeature()
{
  const vector<string> &parameters = m_parameter->GetParam("word-translation-feature");
  if (parameters.empty())
    return true;

  const vector<float> &weight = Scan<float>(m_parameter->GetParam("weight-wt"));
  if (weight.size() > 1) {
    std::cerr << "Only one sparse producer weight allowed for the word translation feature" << std::endl;
    return false;
  }
	
  m_needAlignmentInfo = true;

  for (size_t i=0; i<parameters.size(); ++i) {
    vector<string> tokens = Tokenize(parameters[i]);
    if (tokens.size() != 1 &&  !(tokens.size() >= 4 && tokens.size() <= 8)) {
      UserMessage::Add("Format of word translation feature parameter is: --word-translation-feature <factor-src>-<factor-tgt> "
		       "[simple source-trigger target-trigger] [ignore-punctuation] [domain-trigger] [filename-src] [filename-tgt]");
      return false;
    }
    
    // set factor
    vector <string> factors = Tokenize(tokens[0],"-");
    FactorType factorIdSource = Scan<size_t>(factors[0]);
    FactorType factorIdTarget = Scan<size_t>(factors[1]);
    
    bool simple = true, sourceTrigger = false, targetTrigger = false, ignorePunctuation = false, domainTrigger = false;
    if (tokens.size() >= 4) {
      simple = Scan<size_t>(tokens[1]);
      sourceTrigger = Scan<size_t>(tokens[2]);
      targetTrigger = Scan<size_t>(tokens[3]);
    }
    if (tokens.size() >= 5) {
      ignorePunctuation = Scan<size_t>(tokens[4]);
    }
    
    if (tokens.size() >= 6) {
      domainTrigger = Scan<size_t>(tokens[5]);
    }
    
    m_wordTranslationFeatures.push_back(new WordTranslationFeature(factorIdSource, factorIdTarget, simple,
							sourceTrigger, targetTrigger, ignorePunctuation, domainTrigger));
    if (weight.size() > i)
      m_wordTranslationFeatures[i]->SetSparseProducerWeight(weight[i]);
    
    // load word list for restricted feature set
    if (tokens.size() == 7) {
      string filenameSource = tokens[6];
      if (domainTrigger) {
	const vector<string> &texttype = m_parameter->GetParam("text-type");
	if (texttype.size() != 1) {
	  UserMessage::Add("Need texttype to load dictionary for domain triggers.");
	  return false;
	}
	stringstream filename(filenameSource + "." + texttype[0]);
	filenameSource = filename.str();    
	cerr << "loading word translation term list from " << filenameSource << endl;
      }
      else {
	cerr << "loading word translation word lists from " << filenameSource << endl;
      }
      if (!m_wordTranslationFeatures[i]->Load(filenameSource, "")) {
	UserMessage::Add("Unable to load word lists for word translation feature from files " + filenameSource);
	return false;
      }
    }
    else if (tokens.size() == 8) {
      string filenameSource = tokens[6];
      string filenameTarget = tokens[7];
      cerr << "loading word translation word lists from " << filenameSource << " and " << filenameTarget << endl;
      if (!m_wordTranslationFeatures[i]->Load(filenameSource, filenameTarget)) {
	UserMessage::Add("Unable to load word lists for word translation feature from files " + filenameSource + " and " + filenameTarget);
	return false;
      }
    }
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

}

void StaticData::ReLoadBleuScoreFeatureParameter(float weight)
{
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

}


