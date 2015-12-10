/*
 * InputPath.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#ifndef INPUTPATH_H_
#define INPUTPATH_H_
#include <iostream>
#include <vector>
#include "Phrase.h"
#include "TargetPhrases.h"
#include "legacy/Range.h"
;
class PhraseTable;

class InputPath {
	  friend std::ostream& operator<<(std::ostream &, const InputPath &);
public:
	const InputPath *prefixPath;
	SubPhrase subPhrase;
	Range range;
	std::vector<const TargetPhrases*> targetPhrases;

	InputPath(const SubPhrase &subPhrase, const Range &range, size_t numPt, const InputPath *prefixPath);
	virtual ~InputPath();

	void AddTargetPhrases(const PhraseTable &pt, const TargetPhrases *tps);

	inline bool IsUsed() const
	{ return m_isUsed; }

protected:
	bool m_isUsed;
};

#endif /* INPUTPATH_H_ */
