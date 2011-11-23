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
#include "LexicalReordering.h"
#include "StaticData.h"
#include "TranslationSystem.h"
#include "Util.h"

using namespace std;

namespace Moses
{

const string TranslationSystem::DEFAULT = "default";

TranslationSystem::TranslationSystem(const std::string& id,
                                     const WordPenaltyProducer* wpProducer,
                                     const UnknownWordPenaltyProducer* uwpProducer,
                                     const DistortionScoreProducer* distortionProducer)
  : m_id(id), m_wpProducer(wpProducer), m_unknownWpProducer(uwpProducer), m_distortionScoreProducer(distortionProducer)
{
  AddFeatureFunction(wpProducer);
  AddFeatureFunction(uwpProducer);
  if (distortionProducer) {
    AddFeatureFunction(distortionProducer);
  }
}

//Insert core 'big' features
void TranslationSystem::AddLanguageModel(LanguageModel* languageModel)
{
  m_languageModels.Add(languageModel);
  AddFeatureFunction(languageModel);
}


void TranslationSystem::AddDecodeGraph(DecodeGraph* decodeGraph, size_t backoff)
{
  m_decodeGraphs.push_back(decodeGraph);
  m_decodeGraphBackoff.push_back(backoff);
}


void TranslationSystem::AddReorderModel(LexicalReordering* reorderModel)
{
  m_reorderingTables.push_back(reorderModel);
  AddFeatureFunction(reorderModel);
}


void TranslationSystem::AddGlobalLexicalModel(GlobalLexicalModel* globalLexicalModel)
{
  m_globalLexicalModels.push_back(globalLexicalModel);
  AddFeatureFunction(globalLexicalModel);
}




void TranslationSystem::AddFeatureFunction(const FeatureFunction* ff)
{
  if (ff->IsStateless()) {
    const StatelessFeatureFunction* statelessFF = static_cast<const StatelessFeatureFunction*>(ff);
    if (!statelessFF->ComputeValueInTranslationOption()) {
      m_statelessFFs.push_back(statelessFF);
    }
  } else {
    m_statefulFFs.push_back(static_cast<const StatefulFeatureFunction*>(ff));
  }
}

void TranslationSystem::ConfigDictionaries()
{
  for (vector<DecodeGraph*>::const_iterator i = m_decodeGraphs.begin(); i != m_decodeGraphs.end(); ++i) {
    for (DecodeGraph::const_iterator j = (*i)->begin(); j != (*i)->end(); ++j) {
      const DecodeStep* step = *j;
      PhraseDictionaryFeature* pdict = const_cast<PhraseDictionaryFeature*>(step->GetPhraseDictionaryFeature());
      if (pdict) {
        m_phraseDictionaries.push_back(pdict);
        AddFeatureFunction(pdict);
        const_cast<PhraseDictionaryFeature*>(pdict)->InitDictionary(this);
      }
      GenerationDictionary* gdict = const_cast<GenerationDictionary*>(step->GetGenerationDictionaryFeature());
      if (gdict) {
        m_generationDictionaries.push_back(gdict);
        AddFeatureFunction(gdict);
      }
    }
  }
}

void TranslationSystem::InitializeBeforeSentenceProcessing(const InputType& source) const
{
  for (vector<PhraseDictionaryFeature*>::const_iterator i = m_phraseDictionaries.begin();
       i != m_phraseDictionaries.end(); ++i) {
    (*i)->InitDictionary(this,source);
  }

  for(size_t i=0; i<m_reorderingTables.size(); ++i) {
    m_reorderingTables[i]->InitializeForInput(source);
  }
  for(size_t i=0; i<m_globalLexicalModels.size(); ++i) {
    m_globalLexicalModels[i]->InitializeForInput((Sentence const&)source);
  }

  LMList::const_iterator iterLM;
  for (iterLM = m_languageModels.begin() ; iterLM != m_languageModels.end() ; ++iterLM) {
    LanguageModel &languageModel = **iterLM;
    languageModel.InitializeBeforeSentenceProcessing();
  }
}

void TranslationSystem::CleanUpAfterSentenceProcessing() const
{

  for(size_t i=0; i<m_phraseDictionaries.size(); ++i) {
    PhraseDictionaryFeature &phraseDictionaryFeature = *m_phraseDictionaries[i];
    PhraseDictionary* phraseDictionary = const_cast<PhraseDictionary*>(phraseDictionaryFeature.GetDictionary());
    phraseDictionary->CleanUp();

  }

  for(size_t i=0; i<m_generationDictionaries.size(); ++i)
    m_generationDictionaries[i]->CleanUp();

  //something LMs could do after each sentence
  LMList::const_iterator iterLM;
  for (iterLM = m_languageModels.begin() ; iterLM != m_languageModels.end() ; ++iterLM) {
    LanguageModel &languageModel = **iterLM;
    languageModel.CleanUpAfterSentenceProcessing();
  }
}

float TranslationSystem::GetWeightWordPenalty() const
{
  //const ScoreComponentCollection weights = StaticData::Instance().GetAllWeights();
  size_t wpIndex = StaticData::Instance().GetScoreIndexManager().GetBeginIndex(m_wpProducer->GetScoreBookkeepingID());
  return StaticData::Instance().GetAllWeights()[wpIndex];
}

float TranslationSystem::GetWeightUnknownWordPenalty() const
{
  size_t uwpIndex = StaticData::Instance().GetScoreIndexManager().
                    GetBeginIndex(m_unknownWpProducer->GetScoreBookkeepingID());
  return StaticData::Instance().GetAllWeights()[uwpIndex];
}

float TranslationSystem::GetWeightDistortion() const
{
  CHECK(m_distortionScoreProducer);
  size_t distIndex = StaticData::Instance().GetScoreIndexManager().
                     GetBeginIndex(m_distortionScoreProducer->GetScoreBookkeepingID());
  return StaticData::Instance().GetAllWeights()[distIndex];
}

};
