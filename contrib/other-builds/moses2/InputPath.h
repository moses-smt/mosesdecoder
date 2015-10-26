/*
 * InputPath.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#ifndef INPUTPATH_H_
#define INPUTPATH_H_
#include <vector>
#include "Phrase.h"
#include "moses/Range.h"

class TargetPhrases;

class InputPath {
public:
	InputPath(const SubPhrase &subPhrase, const Moses::Range &range, size_t numPt);
	virtual ~InputPath();

	const SubPhrase &GetSubPhrase() const
	{ return m_subPhrase; }

	const Moses::Range &GetRange() const
	{ return m_range; }

	const std::vector<const TargetPhrases*> &GetTargetPhrases() const
	{ return m_targetPhrases; }

	void AddTargetPhrases(const PhraseTable &pt, const TargetPhrases *pts);

protected:
	SubPhrase m_subPhrase;
	Moses::Range m_range;
	std::vector<const TargetPhrases*> m_targetPhrases;
};

#endif /* INPUTPATH_H_ */
