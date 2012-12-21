// $Id: $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

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

#include <stdexcept>
#include <iostream>

#include "DecodeGraph.h"
#include "DecodeStep.h"
#include "DummyScoreProducers.h"
#include "GlobalLexicalModel.h"
#include "GlobalLexicalModelUnlimited.h"
#include "WordTranslationFeature.h"
#include "PhrasePairFeature.h"
#include "LexicalReordering.h"
#include "StaticData.h"
#include "TranslationSystem.h"
#include "Util.h"

using namespace std;

namespace Moses {
  
  const string TranslationSystem::DEFAULT = "default";

    TranslationSystem::TranslationSystem(const std::string& id, 
                      const WordPenaltyProducer* wpProducer,
                      const UnknownWordPenaltyProducer* uwpProducer,
                      const DistortionScoreProducer* distortionProducer)
    : m_id(id)
    {
      StaticData::InstanceNonConst().AddFeatureFunction(wpProducer);
      StaticData::InstanceNonConst().AddFeatureFunction(uwpProducer);
      if (distortionProducer) {
        StaticData::InstanceNonConst().AddFeatureFunction(distortionProducer);
      }
    }
    
    void TranslationSystem::InitializeBeforeSentenceProcessing(const InputType& source) const {
      const StaticData &staticData = StaticData::Instance();
      const std::vector<PhraseDictionaryFeature*> &phraseDictionaries = staticData.GetPhraseDictionaries();

      for (vector<PhraseDictionaryFeature*>::const_iterator i = phraseDictionaries.begin();
           i != phraseDictionaries.end(); ++i) {
             (*i)->InitDictionary(this,source);
           }
           
      const std::vector<LexicalReordering*> &reorderingTables = StaticData::Instance().GetReorderModels();
      for(size_t i=0;i<reorderingTables.size();++i) {
        reorderingTables[i]->InitializeForInput(source);
      }

      /*
      for(size_t i=0;i<m_globalLexicalModels.size();++i) {
        m_globalLexicalModels[i]->InitializeForInput((Sentence const&)source);
      }
      */

      LMList lmList = StaticData::Instance().GetLMList();
      lmList.InitializeBeforeSentenceProcessing();
    }
    
     void TranslationSystem::CleanUpAfterSentenceProcessing(const InputType& source) const {
       const StaticData &staticData = StaticData::Instance();
       const std::vector<PhraseDictionaryFeature*> &phraseDictionaries = staticData.GetPhraseDictionaries();
       const std::vector<GenerationDictionary*> &generationDictionaries =  staticData.GetGenerationDictionaries();

        for(size_t i=0;i<phraseDictionaries.size();++i)
        {
          PhraseDictionaryFeature &phraseDictionaryFeature = *phraseDictionaries[i];
          PhraseDictionary* phraseDictionary = const_cast<PhraseDictionary*>(phraseDictionaryFeature.GetDictionary());
          phraseDictionary->CleanUp(source);

        }
  
        for(size_t i=0;i<generationDictionaries.size();++i)
            generationDictionaries[i]->CleanUp(source);
  
        LMList lmList = StaticData::Instance().GetLMList();
        lmList.CleanUpAfterSentenceProcessing(source);
     }
};
