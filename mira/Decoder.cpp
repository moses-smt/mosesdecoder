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
		  char *cstr = &(decoder_params[i])[0];
		  mosesargv[BASE_ARGC + i] = cstr;
	  }

	  params->LoadParam(BASE_ARGC + argc,mosesargv);
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

  vector<const Word*> MosesDecoder::getNBest(const std::string& source,
                              size_t sentenceid,
                              size_t count,
                              float bleuObjectiveWeight, 
                              float bleuScoreWeight,
                              vector< ScoreComponentCollection>& featureValues,
                              vector< float>& bleuScores,
                              bool oracle,
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
    m_bleuScoreFeature->SetCurrentReference(sentenceid);

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

    	Phrase bestPhrase = path.GetTargetPhrase();

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

    // get the best
    vector<const Word*> best;
    if (oracle) {
        assert(sentences.GetSize() > 0);
        const TrellisPath &path = sentences.at(0);
        Phrase bestPhrase = path.GetTargetPhrase();

        for (size_t pos = 0; pos < bestPhrase.GetSize(); ++pos) {
        	const Word &word = bestPhrase.GetWord(pos);
        	Word *newWord = new Word(word);
        	best.push_back(newWord);
    	}
    }

//    cerr << "Rank " << rank << ", use cache: " << staticData.GetUseTransOptCache() << ", weights: " << staticData.GetAllWeights() << endl;
    return best;
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

  void MosesDecoder::printReferenceLength(const vector<size_t>& ref_ids) {
  	m_bleuScoreFeature->PrintReferenceLength(ref_ids);
  }

  size_t MosesDecoder::getReferenceLength(size_t ref_id) {
  	return m_bleuScoreFeature->GetReferenceLength(ref_id);
  }

  void MosesDecoder::setBleuParameters(bool scaleByInputLength, bool scaleByRefLength, bool scaleByAvgLength,
  		bool scaleByTargetLengthLinear, bool scaleByTargetLengthTrend,
		  float scaleByX, float historySmoothing, size_t scheme, float relax_BP) {
	  m_bleuScoreFeature->SetBleuParameters(scaleByInputLength, scaleByRefLength, scaleByAvgLength,
	  		scaleByTargetLengthLinear, scaleByTargetLengthTrend,
			  scaleByX, historySmoothing, scheme, relax_BP);
  }
} 

