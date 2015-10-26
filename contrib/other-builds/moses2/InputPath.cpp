/*
 * InputPath.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "InputPath.h"
#include "PhraseTable.h"

InputPath::InputPath(const SubPhrase &subPhrase, const Moses::Range &range, size_t numPt)
:m_subPhrase(subPhrase)
,m_range(range)
,m_targetPhrases(numPt)
{

}

InputPath::~InputPath() {
	// TODO Auto-generated destructor stub
}

void InputPath::AddTargetPhrases(const PhraseTable &pt, const TargetPhrases *tps)
{
	size_t ptInd = pt.GetPtInd();
	m_targetPhrases[ptInd] = tps;
}

std::ostream& operator<<(std::ostream &out, const InputPath &obj)
{
	out << obj.m_range << " " << obj.m_subPhrase;
	return out;
}
