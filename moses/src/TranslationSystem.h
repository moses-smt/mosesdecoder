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

#ifndef moses_TranslationSystem_h
#define moses_TranslationSystem_h

#include <stdexcept>
#include <string>
#include <vector>

#include "LMList.h"

namespace Moses {

  class DecodeGraph;
  class LexicalReordering;
  class PhraseDictionaryFeature;
  class GenerationDictionary;
  class WordPenaltyProducer;

/**
 * Enables the configuration of multiple translation systems.
**/
class TranslationSystem {

    public:
      /** Creates a system with the given configuration */
      TranslationSystem(const std::string& config,
                        const std::vector<DecodeGraph*>& allDecoderGraphs,
                        const std::vector<LexicalReordering*>& allReorderingTables,
                        const LMList& allLMs,
                        const std::vector<WordPenaltyProducer*>& allWordPenalties);
      
      /** Creates a default system */
      TranslationSystem(const std::vector<DecodeGraph*>& allDecoderGraphs,
                        const std::vector<LexicalReordering*>& allReorderingTables,
                        const LMList& allLMs,
                        const std::vector<WordPenaltyProducer*>& allWordPenalties);
        
        const std::string& GetId() const {return m_id;}
        
        //Lists of tables relevant to this system.
        const std::vector<LexicalReordering*>& GetReorderModels() const {return m_reorderingTables;}
        const std::vector<DecodeGraph*>& GetDecodeGraphs() const {return m_decodeGraphs;}
        const LMList& GetLanguageModels() const {return m_languageModels;}
        const std::vector<const GenerationDictionary*>& GetGenerationDictionaries() const {return m_generationDictionaries;}
        const std::vector<const PhraseDictionaryFeature*>& GetPhraseDictionaries() const {return m_phraseDictionaries;}
        const WordPenaltyProducer *GetWordPenaltyProducer() const { return m_wpProducer; }
        
        float GetWeightWordPenalty() const;
        
        //sentence (and thread) specific initialisation
        void InitializeBeforeSentenceProcessing(const InputType& source) const;
        
        
        
        static const  std::string DEFAULT;

        
        
        
    private:
        //checks what dictionaries are required, and initialises them if necessary 
        void configureDictionaries();
      
        std::string m_id;
        std::vector<DecodeGraph*> m_decodeGraphs;
        std::vector<LexicalReordering*> m_reorderingTables;
        std::vector<const PhraseDictionaryFeature*> m_phraseDictionaries;
        std::vector<const GenerationDictionary*> m_generationDictionaries;
        LMList m_languageModels;
        WordPenaltyProducer* m_wpProducer;
};




}
#endif

