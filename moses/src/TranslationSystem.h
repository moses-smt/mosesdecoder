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
      void AddLanguageModel(LanguageModel* languageModel);
      void AddDecodeGraph(DecodeGraph* decodeGraph, size_t backoff);
      void AddReorderModel(LexicalReordering* reorderModel);
      void AddGlobalLexicalModel(GlobalLexicalModel* globalLexicalModel);
      
      //Insert non-core feature function
      void AddFeatureFunction(const FeatureFunction* featureFunction);

      //Insert sparse producer
      void AddSparseProducer(const FeatureFunction* sparseProducer);
      
      //Called after adding the tables in order to set up the dictionaries
      void ConfigDictionaries();
      
        
      const std::string& GetId() const {return m_id;}
      
      //Lists of tables relevant to this system.
      const std::vector<LexicalReordering*>& GetReorderModels() const {return m_reorderingTables;}
      const std::vector<DecodeGraph*>& GetDecodeGraphs() const {return m_decodeGraphs;}
      const std::vector<size_t>& GetDecodeGraphBackoff() const {return m_decodeGraphBackoff;}
      const LMList& GetLanguageModels() const {return m_languageModels;}
      const std::vector<GenerationDictionary*>& GetGenerationDictionaries() const {return m_generationDictionaries;}
      const std::vector<PhraseDictionaryFeature*>& GetPhraseDictionaries() const {return m_phraseDictionaries;}
      
      const std::vector<const StatefulFeatureFunction*>& GetStatefulFeatureFunctions() const {return m_statefulFFs;}
      const std::vector<const StatelessFeatureFunction*>& GetStatelessFeatureFunctions() const {return m_statelessFFs;}
      const std::vector<const FeatureFunction*>& GetSparseProducers() const {return m_sparseProducers;}
      
      const WordPenaltyProducer *GetWordPenaltyProducer() const { return m_wpProducer; }
      const UnknownWordPenaltyProducer *GetUnknownWordPenaltyProducer() const { return m_unknownWpProducer; }
      const DistortionScoreProducer* GetDistortionProducer() const {return m_distortionScoreProducer;}
      
      const PhraseDictionaryFeature *GetTranslationScoreProducer(size_t index) const { return GetPhraseDictionaries().at(index); }
      
      float GetWeightWordPenalty() const;
      float GetWeightUnknownWordPenalty() const;
      float GetWeightDistortion() const;
      std::vector<float> GetTranslationWeights(size_t index) const;
      
      //sentence (and thread) specific initialisationn and cleanup
      void InitializeBeforeSentenceProcessing(const InputType& source) const;
      void CleanUpAfterSentenceProcessing(const InputType& source) const;
      
      const std::vector<const ScoreProducer*>& GetFeatureFunctions() const { return m_producers; }
        
      static const  std::string DEFAULT;
        
        
    private:
        std::string m_id;
        
        std::vector<DecodeGraph*> m_decodeGraphs;
      	std::vector<size_t> m_decodeGraphBackoff;
        std::vector<LexicalReordering*> m_reorderingTables;
        std::vector<PhraseDictionaryFeature*> m_phraseDictionaries;
        std::vector<GenerationDictionary*> m_generationDictionaries;
        LMList m_languageModels;
        std::vector<GlobalLexicalModel*> m_globalLexicalModels;
        
        //All stateless FFs, except those that cache scores in T-Option
        std::vector<const StatelessFeatureFunction*> m_statelessFFs;
        //All statefull FFs
        std::vector<const StatefulFeatureFunction*> m_statefulFFs;
	//All sparse producers that have an activated global weight
	std::vector<const FeatureFunction*> m_sparseProducers;
        
        const WordPenaltyProducer* m_wpProducer;
        const UnknownWordPenaltyProducer* m_unknownWpProducer;
        const DistortionScoreProducer* m_distortionScoreProducer;
	
	std::vector<const ScoreProducer*> m_producers; /**< all the score producers in this run */

};




}
#endif

