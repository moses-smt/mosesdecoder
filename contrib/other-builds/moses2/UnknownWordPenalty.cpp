/*
 * UnknownWordPenalty.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include <contrib/other-builds/moses2/UnknownWordPenalty.h>

UnknownWordPenalty::UnknownWordPenalty(size_t startInd, const std::string &line)
:PhraseTable(startInd, line)
{
	// TODO Auto-generated constructor stub

}

UnknownWordPenalty::~UnknownWordPenalty() {
	// TODO Auto-generated destructor stub
}

const TargetPhrases *UnknownWordPenalty::Lookup(InputPath &inputPath) const
{
	return NULL;
}
