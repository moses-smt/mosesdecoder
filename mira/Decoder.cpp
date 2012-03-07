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
#include "InputType.h"
#include "TranslationSystem.h"
#include "Phrase.h"
#include "TrellisPathList.h"
#include "DummyScoreProducers.h"

using namespace std;
using namespace Moses;


namespace Mira {

  /**
    * Allocates a char* and copies string into it.
  **/
  static char* strToChar(const string& s) {
    char* c = new char[s.size()+1];
    strcpy(c,s.c_str());
    return c;
  }

  MosesDecoder::MosesDecoder(const string& inifile, int debuglevel, int argc, vector<string> decoder_params)
		: m_manager(NULL) {
//	  static int BASE_ARGC = 5;
  	static int BASE_ARGC = 4;
	  Parameter* params = new Parameter();
	  char ** mosesargv = new char*[BASE_ARGC + argc];
	  mosesargv[0] = strToChar("-f");
	  mosesargv[1] = strToChar(inifile);
	  mosesargv[2] = strToChar("-v");
	  stringstream dbgin;
	  dbgin << debuglevel;
	  mosesargv[3] = strToChar(dbgin.str());
//	  mosesargv[4] = strToChar("-mbr"); //so we can do nbest

	  for (int i = 0; i < argc; ++i) {
		  char *cstr = &(decoder_params[i])[0];
		  mosesargv[BASE_ARGC + i] = cstr;
	  }

	  if (!params->LoadParam(BASE_ARGC + argc,mosesargv)) {
		  cerr << "Loading static data failed, exit." << endl;
		  exit(1);
	  }
	  StaticData::LoadDataStatic(params);
	  for (int i = 0; i < BASE_ARGC; ++i) {
		  delete[] mosesargv[i];
	  }
	  delete[] mosesargv;

	  const StaticData &staticData = StaticData::Instance();
      m_bleuScoreFeature = staticData.GetBleuScoreFeature();
  }
  
  void MosesDecoder::cleanup() {
	  delete m_manager;
	  delete m_sentence;
  }

  vector< vector<const Word*> > MosesDecoder::getNBest(const std::string& source,
                              size_t sentenceid,
                              size_t count,
                              float bleuObjectiveWeight, 
                              float bleuScoreWeight,
                              vector< ScoreComponentCollection>& featureValues,
                              vector< float>& bleuScores,
                              vector< float>& modelScores,
                              size_t numReturnedTranslations,
                              bool distinct,
                              size_t rank,
                              size_t epoch)
  {
  	StaticData &staticData = StaticData::InstanceNonConst();

  	m_sentence = new Sentence(Input);
    stringstream in(source + "\n");
    const std::vector<FactorType> &inputFactorOrder = staticData.GetInputFactorOrder();
    m_sentence->Read(in,inputFactorOrder);
    const TranslationSystem& system = staticData.GetTranslationSystem(TranslationSystem::DEFAULT);

    // set weight of BleuScoreFeature
    /*ostringstream bleuWeightStr;
    bleuWeightStr << (bleuObjectiveWeight*bleuScoreWeight);
    PARAM_VEC bleuWeight(1,bleuWeightStr.str());
    staticData.GetParameter()->OverwriteParam("weight-bl", bleuWeight);*/
    staticData.ReLoadBleuScoreFeatureParameter(bleuObjectiveWeight*bleuScoreWeight);

    m_bleuScoreFeature->SetCurrentSourceLength((*m_sentence).GetSize());
    m_bleuScoreFeature->SetCurrentShortestReference(sentenceid);

    //run the decoder
    m_manager = new Moses::Manager(*m_sentence, staticData.GetSearchAlgorithm(), &system); 
    m_manager->ProcessSentence();
    TrellisPathList sentences;
    m_manager->CalcNBest(count,sentences, distinct);
						
    // read off the feature values and bleu scores for each sentence in the nbest list
    Moses::TrellisPathList::const_iterator iter;
    for (iter = sentences.begin() ; iter != sentences.end() ; ++iter) {
    	const Moses::TrellisPath &path = **iter;
    	featureValues.push_back(path.GetScoreBreakdown());
    	float bleuScore = getBleuScore(featureValues.back());
    	bleuScores.push_back(bleuScore);

    	//std::cout << "Score breakdown: " << path.GetScoreBreakdown() << endl;
    	float scoreWithoutBleu = path.GetTotalScore() - (bleuObjectiveWeight * bleuScoreWeight * bleuScore);
    	modelScores.push_back(scoreWithoutBleu);

    	Phrase bestPhrase = path.GetTargetPhrase();

    	if (iter != sentences.begin())
    		cerr << endl;
    	cerr << "Rank " << rank << ", epoch " << epoch << ", \"";
    	Phrase phrase = path.GetTargetPhrase();
    	for (size_t pos = 0; pos < phrase.GetSize(); ++pos) {
    		const Word &word = phrase.GetWord(pos);
    		Word *newWord = new Word(word);
    		cerr << *newWord;
    	}

    	cerr << "\", score: " << scoreWithoutBleu << ", Bleu: " << bleuScore << ", total: " << path.GetTotalScore();

    	// set bleu score to zero in the feature vector since we do not want to optimise its weight
    	setBleuScore(featureValues.back(), 0);
    }

    // prepare translations to return
    vector< vector<const Word*> > translations;
    for (size_t i=0; i < numReturnedTranslations && i < sentences.GetSize(); ++i) {
        const TrellisPath &path = sentences.at(i);
        Phrase phrase = path.GetTargetPhrase();

        vector<const Word*> translation;
        for (size_t pos = 0; pos < phrase.GetSize(); ++pos) {
        	const Word &word = phrase.GetWord(pos);
        	Word *newWord = new Word(word);
        	translation.push_back(newWord);
        }
        translations.push_back(translation);
    }

//    cerr << "Rank " << rank << ", use cache: " << staticData.GetUseTransOptCache() << ", weights: " << staticData.GetAllWeights() << endl;
    return translations;
  }

  size_t MosesDecoder::getCurrentInputLength() {
	  return (*m_sentence).GetSize();
  }

  float MosesDecoder::getBleuScore(const ScoreComponentCollection& scores) {
    return scores.GetScoreForProducer(m_bleuScoreFeature);
  }

  void MosesDecoder::setBleuScore(ScoreComponentCollection& scores, float bleu) {
    scores.Assign(m_bleuScoreFeature, bleu);
  }

  ScoreComponentCollection MosesDecoder::getWeights() {
    return StaticData::Instance().GetAllWeights();
  }

  void MosesDecoder::setWeights(const ScoreComponentCollection& weights) {
    StaticData::InstanceNonConst().SetAllWeights(weights);
  }

  void MosesDecoder::updateHistory(const vector<const Word*>& words) {
    m_bleuScoreFeature->UpdateHistory(words);
  }

  void MosesDecoder::updateHistory(const vector< vector< const Word*> >& words, vector<size_t>& sourceLengths, vector<size_t>& ref_ids, size_t rank, size_t epoch) {
	  m_bleuScoreFeature->UpdateHistory(words, sourceLengths, ref_ids, rank, epoch);
  }

/*  void MosesDecoder::loadReferenceSentences(const vector<vector<string> >& refs) {
  	m_bleuScoreFeature->LoadReferences(refs);
  }*/

  void MosesDecoder::printBleuFeatureHistory(std::ostream& out) {
  	m_bleuScoreFeature->PrintHistory(out);
  }

/*  void MosesDecoder::printReferenceLength(const vector<size_t>& ref_ids) {
  	m_bleuScoreFeature->PrintReferenceLength(ref_ids);
  }*/

  size_t MosesDecoder::getClosestReferenceLength(size_t ref_id, int hypoLength) {
  	return m_bleuScoreFeature->GetClosestReferenceLength(ref_id, hypoLength);
  }

  void MosesDecoder::setBleuParameters(bool scaleByInputLength, bool scaleByRefLength, bool scaleByAvgLength,
  		bool scaleByInverseLinear, float scaleByX, float historySmoothing, size_t scheme, float relax_BP) {
	  m_bleuScoreFeature->SetBleuParameters(scaleByInputLength, scaleByRefLength, scaleByAvgLength,
	  		scaleByInverseLinear, scaleByX, historySmoothing, scheme, relax_BP);
  }
} 

