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
#include "TypeDef.h"
#include "PhraseDictionary.h"
#include "GenerationDictionary.h"
#include "FactorCollection.h"
#include "Parameter.h"
#include "LanguageModel.h"
#include "InputOutput.h"
#include "DecodeStep.h"

class StaticData
{
protected:	
	FactorCollection										m_factorCollection;
	std::vector<PhraseDictionary*>			m_phraseDictionary;
	std::vector<GenerationDictionary*>	m_generationDictionary;
	std::list < DecodeStep >						m_decodeStepList;
	Parameter														m_parameter;
	std::vector<FactorType>							m_inputFactorOrder;
	std::vector<LMList>									m_languageModel;
		// Initial	= 0 = can be used when creating poss trans
		// Other		= 1 = used to calculate LM score once all steps have been processed
	float																m_beamThreshold
																			,m_weightDistortion, m_weightWordPenalty;
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
	size_t m_verboseLevel;

public:
	StaticData();
	~StaticData();

	bool LoadParameters(int argc, char* argv[]);

	void LoadPhraseTables(bool filter
											, const std::string &inputFileHash
											, const std::list< Phrase > &inputPhraseList);
	void LoadPhraseTables()
	{
		LoadPhraseTables(false, "", std::list< Phrase >());
	}
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

	std::list < DecodeStep > &GetDecodeStepList()
	{
		return m_decodeStepList;
	}

	FactorCollection &GetFactorCollection()
	{
		return m_factorCollection;
	}
	
	const LMList &GetLanguageModel(LMListType type) const
	{
		return m_languageModel[type];
	}
	float GetWeightDistortion() const
	{
		return m_weightDistortion;
	}
	float GetWeightWordPenalty() const
	{
		return m_weightWordPenalty;
	}
	unsigned int GetMaxHypoStackSize() const
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

	// for mert
	size_t GetNBestSize() const
	{
		return m_nBestSize;
	}
	const std::string &GetNBestFilePath() const
	{
		return m_nBestFilePath;
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
};

