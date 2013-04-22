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

#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include "moses/TranslationModel/PhraseDictionaryInterpolated.h"
#include "moses/TranslationModel/RuleTable/PhraseDictionarySCFG.h"
#include "moses/TranslationModel/RuleTable/PhraseDictionaryOnDisk.h"
#include "moses/TranslationModel/RuleTable/PhraseDictionaryALSuffixArray.h"
#include "moses/TranslationModel/RuleTable/PhraseDictionaryFuzzyMatch.h"
#include "moses/TranslationModel/PhraseDictionaryMultiModel.h"
#include "moses/TranslationModel/PhraseDictionaryMultiModelCounts.h"

#ifndef WIN32
#include "moses/TranslationModel/PhraseDictionaryDynSuffixArray.h"
#include "moses/TranslationModel/CompactPT/PhraseDictionaryCompact.h"
#endif
#include "moses/TranslationModel/RuleTable/UTrie.h"

#include "moses/StaticData.h"
#include "moses/InputType.h"
#include "moses/TranslationOption.h"
#include "moses/UserMessage.h"

using namespace std;

namespace Moses
{

const TargetPhraseCollection *PhraseDictionary::
GetTargetPhraseCollection(InputType const& src,WordsRange const& range) const
{
  return GetTargetPhraseCollection(src.GetSubString(range));
}

size_t PhraseDictionary::GetDictIndex() const 
{ 
  return m_feature->GetDictIndex(); 
}

PhraseDictionaryFeature::PhraseDictionaryFeature
(PhraseTableImplementation implementation
 , SparsePhraseDictionaryFeature* spdf
 , size_t numScoreComponent
 , unsigned numInputScores
 , const std::vector<FactorType> &input
 , const std::vector<FactorType> &output
 , const std::string &filePath
 , const std::vector<float> &weight
 , size_t dictIndex
 , size_t tableLimit
 , const std::vector<std::string>& config)
  :DecodeFeature("PhraseModel",numScoreComponent,input,output),
  m_dictIndex(dictIndex),
  m_numInputScores(numInputScores),
  m_filePath(filePath),
  m_tableLimit(tableLimit),
  m_implementation(implementation),
  m_config(config),
  m_sparsePhraseDictionaryFeature(spdf)
{
  if (implementation == Memory || implementation == SCFG || implementation == SuffixArray ||
      implementation==Compact || implementation==FuzzyMatch || implementation == MultiModel || implementation == MultiModelCounts) {
    m_useThreadSafePhraseDictionary = true;
  } else {
    m_useThreadSafePhraseDictionary = false;
  }
}

PhraseDictionary* PhraseDictionaryFeature::LoadPhraseTable(const TranslationSystem* system)
{
  const StaticData& staticData = StaticData::Instance();
  std::vector<float> weightT = staticData.GetWeights(this);
  
  if (m_implementation == Memory) {
    // memory phrase table
    VERBOSE(2,"using standard phrase tables" << std::endl);
    if (!FileExists(m_filePath) && FileExists(m_filePath + ".gz")) {
      m_filePath += ".gz";
      VERBOSE(2,"Using gzipped file" << std::endl);
    }
    if (staticData.GetInputType() != SentenceInput) {
      UserMessage::Add("Must use binary phrase table for this input type");
      CHECK(false);
    }

    PhraseDictionaryMemory* pdm  = new PhraseDictionaryMemory(GetNumScoreComponents(),this);
    bool ret = pdm->Load(GetInput(), GetOutput()
                         , m_filePath
                         , weightT
                         , m_tableLimit
                         , system->GetLanguageModels()
                         , system->GetWeightWordPenalty());
    CHECK(ret);
    return pdm;
  } else if (m_implementation == Binary) {
    PhraseDictionaryTreeAdaptor* pdta = new PhraseDictionaryTreeAdaptor(GetNumScoreComponents(), m_numInputScores,this);
    bool ret = pdta->Load(                    GetInput()
               , GetOutput()
               , m_filePath
               , weightT
               , m_tableLimit
               , system->GetLanguageModels()
               , system->GetWeightWordPenalty());
    CHECK(ret);
    return pdta;
  } else if (m_implementation == Interpolated) {
    PhraseDictionaryInterpolated* pdi = new PhraseDictionaryInterpolated(GetNumScoreComponents(), m_numInputScores,this);
    bool ret = pdi->Load(
                 GetInput()
               , GetOutput()
               , m_config
               , weightT
               , m_tableLimit
               , system->GetLanguageModels()
               , system->GetWeightWordPenalty());
    CHECK(ret);
    return pdi;
  } else if (m_implementation == SCFG || m_implementation == Hiero) {
    // memory phrase table
    if (m_implementation == Hiero) {
      VERBOSE(2,"using Hiero format phrase tables" << std::endl);
    } else {
      VERBOSE(2,"using Moses-formatted SCFG phrase tables" << std::endl);
    }
    if (!FileExists(m_filePath) && FileExists(m_filePath + ".gz")) {
      m_filePath += ".gz";
      VERBOSE(2,"Using gzipped file" << std::endl);
    }

    RuleTableTrie *dict;
    if (staticData.GetParsingAlgorithm() == ParseScope3) {
      dict = new RuleTableUTrie(GetNumScoreComponents(), this);
    } else {
      dict = new PhraseDictionarySCFG(GetNumScoreComponents(), this);
    }
    bool ret = dict->Load(GetInput()
                         , GetOutput()
                         , m_filePath
                         , weightT
                         , m_tableLimit
                         , system->GetLanguageModels()
                         , system->GetWordPenaltyProducer());
    CHECK(ret);
    return dict;
  } else if (m_implementation == ALSuffixArray) {
    // memory phrase table
    VERBOSE(2,"using Hiero format phrase tables" << std::endl);
    if (!FileExists(m_filePath) && FileExists(m_filePath + ".gz")) {
      m_filePath += ".gz";
      VERBOSE(2,"Using gzipped file" << std::endl);
    }
    
    PhraseDictionaryALSuffixArray* pdm  = new PhraseDictionaryALSuffixArray(GetNumScoreComponents(),this);
    bool ret = pdm->Load(GetInput()
                         , GetOutput()
                         , m_filePath
                         , weightT
                         , m_tableLimit
                         , system->GetLanguageModels()
                         , system->GetWordPenaltyProducer());
    CHECK(ret);
    return pdm;
  } else if (m_implementation == OnDisk) {

    PhraseDictionaryOnDisk* pdta = new PhraseDictionaryOnDisk(GetNumScoreComponents(), this);
    bool ret = pdta->Load(GetInput()
                          , GetOutput()
                          , m_filePath
                          , weightT
                          , m_tableLimit
                          , system->GetLanguageModels()
                          , system->GetWordPenaltyProducer());
    CHECK(ret);
    return pdta;
  } else if (m_implementation == SuffixArray) {
#ifndef WIN32
    PhraseDictionaryDynSuffixArray *pd = new PhraseDictionaryDynSuffixArray(GetNumScoreComponents(), this);
    CHECK(m_config.size() >= 7);
    if(!(pd->Load(
           GetInput()
           ,GetOutput()
           ,m_filePath
           ,m_config[5]
           ,m_config[6]
           ,weightT, m_tableLimit
           ,system->GetLanguageModels()
	   ,system->GetWeightWordPenalty()))) {
      std::cerr << "FAILED TO LOAD\n" << endl;
      delete pd;
      pd = NULL;
    }
    std::cerr << "Suffix array phrase table loaded" << std::endl;
    return pd;
#else
    CHECK(false);
#endif
  } else if (m_implementation == FuzzyMatch) {
    
    PhraseDictionaryFuzzyMatch *dict = new PhraseDictionaryFuzzyMatch(GetNumScoreComponents(), this);

    bool ret = dict->Load(GetInput()
                          , GetOutput()
                          , m_filePath
                          , weightT
                          , m_tableLimit
                          , system->GetLanguageModels()
                          , system->GetWordPenaltyProducer());
    CHECK(ret);

    return dict;    
  } else if (m_implementation == Compact) {
#ifndef WIN32
    VERBOSE(2,"Using compact phrase table" << std::endl);                                                                                                                               
                                                                                                                                      
    PhraseDictionaryCompact* pd  = new PhraseDictionaryCompact(GetNumScoreComponents(), m_implementation, this);                         
    bool ret = pd->Load(GetInput(), GetOutput()                                                                                      
                         , m_filePath                                                                                                 
                         , weightT                                                                                                  
                         , m_tableLimit                                                                                               
                         , system->GetLanguageModels()                                                                                
                         , system->GetWeightWordPenalty());                                                                           
    CHECK(ret);                                                                                                                      
    return pd;                                                                                                                       
#else
    CHECK(false);
#endif
  } else if (m_implementation == MultiModel ) {
    // memory phrase table
    VERBOSE(2,"multi-model mode" << std::endl);
    if (staticData.GetInputType() != SentenceInput) {
      UserMessage::Add("Must use binary phrase table for this input type");
      CHECK(false);
    }

    PhraseDictionaryMultiModel* pd  = new PhraseDictionaryMultiModel(GetNumScoreComponents(),this);
    bool ret = pd->Load(GetInput(), GetOutput()
                         , m_config
                         , weightT
                         , m_tableLimit
                         , system->GetLanguageModels()
                         , system->GetWeightWordPenalty());
    CHECK(ret);
    return pd;
  } else if (m_implementation == MultiModelCounts) {
    // memory phrase table
    VERBOSE(2,"multi-model mode (count tables)" << std::endl);
    if (staticData.GetInputType() != SentenceInput) {
      UserMessage::Add("Must use binary phrase table for this input type");
      CHECK(false);
    }

    (const_cast<StaticData&>(staticData)).SetNeedAlignmentInfo(true); //needed for lexical weight computation

    PhraseDictionaryMultiModelCounts* pd  = new PhraseDictionaryMultiModelCounts(GetNumScoreComponents(),this);
    bool ret = pd->Load(GetInput(), GetOutput()
                         , m_config
                         , weightT
                         , m_tableLimit
                         , system->GetLanguageModels()
                         , system->GetWeightWordPenalty());
    CHECK(ret);
    return pd;
  }
  else {
    std::cerr << "Unknown phrase table type " << m_implementation << endl;
    CHECK(false);
  }
}

void PhraseDictionaryFeature::InitDictionary(const TranslationSystem* system)
{
  //Thread-safe phrase dictionaries get loaded now
  if (m_useThreadSafePhraseDictionary && !m_threadSafePhraseDictionary.get()) {
    IFVERBOSE(1)
    PrintUserTime("Start loading phrase table from " +  m_filePath);
    m_threadSafePhraseDictionary.reset(LoadPhraseTable(system));
    IFVERBOSE(1)
    PrintUserTime("Finished loading phrase tables");
  }
  //Other types will be lazy loaded
}

void PhraseDictionary::SetNumScoreComponentMultiModel(size_t num)
{
  m_numScoreComponentMultiModel = num;
}

size_t PhraseDictionary::GetNumScoreComponentMultiModel() const
{
  return m_numScoreComponentMultiModel;
}

//Called when we start translating a new sentence
void PhraseDictionaryFeature::InitDictionary(const TranslationSystem* system, const InputType& source)
{
  PhraseDictionary* dict;
  if (m_useThreadSafePhraseDictionary) {
    //thread safe dictionary should already be loaded
    dict = m_threadSafePhraseDictionary.get();
  } else {
    //thread-unsafe dictionary may need to be loaded if this is a new thread.
    if (!m_threadUnsafePhraseDictionary.get()) {
      m_threadUnsafePhraseDictionary.reset(LoadPhraseTable(system));
    }
    dict = m_threadUnsafePhraseDictionary.get();
  }
  CHECK(dict);
  dict->InitializeForInput(source);
}

const PhraseDictionary* PhraseDictionaryFeature::GetDictionary() const
{
  PhraseDictionary* dict;
  if (m_useThreadSafePhraseDictionary) {
    dict = m_threadSafePhraseDictionary.get();
  } else {
    dict = m_threadUnsafePhraseDictionary.get();
  }
  CHECK(dict);
  return dict;
}

PhraseDictionary* PhraseDictionaryFeature::GetDictionary()
{
  PhraseDictionary* dict;
  if (m_useThreadSafePhraseDictionary) {
    dict = m_threadSafePhraseDictionary.get();
  } else {
    dict = m_threadUnsafePhraseDictionary.get();
  }
  CHECK(dict);
  return dict;
}


PhraseDictionaryFeature::~PhraseDictionaryFeature()
{}


std::string PhraseDictionaryFeature::GetScoreProducerWeightShortName(unsigned idx) const
{
  if (idx < GetNumInputScores()){
    return "I";
  }else{
    return "tm";
  }
}


size_t PhraseDictionaryFeature::GetNumInputScores() const
{
  return m_numInputScores;
}

bool PhraseDictionaryFeature::ComputeValueInTranslationOption() const
{
  return true;
}

const PhraseDictionaryFeature* PhraseDictionary::GetFeature() const
{
  return m_feature;
}

size_t PhraseDictionaryFeature::GetDictIndex() const 
{
  return m_dictIndex;
}

}

