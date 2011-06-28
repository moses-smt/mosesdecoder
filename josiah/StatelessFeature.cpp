/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2011 University of Edinburgh

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

#include <sstream>

#include "Gibbler.h"
#include "StatelessFeature.h"

using namespace Moses;
using namespace std;

namespace Josiah {


FeatureFunctionHandle StatelessFeature::getFunction(const Sample& sample) const {
  return FeatureFunctionHandle(new StatelessFeatureFunction(sample,this));
}



StatelessFeatureAdaptor::StatelessFeatureAdaptor(
  const MosesFeatureHandle& mosesFeature):
  m_mosesFeature(mosesFeature)
{
  assert(!mosesFeature->ComputeValueInTranslationOption());
  for (size_t i = 0; i < mosesFeature->GetNumScoreComponents(); ++i) {
    ostringstream id;
    id << i;
    m_featureNames.push_back(FName(mosesFeature->GetScoreProducerDescription(),id.str()));
  }
}



void StatelessFeatureAdaptor::assign
  (const Moses::TranslationOption* toption, FVector& scores) const {
  ScoreComponentCollection scc;
  m_mosesFeature->Evaluate(toption->GetTargetPhrase(),&scc);
  vector<float> mosesScores = scc.GetScoresForProducer(m_mosesFeature.get());
  for (size_t i = 0; i < m_featureNames.size(); ++i) {
    scores[m_featureNames[i]] = mosesScores.at(i);
  }
}


StatelessFeatureFunction::StatelessFeatureFunction
    (const Sample& sample, const StatelessFeature* parent):
      FeatureFunction(sample), m_parent(parent) {}


void StatelessFeatureFunction::assignScore(FVector& scores) {
    const Hypothesis* currHypo = getSample().GetTargetTail();
    while ((currHypo = (currHypo->GetNextHypo()))) {
      m_parent->assign(&(currHypo->GetTranslationOption()), scores);
    }
}


/** Score due to one segment */
void StatelessFeatureFunction::doSingleUpdate
  (const TranslationOption* option, const TargetGap& gap, FVector& scores) {
  m_parent->assign(option,scores);
}

/** Score due to two segments. The left and right refer to the target positions.**/
void StatelessFeatureFunction::doContiguousPairedUpdate
  (const TranslationOption* leftOption,const TranslationOption* rightOption, 
    const TargetGap& gap, FVector& scores) {
  m_parent->assign(leftOption,scores);
  m_parent->assign(rightOption,scores);
}

void StatelessFeatureFunction::doDiscontiguousPairedUpdate
  (const TranslationOption* leftOption,const TranslationOption* rightOption, 
    const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) {
  m_parent->assign(leftOption,scores);
  m_parent->assign(rightOption,scores);
}

/** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
void StatelessFeatureFunction::doFlipUpdate
  (const TranslationOption* leftOption,const TranslationOption* rightOption, 
     const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) {
  //do nothing
}




}
