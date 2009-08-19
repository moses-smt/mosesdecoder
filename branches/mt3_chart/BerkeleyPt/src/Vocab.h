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
#include "../../moses/src/TypeDef.h"

namespace Moses
{
	class Factor;
}

namespace MosesBerkeleyPt
{

class Vocab
{
protected:	
	typedef std::map<std::string, Moses::UINT32> CollType;
	CollType m_vocabColl;

	std::vector<std::string> m_lookup; // opposite of m_vocabColl

	Moses::UINT32 m_nextId; // starts @ 1
	
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
	
	Moses::UINT32 GetVocabId(const std::string &factorString, bool &found) const;
	Moses::UINT32 AddVocabId(const std::string &factorString);
	
	const std::string &GetString(Moses::UINT32 vocabId) const
	{ return m_lookup[vocabId]; }
	const Moses::Factor *GetFactor(Moses::UINT32 vocabId, Moses::FactorType factorType, Moses::FactorDirection direction, bool isNonTerminal) const;

	void Save(const std::string &filePath);
	
};

inline void DebugMem(const char *mem, size_t size)
{
	for (size_t i =0; i < size; i++)
		printf("%x ", (const unsigned char) mem[i]);
	printf("\n");
	
}

}; // namespace
