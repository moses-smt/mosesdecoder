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
#include <cassert>
#include <boost/filesystem/operations.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "PhraseDictionaryMemory.h"
#include "DecodeStepTranslation.h"
#include "DecodeStepGeneration.h"
#include "GenerationDictionary.h"
#include "DummyScoreProducers.h"
#include "StaticData.h"
#include "Util.h"
#include "HypothesisStack.h"
#include "Timer.h"
#include "LanguageModelSingleFactor.h"
#include "LanguageModelMultiFactor.h"
#include "LanguageModelFactory.h"
#include "LexicalReordering.h"
#include "SentenceStats.h"
#include "PhraseDictionaryTreeAdaptor.h"
#include "UserMessage.h"
#include "PrefixPhraseCollection.h"
#include "PhraseList.h"

using namespace std;

static size_t CalcMax(size_t x, const vector<size_t>& y) {
  size_t max = x;
  for (vector<size_t>::const_iterator i=y.begin(); i != y.end(); ++i)
    if (*i > max) max = *i;
  return max;
}

static size_t CalcMax(size_t x, const vector<size_t>& y, const vector<size_t>& z) {
  size_t max = x;
  for (vector<size_t>::const_iterator i=y.begin(); i != y.end(); ++i)
    if (*i > max) max = *i;
  for (vector<size_t>::const_iterator i=z.begin(); i != z.end(); ++i)
    if (*i > max) max = *i;
  return max;
}

StaticData StaticData::s_instance;

StaticData::StaticData()
:m_fLMsLoaded(false)
,m_inputType(0)
,m_numInputScores(0)
,m_distortionScoreProducer(0)
,m_wpProducer(0)
,m_useDistortionFutureCosts(false)
,m_isDetailedTranslationReportingEnabled(false) 
,m_onlyDistinctNBest(false)
,m_computeLMBackoffStats(false)
,m_factorDelimiter("|") // default delimiter between factors
,m_cachePath (GetTempFolder())
{
  m_maxFactorIdx[0] = 0;  // source side
  m_maxFactorIdx[1] = 0;  // target side

	// memory pools
	Phrase::InitializeMemPool();
}

bool StaticData::LoadData(Parameter *parameter)
{
	ResetUserTime();
	m_parameter = parameter;
	
	// verbose level
	m_verboseLevel = 1;
	if (m_parameter->GetParam("verbose").size() == 1)
  {
	m_verboseLevel = Scan<size_t>( m_parameter->GetParam("verbose")[0]);
  }

	if (m_parameter->GetParam("cache-path").size() == 1)
		m_cachePath = m_parameter->GetParam("cache-path")[0];

	// input type has to be specified BEFORE loading the phrase tables!
	if(m_parameter->GetParam("inputtype").size()) 
		m_inputType=Scan<int>(m_parameter->GetParam("inputtype")[0]);
	VERBOSE(2,"input type is: "<<(m_inputType?"confusion net":"text input")<<"\n");

	// factor delimiter
	if (m_parameter->GetParam("factor-delimiter").size() > 0) {
		m_factorDelimiter = m_parameter->GetParam("factor-delimiter")[0];
	}

	// n-best
	if (m_parameter->GetParam("n-best-list").size() >= 2)
	{
		m_nBestFilePath = m_parameter->GetParam("n-best-list")[0];
		m_nBestSize = Scan<size_t>( m_parameter->GetParam("n-best-list")[1] );
		m_onlyDistinctNBest=(m_parameter->GetParam("n-best-list").size()>2 && m_parameter->GetParam("n-best-list")[2]=="distinct");
		
		if (m_parameter->GetParam("n-best-factor").size() > 0) 
		{
			m_nBestFactor = Scan<size_t>( m_parameter->GetParam("n-best-factor")[0]);
		}
	}
	else
	{
		m_nBestSize = 0;
	}
	
	// include feature names in the n-best list
	SetBooleanParameter( &m_labeledNBestList, "labeled-n-best-list", true );

	// printing source phrase spans
	SetBooleanParameter( &m_reportSegmentation, "report-segmentation", false );

	//input factors
	const vector<string> &inputFactorVector = m_parameter->GetParam("input-factors");
	for(size_t i=0; i<inputFactorVector.size(); i++) 
	{
		m_inputFactorOrder.push_back(Scan<FactorType>(inputFactorVector[i]));
	}
	if(m_inputFactorOrder.empty())
	{
		UserMessage::Add(string("no input factor specified in config file"));
		return false;
	}

	//output factors
	const vector<string> &outputFactorVector = m_parameter->GetParam("output-factors");
	for(size_t i=0; i<outputFactorVector.size(); i++) 
	{
		m_outputFactorOrder.push_back(Scan<FactorType>(outputFactorVector[i]));
	}
	if(m_outputFactorOrder.empty())
	{ // default. output factor 0
		m_outputFactorOrder.push_back(0);
	}

	//source word deletion
	SetBooleanParameter( &m_wordDeletionEnabled, "phrase-drop-allowed", false );

	// additional output
	SetBooleanParameter( &m_isDetailedTranslationReportingEnabled, 
			     "translation-details", false );

	SetBooleanParameter( &m_computeLMBackoffStats, "lmstats", false );
	if (m_computeLMBackoffStats && 
	    ! m_isDetailedTranslationReportingEnabled) {
	  TRACE_ERR( "-lmstats implies -translation-details, enabling" << std::endl);
	  m_isDetailedTranslationReportingEnabled = true;
	}

	// score weights
	const vector<string> distortionWeights = m_parameter->GetParam("weight-d");	
	m_weightDistortion				= Scan<float>(distortionWeights[0]);
	m_weightWordPenalty				= Scan<float>( m_parameter->GetParam("weight-w")[0] );
	m_weightUnknownWord				= 1; // do we want to let mert decide weight for this ??

	m_distortionScoreProducer = new DistortionScoreProducer(m_scoreIndexManager);
	m_allWeights.push_back(m_weightDistortion);

	m_wpProducer = new WordPenaltyProducer(m_scoreIndexManager);
	m_allWeights.push_back(m_weightWordPenalty);

	m_unknownWordPenaltyProducer = new UnknownWordPenaltyProducer(m_scoreIndexManager);
	m_allWeights.push_back(m_weightUnknownWord);

	// misc
	m_maxHypoStackSize = (m_parameter->GetParam("stack").size() > 0)
				? Scan<size_t>(m_parameter->GetParam("stack")[0]) : DEFAULT_MAX_HYPOSTACK_SIZE;
	m_maxDistortion = (m_parameter->GetParam("distortion-limit").size() > 0) ?
		Scan<int>(m_parameter->GetParam("distortion-limit")[0])
		: -1;
	m_useDistortionFutureCosts = (m_parameter->GetParam("use-distortion-future-costs").size() > 0) 
		? Scan<bool>(m_parameter->GetParam("use-distortion-future-costs")[0]) : true;
	//TRACE_ERR( "using distortion future costs? "<<UseDistortionFutureCosts()<<"\n");
	
	m_beamThreshold = (m_parameter->GetParam("beam-threshold").size() > 0) ?
		TransformScore(Scan<float>(m_parameter->GetParam("beam-threshold")[0]))
		: TransformScore(DEFAULT_BEAM_THRESHOLD);

	m_maxNoTransOptPerCoverage = (m_parameter->GetParam("max-trans-opt-per-coverage").size() > 0)
				? Scan<size_t>(m_parameter->GetParam("max-trans-opt-per-coverage")[0]) : DEFAULT_MAX_TRANS_OPT_SIZE;
	//TRACE_ERR( "max translation options per coverage span: "<<m_maxNoTransOptPerCoverage<<"\n");

	m_maxNoPartTransOpt = (m_parameter->GetParam("max-partial-trans-opt").size() > 0)
				? Scan<size_t>(m_parameter->GetParam("max-partial-trans-opt")[0]) : DEFAULT_MAX_PART_TRANS_OPT_SIZE;

	// Unknown Word Processing -- wade
	//TODO replace this w/general word dropping -- EVH
	SetBooleanParameter( &m_dropUnknown, "drop-unknown", false );

	if (!LoadLexicalReorderingModel()) return false;
	if (!LoadLanguageModels()) return false;
	if (!LoadGenerationTables()) return false;
	if (!LoadPhraseTables()) return false;
	if (!LoadMapping()) return false;

	return true;
}

void StaticData::SetBooleanParameter( bool *parameter, string parameterName, bool defaultValue ) 
{
  // default value if nothing is specified
  *parameter = defaultValue;
  if (! m_parameter->isParamSpecified( parameterName ) )
  {
    return;
  }

  // if parameter is just specified as, e.g. "-parameter" set it true
  if (m_parameter->GetParam( parameterName ).size() == 0) 
  {
    *parameter = true;
  }

  // if paramter is specified "-parameter true" or "-parameter false"
  else if (m_parameter->GetParam( parameterName ).size() == 1) 
  {
    *parameter = Scan<bool>( m_parameter->GetParam( parameterName )[0]);
  }
}

// helper fn
//! delete old cached files that haven't been used for a while
void DeleteCacheFile(const string &cachePathStr)
{
	using namespace boost::filesystem;
	using namespace boost::posix_time;
	
	ptime now(second_clock::local_time());

	path cachePath(cachePathStr, native);
	directory_iterator end;
	for (directory_iterator iter(cachePath) ; iter != end ; ++iter)
	{
		path &filePath = *iter;
		// delete of old
		if (filePath.leaf().find(PROJECT_NAME) == 0)
		{
			ptime lastWrite;
			time_t t = last_write_time(filePath);
			lastWrite = from_time_t(t);

			if ( (lastWrite + boost::gregorian::days(7) ) < now )
				remove(filePath);
		}
	}
}

StaticData::~StaticData()
{
	delete m_parameter;
	RemoveAllInColl(m_phraseDictionary);
	RemoveAllInColl(m_generationDictionary);
	RemoveAllInColl(m_languageModel);
	RemoveAllInColl(m_decodeStepList);
	RemoveAllInColl(m_reorderModels);
	
	// small score producers
	delete m_distortionScoreProducer;
	delete m_wpProducer;
	delete m_unknownWordPenaltyProducer;

	// memory pools
	Phrase::FinalizeMemPool();

	DeleteCacheFile(GetCachePath());
}

bool StaticData::LoadLexicalReorderingModel()
{
	// load Lexical Reordering model
	
	m_sourceStartPosMattersForRecombination = false;
	
	//distortion weights
	const vector<string> distortionWeights = m_parameter->GetParam("weight-d");	
	//distortional model weights (first weight is distance distortion)
	std::vector<float> distortionModelWeights;
 	for(size_t dist=1; dist < distortionWeights.size(); dist++)
 	{
 		distortionModelWeights.push_back(Scan<float>(distortionWeights[dist]));
 	}

	const vector<string> &lrFileVector = 
		m_parameter->GetParam("distortion-file");	

	for(unsigned int i=0; i< lrFileVector.size(); i++ ) //loops for each distortion model
	{
		vector<string> specification = Tokenize<string>(lrFileVector[i]," ");
			if (specification.size() != 4 )
			{
			  TRACE_ERR("ERROR: Expected format 'factors type weight-count filePath' in specification of distortion file " << i << std::endl << lrFileVector[i] << std::endl);
			  return false;
			}
	  
		//defaults, but at least one of these per model should be explicitly specified in the .ini file
		int orientation = DistortionOrientationType::Msd, 
		  direction = LexReorderType::Backward,
		  condition = LexReorderType::Fe;

		//Loop through, overriding defaults with specifications
		vector<string> parameters = Tokenize<string>(specification[1],"-");
		for (size_t param=0; param<parameters.size(); param++)
		{
			string val = ToLower(parameters[param]);
			//orientation 
			if(val == "monotone" || val == "monotonicity")
				orientation = DistortionOrientationType::Monotone; 
			else if(val == "msd" || val == "orientation")
				orientation = DistortionOrientationType::Msd;
			//direction
			else if(val == "forward")
				direction = LexReorderType::Forward;
			else if(val == "backward" || val == "unidirectional")
				direction = LexReorderType::Backward; 
			else if(val == "bidirectional")
				direction = LexReorderType::Bidirectional;
			//condition
			else if(val == "f")
				condition = LexReorderType::F; 
			else if(val == "fe")
				condition = LexReorderType::Fe; 
			//unknown specification
			else {
			  TRACE_ERR("ERROR: Unknown orientation type specification '" << val << "'" << endl);
			  return false;
			}
			if (orientation == DistortionOrientationType::Msd) 
				m_sourceStartPosMattersForRecombination = true;
		}
  
		//compute the number of weights that ought to be in the table from this
		size_t numWeightsInTable = 0;
		if(orientation == DistortionOrientationType::Monotone)
		{
			numWeightsInTable = 2;
		}
		else
		{
			numWeightsInTable = 3;
		}
		if(direction == LexReorderType::Bidirectional)
		{
			numWeightsInTable *= 2;
		}
		size_t specifiedNumWeights = Scan<size_t>(specification[2]);
		if (specifiedNumWeights != numWeightsInTable) 
		{
			stringstream strme;
		  strme << "specified number of weights (" 
			    << specifiedNumWeights 
			    << ") does not match correct number of weights for this type (" 
			    << numWeightsInTable << std::endl;
		  UserMessage::Add(strme.str());
    }

		//factors involved in this table
		vector<string> inputfactors = Tokenize(specification[0],"-");
		vector<FactorType> 	input,output;
		if(inputfactors.size() > 1)
		{
			input	= Tokenize<FactorType>(inputfactors[0],",");
			output= Tokenize<FactorType>(inputfactors[1],",");
		}
		else
		{
			input.push_back(0); // default, just in case the user is actually using a bidirectional model
			output = Tokenize<FactorType>(inputfactors[0],",");
		}
		std::vector<float> m_lexWeights; 			//will store the weights for this particular distortion reorderer
		std::vector<float> newLexWeights;     //we'll remove the weights used by this distortion reorder, leaving the weights yet to be used
		if(specifiedNumWeights == 1) // this is useful if the user just wants to train one weight for the model
		{
			//add appropriate weight to weight vector
			assert(distortionModelWeights.size()> 0); //if this fails the user has not specified enough weights
			float wgt = distortionModelWeights[0];
			for(size_t i=0; i<numWeightsInTable; i++)
			{
				m_lexWeights.push_back(wgt);
			}
			//update the distortionModelWeight vector to remove these weights
			std::vector<float> newLexWeights; //plus one as the first weight should always be distance-distortion
			for(size_t i=1; i<distortionModelWeights.size(); i++)
			{
				newLexWeights.push_back(distortionModelWeights[i]);
			}
			distortionModelWeights = newLexWeights;
		}
		else
		{
			//add appropriate weights to weight vector
			for(size_t i=0; i< numWeightsInTable; i++)
			{
				assert(i < distortionModelWeights.size()); //if this fails the user has not specified enough weights
				m_lexWeights.push_back(distortionModelWeights[i]);
			}
			//update the distortionModelWeight vector to remove these weights
			for(size_t i=numWeightsInTable; i<distortionModelWeights.size(); i++)
			{
				newLexWeights.push_back(distortionModelWeights[i]);
			}
			distortionModelWeights = newLexWeights;
			
		}
		assert(m_lexWeights.size() == numWeightsInTable);		//the end result should be a weight vector of the same size as the user configured model
		//			TRACE_ERR( "distortion-weights: ");
		//for(size_t weight=0; weight<m_lexWeights.size(); weight++)
		//{
		//	TRACE_ERR( m_lexWeights[weight] << "\t");
		//}
		//TRACE_ERR( endl);

		// loading the file
		std::string	filePath= specification[3];
		PrintUserTime(string("Start loading distortion table ") + filePath);
		m_reorderModels.push_back(new LexicalReordering(filePath
																									, orientation
																									, direction
																									, condition
																									, m_lexWeights
																									, input
																									, output
																									, m_scoreIndexManager));
	}
	
	return true;
}

bool StaticData::LoadLanguageModels()
{
	if (m_parameter->GetParam("lmodel-file").size() > 0)
	{
		// weights
		vector<float> weightAll = Scan<float>(m_parameter->GetParam("weight-l"));
		
		//TRACE_ERR( "weight-l: ");
		//
		for (size_t i = 0 ; i < weightAll.size() ; i++)
		{
			//	TRACE_ERR( weightAll[i] << "\t");
			m_allWeights.push_back(weightAll[i]);
		}
		//TRACE_ERR( endl);
	

	  // initialize n-gram order for each factor. populated only by factored lm
		const vector<string> &lmVector = m_parameter->GetParam("lmodel-file");

		for(size_t i=0; i<lmVector.size(); i++) 
		{
			vector<string>	token		= Tokenize(lmVector[i]);
			if (token.size() != 4 )
			{
				UserMessage::Add("Expected format 'LM-TYPE FACTOR-TYPE NGRAM-ORDER filePath'");
				return false;
			}
			// type = implementation, SRI, IRST etc
			LMImplementation lmImplementation = static_cast<LMImplementation>(Scan<int>(token[0]));
			
			// factorType = 0 = Surface, 1 = POS, 2 = Stem, 3 = Morphology, etc
			vector<FactorType> 	factorTypes		= Tokenize<FactorType>(token[1], ",");
			
			// nGramOrder = 2 = bigram, 3 = trigram, etc
			size_t nGramOrder = Scan<int>(token[2]);
			
			string &languageModelFile = token[3];

			PrintUserTime(string("Start loading LanguageModel ") + languageModelFile);
			
			LanguageModel *lm = LanguageModelFactory::CreateLanguageModel(lmImplementation, factorTypes     
                                   									, nGramOrder, languageModelFile, weightAll[i]
																										, m_scoreIndexManager);
      if (lm == NULL) 
      {
      	UserMessage::Add("no LM created. We probably don't have it compiled");
      	return false;
      }

			m_languageModel.push_back(lm);
		}
	}
  // flag indicating that language models were loaded,
  // since phrase table loading requires their presence
  m_fLMsLoaded = true;
  PrintUserTime("Finished loading LanguageModels");
  return true;
}

bool StaticData::LoadGenerationTables()
{
	if (m_parameter->GetParam("generation-file").size() > 0) 
	{
		const vector<string> &generationVector = m_parameter->GetParam("generation-file");
		const vector<float> &weight = Scan<float>(m_parameter->GetParam("weight-generation"));

		TRACE_ERR( "weight-generation: ");
		for (size_t i = 0 ; i < weight.size() ; i++)
		{
				TRACE_ERR( weight[i] << "\t");
		}
		TRACE_ERR( endl);
		size_t currWeightNum = 0;
		
		for(size_t currDict = 0 ; currDict < generationVector.size(); currDict++) 
		{
			vector<string>			token		= Tokenize(generationVector[currDict]);
			vector<FactorType> 	input		= Tokenize<FactorType>(token[0], ",")
													,output	= Tokenize<FactorType>(token[1], ",");
      m_maxFactorIdx[1] = CalcMax(m_maxFactorIdx[1], input, output);
			string							filePath;
			size_t							numFeatures = 1;
			numFeatures = Scan<size_t>(token[2]);
			filePath = token[3];
			
			TRACE_ERR( filePath << endl);

			m_generationDictionary.push_back(new GenerationDictionary(numFeatures, m_scoreIndexManager));
			assert(m_generationDictionary.back() && "could not create GenerationDictionary");
			if (!m_generationDictionary.back()->Load(input
																		, output
																		, filePath
																		, Output				// always target, should we allow source?
																		))
			{
				delete m_generationDictionary.back();
				return false;
			}
			for(size_t i = 0; i < numFeatures; i++) {
				assert(currWeightNum < weight.size());
				m_allWeights.push_back(weight[currWeightNum++]);
			}
		}
		if (currWeightNum != weight.size()) {
			TRACE_ERR( "  [WARNING] config file has " << weight.size() << " generation weights listed, but the configuration for generation files indicates there should be " << currWeightNum << "!\n");
		}
	}
	
	return true;
}

#undef max

bool StaticData::LoadPhraseTables()
{
	VERBOSE(2,"About to LoadPhraseTables" << endl);

	// language models must be loaded prior to loading phrase tables
	assert(m_fLMsLoaded);
	// load phrase translation tables
  if (m_parameter->GetParam("ttable-file").size() > 0)
	{
		// weights
		vector<float> weightAll									= Scan<float>(m_parameter->GetParam("weight-t"));
		
		//TRACE_ERR("weight-t: ");
		//for (size_t i = 0 ; i < weightAll.size() ; i++)
		//{
		//		TRACE_ERR(weightAll[i] << "\t");
		//}
		//TRACE_ERR( endl;

		PhraseList inputPhrases;
		string inputFileHash;
		if (m_parameter->GetParam("input-file").size() > 0)
		{ 
			inputFileHash = GetMD5Hash(m_parameter->GetParam("input-file")[0]);
			
			// load input for filtering
			TRACE_ERR( "Begin loading input for filtering" << endl);
			inputPhrases.Load(m_parameter->GetParam("input-file")[0]);
			TRACE_ERR( "Completed loading input for filtering" << endl);
		}

		const vector<string> &translationVector = m_parameter->GetParam("ttable-file");
		vector<size_t>	maxTargetPhrase					= Scan<size_t>(m_parameter->GetParam("ttable-limit"));

		size_t index = 0;
		size_t weightAllOffset = 0;
		for(size_t currDict = 0 ; currDict < translationVector.size(); currDict++) 
		{
			vector<string>                  token           = Tokenize(translationVector[currDict]);
			//characteristics of the phrase table
			vector<FactorType>	input		= Tokenize<FactorType>(token[0], ",")
													,output = Tokenize<FactorType>(token[1], ",");
			m_maxFactorIdx[0] = CalcMax(m_maxFactorIdx[0], input);
			m_maxFactorIdx[1] = CalcMax(m_maxFactorIdx[1], output);
      m_maxNumFactors = std::max(m_maxFactorIdx[0], m_maxFactorIdx[1]) + 1;
			string filePath= token[3];
			size_t numScoreComponent = Scan<size_t>(token[2]);

			PrefixPhraseCollection inputPrefix(input, inputPhrases);

			// weights for this phrase dictionary
			// first InputScores (if any), then translation scores
			vector<float> weight;

			if(currDict==0 && m_inputType)
			{	// TODO. find what the assumptions made by confusion network about phrase table output which makes
				// it only work with binrary file. This is a hack 	
				m_numInputScores=m_parameter->GetParam("weight-i").size();
				for(unsigned k=0;k<m_numInputScores;++k)
					weight.push_back(Scan<float>(m_parameter->GetParam("weight-i")[k]));
			}
			else{
				m_numInputScores=0;
			}
			
			for (size_t currScore = 0 ; currScore < numScoreComponent; currScore++)
				weight.push_back(weightAll[weightAllOffset + currScore]);			
			

			if(weight.size() - m_numInputScores != numScoreComponent) 
			{
				stringstream strme;
				strme << "Your phrase table has " << numScoreComponent
							<< " scores, but you specified " << weight.size() << " weights!";
				UserMessage::Add(strme.str());

				return false;
			}
						
			weightAllOffset += numScoreComponent;
			numScoreComponent += m_numInputScores;
						
			assert(numScoreComponent==weight.size());

			std::copy(weight.begin(),weight.end(),std::back_inserter(m_allWeights));
			

			PrintUserTime(string("Start loading PhraseTable ") + filePath);
			if (!FileExists(filePath+".binphr.idx"))
			{					
				// does cached filtering exist for this table, given input ?
				string phraseTableHash	= GetMD5Hash(filePath);

				// input factors of input
				const vector<string> &inputFactorVector = m_parameter->GetParam("input-factors");
				stringstream inputFactorsStrme("");
				for (size_t idx = 0 ; idx < inputFactorVector.size() ; ++idx)
					inputFactorsStrme << inputFactorVector[idx];


   			string hashFilePath			= GetCachePath()
   																+ PROJECT_NAME + "--1--" 	// filter file version
   																+ inputFileHash + "--"
																	+ inputFactorsStrme.str() // input factors of input
   																+ phraseTableHash + "--"
																	+ token[0] // input factors of phrase table
																	+ ".txt";
				bool filter;
				if (FileExists(hashFilePath))
				{ // load filtered file instead
					filter = false;
					filePath = hashFilePath;
				}
				else
				{ // load original file & create hash file
					filter = true;
				}

				// LOAD
				VERBOSE(2,"using standard phrase tables");
				PhraseDictionaryMemory *pd=new PhraseDictionaryMemory(numScoreComponent, m_scoreIndexManager);
				if (!pd->Load(input
								 , output
								 , filePath
								 , weight
								 , maxTargetPhrase[index]
								 , GetAllLM()
								 , GetWeightWordPenalty()
								 , filter
								 , inputPrefix
								 , hashFilePath))
				{
					delete pd;
					return false;
				}

				m_phraseDictionary.push_back(pd);
			}
			else 
			{
				TRACE_ERR( "using binary phrase tables for idx "<<currDict<<"\n");
				PhraseDictionaryTreeAdaptor *pd=new PhraseDictionaryTreeAdaptor(
																									numScoreComponent
																									,(unsigned) (currDict==0 ? m_numInputScores : 0)
																									, m_scoreIndexManager);
				if (!pd->Load(input,output,filePath,weight,
									 maxTargetPhrase[index],
									 GetAllLM(),
									 GetWeightWordPenalty()))
				{
					delete pd;
					return false;
				}
				m_phraseDictionary.push_back(pd);
			}

			index++;
		}

	}	

	PrintUserTime("Finished loading phrase tables");
	return true;
}

bool StaticData::LoadMapping()
{
	// mapping
	const vector<string> &mappingVector = m_parameter->GetParam("mapping");
	DecodeStep *prev = 0;
	for(size_t i=0; i<mappingVector.size(); i++) 
	{
		vector<string>	token		= Tokenize(mappingVector[i]);
		
		// using 0 T filePath - lexi's smoothing format. not used 
		if (token.size() == 3)
		{
			assert(Scan<size_t>(token[0])==0);
			token.erase(token.begin());
		}

		if (token.size() == 2) 
		{
			DecodeType decodeType = token[0] == "T" ? Translate : Generate;
			size_t index = Scan<size_t>(token[1]);
			DecodeStep* decodeStep = 0;
			switch (decodeType) {
				case Translate:
					if(index>=m_phraseDictionary.size())
						{
							stringstream strme;
							strme << "No phrase dictionary with index "
										<< index << " available!";
							UserMessage::Add(strme.str());
							return false;
						}
					decodeStep = new DecodeStepTranslation(m_phraseDictionary[index], prev);
				break;
				case Generate:
					if(index>=m_generationDictionary.size())
						{
							stringstream strme;
							strme << "No generation dictionary with index "
										<< index << " available!";
							UserMessage::Add(strme.str());
							return false;
						}
					decodeStep = new DecodeStepGeneration(m_generationDictionary[index], prev);
				break;
				case InsertNullFertilityWord:
					assert(!"Please implement NullFertilityInsertion.");
				break;
			}
			assert(decodeStep);
			assert(decodeStep->GetId() == m_decodeStepList.size());
			m_decodeStepList.push_back(decodeStep);
			prev = decodeStep;
		} else {
			UserMessage::Add("Malformed mapping!");
			return false;
		}
	}
	
	return true;
}

void StaticData::CleanUpAfterSentenceProcessing() const
{
	for(size_t i=0;i<m_phraseDictionary.size();++i)
		m_phraseDictionary[i]->CleanUp();
	for(size_t i=0;i<m_generationDictionary.size();++i)
		m_generationDictionary[i]->CleanUp();
  
  //something LMs could do after each sentence 
  LMList::const_iterator iterLM;
	for (iterLM = m_languageModel.begin() ; iterLM != m_languageModel.end() ; ++iterLM)
	{
		LanguageModel &languageModel = **iterLM;
    languageModel.CleanUpAfterSentenceProcessing();
	}
}

/** initialize the translation and language models for this sentence 
    (includes loading of translation table entries on demand, if
    binary format is used) */
void StaticData::InitializeBeforeSentenceProcessing(InputType const& in) const
{
	for(size_t i=0;i<m_phraseDictionary.size();++i) 
	{
		m_phraseDictionary[i]->InitializeForInput(in);
  }
  //something LMs could do before translating a sentence
  LMList::const_iterator iterLM;
	for (iterLM = m_languageModel.begin() ; iterLM != m_languageModel.end() ; ++iterLM)
	{
		LanguageModel &languageModel = **iterLM;
    languageModel.InitializeBeforeSentenceProcessing();
	}
  
}

void StaticData::SetWeightsForScoreProducer(const ScoreProducer* sp, const std::vector<float>& weights)
{
  const size_t id = sp->GetScoreBookkeepingID();
  const size_t begin = m_scoreIndexManager.GetBeginIndex(id);
  const size_t end = m_scoreIndexManager.GetEndIndex(id);
  assert(end - begin == weights.size());
  if (m_allWeights.size() < end)
    m_allWeights.resize(end);
  std::vector<float>::const_iterator weightIter = weights.begin();
  for (size_t i = begin; i < end; i++)
    m_allWeights[i] = *weightIter++;
}
