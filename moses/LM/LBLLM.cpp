#include <vector>
#include <boost/archive/text_iarchive.hpp>
#include "LBLLM.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"

using namespace std;

namespace Moses
{
int LBLLMState::Compare(const FFState& other) const
{
  const LBLLMState &otherState = static_cast<const LBLLMState&>(other);

  if (m_targetLen == otherState.m_targetLen)
    return 0;
  return (m_targetLen < otherState.m_targetLen) ? -1 : +1;
}

////////////////////////////////////////////////////////////////


}

