// $Id$

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

#ifndef moses_StaticData_h
#define moses_StaticData_h

#include <stdexcept>
#include <limits>
#include <list>
#include <vector>
#include <map>
#include <memory>
#include <utility>
#include <fstream>
#include <string>
#include "UserMessage.h"

#ifdef WITH_THREADS
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#endif

#include "Parameter.h"
#include "SentenceStats.h"
#include "ScoreComponentCollection.h"
#include "moses/FF/Factory.h"

namespace Moses
{

class InputType;
class DecodeGraph;
class DecodeStep;
class WordPenaltyProducer;
class UnknownWordPenaltyProducer;
class InputFeature;

typedef std::pair<std::string, float> UnknownLHSEntry;
typedef std::vector<UnknownLHSEntry>  UnknownLHSList;

/** Contains global variables and contants.
 *  Only 1 object of this class should be instantiated.
 *  A const object of this class is accessible by any function during decoding by calling StaticData::Instance();
 */
class StaticData
{
private:
  static StaticData									s_instance;
protected:
  Parameter *m_parameter;
  std::vector<FactorType>	m_inputFactorOrder, m_outputFactorOrder;
  mutable ScoreComponentCollection m_allWeights;

  std::vector<DecodeGraph*> m_decodeGraphs;
  std::vector<size_t> m_decodeGraphBackoff;
  // Initial	= 0 = can be used when creating poss trans
  // Other		= 1 = used to calculate LM score once all steps have been processed
  float
  m_beamWidth,
  m_earlyDiscardingThreshold,
  m_translationOptionThreshold,
  m_wordDeletionWeight;


  // PhraseTrans, Generation & LanguageModelScore has multiple weights.
  int				m_maxDistortion;
  // do it differently from old pharaoh
  // -ve	= no limit on distortion
  // 0		= no disortion (monotone in old pharaoh)
  bool m_reorderingConstraint; //! use additional reordering constraints
  bool m_useEarlyDistortionCost;
  size_t
  m_maxHypoStackSize //! hypothesis-stack size that triggers pruning
  , m_minHypoStackDiversity //! minimum number of hypothesis in stack for each source word coverage
  , m_nBestSize
  , m_latticeSamplesSize
  , m_nBestFactor
  , m_maxNoTransOptPerCoverage
  , m_maxNoPartTransOpt
  , m_maxPhraseLength;

  std::string									m_nBestFilePath, m_latticeSamplesFilePath;
  bool                        m_labeledNBestList,m_nBestIncludesSegmentation;
  bool m_dropUnknown; //! false = treat unknown words as unknowns, and translate them as themselves; true = drop (ignore) them
  bool m_markUnknown; //! false = treat unknown words as unknowns, and translate them as themselves; true = mark and (ignore) them
  bool m_wordDeletionEnabled;

  bool m_disableDiscarding;
  bool m_printAllDerivations;

  bool m_sourceStartPosMattersForRecombination;
  bool m_recoverPath;
  bool m_outputHypoScore;

  SearchAlgorithm m_searchAlgorithm;
  InputTypeEnum m_inputType;

  mutable size_t m_verboseLevel;
  WordPenaltyProducer* m_wpProducer;
  UnknownWordPenaltyProducer *m_unknownWordPenaltyProducer;
  const InputFeature *m_inputFeature;

  bool m_reportSegmentation;
  bool m_reportSegmentationEnriched;
  bool m_reportAllFactors;
  bool m_reportAllFactorsNBest;
  std::string m_detailedTranslationReportingFilePath;
  std::string m_detailedTreeFragmentsTranslationReportingFilePath;

  //DIMw
  std::string m_detailedAllTranslationReportingFilePath;

  bool m_onlyDistinctNBest;
  bool m_PrintAlignmentInfo;
  bool m_needAlignmentInfo;
  bool m_PrintAlignmentInfoNbest;

  std::string m_alignmentOutputFile;

  std::string m_factorDelimiter; //! by default, |, but it can be changed

  XmlInputType m_xmlInputType; //! method for handling sentence XML input
  std::pair<std::string,std::string> m_xmlBrackets; //! strings to use as XML tags' opening and closing brackets. Default are "<" and ">"

  bool m_mbr; //! use MBR decoder
  bool m_useLatticeMBR; //! use MBR decoder
  bool m_mira; // do mira training
  bool m_useConsensusDecoding; //! Use Consensus decoding  (DeNero et al 2009)
  size_t m_mbrSize; //! number of translation candidates considered
  float m_mbrScale; //! scaling factor for computing marginal probability of candidate translation
  size_t m_lmbrPruning; //! average number of nodes per word wanted in pruned lattice
  std::vector<float> m_lmbrThetas; //! theta(s) for lattice mbr calculation
  bool m_useLatticeHypSetForLatticeMBR; //! to use nbest as hypothesis set during lattice MBR
  float m_lmbrPrecision; //! unigram precision theta - see Tromble et al 08 for more details
  float m_lmbrPRatio; //! decaying factor for ngram thetas - see Tromble et al 08 for more details
  float m_lmbrMapWeight; //! Weight given to the map solution. See Kumar et al 09 for details

  size_t m_lmcache_cleanup_threshold; //! number of translations after which LM claenup is performed (0=never, N=after N translations; default is 1)
  bool m_lmEnableOOVFeature;

  bool m_timeout; //! use timeout
  size_t m_timeout_threshold; //! seconds after which time out is activated

  bool m_isAlwaysCreateDirectTranslationOption;
  //! constructor. only the 1 static variable can be created

  bool m_outputWordGraph; //! whether to output word graph
  bool m_outputSearchGraph; //! whether to output search graph
  bool m_outputSearchGraphExtended; //! ... in extended format
  bool m_outputSearchGraphSLF; //! whether to output search graph in HTK standard lattice format (SLF)
  bool m_outputSearchGraphHypergraph; //! whether to output search graph in hypergraph
#ifdef HAVE_PROTOBUF
  bool m_outputSearchGraphPB; //! whether to output search graph as a protobuf
#endif
  bool m_unprunedSearchGraph; //! do not exclude dead ends (chart decoder only)
  bool m_includeLHSInSearchGraph; //! include LHS of rules in search graph
  std::string m_outputUnknownsFile; //! output unknowns in this file

  size_t m_cubePruningPopLimit;
  size_t m_cubePruningDiversity;
  bool m_cubePruningLazyScoring;
  size_t m_ruleLimit;

  // Whether to load compact phrase table and reordering table into memory
  bool m_minphrMemory;
  bool m_minlexrMemory;

  // Initial = 0 = can be used when creating poss trans
  // Other = 1 = used to calculate LM score once all steps have been processed
  Word m_inputDefaultNonTerminal, m_outputDefaultNonTerminal;
  SourceLabelOverlap m_sourceLabelOverlap;
  UnknownLHSList m_unknownLHS;
  WordAlignmentSort m_wordAlignmentSort;

  int m_threadCount;
  long m_startTranslationId;

  // alternate weight settings
  mutable std::string m_currentWeightSetting;
  std::map< std::string, ScoreComponentCollection* > m_weightSetting; // core weights
  std::map< std::string, std::set< std::string > > m_weightSettingIgnoreFF; // feature function
  std::map< std::string, std::set< size_t > > m_weightSettingIgnoreDP; // decoding path

  FactorType m_placeHolderFactor;
  bool m_useLegacyPT;

  FeatureRegistry m_registry;

  StaticData();

  void LoadChartDecodingParameters();
  void LoadNonTerminals();

  //! helper fn to set bool param from ini file/command line
  void SetBooleanParameter(bool *paramter, std::string parameterName, bool defaultValue);

  //! load decoding steps
  bool LoadDecodeGraphs();

  void NoCache();

  bool m_continuePartialTranslation;
  std::string m_binPath;


public:

  bool IsAlwaysCreateDirectTranslationOption() const {
    return m_isAlwaysCreateDirectTranslationOption;
  }
  //! destructor
  ~StaticData();

  //! return static instance for use like global variable
  static const StaticData& Instance() {
    return s_instance;
  }

  //! do NOT call unless you know what you're doing
  static StaticData& InstanceNonConst() {
    return s_instance;
  }

  /** delete current static instance and replace with another.
  	* Used by gui front end
  	*/
#ifdef WIN32
  static void Reset() {
    s_instance = StaticData();
  }
#endif

  //! Load data into static instance. This function is required as LoadData() is not const
  static bool LoadDataStatic(Parameter *parameter, const std::string &execPath);

  //! Main function to load everything. Also initialize the Parameter object
  bool LoadData(Parameter *parameter);
  void ClearData();

  const PARAM_VEC &GetParam(const std::string &paramName) const {
    return m_parameter->GetParam(paramName);
  }

  const std::vector<FactorType> &GetInputFactorOrder() const {
    return m_inputFactorOrder;
  }
  const std::vector<FactorType> &GetOutputFactorOrder() const {
    return m_outputFactorOrder;
  }

  inline bool GetSourceStartPosMattersForRecombination() const {
    return m_sourceStartPosMattersForRecombination;
  }
  inline bool GetDropUnknown() const {
    return m_dropUnknown;
  }
  inline bool GetMarkUnknown() const {
    return m_markUnknown;
  }
  inline bool GetDisableDiscarding() const {
    return m_disableDiscarding;
  }
  inline size_t GetMaxNoTransOptPerCoverage() const {
    return m_maxNoTransOptPerCoverage;
  }
  inline size_t GetMaxNoPartTransOpt() const {
    return m_maxNoPartTransOpt;
  }
  inline size_t GetMaxPhraseLength() const {
    return m_maxPhraseLength;
  }
  bool IsWordDeletionEnabled() const {
    return m_wordDeletionEnabled;
  }
  size_t GetMaxHypoStackSize() const {
    return m_maxHypoStackSize;
  }
  size_t GetMinHypoStackDiversity() const {
    return m_minHypoStackDiversity;
  }
  size_t GetCubePruningPopLimit() const {
    return m_cubePruningPopLimit;
  }
  size_t GetCubePruningDiversity() const {
    return m_cubePruningDiversity;
  }
  bool GetCubePruningLazyScoring() const {
    return m_cubePruningLazyScoring;
  }
  size_t IsPathRecoveryEnabled() const {
    return m_recoverPath;
  }
  int GetMaxDistortion() const {
    return m_maxDistortion;
  }
  bool UseReorderingConstraint() const {
    return m_reorderingConstraint;
  }
  float GetBeamWidth() const {
    return m_beamWidth;
  }
  float GetEarlyDiscardingThreshold() const {
    return m_earlyDiscardingThreshold;
  }
  bool UseEarlyDiscarding() const {
    return m_earlyDiscardingThreshold != -std::numeric_limits<float>::infinity();
  }
  bool UseEarlyDistortionCost() const {
    return m_useEarlyDistortionCost;
  }
  float GetTranslationOptionThreshold() const {
    return m_translationOptionThreshold;
  }

  size_t GetVerboseLevel() const {
    return m_verboseLevel;
  }
  void SetVerboseLevel(int x) const {
    m_verboseLevel = x;
  }
  char GetReportSegmentation() const {
    if (m_reportSegmentation) return 1;
    if (m_reportSegmentationEnriched) return 2;
    return 0;
  }
  bool GetReportAllFactors() const {
    return m_reportAllFactors;
  }
  bool GetReportAllFactorsNBest() const {
    return m_reportAllFactorsNBest;
  }
  bool IsDetailedTranslationReportingEnabled() const {
    return !m_detailedTranslationReportingFilePath.empty();
  }

  bool IsDetailedAllTranslationReportingEnabled() const {
    return !m_detailedAllTranslationReportingFilePath.empty();
  }

  const std::string &GetDetailedTranslationReportingFilePath() const {
    return m_detailedTranslationReportingFilePath;
  }
  bool IsDetailedTreeFragmentsTranslationReportingEnabled() const {
    return !m_detailedTreeFragmentsTranslationReportingFilePath.empty();
  }
  const std::string &GetDetailedTreeFragmentsTranslationReportingFilePath() const {
    return m_detailedTreeFragmentsTranslationReportingFilePath;
  }
  bool IsLabeledNBestList() const {
    return m_labeledNBestList;
  }

  bool UseMinphrInMemory() const {
    return m_minphrMemory;
  }

  bool UseMinlexrInMemory() const {
    return m_minlexrMemory;
  }

  const std::vector<std::string> &GetDescription() const {
    return m_parameter->GetParam("description");
  }

  // for mert
  size_t GetNBestSize() const {
    return m_nBestSize;
  }
  const std::string &GetNBestFilePath() const {
    return m_nBestFilePath;
  }
  bool IsNBestEnabled() const {
    return (!m_nBestFilePath.empty()) || m_mbr || m_useLatticeMBR || m_mira || m_outputSearchGraph || m_outputSearchGraphSLF || m_outputSearchGraphHypergraph || m_useConsensusDecoding || !m_latticeSamplesFilePath.empty()
#ifdef HAVE_PROTOBUF
           || m_outputSearchGraphPB
#endif
           ;
  }
  size_t GetLatticeSamplesSize() const {
    return m_latticeSamplesSize;
  }

  const std::string& GetLatticeSamplesFilePath() const {
    return m_latticeSamplesFilePath;
  }

  size_t GetNBestFactor() const {
    return m_nBestFactor;
  }
  bool GetOutputWordGraph() const {
    return m_outputWordGraph;
  }

  //! Sets the global score vector weights for a given FeatureFunction.
  InputTypeEnum GetInputType() const {
    return m_inputType;
  }
  SearchAlgorithm GetSearchAlgorithm() const {
    return m_searchAlgorithm;
  }
  bool IsChart() const {
    return m_searchAlgorithm == ChartDecoding || m_searchAlgorithm == ChartIncremental;
  }
  WordPenaltyProducer *GetWordPenaltyProducer() { // for mira
    return m_wpProducer;
  }

  const UnknownWordPenaltyProducer *GetUnknownWordPenaltyProducer() const {
    return m_unknownWordPenaltyProducer;
  }

  const InputFeature *GetInputFeature() const {
    return m_inputFeature;
  }

  const ScoreComponentCollection& GetAllWeights() const {
    return m_allWeights;
  }

  void SetAllWeights(const ScoreComponentCollection& weights) {
    m_allWeights = weights;
  }

  //Weight for a single-valued feature
  float GetWeight(const FeatureFunction* sp) const {
    return m_allWeights.GetScoreForProducer(sp);
  }

  //Weight for a single-valued feature
  void SetWeight(const FeatureFunction* sp, float weight) ;


  //Weights for feature with fixed number of values
  std::vector<float> GetWeights(const FeatureFunction* sp) const {
    return m_allWeights.GetScoresForProducer(sp);
  }

  float GetSparseWeight(const FName& featureName) const {
    return m_allWeights.GetSparseWeight(featureName);
  }

  //Weights for feature with fixed number of values
  void SetWeights(const FeatureFunction* sp, const std::vector<float>& weights);

  bool GetDistinctNBest() const {
    return m_onlyDistinctNBest;
  }
  const std::string& GetFactorDelimiter() const {
    return m_factorDelimiter;
  }
  bool UseMBR() const {
    return m_mbr;
  }
  bool UseLatticeMBR() const {
    return m_useLatticeMBR ;
  }
  bool UseConsensusDecoding() const {
    return m_useConsensusDecoding;
  }
  void SetUseLatticeMBR(bool flag) {
    m_useLatticeMBR = flag;
  }
  size_t GetMBRSize() const {
    return m_mbrSize;
  }
  float GetMBRScale() const {
    return m_mbrScale;
  }
  void SetMBRScale(float scale) {
    m_mbrScale = scale;
  }
  size_t GetLatticeMBRPruningFactor() const {
    return m_lmbrPruning;
  }
  void SetLatticeMBRPruningFactor(size_t prune) {
    m_lmbrPruning = prune;
  }
  const std::vector<float>& GetLatticeMBRThetas() const {
    return m_lmbrThetas;
  }
  bool  UseLatticeHypSetForLatticeMBR() const {
    return m_useLatticeHypSetForLatticeMBR;
  }
  float GetLatticeMBRPrecision() const {
    return m_lmbrPrecision;
  }
  void SetLatticeMBRPrecision(float p) {
    m_lmbrPrecision = p;
  }
  float GetLatticeMBRPRatio() const {
    return m_lmbrPRatio;
  }
  void SetLatticeMBRPRatio(float r) {
    m_lmbrPRatio = r;
  }

  float GetLatticeMBRMapWeight() const {
    return m_lmbrMapWeight;
  }

  bool UseTimeout() const {
    return m_timeout;
  }
  size_t GetTimeoutThreshold() const {
    return m_timeout_threshold;
  }

  size_t GetLMCacheCleanupThreshold() const {
    return m_lmcache_cleanup_threshold;
  }

  bool GetLMEnableOOVFeature() const {
    return m_lmEnableOOVFeature;
  }

  bool GetOutputSearchGraph() const {
    return m_outputSearchGraph;
  }
  void SetOutputSearchGraph(bool outputSearchGraph) {
    m_outputSearchGraph = outputSearchGraph;
  }
  bool GetOutputSearchGraphExtended() const {
    return m_outputSearchGraphExtended;
  }
  bool GetOutputSearchGraphSLF() const {
    return m_outputSearchGraphSLF;
  }
  bool GetOutputSearchGraphHypergraph() const {
    return m_outputSearchGraphHypergraph;
  }
#ifdef HAVE_PROTOBUF
  bool GetOutputSearchGraphPB() const {
    return m_outputSearchGraphPB;
  }
#endif
  const std::string& GetOutputUnknownsFile() const {
    return m_outputUnknownsFile;
  }

  bool GetUnprunedSearchGraph() const {
    return m_unprunedSearchGraph;
  }

  bool GetIncludeLHSInSearchGraph() const {
    return m_includeLHSInSearchGraph;
  }

  XmlInputType GetXmlInputType() const {
    return m_xmlInputType;
  }

  std::pair<std::string,std::string> GetXmlBrackets() const {
    return m_xmlBrackets;
  }

  bool PrintAllDerivations() const {
    return m_printAllDerivations;
  }

  const UnknownLHSList &GetUnknownLHS() const {
    return m_unknownLHS;
  }

  const Word &GetInputDefaultNonTerminal() const {
    return m_inputDefaultNonTerminal;
  }
  const Word &GetOutputDefaultNonTerminal() const {
    return m_outputDefaultNonTerminal;
  }

  SourceLabelOverlap GetSourceLabelOverlap() const {
    return m_sourceLabelOverlap;
  }

  bool GetOutputHypoScore() const {
    return m_outputHypoScore;
  }
  size_t GetRuleLimit() const {
    return m_ruleLimit;
  }
  float GetRuleCountThreshold() const {
    return 999999; /* TODO wtf! */
  }

  bool ContinuePartialTranslation() const {
    return m_continuePartialTranslation;
  }

  void ReLoadParameter();
  void ReLoadBleuScoreFeatureParameter(float weight);

  Parameter* GetParameter() {
    return m_parameter;
  }

  int ThreadCount() const {
    return m_threadCount;
  }

  long GetStartTranslationId() const {
    return m_startTranslationId;
  }

  void SetExecPath(const std::string &path);
  const std::string &GetBinDirectory() const;

  bool NeedAlignmentInfo() const {
    return m_needAlignmentInfo;
  }
  const std::string &GetAlignmentOutputFile() const {
    return m_alignmentOutputFile;
  }
  bool PrintAlignmentInfo() const {
    return m_PrintAlignmentInfo;
  }
  bool PrintAlignmentInfoInNbest() const {
    return m_PrintAlignmentInfoNbest;
  }
  WordAlignmentSort GetWordAlignmentSort() const {
    return m_wordAlignmentSort;
  }

  bool NBestIncludesSegmentation() const {
    return m_nBestIncludesSegmentation;
  }

  bool GetHasAlternateWeightSettings() const {
    return m_weightSetting.size() > 0;
  }

  /** Alternate weight settings allow the wholesale ignoring of
      feature functions. This function checks if a feature function
      should be evaluated given the current weight setting */
  bool IsFeatureFunctionIgnored( const FeatureFunction &ff ) const {
    if (!GetHasAlternateWeightSettings()) {
      return false;
    }
    std::map< std::string, std::set< std::string > >::const_iterator lookupIgnoreFF
    =  m_weightSettingIgnoreFF.find( m_currentWeightSetting );
    if (lookupIgnoreFF == m_weightSettingIgnoreFF.end()) {
      return false;
    }
    const std::string &ffName = ff.GetScoreProducerDescription();
    const std::set< std::string > &ignoreFF = lookupIgnoreFF->second;
    return ignoreFF.count( ffName );
  }

  /** Alternate weight settings allow the wholesale ignoring of
      decoding graphs (typically a translation table). This function
      checks if a feature function should be evaluated given the
      current weight setting */
  bool IsDecodingGraphIgnored( const size_t id ) const {
    if (!GetHasAlternateWeightSettings()) {
      return false;
    }
    std::map< std::string, std::set< size_t > >::const_iterator lookupIgnoreDP
    =  m_weightSettingIgnoreDP.find( m_currentWeightSetting );
    if (lookupIgnoreDP == m_weightSettingIgnoreDP.end()) {
      return false;
    }
    const std::set< size_t > &ignoreDP = lookupIgnoreDP->second;
    return ignoreDP.count( id );
  }

  /** process alternate weight settings
    * (specified with [alternate-weight-setting] in config file) */
  void SetWeightSetting(const std::string &settingName) const {

    // if no change in weight setting, do nothing
    if (m_currentWeightSetting == settingName) {
      return;
    }

    // model must support alternate weight settings
    if (!GetHasAlternateWeightSettings()) {
      UserMessage::Add("Warning: Input specifies weight setting, but model does not support alternate weight settings.");
      return;
    }

    // find the setting
    m_currentWeightSetting = settingName;
    std::map< std::string, ScoreComponentCollection* >::const_iterator i =
      m_weightSetting.find( settingName );

    // if not found, resort to default
    if (i == m_weightSetting.end()) {
      std::stringstream strme;
      strme << "Warning: Specified weight setting " << settingName
            << " does not exist in model, using default weight setting instead";
      UserMessage::Add(strme.str());
      i = m_weightSetting.find( "default" );
      m_currentWeightSetting = "default";
    }

    // set weights
    m_allWeights = *(i->second);
  }

  float GetWeightWordPenalty() const;
  float GetWeightUnknownWordPenalty() const;

  const std::vector<DecodeGraph*>& GetDecodeGraphs() const {
    return m_decodeGraphs;
  }
  const std::vector<size_t>& GetDecodeGraphBackoff() const {
    return m_decodeGraphBackoff;
  }

  //sentence (and thread) specific initialisationn and cleanup
  void InitializeForInput(const InputType& source) const;
  void CleanUpAfterSentenceProcessing(const InputType& source) const;

  void LoadFeatureFunctions();
  bool CheckWeights() const;
  bool LoadWeightSettings();
  bool LoadAlternateWeightSettings();

  std::map<std::string, std::string> OverrideFeatureNames();
  void OverrideFeatures();

  FactorType GetPlaceholderFactor() const {
    return m_placeHolderFactor;
  }

  const FeatureRegistry &GetFeatureRegistry() const
  { return m_registry; }

  /** check whether we should be using the old code to support binary phrase-table.
  ** eventually, we'll stop support the binary phrase-table and delete this legacy code
  **/
  void CheckLEGACYPT();
  bool GetUseLegacyPT() const {
    return m_useLegacyPT;
  }

};

}
#endif
