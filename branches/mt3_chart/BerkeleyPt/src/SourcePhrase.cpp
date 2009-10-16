/*
 *  SourcePhrase.cpp
 *  BerkeleyPt
 *
 *  Created by Hieu Hoang on 11/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "SourcePhrase.h"
#include "DbWrapper.h"

using namespace std;

namespace MosesBerkeleyPt
{

SourcePhrase::SourcePhrase()
	:m_targetNonTerms(0)
{}

SourcePhrase::SourcePhrase(const 	SourcePhrase &copy)
	:Phrase(copy)
	,m_targetNonTerms(copy.m_targetNonTerms)
{}

SourcePhrase::~SourcePhrase()
{}

size_t SourcePhrase::GetNumNonTerminals() const
{ return m_targetNonTerms.size(); }

Moses::UINT32 SourcePhrase::Save(Db &db, Moses::UINT32 &nextSourceId, size_t sourceWordSize) const
{
	Moses::UINT32 currSourceNodeId = 0;
	size_t targetInd = 0;
	
	// SOURCE
	for (size_t pos = 0; pos < GetSize(); ++pos)
	{
		const Word &word = GetWord(pos);
		currSourceNodeId = SaveWord(currSourceNodeId, word, db, nextSourceId, sourceWordSize);
		
		if (word.IsNonTerminal())
		{ // store the TARGET non-term label straight after source non-term label
			const Word &targetWord = m_targetNonTerms[targetInd];
			targetInd++;
			
			currSourceNodeId = SaveWord(currSourceNodeId, targetWord, db, nextSourceId, sourceWordSize);
		}
	}
	
	return currSourceNodeId;	
}

Moses::UINT32 SourcePhrase::SaveWord(Moses::UINT32 currSourceNodeId, const Word &word, Db &db, Moses::UINT32 &nextSourceId, size_t sourceWordSize) const
{
	Moses::UINT32 retSourceNodeId;
	
	// create ket data - source node id + word 
	size_t memAlloc = sizeof(currSourceNodeId) + sourceWordSize;
	Moses::UINT32 *mem = (Moses::UINT32*) malloc(memAlloc); 
	mem[0] = currSourceNodeId;
	size_t memUsed = word.WriteToMemory((char*) &mem[1]);
	
	memUsed += sizeof(currSourceNodeId);
	assert(memUsed == memAlloc);
	
	Dbt key(mem, memAlloc);
	Dbt data(&nextSourceId, sizeof(Moses::UINT32));
	
	// save
	int ret = db.put(NULL, &key, &data, DB_NOOVERWRITE);
	if (ret == DB_KEYEXIST) 
	{ // already exist. get node id
		db.get(NULL, &key, &data, 0);
		
		assert(data.get_size() == sizeof(Moses::UINT32));
		Moses::UINT32 *sourceId = (Moses::UINT32*) data.get_data();
		
		retSourceNodeId = *sourceId;
	}
	else
	{
		retSourceNodeId = nextSourceId;
		++nextSourceId;
	}

	free(mem);
	return retSourceNodeId;
}

	
void SourcePhrase::SaveTargetNonTerminals(const TargetPhrase &targetPhrase)
{
	// SOURCE
	for (size_t pos = 0; pos < GetSize(); ++pos)
	{
		const Word &word = GetWord(pos);
		
		if (word.IsNonTerminal())
		{ // store the TARGET non-term label in vector
			size_t targetPos = targetPhrase.GetAlign(pos);
			const Word &targetWord = targetPhrase.GetWord(targetPos);
			m_targetNonTerms.push_back(targetWord);
		}
	}	
}

//! transitive comparison
bool SourcePhrase::operator<(const SourcePhrase &compare) const
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
	
} // namespace


