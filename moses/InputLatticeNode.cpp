#include "InputLatticeNode.h"

namespace Moses
{
const TargetPhraseCollection *InputLatticeNode::GetTargetPhrases(const PhraseDictionary &phraseDictionary) const
{
  std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, void*> >::const_iterator iter;
  iter = m_targetPhrases.find(&phraseDictionary);
  CHECK(iter != m_targetPhrases.end());
  return iter->second.first;
}

}
