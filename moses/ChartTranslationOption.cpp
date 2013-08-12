#include "ChartTranslationOptions.h"

namespace Moses
{
ChartTranslationOption::ChartTranslationOption(const TargetPhrase &targetPhrase)
  :m_targetPhrase(targetPhrase)
  ,m_scoreBreakdown(targetPhrase.GetScoreBreakdown())
{
}

}

