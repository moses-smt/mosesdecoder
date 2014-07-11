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

  if (m_left != otherState.m_left) {
	  return (m_left < otherState.m_left) ? -1 : +1;
  }
  else if (m_right != otherState.m_right) {
	  return (m_right < otherState.m_right) ? -1 : +1;
  }
  return 0;
}

////////////////////////////////////////////////////////////////


}

