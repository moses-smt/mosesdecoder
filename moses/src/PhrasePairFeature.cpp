#include "AlignmentInfo.h"
#include "PhrasePairFeature.h"
#include "TargetPhrase.h"


using namespace std;

namespace Moses {


PhrasePairFeature::PhrasePairFeature
  (FactorType sourceFactorId, FactorType targetFactorId) :
  StatelessFeatureFunction("pp"),
  m_sourceFactorId(sourceFactorId),
  m_targetFactorId(targetFactorId) {}

size_t PhrasePairFeature::GetNumScoreComponents() const 
{
  return ScoreProducer::unlimited;
}

string PhrasePairFeature::GetScoreProducerWeightShortName() const
{
  return "pp";
}

size_t PhrasePairFeature::GetNumInputScores() const 
{
  return 0;
}

void PhrasePairFeature::Evaluate(
  const TargetPhrase& cur_hypo,
  ScoreComponentCollection* accumulator) const {
   const Phrase* source = cur_hypo.GetSourcePhrase();
   const AlignmentInfo& align = cur_hypo.GetAlignmentInfo();
  for (AlignmentInfo::const_iterator i = align.begin(); i != align.end(); ++i) {
    const Factor* sourceFactor = source->GetWord(i->first).GetFactor(m_sourceFactorId);
    const Factor* targetFactor = cur_hypo.GetWord(i->second).GetFactor(m_targetFactorId);
    ostringstream namestr;
    namestr << sourceFactor->GetString();
    namestr << ":";
    namestr << targetFactor->GetString();
    accumulator->PlusEquals(this,namestr.str(),1);
  }
}

bool PhrasePairFeature::ComputeValueInTranslationOption() const {
  return false;
} 


}
