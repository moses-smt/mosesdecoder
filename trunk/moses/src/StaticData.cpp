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
#include "boost/algorithm/string/case_conv.hpp" //boost::algorithm::to_lower

#ifdef LM_SRI
#include "LanguageModel_SRI.h"
#else
#ifdef LM_IRST
#include "LanguageModel_IRST.h"
#else
#include "LanguageModel_Internal.h"
#endif
#endif


using namespace std;

extern Timer timer;

StaticData::StaticData()
:m_languageModel(2)
,m_inputOutput(NULL)
,m_fLMsLoaded(false)
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

	// verbose level
	if (m_parameter.GetParam("verbose").size() == 1)
	{
		m_verboseLevel = 1;
		m_verboseLevel = Scan<size_t>( m_parameter.GetParam("verbose")[0]);
	}
	else
	{
		m_verboseLevel = 0;
	}


	//input-factors
	const vector<string> &inputFactorVector = m_parameter.GetParam("input-factors");
	for(size_t i=0; i<inputFactorVector.size(); i++) 
  {
		m_inputFactorOrder.push_back(Scan<FactorType>(inputFactorVector[i]));
	}

	// load Lexical Reordering model
	// check to see if the lexical reordering parameter exists
	const vector<string> &lrFileVector = 
		m_parameter.GetParam("lexreordering-file");	
	if (lrFileVector.size() > 0)
		{
			// if there is a lexical reordering model, then parse the
			// parameters associated with it, and create a new Lexical
			// Reordering object (which will load the probability table)
			const vector<string> &lrTypeVector = 
				m_parameter.GetParam("lexreordering-type");	
			// if type values have been set in the .ini file, then use them;
			// first initialize to the defaults (msd, bidirectional, fe).
			int orientation = LexReorderType::Msd, 
				direction = LexReorderType::Bidirectional, 
				condition = LexReorderType::Fe;
			if (lrTypeVector.size() > 0)
				{
					// loop through type vector and set the orientation,
					// direction, and condition to override the defaults
					int size = lrTypeVector.size();
					string val;
					//if multiple parameters of the same type (direction, orientation, condition)
					//are seen, default behavior is to set the type to the last seen
					for (int i=0; i<size; i++)
						{
							val = lrTypeVector[i];
							boost::algorithm::to_lower(val);
							//TODO:Lowercase val!
							//orientation 
							if(val == "monotone")
								orientation = LexReorderType::Monotone;
							else if(val == "msd")
								orientation = LexReorderType::Msd;
							//direction
							else if(val == "forward")
								direction == LexReorderType::Forward;
							else if(val == "backward")
								direction == LexReorderType::Backward;
							else if(val == "bidirectional")
								direction == LexReorderType::Bidirectional;
							//condition
							else if(val == "f")
								condition = LexReorderType::F;
							else if(val == "fe")
								condition = LexReorderType::Fe;
						} 
				}
			else // inform the user that the defaults are being employed
				{
					//cout << "Lexical reordering is using defaults: Msd, Bidirectional, Fe Parameters" << endl;
				}

			// for now, assume there is just one lexical reordering model
			timer.check("Starting to load lexical reorder table...");
 			m_lexReorder = new LexicalReordering(lrFileVector[0], orientation, direction, condition);
			timer.check("Finished loading lexical reorder table.");
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
	  timer.check("Start loading LanguageModels");
		const vector<string> &lmVector = m_parameter.GetParam("lmodel-file");

		for(size_t i=0; i<lmVector.size(); i++) 
		{
			vector<string>	token		= Tokenize(lmVector[i]);
			if (token.size() != 4 )
			{
				TRACE_ERR("Expected format 'LM-TYPE FACTOR-TYPE NGRAM-ORDER filename'");
				return false;
			}
			// type = whether or not to use in future cost calcs
			// (DEPRECATED, asked hieu)
			LMListType type = static_cast<LMListType>(Scan<int>(token[0]));
			// factorType = (see TypeDef.h)
			//   0 = Surface, 1 = POS, 2 = Stem, 3 = Morphology, etc
			FactorType factorType = Scan<FactorType>(token[1]);
			// nGramOrder = 2 = bigram, 3 = trigram, etc
			size_t nGramOrder = Scan<int>(token[2]);
			// keep track of the largest n-gram length
			// (used by CompareHypothesisCollection)
			if (nGramOrder > nGramMaxOrder)
				nGramMaxOrder = nGramOrder;
			string &languageModelFile = token[3];
			
			timer.check(("Start loading LanguageModel " + languageModelFile).c_str());
      LanguageModel *lm = 0;
#ifdef LM_SRI
      lm = new LanguageModel_SRI();
#else
#ifdef LM_IRST
      lm = new LanguageModel_IRST();
#else
      lm = new LanguageModel_Internal();
#endif
#endif

			// error handling here?
			lm->Load(i, languageModelFile, m_factorCollection, factorType, weightAll[i], nGramOrder);
	  	timer.check(("Finished loading LanguageModel " + languageModelFile).c_str());
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
  // flag indicating that language models were loaded,
  // since phrase table loading requires their presence
  m_fLMsLoaded = true;
	timer.check("Finished loading LanguageModels");

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
																		, Output);		 // always target, for now
		}
	}

	timer.check("Finished loading generation tables");

	// score weights
	m_weightDistortion				= Scan<float>( m_parameter.GetParam("weight-d")[0] );
	m_weightWordPenalty				= Scan<float>( m_parameter.GetParam("weight-w")[0] );

	TRACE_ERR("weight-d: " << m_weightDistortion << endl);
	TRACE_ERR("weight-w: " << m_weightWordPenalty << endl);

	// misc
	m_maxHypoStackSize = (m_parameter.GetParam("stack").size() > 0)
				? Scan<size_t>(m_parameter.GetParam("stack")[0]) : DEFAULT_MAX_HYPOSTACK_SIZE;
	m_maxDistortion = (m_parameter.GetParam("distortion-limit").size() > 0) ?
		Scan<int>(m_parameter.GetParam("distortion-limit")[0])
		: -1;
	m_beamThreshold = (m_parameter.GetParam("beam-threshold").size() > 0) ?
		TransformScore(Scan<float>(m_parameter.GetParam("beam-threshold")[0]))
		: TransformScore(DEFAULT_BEAM_THRESHOLD);

	// Unknown Word Processing -- wade
	if (m_parameter.GetParam("drop-unknown").size() == 1)
	  { m_dropUnknown = Scan<size_t>( m_parameter.GetParam("drop-unknown")[0]); }
	else
	  { m_dropUnknown = 0; }

  	TRACE_ERR("m_dropUnknown: " << m_dropUnknown << endl);

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
	// language models must be loaded prior to loading phrase tables
	assert(m_fLMsLoaded);
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
		vector<size_t>	maxTargetPhrase					= Scan<size_t>(m_parameter.GetParam("ttable-limit"));

		size_t index = 0;
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
				boost::filesystem::path tempFile(hashFilePath, boost::filesystem::native);
				if (boost::filesystem::exists(tempFile))
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
			timer.check("Start loading PhraseTable");
			m_phraseDictionary[currDict]->Load(input
																				, output
																				, m_factorCollection
																				, filePath
																				, hashFilePath
																				, weight
																				, maxTargetPhrase[index]
																				, filterPhrase
																				, inputPhraseList
																				,	this->GetLanguageModel(Initial)
																				,	this->GetWeightWordPenalty());

			index++;
			timer.check("Finished loading PhraseTable");
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
	
