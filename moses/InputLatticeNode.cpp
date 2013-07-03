#include "InputLatticeNode.h"

namespace Moses
{
const TargetPhraseCollection *InputLatticeNode::GetTargetPhrases(const PhraseDictionary &phraseDictionary) const
{
  std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, const void*> >::const_iterator iter;
  iter = m_targetPhrases.find(&phraseDictionary);
  CHECK(iter != m_targetPhrases.end());
  return iter->second.first;
}

const void *InputLatticeNode::GetPtNode(const PhraseDictionary &phraseDictionary) const
{
  std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, const void*> >::const_iterator iter;
  iter = m_targetPhrases.find(&phraseDictionary);
  CHECK(iter != m_targetPhrases.end());
  return iter->second.second;
}

std::ostream& operator<<(std::ostream& out, const InputLatticeNode& obj)
{
	out << &obj << " " << obj.GetWordsRange() << " " << obj.GetPrevNode() << " " << obj.GetPhrase();
	return out;
}

}
