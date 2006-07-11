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

#include <string>
#include <assert.h>

#include "StaticData.h"
#include "Util.h"
#include "FactorCollection.h"
#include "HypothesisCollection.h"
#include "Timer.h"

#include "boost/filesystem/operations.hpp" // boost::filesystem::exists

using namespace std;

extern Timer timer;

StaticData::StaticData()
:m_languageModel(2)
,m_inputOutput(NULL)
{
}

bool StaticData::LoadParameters(int argc, char* argv[])
{
	if (!m_parameter.LoadParam(argc, argv))
		return false;

	// mysql
	m_mySQLParam = m_parameter.GetParam("mysql");

	if (m_parameter.GetParam("cache-path").size() == 1)
		m_cachePath = m_parameter.GetParam("cache-path")[0];
	else
		m_cachePath = GetTempFolder();

	// n-best
	if (m_parameter.GetParam("n-best-list").size() == 2)
	{
		m_nBestFilePath = m_parameter.GetParam("n-best-list")[0];
		m_nBestSize = Scan<size_t>( m_parameter.GetParam("n-best-list")[1] );
	}
	else
	{
		m_nBestSize = 0;
	}

	//input-factors
	const vector<string> &inputFactorVector = m_parameter.GetParam("input-factors");
	for(size_t i=0; i<inputFactorVector.size(); i++) 
	{
		m_inputFactorOrder.push_back(Scan<FactorType>(inputFactorVector[i]));
	}

	// load language models
	if (m_parameter.GetParam("lmodel-file").size() > 0)
	{
		// weights
		vector<float> weightAll = Scan<float>(m_parameter.GetParam("weight-l"));
		
		TRACE_ERR("weight-l: ");
		for (size_t i = 0 ; i < weightAll.size() ; i++)
		{
				TRACE_ERR(weightAll[i] << "\t");
		}
		TRACE_ERR(endl);
		

		size_t nGramMaxOrder = 0;
		const vector<string> &lmVector = m_parameter.GetParam("lmodel-file");

		for(size_t i=0; i<lmVector.size(); i++) 
		{
			vector<string>	token		= Tokenize(lmVector[i]);
			int type = Scan<int>(token[0]);
			FactorType factorType = Scan<FactorType>(token[1]);
			size_t nGramOrder = Scan<int>(token[2]);
			nGramMaxOrder = (std::max)(nGramMaxOrder, nGramOrder);
			string &languageModelFile = token[3];
			
			LanguageModel *lm = new LanguageModel();
			lm->Load(i, languageModelFile, m_factorCollection, factorType, weightAll[i], nGramOrder);
			m_languageModel[type].push_back(lm);

			/*
			const Factor *f0 = m_factorCollection.AddFactor(Target, Surface, "it")
										,*f1 = m_factorCollection.AddFactor(Target, Surface, "market")
										,*f2 = m_factorCollection.AddFactor(Target, Surface, "economy");
			vector<const Factor*> contextFactor(3);
			contextFactor[0] = f0;
			contextFactor[1] = f1;
			contextFactor[2] = f2;
			float v = UntransformSRIScore(lm->GetValue(contextFactor));
			TRACE_ERR(v << endl);	
			*/
		}
		CompareHypothesisCollection::SetMaxNGramOrder(nGramMaxOrder);
	}

	timer.check("Finished loading language models");

	// generation tables
	if (m_parameter.GetParam("generation-file").size() > 0) 
	{
		const vector<string> &generationVector = m_parameter.GetParam("generation-file");
		const vector<float> &weight = Scan<float>(m_parameter.GetParam("weight-generation"));

		TRACE_ERR("weight-generation: ");
		for (size_t i = 0 ; i < weight.size() ; i++)
		{
				TRACE_ERR(weight[i] << "\t");
		}
		TRACE_ERR(endl);
		
		for(size_t currDict = 0 ; currDict < generationVector.size(); currDict++) 
		{
			vector<string>			token		= Tokenize(generationVector[currDict]);
			vector<FactorType> 	input		= Tokenize<FactorType>(token[0], ",")
													,output	= Tokenize<FactorType>(token[1], ",");
			string							filePath= token[2];

			TRACE_ERR(filePath << endl);
			m_generationDictionary.push_back(new GenerationDictionary());
			m_generationDictionary.back()->Load(input
																		, output
																		, m_factorCollection
																		, filePath
																		, weight[currDict]
																		, Output		 // always target, for now
																		, currDict);
		}
	}

	timer.check("Finished loading generation tables");

	// score weights
	m_weightDistortion				= Scan<float>( m_parameter.GetParam("weight-d")[0] );
	m_weightWordPenalty				= Scan<float>( m_parameter.GetParam("weight-w")[0] );

	TRACE_ERR("weight-d: " << m_weightDistortion << endl);
	TRACE_ERR("weight-w: " << m_weightWordPenalty << endl);

	// misc
	m_maxHypoStackSize = (m_parameter.GetParam("max-hypostack-size").size() > 0)
				? Scan<size_t>(m_parameter.GetParam("max-hypostack-size")[0]) : DEFAULT_MAX_HYPOSTACK_SIZE;
	m_maxDistortion = (m_parameter.GetParam("distortion-limit").size() > 0) ?
		Scan<int>(m_parameter.GetParam("distortion-limit")[0])
		: -1;
	m_beamThreshold = (m_parameter.GetParam("beam-threshold").size() > 0) ?
		TransformScore(Scan<float>(m_parameter.GetParam("beam-threshold")[0]))
		: TransformScore(DEFAULT_BEAM_THRESHOLD);

	return true;
}

StaticData::~StaticData()
{
	delete m_inputOutput;
	for (size_t i = 0 ; i < m_phraseDictionary.size() ; i++)
	{
		delete m_phraseDictionary[i];
	}
	for (size_t i = 0 ; i < m_generationDictionary.size() ; i++)
	{
		delete m_generationDictionary[i];
	}

	LMList &lmList = m_languageModel[0];
	LMList::const_iterator iterLM;
	for (iterLM = lmList.begin() ; iterLM != lmList.end() ; ++iterLM)
	{
		delete *iterLM;
	}
	lmList = m_languageModel[1];
	for (iterLM = lmList.begin() ; iterLM != lmList.end() ; ++iterLM)
	{
		delete *iterLM;
	}
}

IOMethod StaticData::GetIOMethod()
{
	if (m_mySQLParam.size() == 6)
		return IOMethodMySQL;
	else if (m_parameter.GetParam("input-file").size() == 1)
		return IOMethodFile;
	else
		return IOMethodCommandLine;
}

void StaticData::SetWeightTransModel(const vector<float> &weight)
{
	size_t currWeight = 0;
	vector<PhraseDictionary*>::iterator iter;
	for(iter = m_phraseDictionary.begin() ; iter != m_phraseDictionary.end(); ++iter) 
	{
		PhraseDictionary *phraseDict = *iter;
		const size_t noScoreComponent 						= phraseDict->GetNoScoreComponent();
		// weights for this particular dictionary
		vector<float> dictWeight(noScoreComponent);
		for (size_t i = 0 ; i < noScoreComponent ; i++)
		{
			dictWeight[i] = weight[currWeight++];
		}
		phraseDict->SetWeightTransModel(dictWeight);
	}
}

void StaticData::SetWeightLM(const std::vector<float> &weight)
{
	assert(weight.size() == m_languageModel[Initial].size() + m_languageModel[Other].size());
	
	size_t currIndex = 0;
	LMList::iterator iter;
	for (iter = m_languageModel[Initial].begin() ; iter != m_languageModel[Initial].end() ; ++iter)
	{
		LanguageModel *languageModel = *iter;
		languageModel->SetWeight(weight[currIndex++]);
	}
	for (iter = m_languageModel[Other].begin() ; iter != m_languageModel[Other].end() ; ++iter)
	{
		LanguageModel *languageModel = *iter;
		languageModel->SetWeight(weight[currIndex++]);
	}	
}

void StaticData::SetWeightGeneration(const std::vector<float> &weight)
{
	assert(weight.size() == GetGenerationDictionarySize());

	size_t currWeight = 0;
	vector<GenerationDictionary*>::iterator iter;
	for(iter = m_generationDictionary.begin() ; iter != m_generationDictionary.end(); ++iter) 
	{
		GenerationDictionary *dict = *iter;
		dict->SetWeight(weight[currWeight++]);
	}
}

const LMList StaticData::GetAllLM() const
{
	LMList allLM;
	std::copy(m_languageModel[Initial].begin(), m_languageModel[Initial].end()
						, std::inserter(allLM, allLM.end()));
	std::copy(m_languageModel[Other].begin(), m_languageModel[Other].end()
						, std::inserter(allLM, allLM.end()));
	
	return allLM;
}

void StaticData::LoadPhraseTables(bool filter
																	, const string &inputFileHash
																	, const list< Phrase > &inputPhraseList)
{
	// load phrase translation tables
  if (m_parameter.GetParam("ttable-file").size() > 0)
	{
		// weights
		vector<float> weightAll									= Scan<float>(m_parameter.GetParam("weight-t"));
		
		TRACE_ERR("weight-t: ");
		for (size_t i = 0 ; i < weightAll.size() ; i++)
		{
				TRACE_ERR(weightAll[i] << "\t");
		}
		TRACE_ERR(endl);
		
		const vector<string> &translationVector = m_parameter.GetParam("ttable-file");
		size_t	maxTargetPhrase										= Scan<size_t>(m_parameter.GetParam("ttable-limit")[0]);

		size_t totalPrevNoScoreComponent = 0;		
		for(size_t currDict = 0 ; currDict < translationVector.size(); currDict++) 
		{
			vector<string>			token		= Tokenize(translationVector[currDict]);
			vector<FactorType> 	input		= Tokenize<FactorType>(token[0], ",")
													,output	= Tokenize<FactorType>(token[1], ",");
			string							filePath= token[3];
			size_t							noScoreComponent	= Scan<size_t>(token[2]);
			// weights for this phrase dictionary
			vector<float> weight(noScoreComponent);
			for (size_t currScore = 0 ; currScore < noScoreComponent ; currScore++)
			{
				weight[currScore] = weightAll[totalPrevNoScoreComponent + currScore]; 
			}
			totalPrevNoScoreComponent += noScoreComponent;

			string phraseTableHash	= GetMD5Hash(filePath);
			string hashFilePath			= GetCachePath() 
															+ PROJECT_NAME + "--"
															+ inputFileHash + "--" 
															+ phraseTableHash + ".txt";
			bool filterPhrase;
			if (filter)
			{
				if (boost::filesystem::exists(hashFilePath))
				{ // load filtered file instead
					filterPhrase = false;
					filePath = hashFilePath;
				}
				else
				{ // load original file & create has file
					filterPhrase = true;
				}
			}
			else
			{ // load original file
				filterPhrase = false;
			}
			TRACE_ERR(filePath << endl);

			m_phraseDictionary.push_back(new PhraseDictionary(noScoreComponent));
			timer.check("Start loading");
			m_phraseDictionary[currDict]->Load(input
																				, output
																				, m_factorCollection
																				, filePath
																				, hashFilePath
																				, weight
																				, maxTargetPhrase
																				, filterPhrase
																				, inputPhraseList);
			timer.check("Finished loading");
		}
	}

	timer.check("Finished loading phrase tables");
}

void StaticData::LoadMapping()
{
	// mapping
	const vector<string> &mappingVector = m_parameter.GetParam("mapping");
	for(size_t i=0; i<mappingVector.size(); i++) 
	{
		vector<string>	token		= Tokenize(mappingVector[i]);
		if (token.size() == 2) 
		{
			DecodeType decodeType = token[0] == "T" ? Translate : Generate;
			size_t index = Scan<size_t>(token[1]);
			DecodeStep decodeStep (decodeType
														,decodeType == Translate ? (Dictionary*) m_phraseDictionary[index] : (Dictionary*) m_generationDictionary[index]);
			m_decodeStepList.push_back(decodeStep);
		}
	}
}
	
