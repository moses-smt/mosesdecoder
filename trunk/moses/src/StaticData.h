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

#pragma once

#include <list>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "TypeDef.h"
#include "ScoreIndexManager.h"
#include "FactorCollection.h"
#include "Parameter.h"
#include "LanguageModel.h"
#include "InputOutput.h"
#include "LMList.h"
#include "SentenceStats.h"
//#include "UnknownWordHandler.h"

class InputType;
class LexicalReordering;
class PhraseDictionaryBase;
class GenerationDictionary;
class DistortionScoreProducer;
class WordPenaltyProducer;
class DecodeStep;

/** Contains global variables and contants */
class StaticData
{
private:
	static StaticData*									s_instance;
protected:	
	FactorCollection										m_factorCollection;
	std::vector<PhraseDictionaryBase*>	m_phraseDictionary;
	std::vector<GenerationDictionary*>	m_generationDictionary;
	std::list < DecodeStep* >						m_decodeStepList;
	Parameter			m_parameter;
	std::vector<FactorType>			m_inputFactorOrder, m_outputFactorOrder;
	LMList									m_languageModel;
	std::vector<float>			m_lexWeights;
	ScoreIndexManager				m_scoreIndexManager;
	std::vector<float>			m_allWeights;
	std::vector<LexicalReordering*>                   m_reorderModels;
		// Initial	= 0 = can be used when creating poss trans
		// Other		= 1 = used to calculate LM score once all steps have been processed
	float
		m_beamThreshold,
		m_weightDistortion, 
		m_weightWordPenalty, 
		m_wordDeletionWeight;
									// PhraseTrans, Generation & LanguageModelScore has multiple weights.
	int																	m_maxDistortion;
									// do it differently from old pharaoh
									// -ve	= no limit on distortion
									// 0		= no disortion (monotone in old pharaoh)
	size_t                              
			m_maxHypoStackSize //hypothesis-stack size that triggers pruning
			, m_nBestSize
			, m_maxNoTransOptPerCoverage
		  , m_maxNoPartTransOpt;
	
	std::string									m_nBestFilePath, m_cachePath;
	std::vector<std::string>		m_mySQLParam;
	InputOutput									*m_inputOutput;
	bool                        m_fLMsLoaded;
	size_t											m_maxNgramOrderForFactor[NUM_FACTORS];
	/***
	 * false = treat unknown words as unknowns, and translate them as themselves;
	 * true = drop (ignore) them
	 */
	bool m_dropUnknown;
	bool m_wordDeletionEnabled;

	int m_inputType;
	unsigned m_numInputScores;

	size_t m_verboseLevel;
	DistortionScoreProducer *m_distortionScoreProducer;
	WordPenaltyProducer *m_wpProducer;
	bool m_reportSourceSpan;
	bool m_reportAllFactors;
	bool m_useDistortionFutureCosts;
	bool m_isDetailedTranslationReportingEnabled;
	bool m_onlyDistinctNBest;

	mutable boost::shared_ptr<SentenceStats> m_sentenceStats;

public:
	StaticData();
	~StaticData();

	static const StaticData* Instance() { return s_instance; }

	/***
	 * also initialize the Parameter object
	 */
	bool LoadParameters(int argc, char* argv[]);

	/***
	 * load not only the main phrase table but also any auxiliary tables that depend on which features are being used
	 * (eg word-deletion, word-insertion tables)
	 */
	void LoadPhraseTables(bool filter
											, const std::string &inputFileHash
											, const std::list< Phrase > &inputPhraseList);
	// what the hell?
	void LoadPhraseTables();
	void LoadMapping();
	const PARAM_VEC &GetParam(const std::string &paramName)
	{
		return m_parameter.GetParam(paramName);
	}

	InputOutput &GetInputOutput()
	{
		return *m_inputOutput;
	}

	const std::vector<FactorType> &GetInputFactorOrder() const
	{
		return m_inputFactorOrder;
	}
	const std::vector<FactorType> &GetOutputFactorOrder() const
	{
		return m_outputFactorOrder;
	}

	std::list < DecodeStep* > &GetDecodeStepList()
	{
		return m_decodeStepList;
	}
	
	inline bool GetDropUnknown() const 
	{ 
		return m_dropUnknown; 
	}
	inline size_t GetMaxNoTransOptPerCoverage() const 
	{ 
		return m_maxNoTransOptPerCoverage;
	}
	inline size_t GetMaxNoPartTransOpt() const 
	{ 
		return m_maxNoPartTransOpt;
	}
	FactorCollection &GetFactorCollection()
	{
		return m_factorCollection;
	}
	std::vector<LexicalReordering*> GetReorderModels() const
	{
		return m_reorderModels;
	}
	float GetWeightDistortion() const
	{
		return m_weightDistortion;
	}
	float GetWeightWordPenalty() const
	{
		return m_weightWordPenalty;
	}
	bool IsWordDeletionEnabled() const
	{
		return m_wordDeletionEnabled;
	}
	size_t GetMaxHypoStackSize() const
	{
		return m_maxHypoStackSize;
	}
	int GetMaxDistortion() const
	{
		return m_maxDistortion;
	}
	float GetBeamThreshold() const
	{
		return m_beamThreshold;
	}
	//! returns the total number of score components across all types, all factors
	size_t GetTotalScoreComponents() const
	{
		return m_scoreIndexManager.GetTotalNumberOfScores();
	}
	const ScoreIndexManager& GetScoreIndexManager() const
	{
		return m_scoreIndexManager;
	}
	IOMethod GetIOMethod();
	const std::vector<std::string> &GetMySQLParam()
	{
		return m_mySQLParam;
	}

	size_t GetLMSize() const
	{
		return m_languageModel.size();
	}
	const LMList &GetAllLM() const
	{
		return m_languageModel;
	}
	size_t GetPhraseDictionarySize() const
	{
		return m_phraseDictionary.size();
	}
	std::vector<PhraseDictionaryBase*> GetPhraseDictionaries() const
	{
		return m_phraseDictionary;
	}
	std::vector<GenerationDictionary*> GetGenerationDictionaries() const
	{
		return m_generationDictionary;
	}
	size_t GetGenerationDictionarySize() const
	{
		return m_generationDictionary.size();
	}
	const std::string GetCachePath() const
	{
		return m_cachePath;
	}
	size_t GetVerboseLevel() const
	{
		return m_verboseLevel;
	}
	bool GetReportSourceSpan() const
	{
		return m_reportSourceSpan;
	}
	bool GetReportAllFactors() const
	{
		return m_reportAllFactors;
	}
	bool IsDetailedTranslationReportingEnabled() const
	{
		return m_isDetailedTranslationReportingEnabled;
	}
	void ResetSentenceStats(const InputType& source) const
	{
		m_sentenceStats = boost::shared_ptr<SentenceStats>(new SentenceStats(source));
	}

	// for mert
	size_t GetNBestSize() const
	{
		return m_nBestSize;
	}
	const std::string &GetNBestFilePath() const
	{
		return m_nBestFilePath;
	}
  // TODO use IsNBestEnabled instead of conditional compilation
  const bool IsNBestEnabled() const {
    return !m_nBestFilePath.empty();
  }
//! Sets the global score vector weights for a given ScoreProducer.
	void SetWeightsForScoreProducer(const ScoreProducer* sp, const std::vector<float>& weights);
	int GetInputType() const {return m_inputType;}
	unsigned GetNumInputScores() const {return m_numInputScores;}
	void InitializeBeforeSentenceProcessing(InputType const&);
	void CleanUpAfterSentenceProcessing();
	SentenceStats& GetSentenceStats() const
	{
		return *m_sentenceStats;
	}
	const std::vector<float>& GetAllWeights() const
	{
		return m_allWeights;
	}
	const DistortionScoreProducer *GetDistortionScoreProducer() const { return m_distortionScoreProducer; }
	const WordPenaltyProducer *GetWordPenaltyProducer() const { return m_wpProducer; }

	bool UseDistortionFutureCosts() const {return m_useDistortionFutureCosts;}
	bool OnlyDistinctNBest() const {return m_onlyDistinctNBest;}
};
