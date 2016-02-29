/*
 * InputPath.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <iostream>
#include <vector>
#include "../InputPathBase.h"

namespace Moses2
{

class InputPath : public InputPathBase
{
	  friend std::ostream& operator<<(std::ostream &, const InputPath &);
public:
	const TargetPhrases** targetPhrases;

	InputPath(MemPool &pool, const SubPhrase &subPhrase, const Range &range, size_t numPt, const InputPath *prefixPath);
	virtual ~InputPath();

	void AddTargetPhrases(const PhraseTable &pt, const TargetPhrases *tps);
	const TargetPhrases *GetTargetPhrases(const PhraseTable &pt) const;

	inline bool IsUsed() const
	{ return m_isUsed; }

protected:
	bool m_isUsed;
};

}

