/*
 *  Phrase.cpp
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "/usr/local/BerkeleyDB.4.7/include/db_cxx.h"
#include "../../moses/src/Util.h"
#include "Phrase.h"
#include "Global.h"

using namespace std;

void Phrase::CreateFromString(const std::string &phraseString)
{
	std::vector<std::string> wordsVec = Moses::Tokenize(phraseString);

	vector<string>::const_iterator iter;
	for (iter = wordsVec.begin(); iter != wordsVec.end(); ++iter)
	{
		const string &wordStr = *iter;
		Word word;
		word.CreateFromString(wordStr);
		m_words.push_back(word);
	}
}

void Phrase::CreateAlignFromString(const std::string &alignString)
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

void Phrase::CreateScoresFromString(const std::string &inString)
{
	m_scores = Moses::Tokenize<float>(inString);
}

void Phrase::CreateHeadwordsFromString(const std::string &inString)
{
	std::vector<string> headWordsStr = Moses::Tokenize(inString);
	assert(headWordsStr.size());
	
	m_headWords.push_back(Word());
	m_headWords.push_back(Word());
	
	m_headWords[0].CreateFromString(headWordsStr[0]);
	m_headWords[1].CreateFromString(headWordsStr[1]);
}

size_t Phrase::GetAlign(size_t sourcePos) const
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

size_t Phrase::WriteAlignToMemory(char *mem) const
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

size_t Phrase::WriteScoresToMemory(char *mem) const
{	
	float *scoreMem = (float*) mem;
	
	for (size_t ind = 0; ind < m_scores.size(); ++ind)
		scoreMem[ind] = m_scores[ind];
	
	size_t memUsed = sizeof(float) * m_scores.size();
	return memUsed;
}

long Phrase::SaveTargetPhrase(Db &dbTarget, long &nextTargetId) const
{
	long targetId;
	size_t memUsed;
	char *mem = WriteToMemory(memUsed);
		
	Dbt key(mem, memUsed);
	
	// get
	Dbt data;
	int ret = dbTarget.get(NULL, &key, &data, 0);
	if (ret == 0)
	{ // existing target
		targetId = *(long*) data.get_data();
		//cerr << "target phrase exists " << targetId << endl;
	}
	else
	{ // new target. save
		data.set_data(&nextTargetId);
		data.set_size(sizeof(long));

		int ret = dbTarget.put(NULL, &key, &data, DB_NOOVERWRITE);
		assert(ret == 0);
		
		targetId = nextTargetId;
		++nextTargetId;
		//cerr << "new target phrase " << targetId << endl;
	}
	
	free(mem);
	
	return targetId;
}

void DebugMem(char *mem, size_t size)
{
	for (size_t i =0; i < size; i++)
		printf("%x", mem[i]);
	printf("\n");
	
}

char *Phrase::WriteToMemory(size_t &memUsed) const
{
	// allocate mem
	const Global &global = Global::Instance();
	
	size_t memNeeded = global.GetSourceWordSize() + global.GetTargetWordSize();
	memNeeded += sizeof(int) +  global.GetTargetWordSize() * GetSize(); // phrase
	memNeeded += sizeof(int) + 2 * sizeof(int) * GetAlign().size(); // align
	memNeeded += sizeof(float) * global.GetNumScores(); // scores
	
	char *mem = (char*) malloc(memNeeded);
	memset(mem, 0, memNeeded);
	
	// head words
	memUsed = 0;
	memUsed += GetHeadWords(0).WriteToMemory(mem);
	memUsed += GetHeadWords(1).WriteToMemory(mem + memUsed);


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
	
	// align
	memUsed += WriteAlignToMemory(mem + memUsed);
	
	// scores
	memUsed += WriteScoresToMemory(mem + memUsed);
	
	DebugMem(mem, memNeeded);
	
	assert(memNeeded == memUsed);	
	return mem;
}




