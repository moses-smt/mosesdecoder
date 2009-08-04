/*
 *  TargetPhrase.cpp
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 31/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <vector>
#include <db_cxx.h>
#include "../../moses/src/Util.h"
#include "TargetPhrase.h"

using namespace std;

namespace MosesBerkeleyPt
{

void TargetPhrase::CreateAlignFromString(const std::string &alignString)
{
	vector<string> alignTok = Moses::Tokenize(alignString);
	
	vector<string>::const_iterator iter;
	for (iter = alignTok.begin(); iter != alignTok.end(); ++iter)
	{
		const string &align1Str = *iter;
		vector<size_t> alignPoints = Moses::Tokenize<size_t>(align1Str, "-");
		assert(alignPoints.size() == 2);
		m_align.push_back(pair<size_t, size_t>(alignPoints[0], alignPoints[1]) );
	}
}

void TargetPhrase::CreateScoresFromString(const std::string &inString)
{
	m_scores = Moses::Tokenize<float>(inString);
}

void TargetPhrase::CreateHeadwordsFromString(const std::string &inString, Vocab &vocab)
{
	std::vector<string> headWordsStr = Moses::Tokenize(inString);
	assert(headWordsStr.size());
	
	m_headWords.push_back(Word());
	m_headWords.push_back(Word());
	
	m_headWords[0].CreateFromString(headWordsStr[0], vocab);
	m_headWords[1].CreateFromString(headWordsStr[1], vocab);
}

size_t TargetPhrase::GetAlign(size_t sourcePos) const
{
	AlignType::const_iterator iter;
	for (iter = m_align.begin(); iter != m_align.end(); ++iter)
	{
		const AlignPair &pair = *iter;
		if (pair.first == sourcePos)
			return pair.second;
	}
	
	assert(false);
	return 0;
}

char *TargetPhrase::WritePhraseToMemory(size_t &memUsed, int numScores, size_t sourceWordSize, size_t targetWordSize) const
{
	// allocate mem	
	size_t memNeeded = sizeof(int) +  targetWordSize * GetSize(); // phrase
	
	char *mem = (char*) malloc(memNeeded);
	//memset(mem, 0, memNeeded);
	
	memUsed = 0;
	
	// phrase
	/// size
	int phraseSize = GetSize();
	memcpy(mem + memUsed, &phraseSize, sizeof(int));
	memUsed += sizeof(int);
	
	// word
	for (size_t pos = 0; pos < GetSize(); ++pos)
	{
		const Word &word = GetWord(pos);
		memUsed += word.WriteToMemory(mem + memUsed);
	}
	
	//DebugMem(mem, memNeeded);
	assert(memNeeded == memUsed);	
	return mem;
}

char *TargetPhrase::WriteOtherInfoToMemory(size_t &memUsed, int numScores, size_t sourceWordSize, size_t targetWordSize) const
{
	// allocate mem	
	size_t memNeeded = sizeof(long); // phrase id
	memNeeded += sourceWordSize + targetWordSize; // LHS words
	memNeeded += sizeof(int) + 2 * sizeof(int) * GetAlign().size(); // align
	memNeeded += sizeof(float) * numScores; // scores
	
	char *mem = (char*) malloc(memNeeded);
	//memset(mem, 0, memNeeded);
	
	memUsed = 0;
	
	// phrase id
	memcpy(mem, &m_targetId, sizeof(long));
	memUsed += sizeof(long);

	// LHS words
	memUsed += GetHeadWords(0).WriteToMemory(mem + memUsed);
	memUsed += GetHeadWords(1).WriteToMemory(mem + memUsed);
	
	// align
	memUsed += WriteAlignToMemory(mem + memUsed);
	
	// scores
	memUsed += WriteScoresToMemory(mem + memUsed);
	
	//DebugMem(mem, memNeeded);
	assert(memNeeded == memUsed);	
	return mem;
}

size_t TargetPhrase::WriteAlignToMemory(char *mem) const
{
	size_t memUsed = 0;
	
	// num of alignments
	int numAlign = m_align.size();
	memcpy(mem, &numAlign, sizeof(int));
	memUsed += sizeof(int);
	
	// actual alignments
	AlignType::const_iterator iter;
	for (iter = m_align.begin(); iter != m_align.end(); ++iter)
	{
		const AlignPair &alignPair = *iter;
		
		memcpy(mem + memUsed, &alignPair.first, sizeof(int));
		memUsed += sizeof(int);
		
		memcpy(mem + memUsed, &alignPair.second, sizeof(int));
		memUsed += sizeof(int);
	}
	
	return memUsed;
}

size_t TargetPhrase::WriteScoresToMemory(char *mem) const
{	
	float *scoreMem = (float*) mem;
	
	for (size_t ind = 0; ind < m_scores.size(); ++ind)
		scoreMem[ind] = m_scores[ind];
	
	size_t memUsed = sizeof(float) * m_scores.size();
	return memUsed;
}

long TargetPhrase::SaveTargetPhrase(Db &dbTarget, long &nextTargetId
																		, int numScores, size_t sourceWordSize, size_t targetWordSize)
{
	size_t memUsed;
	char *mem = WritePhraseToMemory(memUsed, numScores, sourceWordSize, targetWordSize);
	
	Dbt key(mem, memUsed);
	
	// get
	Dbt data;
	int ret = dbTarget.get(NULL, &key, &data, 0);
	if (ret == 0)
	{ // existing target
		m_targetId = *(long*) data.get_data();
		//cerr << "target phrase exists " << targetId << endl;
	}
	else
	{ // new target. save
		data.set_data(&nextTargetId);
		data.set_size(sizeof(long));
		
		int ret = dbTarget.put(NULL, &key, &data, DB_NOOVERWRITE);
		assert(ret == 0);
		
		m_targetId = nextTargetId;
		++nextTargetId;
		//cerr << "new target phrase " << targetId << endl;
	}
	
	free(mem);
	
	return m_targetId;
}

size_t TargetPhrase::ReadPhraseFromMemory(const char *mem, size_t numFactors)
{
	size_t memUsed = 0;
	
	// size
	int phraseSize;
	memcpy(&phraseSize, mem + memUsed, sizeof(int));
	memUsed += sizeof(int);

	Resize(phraseSize);

	// words
	for (size_t pos = 0; pos < GetSize(); ++pos)
	{
		Word &word = GetWord(pos);
		memUsed += word.ReadFromMemory(mem + memUsed, numFactors);
	}

	return memUsed;
}

size_t TargetPhrase::ReadOtherInfoFromMemory(const char *mem
																						 , size_t numSourceFactors, size_t numTargetFactors
																						 , size_t numScores)
{
	// allocate mem	
	size_t memUsed = 0;
	
	// phrase id
	memcpy(&m_targetId, mem, sizeof(long));
	memUsed += sizeof(long);

	// LHS words
	memUsed += m_headWords[0].ReadFromMemory(mem + memUsed, numSourceFactors);
	memUsed += m_headWords[1].ReadFromMemory(mem + memUsed, numTargetFactors);
	
	// align
	memUsed += ReadAlignFromMemory(mem + memUsed);
	
	// scores
	memUsed += ReadScoresFromMemory(mem + memUsed, numScores);
	
	return memUsed;
}


size_t TargetPhrase::ReadAlignFromMemory(const char *mem)
{
	size_t memUsed = 0;
	
	// num of alignments
	int numAlign;
	memcpy(&numAlign, mem, sizeof(int));
	memUsed += sizeof(int);
	
	// actual alignments
	for (size_t ind = 0; ind < numAlign; ++ind)
	{
		int sourcePos, targetPos;
		
		memcpy(&sourcePos, mem + memUsed, sizeof(int));
		memUsed += sizeof(int);
		
		memcpy(&targetPos, mem + memUsed, sizeof(int));
		memUsed += sizeof(int);

		AlignPair alignPair(sourcePos, targetPos);
		m_align.push_back(alignPair);
	}
	
	return memUsed;
}

size_t TargetPhrase::ReadScoresFromMemory(const char *mem, size_t numScores)
{
	m_scores.resize(numScores);

	float *scoreMem = (float*) mem;
	
	for (size_t ind = 0; ind < numScores; ++ind)
		m_scores[ind] = scoreMem[ind];

	size_t memUsed = sizeof(float) * m_scores.size();
	return memUsed;
}

void TargetPhrase::Load(const Db &db, size_t numTargetFactors)
{
	// create db data	
	Dbt key(&m_targetId, sizeof(long));
	Dbt data;
	
	// save
	int dbRet = const_cast<Db&>(db).get(NULL, &key, &data, 0);
	assert(dbRet == 0);

	size_t memUsed = ReadPhraseFromMemory((char*) data.get_data(), numTargetFactors);
	assert(memUsed == data.get_size());
}

}; // namespace

