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
#include "StaticData.h"
#include "TranslationSystem.h"
#include "Util.h"

using namespace std;

namespace Moses {
  
  const string TranslationSystem::DEFAULT = "default";

    TranslationSystem::TranslationSystem(const string& config,
                                        const vector<DecodeGraph*>& allDecodeGraphs,
                                        const vector<LexicalReordering*>& allReorderingTables,
                                        const LMList& allLMs,
                                         const vector<WordPenaltyProducer*>& allWordPenalties) {
        VERBOSE(2,"Creating translation system " << config << endl);
        vector<string> fields;
        Tokenize(fields,config);
        if (fields.size() % 2 != 1) {
            throw runtime_error("Incorrect number of fields in translation system config");
        }
        m_id = fields[0];
        for (size_t i = 1; i < fields.size(); i += 2) {
            const string& key = fields[i];
            vector<size_t> indexes;
            Tokenize<size_t>(fields[i+1],",");
            if (key == "L") {
              //LMs
               
            } else if (key == "D") {
              //decoding graphs
                
            } else if (key == "R") {
              //reordering tables
                
            } else {
                throw runtime_error("Unknown table id in translation systems config");
            }
        }
    }
    
    TranslationSystem::TranslationSystem(const vector<DecodeGraph*>& allDecodeGraphs,
                                         const vector<LexicalReordering*>& allReorderingTables,
                                         const LMList& allLMs,
                                         const vector<WordPenaltyProducer*>& allWordPenalties) :
        m_id(DEFAULT),
    m_decodeGraphs(allDecodeGraphs),
    m_reorderingTables(allReorderingTables),
    m_languageModels(allLMs),
    m_wpProducer(allWordPenalties.at(0))
    {
      configureDictionaries();
    }
    
    void TranslationSystem::configureDictionaries() {
      for (vector<DecodeGraph*>::iterator i = m_decodeGraphs.begin(); i != m_decodeGraphs.end() ; ++i) {
        for (DecodeGraph::const_iterator j = (*i)->begin(); j != (*i)->end(); ++j) {
          const DecodeStep* step = *j;
          const PhraseDictionaryFeature* pdict = step->GetPhraseDictionaryFeature();
          if (pdict) {
            m_phraseDictionaries.push_back(pdict);
            const_cast<PhraseDictionaryFeature*>(pdict)->InitDictionary(this);
          }
          const GenerationDictionary* gdict = step->GetGenerationDictionaryFeature();
          if (gdict) {
            m_generationDictionaries.push_back(gdict);
          }
        }
      }
    }
    
    void TranslationSystem::InitializeBeforeSentenceProcessing(const InputType& source) const {
      for (vector<const PhraseDictionaryFeature*>::const_iterator i = m_phraseDictionaries.begin();
           i != m_phraseDictionaries.end(); ++i) {
             const_cast<PhraseDictionaryFeature*>(*i)->InitDictionary(this,source);
           }
    }
    
    float TranslationSystem::GetWeightWordPenalty() const {
      //const ScoreComponentCollection weights = StaticData::Instance().GetAllWeights();
      size_t wpIndex = StaticData::Instance().GetScoreIndexManager().GetBeginIndex(m_wpProducer->GetScoreBookkeepingID());
      return StaticData::Instance().GetAllWeights()[wpIndex];
    }

};
