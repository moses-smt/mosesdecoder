/*
 * InputPath.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "InputPath.h"

InputPath::InputPath(const SubPhrase &subPhrase, const Moses::WordsRange &range, size_t numPt)
:m_subPhrase(subPhrase)
,m_range(range)
,m_targetPhrases(numPt)
{

}

InputPath::~InputPath() {
	// TODO Auto-generated destructor stub
}

