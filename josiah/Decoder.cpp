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
#include "TrellisPathCollection.h"
#include "TrellisPath.h"

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
    
  
  void initMoses(const string& inifile, const std::string& weightfile, int debuglevel, bool l1normalize, int argc, char** argv) {
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
      const_cast<StaticData&>(StaticData::Instance()).InitWeightsFromFile(weightfile, l1normalize); //l1-normalize weights 
  }
  
  void MosesDecoder::decode(const std::string& source, Hypothesis*& bestHypo, TranslationOptionCollection*& toc, std::vector<Word>& sent_vector, size_t nBestSize) {
     
      const StaticData &staticData = StaticData::Instance();

      //clean up previous sentence
      staticData.CleanUpAfterSentenceProcessing();
      
      //maybe ignore lm
      const LMList& languageModels = StaticData::Instance().GetAllLM();
      vector<float> prevFeatureWeights;
      GetFeatureWeights(&prevFeatureWeights);
      if (m_useZeroWeights) {
        vector<float> newFeatureWeights = prevFeatureWeights; //copy
        fill(newFeatureWeights.begin(),newFeatureWeights.end(),0);
        SetFeatureWeights(newFeatureWeights);
      } else if (m_useNoLM) {
        vector<float> newFeatureWeights = prevFeatureWeights; //copy
        for (LMList::const_iterator i = languageModels.begin(); i != languageModels.end(); ++i) {
          LanguageModel* lm = *i;
          const ScoreIndexManager& siManager = StaticData::Instance().GetScoreIndexManager();
          size_t lmindex = siManager.GetBeginIndex(lm->GetScoreBookkeepingID());
          newFeatureWeights[lmindex] = 0;
        }
        SetFeatureWeights(newFeatureWeights);
      }
  
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
       
      if (nBestSize > 0)
        const_cast<StaticData&>(staticData).SetOutputSearchGraph(true);
    
      m_searcher.reset(createSearch(sentence,*m_toc));
      m_searcher->ProcessSentence();
  
      //get hypo
      bestHypo = const_cast<Hypothesis*>(m_searcher->GetBestHypothesis());
      cerr << "Moses hypothesis: " << *bestHypo << endl;
      toc = m_toc.get();
    
      // n-best
      if (nBestSize > 0) {
        TrellisPathList nBestList;
        CalcNBest(nBestSize, nBestList,true);
        //need to convert m_nBestList into some format the gibbler can use
        TrellisToTranslations(nBestList, m_translations);
      }
      
      const_cast<StaticData&>(staticData).SetMaxDistortion(distortionLimit);
      SetFeatureWeights(prevFeatureWeights);
      if (nBestSize > 0)
        const_cast<StaticData&>(staticData).SetOutputSearchGraph(false);
    
  }
  
  void MosesDecoder::TrellisToTranslations(const TrellisPathList &nBestList, vector<Translation> &m_translations){
    TrellisPathList::const_iterator iter;
    for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter)
    {
      const TrellisPath &path = **iter;
      const std::vector<const Hypothesis *> &edges = path.GetEdges();
      Translation t;
      
      for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--)
      {
          const Hypothesis &edge = *edges[currEdge];
          const Phrase& phrase = edge.GetCurrTargetPhrase();
          size_t size = phrase.GetSize();
          for (size_t pos = 0 ; pos < size ; pos++)
          {
            const Factor *factor = phrase.GetFactor(pos, 0);
            t.push_back(factor);
          }
      }
      //for (size_t i = 0; i < t.size(); ++i) {
//        cout << *(t[i]) << " ";
//      }
//      cout << endl;
      
      m_translations.push_back(t);
    }  
  }
  
  void MosesDecoder::CalcNBest(size_t count, TrellisPathList &ret,bool onlyDistinct) const
  {
    if (count <= 0)
      return;
    
    const std::vector < HypothesisStack* > &hypoStackColl = m_searcher->GetHypothesisStacks();
    
    vector<const Hypothesis*> sortedPureHypo = hypoStackColl.back()->GetSortedList();
    
    if (sortedPureHypo.size() == 0)
      return;
    
    TrellisPathCollection contenders;
    
    set<Phrase> distinctHyps;
    
    // add all pure paths
    vector<const Hypothesis*>::const_iterator iterBestHypo;
    for (iterBestHypo = sortedPureHypo.begin() 
         ; iterBestHypo != sortedPureHypo.end()
         ; ++iterBestHypo)
    {
      contenders.Add(new TrellisPath(*iterBestHypo));
    }
    
    // factor defines stopping point for distinct n-best list if too many candidates identical
    size_t nBestFactor = StaticData::Instance().GetNBestFactor();
    if (nBestFactor < 1) nBestFactor = 1000; // 0 = unlimited
    
    // MAIN loop
    for (size_t iteration = 0 ; (onlyDistinct ? distinctHyps.size() : ret.GetSize()) < count && contenders.GetSize() > 0 && (iteration < count * nBestFactor) ; iteration++)
    {
      // get next best from list of contenders
      TrellisPath *path = contenders.pop();
      assert(path);
      if(onlyDistinct)
      {
        Phrase tgtPhrase = path->GetSurfacePhrase();
        if (distinctHyps.insert(tgtPhrase).second) 
          ret.Add(path);
      }
      else 
      {
        ret.Add(path);
      }
      
      // create deviations from current best
      path->CreateDeviantPaths(contenders);		
      
      if(onlyDistinct)
      {
        const size_t nBestFactor = StaticData::Instance().GetNBestFactor();
        if (nBestFactor > 0)
          contenders.Prune(count * nBestFactor);
      }
      else
      {
        contenders.Prune(count);
      }
    }
  }
  
  
  Search* MosesDecoder::createSearch(Moses::Sentence& sentence, Moses::TranslationOptionCollection& toc) {
    return new SearchNormal(sentence,toc);
  }

  void GetFeatureNames(std::vector<std::string>* featureNames) {
    const StaticData &staticData = StaticData::Instance();
    const ScoreIndexManager& sim = staticData.GetScoreIndexManager();
    featureNames->resize(sim.GetTotalNumberOfScores());
    for (size_t i = 0; i < featureNames->size(); ++i)
      (*featureNames)[i] = sim.GetFeatureName(i);
  }

  void GetFeatureWeights(std::vector<float>* weights){
    const StaticData &staticData = StaticData::Instance();
    *weights = staticData.GetAllWeights();  
  }

  void SetFeatureWeights(const std::vector<float>& weights, bool computeScaleGradient ) {
    if (computeScaleGradient) {
      vector<float> featWeights(weights.size() -1);
      copy(weights.begin(), weights.end() - 1, featWeights.begin()); 
      const_cast<StaticData&>(StaticData::Instance()).SetAllWeights(featWeights);   
    }
    else {
      const_cast<StaticData&>(StaticData::Instance()).SetAllWeights(weights);  
    }
  }
  
  void OutputWeights(std::ostream& out) {
    vector<string> names;
    GetFeatureNames(&names);
    vector<float> weights;
    GetFeatureWeights(&weights);
    assert(names.size() == weights.size());
    for (size_t i = 0; i < weights.size(); ++i) {
      out << names[i] << " " << weights[i] << endl;
    }
  }
  
  void OutputWeights(const std::vector<float>& weights, std::ostream& out) {
    vector<string> names;
    GetFeatureNames(&names);
    assert(names.size() == weights.size());
    for (size_t i = 0; i < weights.size(); ++i) {
      out << names[i] << " " << weights[i] << endl;
    }
  }

  Search* RandomDecoder::createSearch(Moses::Sentence& sentence, Moses::TranslationOptionCollection& toc) {
    return new SearchRandom(sentence,toc);
  }
  
 
  
}
