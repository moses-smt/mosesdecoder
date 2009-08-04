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
#include <vector>
#include <db_cxx.h>

namespace MosesBerkeleyPt
{

typedef int VocabId;

class Vocab
{
protected:	
	typedef std::map<std::string, VocabId> CollType;
	CollType m_vocabColl;

	std::vector<std::string> m_lookup; // opposite of m_vocabColl

	VocabId m_nextId; // starts @ 1
	
public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	//! iterators
	const_iterator begin() const { return m_vocabColl.begin(); }
	const_iterator end() const { return m_vocabColl.end(); }
			
	Vocab()
	:m_nextId(1)
	{}
	Vocab(const Vocab &copy); // not implemented

	void Load(Db &db);

	size_t GetSize() const
	{ return m_vocabColl.size(); }
	
	VocabId GetVocabId(const std::string &factorString, bool &found) const;
	VocabId AddVocabId(const std::string &factorString);
	
	const std::string &GetString(VocabId vocabId) const
	{ return m_lookup[vocabId]; }

	void Save(const std::string &filePath);
	
};
}; // namespace
