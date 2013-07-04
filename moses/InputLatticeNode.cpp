#include "InputLatticeNode.h"

namespace Moses
{
const TargetPhraseCollection *InputLatticeNode::GetTargetPhrases(const PhraseDictionary &phraseDictionary) const
{
  std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, const void*> >::const_iterator iter;
  iter = m_targetPhrases.find(&phraseDictionary);
  if (iter == m_targetPhrases.end()) {
	  return NULL;
  }
  return iter->second.first;
}

const void *InputLatticeNode::GetPtNode(const PhraseDictionary &phraseDictionary) const
{
  std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, const void*> >::const_iterator iter;
  iter = m_targetPhrases.find(&phraseDictionary);
  if (iter == m_targetPhrases.end()) {
	  return NULL;
  }
  return iter->second.second;
}

std::ostream& operator<<(std::ostream& out, const InputLatticeNode& obj)
{
	out << &obj << " " << obj.GetWordsRange() << " " << obj.GetPrevNode() << " " << obj.GetPhrase();

	out << "pt: ";
	std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, const void*> >::const_iterator iter;
	for (iter = obj.m_targetPhrases.begin(); iter != obj.m_targetPhrases.end(); ++iter) {
		const PhraseDictionary *pt = iter->first;
		out << pt << " ";
	}

	return out;
}

}
