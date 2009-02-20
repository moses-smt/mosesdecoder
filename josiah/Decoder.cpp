/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

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

#include "Decoder.h"

using namespace std;
using namespace Moses;


namespace Josiah {

  Decoder::~Decoder() {}

  /**
    * Allocates a char* and copies string into it.
  **/
  static char* strToChar(const string& s) {
    char* c = new char[s.size()+1];
    strcpy(c,s.c_str());
    return c;
  }
    
  
  void initMoses(const string& inifile, const std::string& weightfile, int debuglevel, int argc, char** argv) {
    static int BASE_ARGC = 4;
    Parameter* params = new Parameter();
    char ** mosesargv = new char*[BASE_ARGC + argc];
    mosesargv[0] = strToChar("-f");
    mosesargv[1] = strToChar(inifile);
    mosesargv[2] = strToChar("-v");
    stringstream dbgin;
    dbgin << debuglevel;
    mosesargv[3] = strToChar(dbgin.str());
    
    for (int i = 0; i < argc; ++i) {
      mosesargv[BASE_ARGC + i] = argv[i];
    }
    params->LoadParam(BASE_ARGC + argc,mosesargv);
    StaticData::LoadDataStatic(params);
    for (int i = 0; i < BASE_ARGC; ++i) {
      delete[] mosesargv[i];
    }
    delete[] mosesargv;
    if (!weightfile.empty())
      const_cast<StaticData&>(StaticData::Instance()).InitWeightsFromFile(weightfile);
  }
  
  
  void MosesDecoder::decode(const std::string& source, Hypothesis*& bestHypo, TranslationOptionCollection*& toc, std::vector<Word>& sent_vector) {
     
      const StaticData &staticData = StaticData::Instance();

      //clean up previous sentence
      staticData.CleanUpAfterSentenceProcessing();
  
      //the sentence
      Sentence sentence(Input);
      stringstream in(source + "\n");
      const std::vector<FactorType> &inputFactorOrder = staticData.GetInputFactorOrder();
      sentence.Read(in,inputFactorOrder);
      for (size_t i=0; i<sentence.GetSize(); ++i){ sent_vector.push_back(sentence.GetWord(i)); } 
      //monotone
      int distortionLimit = staticData.GetMaxDistortion();
      if (m_isMonotone) {
        const_cast<StaticData&>(staticData).SetMaxDistortion(0);
      }
  
      //the searcher
      staticData.ResetSentenceStats(sentence);
      staticData.InitializeBeforeSentenceProcessing(sentence);
      m_toc.reset(sentence.CreateTranslationOptionCollection());
      const vector <DecodeGraph*>
            &decodeStepVL = staticData.GetDecodeStepVL();
      m_toc->CreateTranslationOptions(decodeStepVL);
      
      m_searcher.reset(new SearchNormal(sentence,*m_toc));
      m_searcher->ProcessSentence();
  
      //get hypo
      bestHypo = const_cast<Hypothesis*>(m_searcher->GetBestHypothesis());
      toc = m_toc.get();
      
      const_cast<StaticData&>(staticData).SetMaxDistortion(distortionLimit);
  }

  void MosesDecoder::GetFeatureNames(std::vector<std::string>* featureNames) const {
    const StaticData &staticData = StaticData::Instance();
    const ScoreIndexManager& sim = staticData.GetScoreIndexManager();
    featureNames->resize(sim.GetTotalNumberOfScores());
    for (size_t i = 0; i < featureNames->size(); ++i)
      (*featureNames)[i] = sim.GetFeatureName(i);
  }

  void MosesDecoder::GetFeatureWeights(std::vector<float>* weights) const {
    const StaticData &staticData = StaticData::Instance();
    *weights = staticData.GetAllWeights();
  }

  void MosesDecoder::SetFeatureWeights(const std::vector<float>& weights) {
    const_cast<StaticData&>(StaticData::Instance()).SetAllWeights(weights);
  }
  
  void MosesDecoder::SetMonotone(bool isMonotone) {
    m_isMonotone = isMonotone;
  }
}
