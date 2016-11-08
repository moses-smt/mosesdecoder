// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifdef WITH_THREADS
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#endif

#include "Parameter.h"
#include "SentenceStats.h"
#include "ScoreComponentCollection.h"
#include "moses/FF/Factory.h"
#include "moses/PP/Factory.h"

#include "moses/parameters/AllOptions.h"
#include "moses/parameters/BookkeepingOptions.h"

namespace Moses
{

class InputType;
class DecodeGraph;
class DecodeStep;

class DynamicCacheBasedLanguageModel;
class PhraseDictionaryDynamicCacheBased;

typedef std::pair<std::string, float> UnknownLHSEntry;
typedef std::vector<UnknownLHSEntry>  UnknownLHSList;

/** Contains global variables and constants.
 *  Only 1 object of this class should be instantiated.
 *  A const object of this class is accessible by any function during decoding by calling StaticData::Instance();
 */
class StaticData
{
  friend class HyperParameterAsWeight;

private:
  static StaticData s_instance;
protected:
  Parameter *m_parameter;
  boost::shared_ptr<AllOptions> m_options;

  mutable ScoreComponentCollection m_allWeights;

  std::vector<DecodeGraph*> m_decodeGraphs;

  // Initial	= 0 = can be used when creating poss trans
  // Other		= 1 = used to calculate LM score once all steps have been processed
  float
  m_wordDeletionWeight;


  // PhraseTrans, Generation & LanguageModelScore has multiple weights.
  // int				m_maxDistortion;
  // do it differently from old pharaoh
  // -ve	= no limit on distortion
  // 0		= no disortion (monotone in old pharaoh)
  bool m_reorderingConstraint; //! use additional reordering constraints
  BookkeepingOptions m_bookkeeping_options;


  bool m_requireSortingAfterSourceContext;

  mutable size_t m_verboseLevel;

  std::string m_factorDelimiter; //! by default, |, but it can be changed


  size_t m_lmcache_cleanup_threshold; //! number of translations after which LM claenup is performed (0=never, N=after N translations; default is 1)

  std::string m_outputUnknownsFile; //! output unknowns in this file

  // Initial = 0 = can be used when creating poss trans
  // Other = 1 = used to calculate LM score once all steps have been processed
  Word m_inputDefaultNonTerminal, m_outputDefaultNonTerminal;
  SourceLabelOverlap m_sourceLabelOverlap;
  UnknownLHSList m_unknownLHS;

  int m_threadCount;
  // long m_startTranslationId;

  // alternate weight settings
  mutable std::string m_currentWeightSetting;
  std::map< std::string, ScoreComponentCollection* > m_weightSetting; // core weights
  std::map< std::string, std::set< std::string > > m_weightSettingIgnoreFF; // feature function
  std::map< std::string, std::set< size_t > > m_weightSettingIgnoreDP; // decoding path

  bool m_useLegacyPT;
  // bool m_defaultNonTermOnlyForEmptyRange;
  // S2TParsingAlgorithm m_s2tParsingAlgorithm;

  FeatureRegistry m_registry;
  PhrasePropertyFactory m_phrasePropertyFactory;

  StaticData();

  void LoadChartDecodingParameters();
  void LoadNonTerminals();

  //! load decoding steps
  void LoadDecodeGraphs();
  void LoadDecodeGraphsOld(const std::vector<std::string> &mappingVector,
                           const std::vector<size_t> &maxChartSpans);
  void LoadDecodeGraphsNew(const std::vector<std::string> &mappingVector,
                           const std::vector<size_t> &maxChartSpans);

  void NoCache();

  std::string m_binPath;

  // soft NT lookup for chart models
  std::vector<std::vector<Word> > m_softMatchesMap;

  const StatefulFeatureFunction* m_treeStructure;

  void ini_oov_options();
  bool ini_output_options();
  bool ini_performance_options();

  void initialize_features();

  // Coordinate space name map for matching spaces across XML input ("coord"
  // tag) and feature functions that assign or use coordinates on target phrases
  std::map< std::string const, size_t > m_coordSpaceMap;
  size_t m_coordSpaceNextID;

public:

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

  //! Load data into static instance. This function is required as
  //  LoadData() is not const
  static bool LoadDataStatic(Parameter *parameter, const std::string &execPath);

  //! Main function to load everything. Also initialize the Parameter object
  bool LoadData(Parameter *parameter);
  void ClearData();

  const Parameter &GetParameter() const {
    return *m_parameter;
  }

  AllOptions::ptr const
    options() const {
    return m_options;
  }

  size_t
  GetVerboseLevel() const {
    return m_verboseLevel;
  }

  void
  SetVerboseLevel(int x) const {
    m_verboseLevel = x;
  }

  const ScoreComponentCollection&
  GetAllWeights() const {
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

  //Weights for feature with fixed number of values
  void SetWeights(const FeatureFunction* sp, const std::vector<float>& weights);

  const std::string& GetFactorDelimiter() const {
    return m_factorDelimiter;
  }

  size_t GetLMCacheCleanupThreshold() const {
    return m_lmcache_cleanup_threshold;
  }

  const std::string& GetOutputUnknownsFile() const {
    return m_outputUnknownsFile;
  }

  const UnknownLHSList &GetUnknownLHS() const {
    return m_unknownLHS;
  }

  float GetRuleCountThreshold() const {
    return 999999; /* TODO wtf! */
  }

  void ReLoadBleuScoreFeatureParameter(float weight);

  Parameter* GetParameter() {
    return m_parameter;
  }

  int ThreadCount() const {
    return m_threadCount;
  }

  void SetExecPath(const std::string &path);
  const std::string &GetBinDirectory() const;

  bool NeedAlignmentInfo() const {
    return m_bookkeeping_options.need_alignment_info;
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
      std::cerr << "Warning: Input specifies weight setting, but model does not support alternate weight settings.";
      return;
    }

    // find the setting
    m_currentWeightSetting = settingName;
    std::map< std::string, ScoreComponentCollection* >::const_iterator i =
      m_weightSetting.find( settingName );

    // if not found, resort to default
    if (i == m_weightSetting.end()) {
      std::cerr << "Warning: Specified weight setting " << settingName
                << " does not exist in model, using default weight setting instead";
      i = m_weightSetting.find( "default" );
      m_currentWeightSetting = "default";
    }

    // set weights
    m_allWeights = *(i->second);
  }

  float GetWeightWordPenalty() const;

  const std::vector<DecodeGraph*>& GetDecodeGraphs() const {
    return m_decodeGraphs;
  }

  //sentence (and thread) specific initialisationn and cleanup
  void InitializeForInput(ttasksptr const& ttask) const;
  void CleanUpAfterSentenceProcessing(ttasksptr const& ttask) const;

  void LoadFeatureFunctions();
  bool CheckWeights() const;
  void LoadSparseWeightsFromConfig();
  bool LoadWeightSettings();
  bool LoadAlternateWeightSettings();

  std::map<std::string, std::string> OverrideFeatureNames();
  void OverrideFeatures();

  const FeatureRegistry &GetFeatureRegistry() const {
    return m_registry;
  }

  const PhrasePropertyFactory &GetPhrasePropertyFactory() const {
    return m_phrasePropertyFactory;
  }

  /** check whether we should be using the old code to support binary phrase-table.
  ** eventually, we'll stop support the binary phrase-table and delete this legacy code
  **/
  void CheckLEGACYPT();
  bool GetUseLegacyPT() const {
    return m_useLegacyPT;
  }

  void SetSoftMatches(std::vector<std::vector<Word> >& softMatchesMap) {
    m_softMatchesMap = softMatchesMap;
  }

  const std::vector< std::vector<Word> >& GetSoftMatches() const {
    return m_softMatchesMap;
  }

  void ResetWeights(const std::string &denseWeights, const std::string &sparseFile);

  // need global access for output of tree structure
  const StatefulFeatureFunction* GetTreeStructure() const {
    return m_treeStructure;
  }

  void SetTreeStructure(const StatefulFeatureFunction* treeStructure) {
    m_treeStructure = treeStructure;
  }

  bool RequireSortingAfterSourceContext() const {
    return m_requireSortingAfterSourceContext;
  }

  // Coordinate spaces
  size_t GetCoordSpace(std::string space) const;
  size_t MapCoordSpace(std::string space);
};

}
#endif
