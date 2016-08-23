
#include "HypothesisStack.h"

namespace Moses
{
HypothesisStack::~HypothesisStack()
{
  // delete all hypos
  while (m_hypos.begin() != m_hypos.end()) {
    Remove(m_hypos.begin());
  }
}

/** Remove hypothesis pointed to by iterator but don't delete the object. */
void HypothesisStack::Detach(const HypothesisStack::iterator &iter)
{
  m_hypos.erase(iter);
}


void HypothesisStack::Remove(const HypothesisStack::iterator &iter)
{
  Hypothesis *h = *iter;
  Detach(iter);
  delete h;
}


}

