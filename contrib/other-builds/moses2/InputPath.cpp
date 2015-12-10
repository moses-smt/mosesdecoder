/*
 * InputPath.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "InputPath.h"
#include "TranslationModel/PhraseTable.h"

namespace Moses2
{

InputPath::InputPath(const SubPhrase &subPhrase,
		const Range &range,
		size_t numPt,
		const InputPath *prefixPath)
:subPhrase(subPhrase)
,range(range)
,targetPhrases(numPt, NULL)
,prefixPath(prefixPath)
,m_isUsed(false)
{

}

InputPath::~InputPath() {
	// TODO Auto-generated destructor stub
}

void InputPath::AddTargetPhrases(const PhraseTable &pt, const TargetPhrases *tps)
{
	size_t ptInd = pt.GetPtInd();
	targetPhrases[ptInd] = tps;

	if (tps && tps->GetSize()) {
		m_isUsed = true;
	}
}
/*
bool InputPath::IsUsed() const
{
  BOOST_FOREACH(const TargetPhrases::shared_const_ptr &sharedPtr, targetPhrases) {
	  const TargetPhrases *tps = sharedPtr.get();
	  if (tps && tps->GetSize()) {
		  return true;
	  }
  }
  return false;
}
*/
std::ostream& operator<<(std::ostream &out, const InputPath &obj)
{
	out << obj.range << " " << obj.subPhrase;
	return out;
}

}

