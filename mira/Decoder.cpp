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
#include "Manager.h"
#include "Sentence.h"
#include "TranslationSystem.h"
#include "Phrase.h"
#include "TrellisPathList.h"
#include "DummyScoreProducers.h"

using namespace std;
using namespace Moses;


namespace Mira {

  //Decoder::~Decoder() {}

  /**
    * Allocates a char* and copies string into it.
  **/
  static char* strToChar(const string& s) {
    char* c = new char[s.size()+1];
    strcpy(c,s.c_str());
    return c;
  }
      
  void initMoses(const string& inifile, int debuglevel,  int argc, char** argv) {
    static int BASE_ARGC = 5;
    Parameter* params = new Parameter();
    char ** mosesargv = new char*[BASE_ARGC + argc];
    mosesargv[0] = strToChar("-f");
    mosesargv[1] = strToChar(inifile);
    mosesargv[2] = strToChar("-v");
    stringstream dbgin;
    dbgin << debuglevel;
    mosesargv[3] = strToChar(dbgin.str());
    mosesargv[4] = strToChar("-mbr"); //so we can do nbest
    
    for (int i = 0; i < argc; ++i) {
      mosesargv[BASE_ARGC + i] = argv[i];
    }
    params->LoadParam(BASE_ARGC + argc,mosesargv);
    StaticData::LoadDataStatic(params);
    for (int i = 0; i < BASE_ARGC; ++i) {
      delete[] mosesargv[i];
    }
    delete[] mosesargv;
  }
 
  MosesDecoder::MosesDecoder() 
		: m_manager(NULL)
		{
      //force initialisation of the phrase dictionary
      string source("hello");
      vector<const ScoreComponentCollection*> featureScores;
      vector<float> totalScores;
      getNBest(source,1,featureScores,totalScores);

      //Add the bleu feature
      m_bleuScoreFeature = new BleuScoreFeature();
      const TranslationSystem& system = StaticData::Instance().GetTranslationSystem
          (TranslationSystem::DEFAULT);
      (const_cast<TranslationSystem&>(system)).AddFeatureFunction(m_bleuScoreFeature);
    }
  
	void MosesDecoder::cleanup()
	{
		delete m_manager;
		delete m_sentence;
	}
	
  void MosesDecoder::getNBest(const std::string& source,
                              size_t count,
                              vector<const ScoreComponentCollection*>& featureScores,
                              std::vector<float>& totalScores  )
  {

    const StaticData &staticData = StaticData::Instance();

		m_sentence = new Sentence(Input);
    stringstream in(source + "\n");
    const std::vector<FactorType> &inputFactorOrder = staticData.GetInputFactorOrder();
    m_sentence->Read(in,inputFactorOrder);
    const TranslationSystem& system = staticData.GetTranslationSystem
        (TranslationSystem::DEFAULT);

    m_manager = new Moses::Manager(*m_sentence, staticData.GetSearchAlgorithm(), &system); 
    m_manager->ProcessSentence();
    TrellisPathList sentences;
    m_manager->CalcNBest(count,sentences);
						
		Moses::TrellisPathList::const_iterator iter;
		for (iter = sentences.begin() ; iter != sentences.end() ; ++iter)
		{
			const Moses::TrellisPath &path = **iter;
			cerr << path << endl << endl;
			
			featureScores.push_back(&path.GetScoreBreakdown());
      totalScores.push_back(path.GetTotalScore());
		}
			
	}

  float MosesDecoder::getBleuScore(const ScoreComponentCollection& scores) {
    return scores.GetScoreForProducer(m_bleuScoreFeature);
  }
	
} 

