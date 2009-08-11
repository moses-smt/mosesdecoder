#pragma once
/*
 *  SourcePhrase.h
 *  BerkeleyPt
 *
 *  Created by Hieu Hoang on 11/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <vector>
#include "Phrase.h"
#include "Word.h"

namespace MosesBerkeleyPt
{
	
class SourcePhrase : public Phrase
{
protected:
	std::vector<Word> m_targetNonTerms;

public:
	
	//! transitive comparison
	inline bool operator<(const SourcePhrase &compare) const
	{
		bool ret = m_targetNonTerms < compare.m_targetNonTerms;
		if (ret)
			return true;
		
		ret = m_targetNonTerms > compare.m_targetNonTerms;
		if (ret)
			return false;
		
		// equal, test the underlying source words
		return m_words < compare.m_words;
	}
	
};

}

