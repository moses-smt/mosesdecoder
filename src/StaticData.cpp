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
#include "PhraseDictionaryMemory.h"
#include "DecodeStepTranslation.h"
#include "DecodeStepGeneration.h"
#include "GenerationDictionary.h"
#include "DummyScoreProducers.h"
#include "StaticData.h"
#include "Util.h"
#include "FactorCollection.h"
#include "HypothesisStack.h"
#include "Timer.h"
#include "LanguageModelSingleFactor.h"
#include "LanguageModelMultiFactor.h"
#include "LanguageModelFactory.h"
#include "LexicalReordering.h"
#include "SentenceStats.h"
#include "PhraseDictionaryTreeAdaptor.h"
#include "UserMessage.h"

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
,m_inputType(SentenceInput)
,m_numInputScores(0)
,m_distortionScoreProducer(0)
,m_wpProducer(0)
,m_useDistortionFutureCosts(false)
,m_isDetailedTranslationReportingEnabled(false) 
,m_onlyDistinctNBest(false)
,m_computeLMBackoffStats(false)
,m_factorDelimiter("|") // default delimiter between factors
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

	// input type has to be specified BEFORE loading the phrase tables!
	if(m_parameter->GetParam("inputtype").size()) 
		m_inputType= (InputTypeEnum) Scan<int>(m_parameter->GetParam("inputtype")[0]);
	std::string s_it = "text input";
	if (m_inputType == 1) { s_it = "confusion net"; }
	if (m_inputType == 2) { s_it = "word lattice"; }
	VERBOSE(2,"input type is: "<<s_it<<"\n");

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

	// include word alignment in the n-best list
	SetBooleanParameter( &m_nBestIncludesAlignment, "include-alignment-in-n-best", false );

	// printing source phrase spans
	SetBooleanParameter( &m_reportSegmentation, "report-segmentation", false );

	// print all factors of output translations
	SetBooleanParameter( &m_reportAllFactors, "report-all-factors", false );
	
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
	  VERBOSE(1, "-lmstats implies -translation-details, enabling" << std::endl);
	  m_isDetailedTranslationReportingEnabled = true;
	}

	// score weights
	const vector<string> distortionWeights = m_parameter->GetParam("weight-d");	
	m_weightDistortion				= Scan<float>(distortionWeights[0]);
	m_weightWordPenalty				= Scan<float>( m_parameter->GetParam("weight-w")[0] );
	m_weightUnknownWord				= 1; // do we want to let mert decide weight for this ???

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
		? Scan<bool>(m_parameter->GetParam("use-distortion-future-costs")[0]) : false;
	
	m_beamThreshold = (m_parameter->GetParam("beam-threshold").size() > 0) ?
		TransformScore(Scan<float>(m_parameter->GetParam("beam-threshold")[0]))
		: TransformScore(DEFAULT_BEAM_THRESHOLD);

	m_maxNoTransOptPerCoverage = (m_parameter->GetParam("max-trans-opt-per-coverage").size() > 0)
				? Scan<size_t>(m_parameter->GetParam("max-trans-opt-per-coverage")[0]) : DEFAULT_MAX_TRANS_OPT_SIZE;
	
	m_maxNoPartTransOpt = (m_parameter->GetParam("max-partial-trans-opt").size() > 0)
				? Scan<size_t>(m_parameter->GetParam("max-partial-trans-opt")[0]) : DEFAULT_MAX_PART_TRANS_OPT_SIZE;

	// Unknown Word Processing -- wade
	//TODO replace this w/general word dropping -- EVH
	SetBooleanParameter( &m_dropUnknown, "drop-unknown", false );
	  
	m_decoderType = (DecoderType) ((m_parameter->GetParam("decoder-type").size() > 0) ? Scan<int>(m_parameter->GetParam("decoder-type")[0]) : 0);
	m_mbrScale = (m_parameter->GetParam("mbr-scale").size() > 0)
				? Scan<float>(m_parameter->GetParam("mbr-scale")[0]) : 1.0f;
	
	//default case
	
	if (m_parameter->GetParam("xml-input").size() == 0) m_xmlInputType = XmlPassThrough;
	else if (m_parameter->GetParam("xml-input")[0]=="exclusive") m_xmlInputType = XmlExclusive;
	else if (m_parameter->GetParam("xml-input")[0]=="inclusive") m_xmlInputType = XmlInclusive;
	else if (m_parameter->GetParam("xml-input")[0]=="ignore") m_xmlInputType = XmlIgnore;
	else if (m_parameter->GetParam("xml-input")[0]=="pass-through") m_xmlInputType = XmlPassThrough;
	else {
		UserMessage::Add("invalid xml-input value, must be pass-through, exclusive, inclusive, or ignore");
		return false;
	}
	
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

StaticData::~StaticData()
{
	delete m_parameter;
	RemoveAllInColl(m_phraseDictionary);
	RemoveAllInColl(m_generationDictionary);
	RemoveAllInColl(m_languageModel);
	//need to delete lists within vector as well 
	while (! m_decodeStepVL.empty() )
	{
			list <const DecodeStep *> * ptrList = m_decodeStepVL.back();
			m_decodeStepVL.pop_back();
			while( ! ptrList->empty() ) 
			{
				 const DecodeStep * ptrDecodeStep = ptrList->back();
				 ptrList->pop_back();
				 if (ptrDecodeStep != NULL) 
				 {
					 delete ptrDecodeStep;
					 ptrDecodeStep = NULL;
				 }
			}
			//cout << "list size " << ptrList->size() << endl;
			if (ptrList != NULL) 
			{
				delete ptrList;
				ptrList = NULL;
			}
	}

	RemoveAllInColl(m_reorderModels);
	
	// small score producers
	delete m_distortionScoreProducer;
	delete m_wpProducer;
	delete m_unknownWordPenaltyProducer;

	// memory pools
	Phrase::FinalizeMemPool();

}

bool StaticData::LoadLexicalReorderingModel()
{
  std::cerr << "Loading lexical distortion models...\n";
  const vector<string> fileStr    = m_parameter->GetParam("distortion-file");
  const vector<string> weightsStr = m_parameter->GetParam("weight-d");
  /*old code
  const vector<string> modelStr   = m_parameter.GetParam("distortion-type"); //TODO check name?
  const vector<string> fileStr    = m_parameter.GetParam("distortion-file");
  const vector<string> weightsStr = m_parameter.GetParam("weight-d");
  */
  std::vector<float>   weights;
  int w = 1; //cur weight
  int f = 0; //cur file
  //get weights values
  std::cerr << "have " << fileStr.size() << " models\n";
  for(size_t j = 0; j < weightsStr.size(); ++j){
    weights.push_back(Scan<float>(weightsStr[j]));
  }
  //load all models
  for(size_t i = 0; i < fileStr.size(); ++i)
	{
		//std::cerr << "Model " << i << ":";
    //Todo: 'else' should be 'else if(...)' to check it is a lexical model...
    vector<string> spec = Tokenize<string>(fileStr[f], " ");
    ++f; //mark file as consumed
    if(4 != spec.size()){
			//wrong file specification string...
			std::cerr << "Wrong Lexical Reordering Model Specification for model " << i << "!\n";
			return false;
    }
    //spec[0] = factor map
    //spec[1] = name
    //spec[2] = num weights
    //spec[3] = fileName
    //decode data into these
    vector<FactorType> input,output;
    LexicalReordering::Direction direction;
    LexicalReordering::Condition condition;
    int numWeights;
    //decode factor map
    vector<string> inputfactors = Tokenize(spec[0],"-");
    if(inputfactors.size() == 2){
			input  = Tokenize<FactorType>(inputfactors[0],",");
			output = Tokenize<FactorType>(inputfactors[1],",");
    } 
		else if(inputfactors.size() == 1)
		{
			//if there is only one side assume it is on e side... why?
			output = Tokenize<FactorType>(inputfactors[0],",");
    } 
		else 
		{
			//format error
			return false;
    }
    //decode name
    vector<string> params = Tokenize<string>(spec[1],"-");
    std::string type(ToLower(params[0]));
		std::string dir;
		std::string cond;

		if(3 == params.size())
		{
			//name format is 'type'-'direction'-'condition'
			dir  = ToLower(params[1]);
			cond = ToLower(params[2]);
		} 
		else if(2 == params.size()) 
		{
			//assume name format is 'type'-'condition' with implicit unidirectional
			std::cerr << "Warning: Lexical model type underspecified...assuming unidirectional in model " << i << "\n";
			dir  = "unidirectional";
			cond = ToLower(params[1]);
		} 
		else 
		{
			std::cerr << "Lexical model type underspecified for model " << i << "!\n";
			return false;
		}
    
		if(dir == "forward"){
			direction = LexicalReordering::Forward;
		 } 
		else if(dir == "backward" || dir == "unidirectional" || dir == "uni")
		{
			direction = LexicalReordering::Backward; 
		} 
		else if(dir == "bidirectional" || dir == "bi") 
		{
			direction = LexicalReordering::Bidirectional;
		}
		else 
		{
			std::cerr << "Unknown direction declaration '" << dir << "'for lexical reordering model " << i << "\n";
			return false;
		}
      
		if(cond == "f"){
			condition = LexicalReordering::F; 
		}
		else if(cond == "fe")
		{
			condition = LexicalReordering::FE; 
		 } 
		else if(cond == "fec")
		{
			condition = LexicalReordering::FEC;
		} 
		else 
		{
			std::cerr << "Unknown conditioning declaration '" << cond << "'for lexical reordering model " << i << "!\n";
			return false;
		}

		//decode num weights (and fetch weight from array...)
		std::vector<float> mweights;
		numWeights = atoi(spec[2].c_str());
		for(size_t k = 0; k < numWeights; ++k, ++w)
		{
			if(w >= weights.size()){
				//error not enough weights...
				std::cerr << "Lexicalized distortion model: Not enough weights, add to [weight-d]\n";
				return false;
			} else {
				mweights.push_back(weights[w]);
			}
		}
    
		//decode filename
		string filePath = spec[3];

		//all ready load it
		//std::cerr << type;
		if("monotonicity" == type){
			m_reorderModels.push_back(new LexicalMonotonicReordering(filePath, mweights, direction, condition, input, output));
		} 
		else if("orientation" == type || "msd" == type)
		{
			m_reorderModels.push_back(new LexicalOrientationReordering(filePath, mweights, direction, condition, input, output));
		} 
		else if("directional" == type)
		{
			m_reorderModels.push_back(new LexicalDirectionalReordering(filePath, mweights, direction, condition, input, output));
		} 
		else 
		{
			//error unknown type!
			std::cerr << " ...unknown type!\n";
			return false;
		}
		//std::cerr << "\n";

	} 
  return true;
}

bool StaticData::LoadLanguageModels()
{
	if (m_parameter->GetParam("lmodel-file").size() > 0)
	{
		// weights
		vector<float> weightAll = Scan<float>(m_parameter->GetParam("weight-l"));
		
		for (size_t i = 0 ; i < weightAll.size() ; i++)
		{
			m_allWeights.push_back(weightAll[i]);
		}

	  // initialize n-gram order for each factor. populated only by factored lm
		const vector<string> &lmVector = m_parameter->GetParam("lmodel-file");

		for(size_t i=0; i<lmVector.size(); i++) 
		{
			vector<string>	token		= Tokenize(lmVector[i]);
			if (token.size() != 4 && token.size() != 5 )
			{
				UserMessage::Add("Expected format 'LM-TYPE FACTOR-TYPE NGRAM-ORDER filePath [mapFilePath (only for IRSTLM)]'");
				return false;
			}
			// type = implementation, SRI, IRST etc
			LMImplementation lmImplementation = static_cast<LMImplementation>(Scan<int>(token[0]));
			
			// factorType = 0 = Surface, 1 = POS, 2 = Stem, 3 = Morphology, etc
			vector<FactorType> 	factorTypes		= Tokenize<FactorType>(token[1], ",");
			
			// nGramOrder = 2 = bigram, 3 = trigram, etc
			size_t nGramOrder = Scan<int>(token[2]);
			
			string &languageModelFile = token[3];
			if (token.size() == 5)
			  if (lmImplementation==IRST)
			    languageModelFile += " " + token[4];
			  else {
			    UserMessage::Add("Expected format 'LM-TYPE FACTOR-TYPE NGRAM-ORDER filePath [mapFilePath (only for IRSTLM)]'");
			    return false;
			  }
			IFVERBOSE(1)
				PrintUserTime(string("Start loading LanguageModel ") + languageModelFile);
			
			LanguageModel *lm = LanguageModelFactory::CreateLanguageModel(
																									lmImplementation
																									, factorTypes     
                                   								, nGramOrder
																									, languageModelFile
																									, weightAll[i]
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
	IFVERBOSE(1)
		PrintUserTime("Finished loading LanguageModels");
  return true;
}

bool StaticData::LoadGenerationTables()
{
	if (m_parameter->GetParam("generation-file").size() > 0) 
	{
		const vector<string> &generationVector = m_parameter->GetParam("generation-file");
		const vector<float> &weight = Scan<float>(m_parameter->GetParam("weight-generation"));

		IFVERBOSE(1)
		{
			TRACE_ERR( "weight-generation: ");
			for (size_t i = 0 ; i < weight.size() ; i++)
			{
					TRACE_ERR( weight[i] << "\t");
			}
			TRACE_ERR(endl);
		}
		size_t currWeightNum = 0;
		
		for(size_t currDict = 0 ; currDict < generationVector.size(); currDict++) 
		{
			vector<string>			token		= Tokenize(generationVector[currDict]);
			vector<FactorType> 	input		= Tokenize<FactorType>(token[0], ",")
													,output	= Tokenize<FactorType>(token[1], ",");
      m_maxFactorIdx[1] = CalcMax(m_maxFactorIdx[1], input, output);
			string							filePath;
			size_t							numFeatures;

			numFeatures = Scan<size_t>(token[2]);
			filePath = token[3];

			if (!FileExists(filePath) && FileExists(filePath + ".gz")) {
				filePath += ".gz";
			}

			VERBOSE(1, filePath << endl);

			m_generationDictionary.push_back(new GenerationDictionary(numFeatures, m_scoreIndexManager));
			assert(m_generationDictionary.back() && "could not create GenerationDictionary");
			if (!m_generationDictionary.back()->Load(input
																		, output
																		, filePath
																		, Output))
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
		
		const vector<string> &translationVector = m_parameter->GetParam("ttable-file");
		vector<size_t>	maxTargetPhrase					= Scan<size_t>(m_parameter->GetParam("ttable-limit"));
		
		size_t index = 0;
		size_t weightAllOffset = 0;
		for(size_t currDict = 0 ; currDict < translationVector.size(); currDict++) 
		{
			vector<string>                  token           = Tokenize(translationVector[currDict]);
			//characteristics of the phrase table
			vector<FactorType>      input           = Tokenize<FactorType>(token[0], ",")
				,output = Tokenize<FactorType>(token[1], ",");
			m_maxFactorIdx[0] = CalcMax(m_maxFactorIdx[0], input);
			m_maxFactorIdx[1] = CalcMax(m_maxFactorIdx[1], output);
      m_maxNumFactors = std::max(m_maxFactorIdx[0], m_maxFactorIdx[1]) + 1;
			string filePath= token[3];
			size_t numScoreComponent = Scan<size_t>(token[2]);

			assert(weightAll.size() >= weightAllOffset + numScoreComponent);

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
			
			IFVERBOSE(1)
				PrintUserTime(string("Start loading PhraseTable ") + filePath);
			if (!FileExists(filePath+".binphr.idx"))
			{	// memory phrase table
				VERBOSE(2,"using standard phrase tables");
				if (m_inputType != SentenceInput)
				{
					UserMessage::Add("Must use binary phrase table for this input type");
					return false;
				}
				
				PhraseDictionaryMemory *pd=new PhraseDictionaryMemory(numScoreComponent);
				if (!pd->Load(input
								 , output
								 , filePath
								 , weight
								 , maxTargetPhrase[index]
								 , GetAllLM()
								 , GetWeightWordPenalty()))
				{
					delete pd;
					return false;
				}
				m_phraseDictionary.push_back(pd);
			}
			else 
			{ // binary phrase table
				VERBOSE(1, "using binary phrase tables for idx "<<currDict<<"\n");
				PhraseDictionaryTreeAdaptor *pd=new PhraseDictionaryTreeAdaptor(numScoreComponent,(currDict==0 ? m_numInputScores : 0));
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
	
	IFVERBOSE(1)
		PrintUserTime("Finished loading phrase tables");
	return true;
}

bool StaticData::LoadMapping()
{
	// mapping
	const vector<string> &mappingVector = m_parameter->GetParam("mapping");
	DecodeStep *prev = 0;
	size_t previousVectorList = 0;
	for(size_t i=0; i<mappingVector.size(); i++) 
	{
		vector<string>	token		= Tokenize(mappingVector[i]);
		size_t vectorList;
		DecodeType decodeType;
		size_t index;
		if (token.size() == 2) 
		{
		  vectorList = 0;
			decodeType = token[0] == "T" ? Translate : Generate;
			index = Scan<size_t>(token[1]);
		}
		//Smoothing
		else if (token.size() == 3) 
		{
		  vectorList = Scan<size_t>(token[0]);
			//the vectorList index can only increment by one 
			assert(vectorList == previousVectorList || vectorList == previousVectorList + 1);
      if (vectorList > previousVectorList) 
      {
        prev = NULL;
      }
			decodeType = token[1] == "T" ? Translate : Generate;
			index = Scan<size_t>(token[2]);
		}		 
		else 
		{
			UserMessage::Add("Malformed mapping!");
			return false;
		}
		
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
		list <const DecodeStep *> * decodeList=NULL;
		if (m_decodeStepVL.size() < vectorList + 1) 
		{
		  decodeList = new list <const DecodeStep *>;
			m_decodeStepVL.push_back(decodeList);
		}
		m_decodeStepVL[vectorList]->push_back(decodeStep);
		prev = decodeStep;
		previousVectorList = vectorList;
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
  for(size_t i=0;i<m_phraseDictionary.size();++i) {
	m_phraseDictionary[i]->InitializeForInput(in);
  }
  for(size_t j=0;j<m_reorderModels.size();++j){
	m_reorderModels[j]->InitializeForInput(in);
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
