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
#include <cassert>
#include <boost/filesystem/operations.hpp> // boost::filesystem::exists
#include <boost/algorithm/string/case_conv.hpp> //boost::algorithm::to_lower
#include "PhraseDictionary.h"
#include "DecodeStep_Translation.h"
#include "DecodeStep_Generation.h"
#include "GenerationDictionary.h"
#include "DummyScoreProducers.h"
#include "StaticData.h"
#include "Util.h"
#include "FactorCollection.h"
#include "HypothesisCollection.h"
#include "Timer.h"
#include "LanguageModelSingleFactor.h"
#include "LanguageModelMultiFactor.h"
#include "LanguageModelFactory.h"
#include "LexicalReordering.h"
#include "SentenceStats.h"
#include "PhraseDictionaryTreeAdaptor.h"

using namespace std;

extern Timer timer;

StaticData* StaticData::s_instance(0);

StaticData::StaticData()
:m_inputOutput(NULL)
,m_fLMsLoaded(false)
,m_inputType(0)
,m_numInputScores(0)
,m_distortionScoreProducer(0)
,m_wpProducer(0)
,m_useDistortionFutureCosts(false)
,m_isDetailedTranslationReportingEnabled(false) 
,m_onlyDistinctNBest(false)
,m_computeLMBackoffStats(false)
{
	s_instance = this;

	// memory pools
	Phrase::InitializeMemPool();
}

bool StaticData::LoadParameters(int argc, char* argv[])
{
	if (!m_parameter.LoadParam(argc, argv)) {
		m_parameter.Explain();
		return false;
	}

	// input type has to be specified BEFORE loading the phrase tables!
	if(m_parameter.GetParam("inputtype").size()) 
		m_inputType=Scan<int>(m_parameter.GetParam("inputtype")[0]);
	TRACE_ERR("input type is: "<<m_inputType<<"  (0==default: text input, else confusion net format)\n");

	// mysql
	m_mySQLParam = m_parameter.GetParam("mysql");

	if (m_parameter.GetParam("cache-path").size() == 1)
		m_cachePath = m_parameter.GetParam("cache-path")[0];
	else
		m_cachePath = GetTempFolder();

	// n-best
	if (m_parameter.GetParam("n-best-list").size() >= 2)
	{
		m_nBestFilePath = m_parameter.GetParam("n-best-list")[0];
		m_nBestSize = Scan<size_t>( m_parameter.GetParam("n-best-list")[1] );
		m_onlyDistinctNBest=(m_parameter.GetParam("n-best-list").size()>2 && m_parameter.GetParam("n-best-list")[2]=="distinct");
	}
	else
	{
		m_nBestSize = 0;
	}
	
	TRACE_ERR(m_parameter.GetParam("labeled-n-best-list").size() << endl);
	if (m_parameter.GetParam("labeled-n-best-list").size() == 1)
	{
		m_labeledNBestList = Scan<bool>( m_parameter.GetParam("labeled-n-best-list")[0]);
	}
	else
	{
		m_labeledNBestList = true;
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

	// printing source phrase spans
	if (m_parameter.GetParam("report-source-span").size() > 0)
		m_reportSourceSpan = Scan<bool>(m_parameter.GetParam("report-source-span")[0]);
	else
        m_reportSourceSpan = false;

	// print all factors of output translations
	if (m_parameter.GetParam("report-all-factors").size() > 0)
		m_reportAllFactors = Scan<bool>(m_parameter.GetParam("report-all-factors")[0]);
	else
        m_reportAllFactors = false;

	//distortion weights
	 const vector<string> distortionWeights = m_parameter.GetParam("weight-d");	
	 //distortional model weights (first weight is distance distortion)
	 std::vector<float> distortionModelWeights;
	 	for(size_t dist=1; dist < distortionWeights.size(); dist++)
	 	{
	 			distortionModelWeights.push_back(Scan<float>(distortionWeights[dist]));
	 	}


	//input factors
	const vector<string> &inputFactorVector = m_parameter.GetParam("input-factors");
	for(size_t i=0; i<inputFactorVector.size(); i++) 
	{
		m_inputFactorOrder.push_back(Scan<FactorType>(inputFactorVector[i]));
	}
	if(m_inputFactorOrder.empty())
	{
		std::cerr<<"ERROR: no input factor specified in config file"
			" (param input-factors) -> abort!\n";
		abort();
	}

	//output factors
	const vector<string> &outputFactorVector = m_parameter.GetParam("output-factors");
	for(size_t i=0; i<outputFactorVector.size(); i++) 
	{
		m_outputFactorOrder.push_back(Scan<FactorType>(outputFactorVector[i]));
	}
	if(m_outputFactorOrder.empty())
	{ // default. output factor 0
		m_outputFactorOrder.push_back(0);
	}

	//source word deletion
	if(m_parameter.GetParam("phrase-drop-allowed").size() > 0)
	{
		m_wordDeletionEnabled = Scan<bool>(m_parameter.GetParam("phrase-drop-allowed")[0]);
	}
	else
	{
		m_wordDeletionEnabled = false;
	}
	if(m_parameter.GetParam("translation-details").size() > 0) {
	  m_isDetailedTranslationReportingEnabled = Scan<bool>( m_parameter.GetParam("translation-details")[0]);
	}

	if(m_parameter.GetParam("lmstats").size() > 0) {
	  m_computeLMBackoffStats = Scan<bool>( m_parameter.GetParam("lmstats")[0]);
		if (!m_isDetailedTranslationReportingEnabled) {
			std::cerr << "-lmstats implies -translation-details, enabling" << std::endl;
			m_isDetailedTranslationReportingEnabled = true;
		}
	}
	// load Lexical Reordering model
	const vector<string> &lrFileVector = 
		m_parameter.GetParam("distortion-file");	
		for(unsigned int i=0; i< lrFileVector.size(); i++ ) //loops for each distortion model
		{
			vector<string>	token	= Tokenize(lrFileVector[i]);
			//characteristics of the phrase table
			//TODO: need to change in the case that the phrase table not dependent on the source, don't need to configure the input format in that case.
			vector<string> inputfactors = Tokenize(token[0],"-");
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
			size_t mertOneWeight = Scan<size_t>(token[1]);
			size_t numberWeights = Scan<size_t>(token[2]);
			std::string	filePath= token[3];

			std::vector<float> m_lexWeights; 			//get the weights for this particular distortion reorderer
			std::vector<float> newLexWeights;     //will remove the weights used by this distortion reorder, leaving the weights yet to be used
			if(mertOneWeight == 1) // this is useful if the user just wants to train one weight for the model
			{
				//add appropriate weight to weight vector
				assert(distortionModelWeights.size()> 0); //if this fails the user has not specified enough weights
				float wgt = distortionModelWeights[0];
				for(size_t i=0; i<numberWeights; i++)
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
				for(size_t i=0; i< numberWeights; i++)
				{
					assert(i < distortionModelWeights.size()); //if this fails the user has not specified enough weights
					m_lexWeights.push_back(distortionModelWeights[i]);
				}
				//update the distortionModelWeight vector to remove these weights
				for(size_t i=numberWeights; i<distortionModelWeights.size(); i++)
				{
					newLexWeights.push_back(distortionModelWeights[i]);
				}
				distortionModelWeights = newLexWeights;
				
			}
			TRACE_ERR("distortion-weights: ");
			for(size_t weight=0; weight<m_lexWeights.size(); weight++)
			{
					TRACE_ERR(m_lexWeights[weight] << "\t");
			}
			TRACE_ERR(endl);
			//if this went wrong, something went wrong in the parsing.
			const vector<string> &lrTypeVector = 	m_parameter.GetParam("distortion");	
			//defaults, but at least one of these per model should be explicitly specified in the .ini file
			int orientation = DistortionOrientationType::Msd, 
					direction = LexReorderType::Bidirectional, 
					condition = LexReorderType::Fe;

			if(lrTypeVector.size() < i)			
			{
				std::cerr<<"ERROR: please specify one line of configuration under [distortion] per distortion model in the moses configuration file\n";
				abort();
			}

			//Loop through, overriding defaults with specifications
			vector<string> parameters = Tokenize<string>(lrTypeVector[i],"-");
			for (size_t param=0; param<parameters.size(); param++)
			{
				string val = parameters[param];
				boost::algorithm::to_lower(val);
				//orientation 
				if(val == "monotone")
					{orientation = DistortionOrientationType::Monotone; TRACE_ERR("monotone\n");}
				else if(val == "msd")
					{orientation = DistortionOrientationType::Msd;  TRACE_ERR("msd\n");}
				//direction
				else if(val == "forward")
					{direction = LexReorderType::Forward;  TRACE_ERR("forward\n");}
				else if(val == "backward")
					{direction = LexReorderType::Backward;  TRACE_ERR("backward\n");}
				else if(val == "bidirectional")
					{direction = LexReorderType::Bidirectional;  TRACE_ERR("bidirectional\n");}
				//condition
				else if(val == "f")
					{condition = LexReorderType::F;  TRACE_ERR("f\n");}
				else if(val == "fe")
					{condition = LexReorderType::Fe;  TRACE_ERR("fe\n");}
			}
			timer.check("Starting to load lexical reorder table...");
			TRACE_ERR(filePath << "...");
 			m_reorderModels.push_back(new LexicalReordering(filePath, orientation, direction, condition, m_lexWeights, input, output));
			timer.check("Finished loading lexical reorder table.");
		}
		if (m_parameter.GetParam("lmodel-file").size() > 0)
		{
			// weights
			vector<float> weightAll = Scan<float>(m_parameter.GetParam("weight-l"));
			
			TRACE_ERR("weight-l: ");
	
			for (size_t i = 0 ; i < weightAll.size() ; i++)
			{
				TRACE_ERR(weightAll[i] << "\t");
				m_allWeights.push_back(weightAll[i]);
			}
			TRACE_ERR(endl);
		

	  timer.check("Start loading LanguageModels");
	  // initialize n-gram order for each factor. populated only by factored lm
	  for(size_t i=0; i < MAX_NUM_FACTORS ; i++)
	  	m_maxNgramOrderForFactor[i] = 0;
	  
		const vector<string> &lmVector = m_parameter.GetParam("lmodel-file");

		for(size_t i=0; i<lmVector.size(); i++) 
		{
				vector<string>	token		= Tokenize(lmVector[i]);
				if (token.size() != 4 )
				{
					TRACE_ERR("Expected format 'LM-TYPE FACTOR-TYPE NGRAM-ORDER filename'");
					return false;
				}
				// type = implementation, SRI, IRST etc
				LMImplementation lmImplementation = static_cast<LMImplementation>(Scan<int>(token[0]));
				
				// factorType = 0 = Surface, 1 = POS, 2 = Stem, 3 = Morphology, etc
				vector<FactorType> 	factorTypes		= Tokenize<FactorType>(token[1], ",");
				
				// nGramOrder = 2 = bigram, 3 = trigram, etc
				size_t nGramOrder = Scan<int>(token[2]);
				
				string &languageModelFile = token[3];
	
				timer.check(("Start loading LanguageModel " + languageModelFile).c_str());
				
				LanguageModel *lm = LanguageModelFactory::CreateLanguageModel(lmImplementation, factorTypes     
	                                   									, nGramOrder, languageModelFile, weightAll[i], m_factorCollection);
	      if (lm == NULL) // no LM created. we prob don't have it compiled
	      	return false;
	
				m_languageModel.push_back(lm);
		  	timer.check(("Finished loading LanguageModel " + languageModelFile).c_str());
			}
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
		size_t currWeightNum = 0;
		
		for(size_t currDict = 0 ; currDict < generationVector.size(); currDict++) 
		{
			vector<string>			token		= Tokenize(generationVector[currDict]);
			bool oldFormat = (token.size() == 3);
			vector<FactorType> 	input		= Tokenize<FactorType>(token[0], ",")
													,output	= Tokenize<FactorType>(token[1], ",");
			string							filePath;
			size_t							numFeatures = 1;
			if (oldFormat)
				filePath = token[2];
			else {
				numFeatures = Scan<size_t>(token[2]);
				filePath = token[3];
			}
			if (!boost::filesystem::exists(boost::filesystem::path(filePath, boost::filesystem::native)))
				{
					std::cerr<<"ERROR: generation dictionary '"<<filePath<<"' does not exist!\n";
					abort();
				}

			TRACE_ERR(filePath << endl);
			if (oldFormat) {
				std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
				             "  [WARNING] config file contains old style generation config format.\n"
				             "  Only the first feature value will be read.  Please use the 4-format\n"
				             "  form (similar to the phrase table spec) to specify the # of features.\n"
				             "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
			}

			m_generationDictionary.push_back(new GenerationDictionary(numFeatures));
			assert(m_generationDictionary.back() && "could not create GenerationDictionary");
			m_generationDictionary.back()->Load(input
																		, output
																		, m_factorCollection
																		, filePath
																		, Output				// always target, should we allow source?
																		, oldFormat);
			for(size_t i = 0; i < numFeatures; i++) {
				assert(currWeightNum < weight.size());
				m_allWeights.push_back(weight[currWeightNum++]);
			}
		}
		if (currWeightNum != weight.size()) {
			std::cerr << "  [WARNING] config file has " << weight.size() << " generation weights listed, but the configuration for generation files indicates there should be " << currWeightNum << "!\n";
		}
	}

	timer.check("Finished loading generation tables");

	// score weights
	m_weightDistortion				= Scan<float>(distortionWeights[0]);
	m_weightWordPenalty				= Scan<float>( m_parameter.GetParam("weight-w")[0] );

	TRACE_ERR("weight-d: " << m_weightDistortion << endl);
	m_distortionScoreProducer = new DistortionScoreProducer;
	m_allWeights.push_back(m_weightDistortion);

	TRACE_ERR("weight-w: " << m_weightWordPenalty << endl);
	m_wpProducer = new WordPenaltyProducer;
	m_allWeights.push_back(m_weightWordPenalty);

	// misc
	m_maxHypoStackSize = (m_parameter.GetParam("stack").size() > 0)
				? Scan<size_t>(m_parameter.GetParam("stack")[0]) : DEFAULT_MAX_HYPOSTACK_SIZE;
	m_maxDistortion = (m_parameter.GetParam("distortion-limit").size() > 0) ?
		Scan<int>(m_parameter.GetParam("distortion-limit")[0])
		: -1;
	m_useDistortionFutureCosts = (m_parameter.GetParam("use-distortion-future-costs").size() > 0) 
		? Scan<int>(m_parameter.GetParam("use-distortion-future-costs")[0]) : 0;
	TRACE_ERR("using distortion future costs? "<<UseDistortionFutureCosts()<<"\n");
	
	m_beamThreshold = (m_parameter.GetParam("beam-threshold").size() > 0) ?
		TransformScore(Scan<float>(m_parameter.GetParam("beam-threshold")[0]))
		: TransformScore(DEFAULT_BEAM_THRESHOLD);

	m_maxNoTransOptPerCoverage = (m_parameter.GetParam("max-trans-opt-per-coverage").size() > 0)
				? Scan<size_t>(m_parameter.GetParam("max-trans-opt-per-coverage")[0]) : DEFAULT_MAX_TRANS_OPT_SIZE;
	TRACE_ERR("max translation options per coverage span: "<<m_maxNoTransOptPerCoverage<<"\n");

	m_maxNoPartTransOpt = (m_parameter.GetParam("max-partial-trans-opt").size() > 0)
				? Scan<size_t>(m_parameter.GetParam("max-partial-trans-opt")[0]) : DEFAULT_MAX_PART_TRANS_OPT_SIZE;
	TRACE_ERR("max partial translation options: "<<m_maxNoPartTransOpt<<"\n");

	// Unknown Word Processing -- wade
	//TODO replace this w/general word dropping -- EVH
	if (m_parameter.GetParam("drop-unknown").size() == 1)
	  { m_dropUnknown = Scan<bool>( m_parameter.GetParam("drop-unknown")[0]); }
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

	LMList::const_iterator iterLM;
	for (iterLM = m_languageModel.begin() ; iterLM != m_languageModel.end() ; ++iterLM)
	{
		delete *iterLM;
	}
	// small score producers
	delete m_distortionScoreProducer;
	delete m_wpProducer;

	// memory pools
	Phrase::FinalizeMemPool();

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

void StaticData::LoadPhraseTables()
{
  LoadPhraseTables(false, "", std::list< Phrase >());
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
		cerr<<"ttable-limits: ";copy(maxTargetPhrase.begin(),maxTargetPhrase.end(),ostream_iterator<size_t>(cerr," "));cerr<<"\n";

		size_t index = 0;
		size_t totalPrevNoScoreComponent = 0;		
		for(size_t currDict = 0 ; currDict < translationVector.size(); currDict++) 
		{
			vector<string>			token		= Tokenize(translationVector[currDict]);
			//characteristics of the phrase table
			vector<FactorType> 	input		= Tokenize<FactorType>(token[0], ",")
													,output	= Tokenize<FactorType>(token[1], ",");
			string							filePath= token[3];
			size_t							noScoreComponent	= Scan<size_t>(token[2]);
			// weights for this phrase dictionary
			vector<float> weight(noScoreComponent);
			for (size_t currScore = 0 ; currScore < noScoreComponent ; currScore++)
				weight[currScore] = weightAll[totalPrevNoScoreComponent + currScore]; 

			if(weight.size()!=noScoreComponent) 
				{
					std::cerr<<"ERROR: your phrase table has "<<noScoreComponent<<" scores, but you specified "<<weight.size()<<" weights!\n";
					abort();
				}

			if(currDict==0 && m_inputType)
				{
					m_numInputScores=m_parameter.GetParam("weight-i").size();
					for(unsigned k=0;k<m_numInputScores;++k)
						weight.push_back(Scan<float>(m_parameter.GetParam("weight-i")[k]));

					noScoreComponent+=m_numInputScores;
				}

			assert(noScoreComponent==weight.size());

			std::copy(weight.begin(),weight.end(),std::back_inserter(m_allWeights));

			totalPrevNoScoreComponent += noScoreComponent;
			string phraseTableHash	= GetMD5Hash(filePath);
			string hashFilePath			= GetCachePath() 
															+ PROJECT_NAME + "--"
															+ token[0] + "--"
															+ inputFileHash + "--" 
															+ phraseTableHash + ".txt";

			timer.check("Start loading PhraseTable");
			using namespace boost::filesystem; 
			if (!exists(path(filePath+".binphr.idx", native)))
				{
					bool filterPhrase;
					/*
					if (filter)
						{
							boost::filesystem::path tempFile(hashFilePath, boost::filesystem::native);
							if (boost::filesystem::exists(tempFile))
								{ // load filtered file instead
									filterPhrase = false;
									filePath = hashFilePath;
								}
							else
								{ // load original file & create hash file
									filterPhrase = true;
								}
						}
					else
						{ // load original file
							filterPhrase = false;
						}
					*/
					// don't do filtering
					filterPhrase = false;
					
					TRACE_ERR(filePath << endl);


					TRACE_ERR("using standard phrase tables");
					PhraseDictionary *pd=new PhraseDictionary(noScoreComponent);
					pd->Load(input
									 , output
									 , m_factorCollection
									 , filePath
									 , hashFilePath
									 , weight
									 , maxTargetPhrase[index]
									 , filterPhrase
									 , inputPhraseList
									 , GetAllLM()
									 , GetWeightWordPenalty()
									 , *this);
					m_phraseDictionary.push_back(pd);
				}
			else 
				{
					TRACE_ERR("using binary phrase tables for idx "<<currDict<<"\n");
					PhraseDictionaryTreeAdaptor *pd=new PhraseDictionaryTreeAdaptor(noScoreComponent,(currDict==0 ? m_numInputScores : 0));
					pd->Create(input,output,m_factorCollection,filePath,weight,
										 maxTargetPhrase[index],
										 GetAllLM(),
										 GetWeightWordPenalty());
					m_phraseDictionary.push_back(pd);
				}

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
	DecodeStep *prev = 0;
	for(size_t i=0; i<mappingVector.size(); i++) 
	{
		vector<string>	token		= Tokenize(mappingVector[i]);
		if (token.size() == 2) 
		{
			DecodeType decodeType = token[0] == "T" ? Translate : Generate;
			size_t index = Scan<size_t>(token[1]);
			DecodeStep* decodeStep = 0;
			switch (decodeType) {
				case Translate:
					if(index>=m_phraseDictionary.size())
						{
							std::cerr<<"ERROR: no phrase dictionary with index "<<index<<" available!\n";
							abort();
						}
					decodeStep = new TranslationDecodeStep(m_phraseDictionary[index], prev);
				break;
				case Generate:
					if(index>=m_generationDictionary.size())
						{
							std::cerr<<"ERROR: no generation dictionary with index "<<index<<" available!\n";
							abort();
						}
					decodeStep = new GenerationDecodeStep(m_generationDictionary[index], prev);
				break;
				case InsertNullFertilityWord:
					assert(!"Please implement NullFertilityInsertion.");
				break;
			}
			assert(decodeStep);
			m_decodeStepList.push_back(decodeStep);
			prev = decodeStep;
		} else {
			std::cerr << "Malformed mapping!\n";
			abort();
		}
	}
}
	
void StaticData::CleanUpAfterSentenceProcessing() 
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

void StaticData::InitializeBeforeSentenceProcessing(InputType const& in) 
{
	for(size_t i=0;i<m_phraseDictionary.size();++i)
  m_phraseDictionary[i]->InitializeForInput(in);
  
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
