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
#ifndef _MIRA_DECODER_H_
#define _MIRA_DECODER_H_

#include <iostream>
#include <cstring>
#include <sstream>


#include "moses/ChartTrellisPathList.h"
#include "moses/Hypothesis.h"
#include "moses/Parameter.h"
#include "moses/SearchNormal.h"
#include "moses/Sentence.h"
#include "moses/StaticData.h"
#include "moses/FF/BleuScoreFeature.h"

//
// Wrapper functions and objects for the decoder.
//

namespace Mira
{

/**
  * Wraps moses decoder.
 **/
class MosesDecoder
{
public:
  /**
   * Initialise moses (including StaticData) using the given ini file and debuglevel, passing through any
   * other command line arguments.
   **/
  MosesDecoder(const std::string& inifile, int debuglevel,  int argc, std::vector<std::string> decoder_params);

  //returns the best sentence
  std::vector< std::vector<const Moses::Word*> > getNBest(const std::string& source,
      size_t sentenceid,
      size_t nbestSize,
      float bleuObjectiveweight, //weight of bleu in objective
      float bleuScoreWeight, //weight of bleu in score
      std::vector< Moses::ScoreComponentCollection>& featureValues,
      std::vector< float>& bleuScores,
      std::vector< float>& modelScores,
      size_t numReturnedTranslations,
      bool realBleu,
      bool distinct,
      bool avgRefLength,
      size_t rank,
      size_t epoch,
      std::string filename);
  std::vector< std::vector<const Moses::Word*> > runDecoder(const std::string& source,
      size_t sentenceid,
      size_t nbestSize,
      float bleuObjectiveweight, //weight of bleu in objective
      float bleuScoreWeight, //weight of bleu in score
      std::vector< Moses::ScoreComponentCollection>& featureValues,
      std::vector< float>& bleuScores,
      std::vector< float>& modelScores,
      size_t numReturnedTranslations,
      bool realBleu,
      bool distinct,
      size_t rank,
      size_t epoch,
      Moses::SearchAlgorithm& seach,
      std::string filename);
  std::vector< std::vector<const Moses::Word*> > runChartDecoder(const std::string& source,
      size_t sentenceid,
      size_t nbestSize,
      float bleuObjectiveweight, //weight of bleu in objective
      float bleuScoreWeight, //weight of bleu in score
      std::vector< Moses::ScoreComponentCollection>& featureValues,
      std::vector< float>& bleuScores,
      std::vector< float>& modelScores,
      size_t numReturnedTranslations,
      bool realBleu,
      bool distinct,
      size_t rank,
      size_t epoch);
  void initialize(Moses::StaticData& staticData, const std::string& source, size_t sentenceid,
                  float bleuObjectiveWeight, float bleuScoreWeight, bool avgRefLength, bool chartDecoding);
  void updateHistory(const std::vector<const Moses::Word*>& words);
  void updateHistory(const std::vector< std::vector< const Moses::Word*> >& words, std::vector<size_t>& sourceLengths, std::vector<size_t>& ref_ids, size_t rank, size_t epoch);
  void printBleuFeatureHistory(std::ostream& out);
  void printReferenceLength(const std::vector<size_t>& ref_ids);
  size_t getReferenceLength(size_t ref_id);
  size_t getClosestReferenceLength(size_t ref_id, int hypoLength);
  size_t getShortestReferenceIndex(size_t ref_id);
  void setBleuParameters(bool disable, bool sentenceBleu, bool scaleByInputLength, bool scaleByAvgInputLength,
                         bool scaleByInverseLength, bool scaleByAvgInverseLength,
                         float scaleByX, float historySmoothing, size_t scheme, bool simpleHistoryBleu);
  void setAvgInputLength (float l) {
    m_bleuScoreFeature->SetAvgInputLength(l);
  }
  Moses::ScoreComponentCollection getWeights();
  void setWeights(const Moses::ScoreComponentCollection& weights);
  void cleanup(bool chartDecoding);

  float getSourceLengthHistory() {
    return m_bleuScoreFeature->GetSourceLengthHistory();
  }
  float getTargetLengthHistory() {
    return m_bleuScoreFeature->GetTargetLengthHistory();
  }
  float getAverageInputLength() {
    return m_bleuScoreFeature->GetAverageInputLength();
  }

private:
  float getBleuScore(const Moses::ScoreComponentCollection& scores);
  void setBleuScore(Moses::ScoreComponentCollection& scores, float bleu);
  Moses::Manager *m_manager;
  Moses::ChartManager *m_chartManager;
  Moses::Sentence *m_sentence;
  Moses::BleuScoreFeature *m_bleuScoreFeature;
};


} //namespace

#endif
