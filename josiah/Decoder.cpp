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

#include <algorithm>

#include "Decoder.h"
#include "DummyScoreProducers.h"
#include "Manager.h"
#include "PhraseFeature.h"
#include "TranslationSystem.h"
#include "TrellisPathCollection.h"
#include "TrellisPath.h"

using namespace std;
using namespace Moses;


namespace Josiah {

  /**
    * Allocates a char* and copies string into it.
  **/
  static char* strToChar(const string& s) {
    char* c = new char[s.size()+1];
    strcpy(c,s.c_str());
    return c;
  }
    
  
  void initMoses(const string& inifile, int debuglevel, const  vector<string>& extraArgs) {
    static int BASE_ARGC = 6;
    Parameter* params = new Parameter();
    char ** mosesargv = new char*[BASE_ARGC + extraArgs.size()];
    mosesargv[0] = strToChar("-f");
    mosesargv[1] = strToChar(inifile);
    mosesargv[2] = strToChar("-max-trans-opt-per-coverage");
    mosesargv[3] = strToChar("0");
    mosesargv[4] = strToChar("-v");
    stringstream dbgin;
    dbgin << debuglevel;
    mosesargv[5] = strToChar(dbgin.str());
    for (size_t i = 0; i < extraArgs.size(); ++i) {
      mosesargv[BASE_ARGC+i] = strToChar(extraArgs[i]);
    }
    
    params->LoadParam(BASE_ARGC + extraArgs.size(),mosesargv);
    StaticData::LoadDataStatic(params);
    for (int i = 0; i < BASE_ARGC; ++i) {
      delete[] mosesargv[i];
    }
    delete[] mosesargv;
  }

  void setMosesWeights(const FVector& currentWeights) {
    PhraseFeature::updateWeights(currentWeights);
    StaticData& staticData =
      const_cast<StaticData&>(StaticData::Instance());
    TranslationSystem& system = 
      const_cast<TranslationSystem&>(staticData.GetTranslationSystem(
        TranslationSystem::DEFAULT));

    ScoreComponentCollection  mosesWeights = staticData.GetAllWeights();
    for (LMList::const_iterator i = system.GetLanguageModels().begin();
     i !=  system.GetLanguageModels().end(); ++i) {
      LanguageModel* lm = const_cast<LanguageModel*>(*i);
      float lmWeight = currentWeights[lm->GetScoreProducerDescription()];
      //lm->SetWeight(lmWeight);
      mosesWeights.Assign(lm,lmWeight);
    }
    const ScoreProducer* wp  = system.GetWordPenaltyProducer();
    const string wpName = wp->GetScoreProducerDescription();
    //staticData.SetWeightWordPenalty(currentWeights[wpName]);
    mosesWeights.Assign(wp,currentWeights[wpName]);

    const ScoreProducer* dp = system.GetDistortionProducer();
    string distName = dp->GetScoreProducerDescription();
    //staticData.SetWeightDistortion(currentWeights[distName]);
    mosesWeights.Assign(dp, currentWeights[distName]);

    staticData.SetAllWeights(mosesWeights);
  }

  struct TOptCompare {
    bool operator()(const TranslationOption* lhs, const TranslationOption* rhs) {
      return lhs->GetFutureScore() > rhs->GetFutureScore();
    }
  };

  static const TargetPhrase& emptyTarget() {
    static TargetPhrase* tp = new TargetPhrase(Input);
    return *tp;
  }

  //Ensures that cleanup is not run the first time around
  bool TranslationHypothesis::m_cleanup = false;

  TranslationHypothesis::TranslationHypothesis(const string& source) 
    {

    const StaticData &staticData = StaticData::Instance();
    const TranslationSystem& system = 
      staticData.GetTranslationSystem(TranslationSystem::DEFAULT);

    //clean up previous sentence
    if (m_cleanup) {
      system.CleanUpAfterSentenceProcessing();
    } else {
      m_cleanup = true;
    }
    
    //the sentence
    Sentence sentence(Input);
    stringstream in(source + "\n");
    const std::vector<FactorType> &inputFactorOrder = staticData.GetInputFactorOrder();
    sentence.Read(in,inputFactorOrder);
    for (size_t i=0; i<sentence.GetSize(); ++i){ m_words.push_back(sentence.GetWord(i)); } 

    //translation options
    m_manager.reset(new Manager(sentence, Normal, &system));
    m_manager->ResetSentenceStats(sentence);
    system.InitializeBeforeSentenceProcessing(sentence);
    m_toc.reset(sentence.CreateTranslationOptionCollection(&system));
    //const vector <DecodeGraph*>
    //      &decodeStepVL = staticData.GetDecodeGraphs();
    m_toc->CreateTranslationOptions();

    //sort the options
    size_t maxPhraseSize = staticData.GetMaxPhraseLength();
    for (size_t start = 0; start < m_words.size(); ++start) {
      for (size_t end = start; end < start + maxPhraseSize && end < m_words.size(); ++end) {
        TranslationOptionList& options = 
          m_toc->GetTranslationOptionList(start,end);
        sort(options.begin(), options.end(), TOptCompare());
        /*
        while (options.size() > ttableLimit) {
          size_t pos = options.size() - 1;
          delete options.Get(pos);
          options.Remove(pos); 
        }*/
      }
    }

    //hypothesis
    m_hypothesis.reset(Hypothesis::Create(*m_manager,sentence, emptyTarget()));
    for (size_t i = 0; i < m_words.size(); ++i) {
      m_allHypos.push_back(m_hypothesis);
      WordsRange segment(i,i);
      const TranslationOptionList& options = 
        m_toc->GetTranslationOptionList(segment);
      
      
      /*
      cerr << "Options for " << *(options.Get(0)->GetSourcePhrase()) << endl;
      for (size_t j = 0; j < options.size(); ++j) {
        cerr << *(options.Get(j)) << endl;
      }*/
      assert(options.size());
      m_hypothesis.reset(
        Hypothesis::Create(*m_hypothesis, *(options.Get(0)), NULL)); 
    }
}

  TranslationOptionCollection* TranslationHypothesis::getToc() const {
    return m_toc.get();
  }

  Hypothesis* TranslationHypothesis::getHypothesis() const {
    return m_hypothesis.get();
  }

  const vector<Word>& TranslationHypothesis::getWords() const {
    return m_words;
  }
  
}
