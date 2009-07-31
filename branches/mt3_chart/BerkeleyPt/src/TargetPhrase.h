/*
 *  TargetPhrase.h
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 31/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Phrase.h"

class TargetPhrase : public Phrase
{
protected:
	typedef std::pair<int, int>  AlignPair;
	typedef std::vector<AlignPair> AlignType;
	AlignType m_align;
	std::vector<float>				m_scores;
	std::vector<Word>	m_headWords;

	char *WriteToMemory(size_t &memUsed) const;
	size_t WriteAlignToMemory(char *mem) const;
	size_t WriteScoresToMemory(char *mem) const;

public:
	void CreateAlignFromString(const std::string &alignString);
	void CreateScoresFromString(const std::string &inString);
	void CreateHeadwordsFromString(const std::string &inString);

	const Word &GetHeadWords(size_t ind) const
	{ return m_headWords[ind]; }
	
	
	const AlignType &GetAlign() const
	{ return m_align; }
	size_t GetAlign(size_t sourcePos) const;
	
	
	long SaveTargetPhrase(Db &dbTarget, long &nextTargetId) const;	

};

