/*
 *  TargetPhrase.cpp
 *  CreateOnDisk
 *
 *  Created by Hieu Hoang on 31/12/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include <algorithm>
#include <iostream>
#include "../../moses/src/Util.h"
#include "../../moses/src/TargetPhrase.h"
#include "../../moses/src/PhraseDictionary.h"
#include "TargetPhrase.h"
#include "OnDiskWrapper.h"

using namespace std;

namespace OnDiskPt
{

TargetPhrase::TargetPhrase(size_t numScores)
:m_scores(numScores)
{
}

TargetPhrase::TargetPhrase(const 	TargetPhrase &copy)
:Phrase(copy)
,m_scores(copy.m_scores)
{
	
}

TargetPhrase::~TargetPhrase()
{
}

void TargetPhrase::SetLHS(Word *lhs)
{
	AddWord(lhs);
}

void TargetPhrase::Create1AlignFromString(const std::string &align1Str)
{
	vector<size_t> alignPoints;
	Moses::Tokenize<size_t>(alignPoints, align1Str, "-");
	assert(alignPoints.size() == 2);
	m_align.push_back(pair<size_t, size_t>(alignPoints[0], alignPoints[1]) );	
}

void TargetPhrase::SetScore(float score, size_t ind)
{
	assert(ind < m_scores.size());
	m_scores[ind] = score;	
}

class AlignOrderer
{
public:	
	bool operator()(const AlignPair &a, const AlignPair &b) const
	{
		return a.first < b.first;
	}
};

void TargetPhrase::SortAlign()
{
	std::sort(m_align.begin(), m_align.end(), AlignOrderer());
}

char *TargetPhrase::WriteToMemory(OnDiskWrapper &onDiskWrapper, size_t &memUsed) const
{
	size_t phraseSize = GetSize();
	size_t targetWordSize = onDiskWrapper.GetTargetWordSize();
	
	size_t memNeeded = sizeof(UINT64)						// num of words
										+ targetWordSize * phraseSize;	// actual words. lhs as last words
	memUsed = 0;
	UINT64 *mem = (UINT64*) malloc(memNeeded); 
	
	// write size
	mem[0] = phraseSize;
	memUsed += sizeof(UINT64);

	// write each word
	for (size_t pos = 0; pos < phraseSize; ++pos)
	{
		const Word &word = GetWord(pos);
		char *currPtr = (char*)mem + memUsed;
		memUsed += word.WriteToMemory((char*) currPtr);
	}
	
	assert(memUsed == memNeeded);
	return (char *) mem;
}

void TargetPhrase::Save(OnDiskWrapper &onDiskWrapper)
{
	// save in target ind
	size_t memUsed;
	char *mem = WriteToMemory(onDiskWrapper, memUsed);

	std::fstream &file = onDiskWrapper.GetFileTargetInd();
	
	UINT64 startPos = file.tellp();
	
	file.seekp(0, ios::end);
	file.write(mem, memUsed);
	
	UINT64 endPos = file.tellp();
	assert(startPos + memUsed == endPos);
	
	m_filePos = startPos;
	free(mem);
}

char *TargetPhrase::WriteOtherInfoToMemory(OnDiskWrapper &onDiskWrapper, size_t &memUsed) const
{
	// allocate mem	
	size_t numScores = onDiskWrapper.GetNumScores()
				,numAlign = GetAlign().size();
	
	size_t memNeeded = sizeof(UINT64); // file pos (phrase id)
	memNeeded += sizeof(UINT64) + 2 * sizeof(UINT64) * numAlign; // align
	memNeeded += sizeof(float) * numScores; // scores
	
	char *mem = (char*) malloc(memNeeded);
	//memset(mem, 0, memNeeded);
	
	memUsed = 0;
	
	// phrase id
	memcpy(mem, &m_filePos, sizeof(UINT64));
	memUsed += sizeof(UINT64);
		
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
	UINT64 numAlign = m_align.size();
	memcpy(mem, &numAlign, sizeof(numAlign));
	memUsed += sizeof(numAlign);
	
	// actual alignments
	AlignType::const_iterator iter;
	for (iter = m_align.begin(); iter != m_align.end(); ++iter)
	{
		const AlignPair &alignPair = *iter;
		
		memcpy(mem + memUsed, &alignPair.first, sizeof(alignPair.first));
		memUsed += sizeof(alignPair.first);
		
		memcpy(mem + memUsed, &alignPair.second, sizeof(alignPair.second));
		memUsed += sizeof(alignPair.second);
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


Moses::TargetPhrase *TargetPhrase::ConvertToMoses(const std::vector<Moses::FactorType> &inputFactors
																		, const std::vector<Moses::FactorType> &outputFactors
																		, const Vocab &vocab
																		, const Moses::PhraseDictionary &phraseDict
																		, const std::vector<float> &weightT
																		, float weightWP
																		, const Moses::LMList &lmList
																		, const Moses::Phrase &sourcePhrase) const
{
	Moses::TargetPhrase *ret = new Moses::TargetPhrase(Moses::Output);
	
	// source phrase
	ret->SetSourcePhrase(&sourcePhrase);
	
	// words
	size_t phraseSize = GetSize();
	assert(phraseSize > 0); // last word is lhs
	--phraseSize;
	
	for (size_t pos = 0; pos < phraseSize; ++pos)
	{
		Moses::Word *mosesWord = GetWord(pos).ConvertToMoses(Moses::Output, outputFactors, vocab);
		ret->AddWord(*mosesWord);
		delete mosesWord;
	}
	
	// scores
	ret->SetScoreChart(phraseDict.GetFeature(), m_scores, weightT, lmList);
	
	// alignments
	std::list<std::pair<size_t, size_t> > alignmentInfo;
	for (size_t ind = 0; ind < m_align.size(); ++ind)
	{
		const std::pair<size_t, size_t> &entry = m_align[ind];
		alignmentInfo.push_back(entry);
	}
	ret->SetAlignmentInfo(alignmentInfo);
		
	Moses::Word *lhs = GetWord(GetSize() - 1).ConvertToMoses(Moses::Output, outputFactors, vocab);
	ret->SetTargetLHS(*lhs);
	delete lhs;
	
	return ret;		
}

UINT64 TargetPhrase::ReadOtherInfoFromFile(UINT64 filePos, std::fstream &fileTPColl)
{
	assert(filePos == fileTPColl.tellg());
	
	UINT64 memUsed = 0;
	fileTPColl.read((char*) &m_filePos, sizeof(UINT64));
	memUsed += sizeof(UINT64);
	assert(m_filePos != 0);
	
	memUsed += ReadAlignFromFile(fileTPColl);
	assert((memUsed + filePos) == fileTPColl.tellg());
	
	memUsed += ReadScoresFromFile(fileTPColl);
	assert((memUsed + filePos) == fileTPColl.tellg());

	return memUsed;
}
	
UINT64 TargetPhrase::ReadFromFile(std::fstream &fileTP, size_t numFactors)
{
	UINT64 bytesRead = 0;

	fileTP.seekg(m_filePos);

	UINT64 numWords;
	fileTP.read((char*) &numWords, sizeof(UINT64));
	bytesRead += sizeof(UINT64);
	
	for (size_t ind = 0; ind < numWords; ++ind)
	{
		Word *word = new Word();
		bytesRead += word->ReadFromFile(fileTP, numFactors);
		AddWord(word);
	}
	
	return bytesRead;
}

UINT64 TargetPhrase::ReadAlignFromFile(std::fstream &fileTPColl)
{
	UINT64 bytesRead = 0;
	
	UINT64 numAlign;
	fileTPColl.read((char*) &numAlign, sizeof(UINT64));
	bytesRead += sizeof(UINT64);
	
	for (size_t ind = 0; ind < numAlign; ++ind)
	{
		AlignPair alignPair;
		fileTPColl.read((char*) &alignPair.first, sizeof(UINT64));
		fileTPColl.read((char*) &alignPair.second, sizeof(UINT64));
		m_align.push_back(alignPair);
		
		bytesRead += sizeof(UINT64) * 2;
	}
	
	return bytesRead;
}

UINT64 TargetPhrase::ReadScoresFromFile(std::fstream &fileTPColl)
{
	assert(m_scores.size() > 0);
	
	UINT64 bytesRead = 0;
	
	for (size_t ind = 0; ind < m_scores.size(); ++ind)
	{
		fileTPColl.read((char*) &m_scores[ind], sizeof(float));
		
		bytesRead += sizeof(float);
	}
	
	std::transform(m_scores.begin(),m_scores.end(),m_scores.begin(), Moses::TransformScore);
	std::transform(m_scores.begin(),m_scores.end(),m_scores.begin(), Moses::FloorScore);

	return bytesRead;	
}

std::ostream& operator<<(std::ostream &out, const TargetPhrase &phrase)
{
	out << (const Phrase&) phrase << ", " ;
	
	for (size_t ind = 0; ind < phrase.m_align.size(); ++ind)
	{
		const AlignPair &alignPair = phrase.m_align[ind];
		out << alignPair.first << "-" << alignPair.second << " ";
	}
	out << ", ";
	
	for (size_t ind = 0; ind < phrase.m_scores.size(); ++ind)
	{
		out << phrase.m_scores[ind] << " ";
	}

	return out;
}
	
} // namespace

