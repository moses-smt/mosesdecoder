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
#include "moses/Range.h"
;
class PhraseTable;

class InputPath {
	  friend std::ostream& operator<<(std::ostream &, const InputPath &);
public:
	InputPath(const SubPhrase &subPhrase, const Moses::Range &range, size_t numPt);
	virtual ~InputPath();

	const SubPhrase &GetSubPhrase() const
	{ return m_subPhrase; }

	const Moses::Range &GetRange() const
	{ return m_range; }

	const std::vector<TargetPhrases::shared_const_ptr> &GetTargetPhrases() const
	{ return m_targetPhrases; }

	void AddTargetPhrases(const PhraseTable &pt, TargetPhrases::shared_const_ptr tps);

protected:
	SubPhrase m_subPhrase;
	Moses::Range m_range;
	std::vector<TargetPhrases::shared_const_ptr> m_targetPhrases;
};

#endif /* INPUTPATH_H_ */
