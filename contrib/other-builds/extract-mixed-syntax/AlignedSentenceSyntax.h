/*
 * AlignedSentenceSyntax.h
 *
 *  Created on: 26 Feb 2014
 *      Author: hieu
 */

#ifndef ALIGNEDSENTENCESYNTAX_H_
#define ALIGNEDSENTENCESYNTAX_H_

#include "AlignedSentence.h"

class AlignedSentenceSyntax : public AlignedSentence
{
public:
	AlignedSentenceSyntax(const std::string &source,
			const std::string &target,
			const std::string &alignment);
	virtual ~AlignedSentenceSyntax();

	void CreateConsistentPhrases(const Parameter &params);
};

#endif /* ALIGNEDSENTENCESYNTAX_H_ */
