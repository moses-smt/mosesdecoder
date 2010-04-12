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

#include "PhraseDictionary.h"
#include "PhraseDictionaryTreeAdaptor.h"
#include "PhraseDictionaryNewFormat.h"
#include "PhraseDictionaryOnDisk.h"
#ifndef WIN32
#include "PhraseDictionaryDynSuffixArray.h"
#endif
#include "StaticData.h"
#include "InputType.h"
#include "TranslationOption.h"
#include "UserMessage.h"

namespace Moses {

const TargetPhraseCollection *PhraseDictionary::
GetTargetPhraseCollection(InputType const& src,WordsRange const& range) const 
{
    return GetTargetPhraseCollection(src.GetSubString(range));
}

PhraseDictionaryFeature::PhraseDictionaryFeature 
                            (PhraseTableImplementation implementation
														, size_t numScoreComponent
                            , unsigned numInputScores
                            , const std::vector<FactorType> &input
                            , const std::vector<FactorType> &output
                            , const std::string &filePath
                            , const std::vector<float> &weight
                            , size_t tableLimit
														, const std::string &targetFile  // default param
                            , const std::string &alignmentsFile) // default param
	:m_numScoreComponent(numScoreComponent),
	m_numInputScores(numInputScores),
	m_input(input),
	m_output(output),
	m_filePath(filePath),
	m_weight(weight),
	m_tableLimit(tableLimit)
{
	const StaticData& staticData = StaticData::Instance();
	const_cast<ScoreIndexManager&>(staticData.GetScoreIndexManager()).AddScoreProducer(this);

	m_implementation = implementation;
	
	//if we're using an in-memory phrase table, then load it now, otherwise wait
	if (implementation == Memory)
	{   // memory phrase table
			VERBOSE(2,"using standard phrase tables" << std::endl);
			if (!FileExists(m_filePath) && FileExists(m_filePath + ".gz")) {
					m_filePath += ".gz";
					VERBOSE(2,"Using gzipped file" << std::endl);
			}
			if (staticData.GetInputType() != SentenceInput)
			{
					UserMessage::Add("Must use binary phrase table for this input type");
					assert(false);
			}
			
			PhraseDictionaryMemory* pdm  = new PhraseDictionaryMemory(m_numScoreComponent,this);
			assert(pdm->Load(m_input
													, m_output
													, m_filePath
													, m_weight
													, m_tableLimit
													, staticData.GetAllLM()
													, staticData.GetWeightWordPenalty()));
			m_phraseDictionary.reset(pdm);
	}
	else if (implementation == Binary)
	{   
		//load the tree dictionary for this thread   
		const StaticData& staticData = StaticData::Instance();
		PhraseDictionaryTreeAdaptor* pdta = new PhraseDictionaryTreeAdaptor(m_numScoreComponent, m_numInputScores,this);
		assert(pdta->Load(
											m_input
											, m_output
											, m_filePath
											, m_weight
											, m_tableLimit
											, staticData.GetAllLM()
											, staticData.GetWeightWordPenalty()));
		m_phraseDictionary.reset(pdta);
	}
	else if (implementation == NewFormat)
	{   // memory phrase table
		VERBOSE(2,"using New Format phrase tables" << std::endl);
		if (!FileExists(m_filePath) && FileExists(m_filePath + ".gz")) {
			m_filePath += ".gz";
			VERBOSE(2,"Using gzipped file" << std::endl);
		}
		
		PhraseDictionaryNewFormat* pdm  = new PhraseDictionaryNewFormat(m_numScoreComponent,this);
		assert(pdm->Load(m_input
										 , m_output
										 , m_filePath
										 , m_weight
										 , m_tableLimit
										 , staticData.GetAllLM()
										 , staticData.GetWeightWordPenalty()));
		m_phraseDictionary.reset(pdm);
	}
	else if (implementation == OnDisk)
	{   
		//load the tree dictionary for this thread   
		//const StaticData& staticData = StaticData::Instance();
		PhraseDictionaryOnDisk* pdta = new PhraseDictionaryOnDisk(m_numScoreComponent, this);
		pdta->Load(
											m_input
											, m_output
											, m_filePath
											, m_weight
											, m_tableLimit);
											//, staticData.GetAllLM()
											//, staticData.GetWeightWordPenalty()));
		assert(pdta);
		m_phraseDictionary.reset(pdta);
	}
	else if (implementation == Binary)
	{   
		//load the tree dictionary for this thread   
		const StaticData& staticData = StaticData::Instance();
		PhraseDictionaryTreeAdaptor* pdta = new PhraseDictionaryTreeAdaptor(m_numScoreComponent, m_numInputScores,this);
		assert(pdta->Load(
											m_input
											, m_output
											, m_filePath
											, m_weight
											, m_tableLimit
											, staticData.GetAllLM()
											, staticData.GetWeightWordPenalty()));
		m_phraseDictionary.reset(pdta);
	}
	else if (implementation == SuffixArray)
	{   
		#ifndef WIN32
			PhraseDictionaryDynSuffixArray *pd = new PhraseDictionaryDynSuffixArray(numScoreComponent, this); 	 
			if(!(pd && pd->Load(filePath, targetFile, alignmentsFile 	 
													, weight, tableLimit 	 
													, staticData.GetAllLM() 	 
													, staticData.GetWeightWordPenalty()))) 	 
			{ 	 
				std::cerr << "FAILED TO LOAD\n" << endl; 	 
				delete pd; 	 
			} 	 
			m_phraseDictionary.reset(pd); 	 
			std::cerr << "Suffix array phrase table loaded" << std::endl;
		#else
			assert(false);
		#endif
	}
}
  
const PhraseDictionary* PhraseDictionaryFeature::GetDictionary(const InputType& source) const
{
	PhraseDictionary* dict = m_phraseDictionary.get();
	assert(dict);
	
	dict->InitializeForInput(source);
	return dict;
}



PhraseDictionaryFeature::~PhraseDictionaryFeature() 
{}
	

std::string PhraseDictionaryFeature::GetScoreProducerDescription() const
{
	return "PhraseModel";
}

size_t PhraseDictionaryFeature::GetNumScoreComponents() const
{
	return m_numScoreComponent;
}

size_t PhraseDictionaryFeature::GetNumInputScores() const 
{ 
      return m_numInputScores;
}

bool PhraseDictionaryFeature::ComputeValueInTranslationOption() const {
	return true;
}

 const PhraseDictionaryFeature* PhraseDictionary::GetFeature() const {
    return m_feature;
 }

}

