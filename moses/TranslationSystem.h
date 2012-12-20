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

#include "FeatureFunction.h"
#include "LMList.h"

namespace Moses
{

class DecodeGraph;
class LexicalReordering;
class PhraseDictionaryFeature;
class GenerationDictionary;
class WordPenaltyProducer;
class DistortionScoreProducer;
class UnknownWordPenaltyProducer;
class MetaFeatureProducer;
class GlobalLexicalModel;

/**
 * Enables the configuration of multiple translation systems.
**/
class TranslationSystem {

    public:
      /** Creates a system with the given id */
      TranslationSystem(const std::string& id, 
                        const WordPenaltyProducer* wpProducer,
                        const UnknownWordPenaltyProducer* uwpProducer,
                        const DistortionScoreProducer* distortionProducer);
      
      //Insert core 'big' features
      void AddDecodeGraph(DecodeGraph* decodeGraph, size_t backoff);

      //Called after adding the tables in order to set up the dictionaries
      void ConfigDictionaries();
      
        
      const std::string& GetId() const {return m_id;}
      
      //Lists of tables relevant to this system.
      const std::vector<DecodeGraph*>& GetDecodeGraphs() const {return m_decodeGraphs;}
      const std::vector<size_t>& GetDecodeGraphBackoff() const {return m_decodeGraphBackoff;}
      const std::vector<GenerationDictionary*>& GetGenerationDictionaries() const {return m_generationDictionaries;}
      const std::vector<PhraseDictionaryFeature*>& GetPhraseDictionaries() const {return m_phraseDictionaries;}
      
      const PhraseDictionaryFeature *GetTranslationScoreProducer(size_t index) const { return GetPhraseDictionaries().at(index); }
      
      std::vector<float> GetTranslationWeights(size_t index) const;
      
      //sentence (and thread) specific initialisationn and cleanup
      void InitializeBeforeSentenceProcessing(const InputType& source) const;
      void CleanUpAfterSentenceProcessing(const InputType& source) const;

      static const  std::string DEFAULT;
        
        
    private:
        std::string m_id;
        
        std::vector<DecodeGraph*> m_decodeGraphs;
      	std::vector<size_t> m_decodeGraphBackoff;
        std::vector<PhraseDictionaryFeature*> m_phraseDictionaries;
        std::vector<GenerationDictionary*> m_generationDictionaries;

	
};




}
#endif

