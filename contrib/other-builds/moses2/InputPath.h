/*
 * InputPath.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#ifndef INPUTPATH_H_
#define INPUTPATH_H_

#include "Phrase.h"
#include "moses/WordsRange.h"

class InputPath {
public:
	InputPath(const SubPhrase &subPhrase, const Moses::WordsRange &range);
	virtual ~InputPath();

protected:
	SubPhrase m_subPhrase;
	Moses::WordsRange m_range;
};

#endif /* INPUTPATH_H_ */
