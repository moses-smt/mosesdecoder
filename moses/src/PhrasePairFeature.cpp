#include "AlignmentInfo.h"
#include "PhrasePairFeature.h"
#include "TargetPhrase.h"


using namespace std;

namespace Moses {


PhrasePairFeature::PhrasePairFeature
  (FactorType sourceFactorId, FactorType targetFactorId) :
  StatelessFeatureFunction("pp", ScoreProducer::unlimited),
  m_sourceFactorId(sourceFactorId),
  m_targetFactorId(targetFactorId) {}

string PhrasePairFeature::GetScoreProducerWeightShortName(unsigned) const
{
  return "pp";
}

size_t PhrasePairFeature::GetNumInputScores() const 
{
  return 0;
}

void PhrasePairFeature::Evaluate(const TargetPhrase& target, ScoreComponentCollection* accumulator) const {
   const Phrase& source = target.GetSourcePhrase();
/*   const AlignmentInfo& align = cur_hypo.GetAlignmentInfo();
   for (AlignmentInfo::const_iterator i = align.begin(); i != align.end(); ++i) {
    const Factor* sourceFactor = source.GetWord(i->first).GetFactor(m_sourceFactorId);
    const Factor* targetFactor = cur_hypo.GetWord(i->second).GetFactor(m_targetFactorId);
    ostringstream namestr;
    namestr << sourceFactor->GetString();
    namestr << ":";
    namestr << targetFactor->GetString();
    accumulator->PlusEquals(this,namestr.str(),1);
  }*/
   ostringstream namestr;
   namestr << source.GetWord(0).GetFactor(m_sourceFactorId)->GetString();
   for (size_t i = 1; i < source.GetSize(); ++i) {
  	 const Factor* sourceFactor = source.GetWord(i).GetFactor(m_sourceFactorId);
  	 namestr << ",";
  	 namestr << sourceFactor->GetString();
   }
   namestr << "~";
   namestr << target.GetWord(0).GetFactor(m_targetFactorId)->GetString();
   for (size_t i = 1; i < target.GetSize(); ++i) {
  	 const Factor* targetFactor = target.GetWord(i).GetFactor(m_targetFactorId);
  	 namestr << ",";
  	 namestr << targetFactor->GetString();
   }
   accumulator->PlusEquals(this,namestr.str(),1);
}

bool PhrasePairFeature::ComputeValueInTranslationOption() const {
  return false;
} 


}
