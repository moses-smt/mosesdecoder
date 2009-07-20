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
#include "StaticData.h"
#include "InputType.h"
#include "TranslationOption.h"

namespace Moses {

const TargetPhraseCollection *PhraseDictionary::
GetTargetPhraseCollection(InputType const& src,WordsRange const& range) const 
{
    return GetTargetPhraseCollection(src.GetSubString(range));
}

PhraseDictionaryFeature::PhraseDictionaryFeature 
                            ( size_t numScoreComponent
                            , unsigned numInputScores
                            , const std::vector<FactorType> &input
                            , const std::vector<FactorType> &output
                            , const std::string &filePath
                            , const std::vector<float> &weight
                            , size_t tableLimit):
                            m_numScoreComponent(numScoreComponent),
                            m_numInputScores(numInputScores),
                            m_input(input),
                            m_output(output),
                            m_filePath(filePath),
                            m_weight(weight),
                            m_tableLimit(tableLimit)
  {
    const StaticData& staticData = StaticData::Instance();
    const_cast<ScoreIndexManager&>(staticData.GetScoreIndexManager()).AddScoreProducer(this);
    
    
    //if we're using an in-memory phrase table, then load it now, otherwise wait
    if (!FileExists(filePath+".binphr.idx"))
    {   // memory phrase table
        VERBOSE(2,"using standard phrase tables" << endl);
        if (!FileExists(m_filePath) && FileExists(m_filePath + ".gz")) {
            m_filePath += ".gz";
            VERBOSE(2,"Using gzipped file" << endl);
        }
        if (staticData.GetInputType() != SentenceInput)
        {
            UserMessage::Add("Must use binary phrase table for this input type");
            assert(false);
        }
        
        boost::shared_ptr<PhraseDictionaryMemory> pdm(new PhraseDictionaryMemory(m_numScoreComponent,this));
        assert(pdm->Load(m_input
                            , m_output
                            , m_filePath
                            , m_weight
                            , m_tableLimit
                            , staticData.GetAllLM()
                            , staticData.GetWeightWordPenalty()));
        m_dictionary = pdm;
    }
    else 
    {   
        //don't initialise the dictionary until it's required
    }
  
  
  }
  
  PhraseDictionaryHandle PhraseDictionaryFeature::GetDictionary(const InputType& source) const {
    if (m_dictionary) {
        return m_dictionary;
    } else {
        const StaticData& staticData = StaticData::Instance();
        boost::shared_ptr<PhraseDictionaryTreeAdaptor> 
            pdta(new PhraseDictionaryTreeAdaptor(m_numScoreComponent, m_numInputScores,this));
        assert(pdta->Load(
                              m_input
                            , m_output
                            , m_filePath
                            , m_weight
                            , m_tableLimit
                            , staticData.GetAllLM()
                            , staticData.GetWeightWordPenalty()
                            , source));
        return pdta;
    }
  }



PhraseDictionaryFeature::~PhraseDictionaryFeature() {}
	


std::string PhraseDictionaryFeature::GetScoreProducerDescription() const
{
	return "PhraseModel";
}

size_t PhraseDictionaryFeature::GetNumScoreComponents() const
{
	return m_numScoreComponent;
}

size_t PhraseDictionaryFeature::GetNumInputScores() const { return 0;}

bool PhraseDictionaryFeature::ComputeValueInTranslationOption() const {
	return true;
}

 const PhraseDictionaryFeature* PhraseDictionary::GetFeature() const {
    return m_feature;
 }

}

