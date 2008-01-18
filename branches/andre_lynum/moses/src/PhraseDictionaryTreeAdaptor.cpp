// $Id$

#include "PhraseDictionaryTreeAdaptor.h"
#include <sys/stat.h>
#include <algorithm>
#include "PhraseDictionaryTree.h"
#include "Phrase.h"
#include "FactorCollection.h"
#include "InputFileStream.h"
#include "InputType.h"
#include "ConfusionNet.h"
#include "Sentence.h"
#include "StaticData.h"
#include "UniqueObject.h"
#include "PDTAimp.h"
#include "UserMessage.h"

/*************************************************************
	function definitions of the interface class
	virtually everything is forwarded to the implementation class
*************************************************************/

PhraseDictionaryTreeAdaptor::
PhraseDictionaryTreeAdaptor(size_t numScoreComponent,unsigned numInputScores)
	: MyBase(numScoreComponent),imp(new PDTAimp(this,numInputScores)) {}

PhraseDictionaryTreeAdaptor::~PhraseDictionaryTreeAdaptor() 
{
	imp->CleanUp();
	delete imp;
}

void PhraseDictionaryTreeAdaptor::CleanUp() 
{
	imp->CleanUp();
	MyBase::CleanUp();
}

void PhraseDictionaryTreeAdaptor::InitializeForInput(InputType const& source)
{
	// caching only required for confusion net
	if(ConfusionNet const* cn=dynamic_cast<ConfusionNet const*>(&source))
		imp->CacheSource(*cn);
	//else if(Sentence const* s=dynamic_cast<Sentence const*>(&source))
	// following removed by phi, not helpful
	//	imp->CacheSource(ConfusionNet(*s));
}

bool PhraseDictionaryTreeAdaptor::Load(const std::vector<FactorType> &input
																				 , const std::vector<FactorType> &output
																				 , const std::string &filePath
																				 , const std::vector<float> &weight
																				 , size_t tableLimit
																				 , const LMList &languageModels
																				 , float weightWP
																				 )
{
	FactorCollection &factorCollection = FactorCollection::Instance();

	if(m_numScoreComponent!=weight.size()) {
		stringstream strme;
		strme << "ERROR: mismatch of number of scaling factors: "<<weight.size()
						 <<" "<<m_numScoreComponent<<"\n";
		UserMessage::Add(strme.str());
		return false;
	}
	m_filePath = filePath;

	// set Dictionary members
	m_inputFactors = FactorMask(input);
	m_outputFactors = FactorMask(output);
	VERBOSE(2,"PhraseDictionaryTreeAdaptor: input=" << m_inputFactors << "  output=" << m_outputFactors << std::endl);

	// set PhraseDictionary members
	m_tableLimit=tableLimit;

	imp->Create(input,output,filePath,
							weight,languageModels,weightWP);
	return true;
}

TargetPhraseCollection const* 
PhraseDictionaryTreeAdaptor::GetTargetPhraseCollection(Phrase const &src) const
{
	return imp->GetTargetPhraseCollection(src);
}
TargetPhraseCollection const* 
PhraseDictionaryTreeAdaptor::GetTargetPhraseCollection(InputType const& src,WordsRange const &range) const
{
	if(imp->m_rangeCache.empty())
	{
		return imp->GetTargetPhraseCollection(src.GetSubString(range));
	}
	else
	{
		return imp->m_rangeCache[range.GetStartPos()][range.GetEndPos()];
	}
}

void PhraseDictionaryTreeAdaptor::
SetWeightTransModel(const std::vector<float> &weightT)
{
	CleanUp();
	imp->m_weights=weightT;
}

void PhraseDictionaryTreeAdaptor::
AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase) 
{
	imp->AddEquivPhrase(source,targetPhrase);
}
void PhraseDictionaryTreeAdaptor::EnableCache()
{
	imp->useCache=1;
}
void PhraseDictionaryTreeAdaptor::DisableCache()
{
	imp->useCache=0;
}



size_t PhraseDictionaryTreeAdaptor::GetNumInputScores() const {
	return imp->GetNumInputScores();
}

std::string PhraseDictionaryTreeAdaptor::GetScoreProducerDescription() const
{
	return "Translation score, file=" + m_filePath;
}

