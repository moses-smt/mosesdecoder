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
#ifndef _MIRA_OPTIMISER_H_
#define _MIRA_OPTIMISER_H_

#include <vector>

#include "moses/ScoreComponentCollection.h"


namespace Mira
{

class Optimiser
{
public:
  Optimiser() {}

  virtual size_t updateWeightsHopeFear(
    Moses::ScoreComponentCollection& weightUpdate,
    const std::vector<std::vector<Moses::ScoreComponentCollection> >& featureValuesHope,
    const std::vector<std::vector<Moses::ScoreComponentCollection> >& featureValuesFear,
    const std::vector<std::vector<float> >& bleuScoresHope,
    const std::vector<std::vector<float> >& bleuScoresFear,
    const std::vector<std::vector<float> >& modelScoresHope,
    const std::vector<std::vector<float> >& modelScoresFear,
    float learning_rate,
    size_t rank,
    size_t epoch,
    int updatePosition = -1) = 0;
};

class Perceptron : public Optimiser
{
public:
  virtual size_t updateWeightsHopeFear(
    Moses::ScoreComponentCollection& weightUpdate,
    const std::vector<std::vector<Moses::ScoreComponentCollection> >& featureValuesHope,
    const std::vector<std::vector<Moses::ScoreComponentCollection> >& featureValuesFear,
    const std::vector<std::vector<float> >& bleuScoresHope,
    const std::vector<std::vector<float> >& bleuScoresFear,
    const std::vector<std::vector<float> >& modelScoresHope,
    const std::vector<std::vector<float> >& modelScoresFear,
    float learning_rate,
    size_t rank,
    size_t epoch,
    int updatePosition = -1);
};

class MiraOptimiser : public Optimiser
{
public:
  MiraOptimiser() :
    Optimiser() { }

  MiraOptimiser(float slack) :
    Optimiser(),
    m_slack(slack),
    m_scale_margin(false),
    m_scale_update(false),
    m_boost(false),
    m_normaliseMargin(false),
    m_sigmoidParam(1.0) { }

  MiraOptimiser(float slack, bool scale_margin, bool scale_update,
                bool boost, bool normaliseMargin, float sigmoidParam) :
    Optimiser(),
    m_slack(slack),
    m_scale_margin(scale_margin),
    m_scale_update(scale_update),
    m_boost(boost),
    m_normaliseMargin(normaliseMargin),
    m_sigmoidParam(sigmoidParam) { }

  size_t updateWeights(
    Moses::ScoreComponentCollection& weightUpdate,
    const std::vector<std::vector<Moses::ScoreComponentCollection> >& featureValues,
    const std::vector<std::vector<float> >& losses,
    const std::vector<std::vector<float> >& bleuScores,
    const std::vector<std::vector<float> >& modelScores,
    const std::vector< Moses::ScoreComponentCollection>& oracleFeatureValues,
    const std::vector< float> oracleBleuScores,
    const std::vector< float> oracleModelScores,
    float learning_rate,
    size_t rank,
    size_t epoch);
  virtual size_t updateWeightsHopeFear(
    Moses::ScoreComponentCollection& weightUpdate,
    const std::vector<std::vector<Moses::ScoreComponentCollection> >& featureValuesHope,
    const std::vector<std::vector<Moses::ScoreComponentCollection> >& featureValuesFear,
    const std::vector<std::vector<float> >& bleuScoresHope,
    const std::vector<std::vector<float> >& bleuScoresFear,
    const std::vector<std::vector<float> >& modelScoresHope,
    const std::vector<std::vector<float> >& modelScoresFear,
    float learning_rate,
    size_t rank,
    size_t epoch,
    int updatePosition = -1);
  size_t updateWeightsAnalytically(
    Moses::ScoreComponentCollection& weightUpdate,
    Moses::ScoreComponentCollection& featureValuesHope,
    Moses::ScoreComponentCollection& featureValuesFear,
    float bleuScoreHope,
    float bleuScoreFear,
    float modelScoreHope,
    float modelScoreFear,
    float learning_rate,
    size_t rank,
    size_t epoch);

  void setSlack(float slack) {
    m_slack = slack;
  }

private:
  // regularise Hildreth updates
  float m_slack;


  // scale margin with BLEU score
  bool m_scale_margin;

  // scale update with oracle BLEU score
  bool m_scale_update;

  // boosting of updates on misranked candidates
  bool m_boost;

  // squash margin between 0 and 1 (or depending on m_sigmoidParam)
  bool m_normaliseMargin;

  // y=sigmoidParam is the axis that this sigmoid approaches
  float m_sigmoidParam ;
};
}

#endif
