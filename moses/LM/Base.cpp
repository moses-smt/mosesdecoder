// $Id$

/***********************************************************************
Moses - statistical machine translation system
Copyright (C) 2006-2011 University of Edinburgh

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

#include "Base.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/Manager.h"
#include "moses/ChartManager.h"
#include "moses/FactorCollection.h"
#include "moses/Phrase.h"
#include "moses/StaticData.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{

LanguageModel::LanguageModel(const std::string &line) :
  StatefulFeatureFunction(StaticData::Instance().GetLMEnableOOVFeature() ? 2 : 1, line )
{
  m_enableOOVFeature = StaticData::Instance().GetLMEnableOOVFeature();
}


LanguageModel::~LanguageModel() {}

float LanguageModel::GetWeight() const
{
  //return StaticData::Instance().GetAllWeights().GetScoresForProducer(this)[0];
  return StaticData::Instance().GetWeights(this)[0];
}

float LanguageModel::GetOOVWeight() const
{
  if (m_enableOOVFeature) {
    //return StaticData::Instance().GetAllWeights().GetScoresForProducer(this)[1];
    return StaticData::Instance().GetWeights(this)[1];
  } else {
    return 0;
  }
}

void LanguageModel::IncrementalCallback(Incremental::Manager &manager) const
{
  UTIL_THROW(util::Exception, "Incremental search is only supported by KenLM.");
}

void LanguageModel::ReportHistoryOrder(std::ostream &out,const Phrase &phrase) const
{
  // out << "ReportHistoryOrder not implemented";
}

void LanguageModel::Evaluate(const Phrase &source
                             , const TargetPhrase &targetPhrase
                             , ScoreComponentCollection &scoreBreakdown
                             , ScoreComponentCollection &estimatedFutureScore) const
{
  // contains factors used by this LM
  float fullScore, nGramScore;
  size_t oovCount;

  CalcScore(targetPhrase, fullScore, nGramScore, oovCount);
  float estimateScore = fullScore - nGramScore;

  if (StaticData::Instance().GetLMEnableOOVFeature()) {
    vector<float> scores(2), estimateScores(2);
    scores[0] = nGramScore;
    scores[1] = oovCount;
    scoreBreakdown.Assign(this, scores);

    estimateScores[0] = estimateScore;
    estimateScores[1] = 0;
    estimatedFutureScore.Assign(this, estimateScores);
  } else {
    scoreBreakdown.Assign(this, nGramScore);
    estimatedFutureScore.Assign(this, estimateScore);
  }
}

const LanguageModel &LanguageModel::GetFirstLM()
{
  static const LanguageModel *lmStatic = NULL;

  if (lmStatic) {
    return *lmStatic;
  }

  // 1st time looking up lm
  const std::vector<const StatefulFeatureFunction*> &statefulFFs = StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for (size_t i = 0; i < statefulFFs.size(); ++i) {
    const StatefulFeatureFunction *ff = statefulFFs[i];
    const LanguageModel *lm = dynamic_cast<const LanguageModel*>(ff);

    if (lm) {
      lmStatic = lm;
      return *lmStatic;
    }
  }

  throw std::logic_error("Incremental search only supports one language model.");
}

} // namespace Moses
