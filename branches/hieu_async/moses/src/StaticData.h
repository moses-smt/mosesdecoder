// $Id: StaticData.h 186 2007-11-09 15:51:00Z hieu $

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
#include <memory>
#include "TypeDef.h"
#include "ScoreIndexManager.h"
#include "Parameter.h"
#include "LanguageModel.h"
#include "LMList.h"
#include "SentenceStats.h"
#include "DecodeStepCollection.h"
//#include "UnknownWordHandler.h"

class InputType;
class LexicalReordering;
class PhraseDictionary;
class GenerationDictionary;
class DistortionScoreProducer;
class WordPenaltyProducer;
class UnknownWordPenaltyProducer;
class DecodeStepTranslation;
class DecodeStepGeneration;
class PrefixPhraseCollection;

/** Contains global variables and contants */
class StaticData
{
private:
	static StaticData				s_instance;
protected:	
	DecodeStepCollection								m_decodeStepCollection;
	Parameter			*m_parameter;
	std::vector<FactorType>			m_inputFactorOrder, m_outputFactorOrder;
	LMList									m_languageModel;
	ScoreIndexManager				m_scoreIndexManager;
	std::vector<float>			m_allWeights, m_weightDistortion;
	std::vector<LexicalReordering*>                   m_reorderModels;
		// Initial	= 0 = can be used when creating poss trans
		// Other		= 1 = used to calculate LM score once all steps have been processed
	float
		m_beamThreshold,
		m_wordDeletionWeight,
		m_weightUnknownWord;
									// PhraseTrans, Generation & LanguageModelScore has multiple weights.
	std::vector<int>	m_maxDistortion;
									// do it differently from old pharaoh
									// -ve	= no limit on distortion
									// 0		= no disortion (monotone in old pharaoh)
	std::vector<size_t> 
			m_maxNoTransOptPerCoverage;
	size_t                              
			m_maxHypoStackSize //hypothesis-stack size that triggers pruning
			, m_nBestSize
			, m_nBestFactor
		  , m_verboseLevel
		  , m_numInputScores
			, m_diagSlack
			, m_nonDiagStackSize;
		  
	AsyncMethod m_asyncMethod;
	
	std::string									m_nBestFilePath, m_cachePath;
	bool                        m_fLMsLoaded, m_labeledNBestList;
	/***
	 * false = treat unknown words as unknowns, and translate them as themselves;
	 * true = drop (ignore) them
	 */
	bool m_dropUnknown;
	bool m_wordDeletionEnabled;
	bool m_sourceStartPosMattersForRecombination;

	InputTypeEnum m_inputType;

	std::vector<DistortionScoreProducer*> m_distortionScoreProducer;
	std::vector<WordPenaltyProducer*>			m_wpProducer;
	UnknownWordPenaltyProducer *m_unknownWordPenaltyProducer;
	bool m_reportSegmentation;
	bool m_useDistortionFutureCosts;
	bool m_isDetailedTranslationReportingEnabled;
	bool m_onlyDistinctNBest;
	bool m_computeLMBackoffStats;
	bool m_useAlignmentInfo;

	mutable std::auto_ptr<SentenceStats> m_sentenceStats;
	std::string m_factorDelimiter; //! by default, |, but it can be changed
	size_t m_maxFactorIdx[2];  //! number of factors on source and target side
	size_t m_maxNumFactors;  //! max number of factors on both source and target sides

	//! constructor. only the 1 static variable can be created
	StaticData();

	//! helper fn to set bool param from ini file/command line
	void SetBooleanParameter(bool *paramter, string parameterName, bool defaultValue);

	/***
	 * load all language models as specified in ini file
	 */
	bool LoadLanguageModels();
	//! load decoding steps
	bool LoadMapping();
	bool LoadLexicalReorderingModel();
	
	DecodeStepTranslation *LoadTranslationTable(size_t index);
	DecodeStepGeneration *LoadGenerationTable(size_t index);

public:
	~StaticData();

	static const StaticData &Instance() 
	{ 
		return s_instance; 
	}

	/** Main function to load everything.
	 * Also initialize the Parameter object
	 */
	bool LoadData(Parameter *parameter);

	static bool LoadDataStatic(Parameter *parameter)
	{
		return s_instance.LoadData(parameter);
	}

	const PARAM_VEC &GetParam(const std::string &paramName) const
	{
		return m_parameter->GetParam(paramName);
	}

	std::string GetCachePath() const
 	{
 		return m_cachePath;
 	}
	bool IsComputeLMBackoffStats() const
	{
		return m_computeLMBackoffStats;
	}
	const std::vector<FactorType> &GetInputFactorOrder() const
	{
		return m_inputFactorOrder;
	}
	const std::vector<FactorType> &GetOutputFactorOrder() const
	{
		return m_outputFactorOrder;
	}

	const DecodeStepCollection &GetDecodeStepCollection() const
	{
		return m_decodeStepCollection;
	}
	const DecodeStepTranslation &GetDecodeStep(size_t decodeStepId) const
	{
		return *m_decodeStepCollection[decodeStepId];
	}
	
	inline bool GetSourceStartPosMattersForRecombination() const
	{ 
		return m_sourceStartPosMattersForRecombination; 
	}
	inline bool GetDropUnknown() const 
	{ 
		return m_dropUnknown; 
	}
	std::vector<LexicalReordering*> GetReorderModels() const
	{
		return m_reorderModels;
	}
	float GetWeightDistortion(size_t decodeStepId) const
	{
		return m_weightDistortion[decodeStepId];
	}
	bool IsWordDeletionEnabled() const
	{
		return m_wordDeletionEnabled;
	}
	size_t GetMaxHypoStackSize() const
	{
		return m_maxHypoStackSize;
	}
	int GetMaxDistortion(size_t decodeStepId) const
	{
		return m_maxDistortion[decodeStepId];
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

	size_t GetLMSize() const
	{
		return m_languageModel.size();
	}
	const LMList &GetAllLM() const
	{
		return m_languageModel;
	}
	size_t GetVerboseLevel() const
	{
		return m_verboseLevel;
	}
	bool GetReportSegmentation() const
	{
		return m_reportSegmentation;
	}
	bool IsDetailedTranslationReportingEnabled() const
	{
		return m_isDetailedTranslationReportingEnabled;
	}
	void ResetSentenceStats(const InputType& source) const
	{
		m_sentenceStats = std::auto_ptr<SentenceStats>(new SentenceStats(source));
	}
	bool IsLabeledNBestList() const
	{
		return m_labeledNBestList;
	}
	const std::vector<std::string> &GetDescription() const
	{
		return m_parameter->GetParam("description");
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
  bool IsNBestEnabled() const {
    return !m_nBestFilePath.empty();
  }
	size_t GetNBestFactor() const
	{
		return m_nBestFactor;
	}

//! Sets the global score vector weights for a given ScoreProducer.
	void SetWeightsForScoreProducer(const ScoreProducer* sp, const std::vector<float>& weights);
	InputTypeEnum GetInputType() const {return m_inputType;}
	size_t GetNumInputScores() const {return m_numInputScores;}
	void InitializeBeforeSentenceProcessing(InputType const&) const;
	void CleanUpAfterSentenceProcessing() const;
	SentenceStats& GetSentenceStats() const
	{
		return *m_sentenceStats;
	}
	const std::vector<float>& GetAllWeights() const
	{
		return m_allWeights;
	}
	const UnknownWordPenaltyProducer *GetUnknownWordPenaltyProducer() const { return m_unknownWordPenaltyProducer; }

	bool UseDistortionFutureCosts() const {return m_useDistortionFutureCosts;}
	bool UseAlignmentInfo() const {return m_useAlignmentInfo;}
	bool GetDistinctNBest() const {return m_onlyDistinctNBest;}
	const std::string& GetFactorDelimiter() const {return m_factorDelimiter;}
	size_t GetMaxNumFactors(FactorDirection direction) const { return m_maxFactorIdx[(size_t)direction]+1; }
	size_t GetMaxNumFactors() const { return m_maxNumFactors; }
	AsyncMethod GetAsyncMethod() const { return m_asyncMethod; }

	size_t GetDiagSlack() const
	{ return m_diagSlack; }
	size_t GetNonDiagStackSize() const
	{ return m_nonDiagStackSize; }
};
