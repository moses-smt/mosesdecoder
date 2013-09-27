#include "ChartTranslationOptions.h"
#include "InputType.h"
#include "InputPath.h"

namespace Moses
{
ChartTranslationOption::ChartTranslationOption(const TargetPhrase &targetPhrase)
  :m_targetPhrase(targetPhrase)
  ,m_scoreBreakdown(targetPhrase.GetScoreBreakdown())
{
}

void ChartTranslationOption::Evaluate(const InputType &input, const InputPath &inputPath)
{
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();

  for (size_t i = 0; i < ffs.size(); ++i) {
    const FeatureFunction &ff = *ffs[i];
    ff.Evaluate(input, inputPath, m_targetPhrase, m_scoreBreakdown);
  }
}


std::ostream& operator<<(std::ostream &out, const ChartTranslationOption &transOpt)
{
  out << transOpt.m_targetPhrase << " " << transOpt.m_scoreBreakdown;
  return out;
}

}

