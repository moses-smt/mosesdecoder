#include "HypothesisStackCubePruningPipelined.h"

using namespace std;

namespace Moses
{
  HypothesisStackCubePruningPipelined::Search& HypothesisStackCubePruningPipelined::ExtractSearch() 
  {
    LanguageModelKen<lm::ngram::ProbingModel>* lm = NULL;
    const std::vector<const StatefulFeatureFunction*>& sff = StatefulFeatureFunction::GetStatefulFeatureFunctions();
    for (size_t i = 0; i < sff.size(); ++i) {
      if(sff[i]->GetScoreProducerDescription() == "LM") {
        std::cout << "Found it\n";
        lm = const_cast<LanguageModelKen<lm::ngram::ProbingModel>* >(static_cast<const LanguageModelKen<lm::ngram::ProbingModel>* >(sff[i]));
        break;
      }
    }
    return lm->GetModel()->GetSearch();
  }

  bool HypothesisStackCubePruningPipelined::AddPrune(Hypothesis* hypo)
  {
    cout << "Adding a hypo to pipeline\n";
    return true;
    //return HypothesisStackCubePruning::AddPrune(hypo);
  }
}
