/*
 * InputPath.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "InputPath.h"
#include "TranslationModel/PhraseTable.h"

InputPath::InputPath(const SubPhrase &subPhrase,
		const Moses::Range &range,
		size_t numPt,
		const InputPath *prefixPath)
:subPhrase(subPhrase)
,range(range)
,targetPhrases(numPt)
,prefixPath(prefixPath)
{

}

InputPath::~InputPath() {
	// TODO Auto-generated destructor stub
}

void InputPath::AddTargetPhrases(const PhraseTable &pt, TargetPhrases::shared_const_ptr tps)
{
	size_t ptInd = pt.GetPtInd();
	targetPhrases[ptInd] = tps;
}

bool InputPath::IsUsed() const
{
  BOOST_FOREACH(const TargetPhrases::shared_const_ptr &sharedPtr, targetPhrases) {
	  const TargetPhrases *tps = sharedPtr.get();
	  if (tps && tps->GetSize()) {
		  return false;
	  }
  }
  return true;
}

std::ostream& operator<<(std::ostream &out, const InputPath &obj)
{
	out << obj.range << " " << obj.subPhrase;
	return out;
}
