#pragma once

/*
 *  Vocab.h
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <map>
#include <string>

namespace MosesBerkeleyPt
{

typedef int VocabId;

class Vocab
{
protected:
	static Vocab s_instance;
	
	typedef std::map<std::string, VocabId> CollType;
	CollType m_vocabColl;
	VocabId m_nextId; // starts @ 1
	
public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	//! iterators
	const_iterator begin() const { return m_vocabColl.begin(); }
	const_iterator end() const { return m_vocabColl.end(); }
	
	static Vocab &Instance()
	{ return s_instance; }
		
	Vocab();
	VocabId AddFactor(const std::string &factorString);
	
	void Save(const std::string &filePath);
	
};
}; // namespace
