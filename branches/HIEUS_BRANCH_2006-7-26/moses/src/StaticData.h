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
#include "PhraseDictionary.h"
#include "GenerationDictionary.h"
#include "FactorCollection.h"
#include "Parameter.h"
#include "LanguageModel.h"
#include "LexicalReordering.h"
#include "InputOutput.h"
#include "DecodeStep.h"
#include "LMList.h"
//#include "UnknownWordHandler.h"

class InputType;

class StaticData
{
private:
	static StaticData*									s_instance;
protected:	
	FactorCollection										m_factorCollection;
	std::vector<PhraseDictionaryBase*>	m_phraseDictionary;
	std::vector<GenerationDictionary*>	m_generationDictionary;
	std::list < DecodeStep >						m_decodeStepList;
	Parameter			m_parameter;
	std::vector<FactorType>			m_inputFactorOrder;
//	boost::shared_ptr<UnknownWordHandler>      m_unknownWordHandler; //defaults to NULL; pointer allows polymorphism
	std::vector<LMList>			m_languageModel;
	LexicalReordering                   *m_lexReorder;
		// Initial	= 0 = can be used when creating poss trans
		// Other		= 1 = used to calculate LM score once all steps have been processed
	float
		m_beamThreshold,
		m_weightDistortion, 
		m_weightWordPenalty, 
		m_wordDeletionWeight, 
		m_weightInput;
									// PhraseTrans, Generation & LanguageModelScore has multiple weights.
	int																	m_maxDistortion;
									// do it differently from old pharaoh
									// -ve	= no limit on distortion
									// 0		= no disortion (monotone in old pharaoh)
	size_t                              m_maxHypoStackSize; //hypothesis-stack size that triggers pruning
	size_t															m_nBestSize;
	std::string													m_nBestFilePath, m_cachePath;
	std::vector<std::string>						m_mySQLParam;
	InputOutput													*m_inputOutput;
	bool                                m_fLMsLoaded;
	/***
	 * false = treat unknown words as proper nouns, and translate them as themselves;
	 * true = drop (ignore) them
	 */
	bool m_dropUnknown;
	bool m_wordDeletionEnabled;

	int m_inputType;
		
	size_t m_verboseLevel;

	bool m_reportSourceSpan;
	bool m_reportAllFactors;

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
	void LoadPhraseTables()
	{
		LoadPhraseTables(false, "", std::list< Phrase >());
	}
	void LoadMapping();
/*	void SetUnknownWordHandler(boost::shared_ptr<UnknownWordHandler> unknownWordHandler)
	{
		m_unknownWordHandler = unknownWordHandler;
	}
*/
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

	std::list < DecodeStep > &GetDecodeStepList()
	{
		return m_decodeStepList;
	}
	
	inline bool GetDropUnknown() const 
	{ 
		return m_dropUnknown; 
	}
/*	
	boost::shared_ptr<UnknownWordHandler> GetUnknownWordHandler()
	{
		return m_unknownWordHandler;
	}
*/
	FactorCollection &GetFactorCollection()
	{
		return m_factorCollection;
	}
	
	const LMList &GetLanguageModel(LMListType type) const
	{
		return m_languageModel[type];
	}
	
	LexicalReordering *GetLexReorder() const
	{
		return m_lexReorder;
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

	IOMethod GetIOMethod();
	const std::vector<std::string> &GetMySQLParam()
	{
		return m_mySQLParam;
	}

	size_t GetLMSize() const
	{
		return m_languageModel[Initial].size() + m_languageModel[Other].size();
	}
	const LMList GetAllLM() const;

	size_t GetPhraseDictionarySize() const
	{
		return m_phraseDictionary.size();
	}
	std::vector<PhraseDictionaryBase*> GetPhraseDictionaries() const
	{
		return m_phraseDictionary;
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
	void SetWeightDistortion(float weightDistortion)
	{
		m_weightDistortion = weightDistortion;
	}
	void SetWeightWordPenalty(float weightWordPenalty)
	{
		m_weightWordPenalty = weightWordPenalty;
	}
	void SetWeightTransModel(const std::vector<float> &weight);
	void SetWeightLM(const std::vector<float> &weight);
	void SetWeightGeneration(const std::vector<float> &weight);
	int GetInputType() const {return m_inputType;}
	void InitializeBeforeSentenceProcessing(InputType const&);
	void CleanUpAfterSentenceProcessing();
};

