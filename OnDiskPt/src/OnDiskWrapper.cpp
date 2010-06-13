/*
 *  OnDiskWrapper.cpp
 *  CreateOnDisk
 *
 *  Created by Hieu Hoang on 31/12/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#ifdef WIN32
#include <direct.h>
#endif
#include <sys/stat.h>
#include <cassert>
#include <string>
#include "OnDiskWrapper.h"

using namespace std;

namespace OnDiskPt
{

OnDiskWrapper::OnDiskWrapper()
{
}

OnDiskWrapper::~OnDiskWrapper()
{
  delete m_rootSourceNode;
}

bool OnDiskWrapper::BeginLoad(const std::string &filePath)
{
	if (!OpenForLoad(filePath))
		return false;
	
	if (!m_vocab.Load(*this))
			return false;
	
	UINT64 rootFilePos = GetMisc("RootNodeOffset");
	m_rootSourceNode = new PhraseNode(rootFilePos, *this);	

	return true;
}

bool OnDiskWrapper::OpenForLoad(const std::string &filePath)
{
	m_fileSource.open((filePath + "/Source.dat").c_str(), ios::in | ios::binary);
	assert(m_fileSource.is_open());
	
	m_fileTargetInd.open((filePath + "/TargetInd.dat").c_str(), ios::in | ios::binary);
	assert(m_fileTargetInd.is_open());
	
	m_fileTargetColl.open((filePath + "/TargetColl.dat").c_str(), ios::in | ios::binary);
	assert(m_fileTargetColl.is_open());
	
	m_fileVocab.open((filePath + "/Vocab.dat").c_str(), ios::in);
	assert(m_fileVocab.is_open());
	
	m_fileMisc.open((filePath + "/Misc.dat").c_str(), ios::in);
	assert(m_fileMisc.is_open());
	
	// set up root node
	LoadMisc();
	m_numSourceFactors = GetMisc("NumSourceFactors");
	m_numTargetFactors = GetMisc("NumTargetFactors");
	m_numScores = GetMisc("NumScores");	
	
	return true;
}
	
bool OnDiskWrapper::LoadMisc()
{
	char line[100000];
	
	while(m_fileMisc.getline(line, 100000))
	{
		vector<string> tokens;
		Moses::Tokenize(tokens, line);
		assert(tokens.size() == 2);
		const string &key = tokens[0];
		m_miscInfo[key] =  Moses::Scan<UINT64>(tokens[1]);
	}
	
	return true;
}

bool OnDiskWrapper::BeginSave(const std::string &filePath
														, int numSourceFactors, int	numTargetFactors, int numScores)
{
	m_numSourceFactors = numSourceFactors;
	m_numTargetFactors = numTargetFactors;
	m_numScores = numScores;
	m_filePath = filePath;
	
#ifdef WIN32
	mkdir(filePath.c_str());
#else
	mkdir(filePath.c_str(), 0777);
#endif
	
	m_fileSource.open((filePath + "/Source.dat").c_str(), ios::out | ios::in | ios::binary | ios::ate | ios::trunc);
	assert(m_fileSource.is_open());

	m_fileTargetInd.open((filePath + "/TargetInd.dat").c_str(), ios::out | ios::binary | ios::ate | ios::trunc);
	assert(m_fileTargetInd.is_open());

	m_fileTargetColl.open((filePath + "/TargetColl.dat").c_str(), ios::out | ios::binary | ios::ate | ios::trunc);
	assert(m_fileTargetColl.is_open());

	m_fileVocab.open((filePath + "/Vocab.dat").c_str(), ios::out | ios::ate | ios::trunc);
	assert(m_fileVocab.is_open());

	m_fileMisc.open((filePath + "/Misc.dat").c_str(), ios::out | ios::ate | ios::trunc);
	assert(m_fileMisc.is_open());

	// offset by 1. 0 offset is reserved
	char c = 0xff;
	m_fileSource.write(&c, 1);
	assert(1 == m_fileSource.tellp());
	
	m_fileTargetInd.write(&c, 1);
	assert(1 == m_fileTargetInd.tellp());

	m_fileTargetColl.write(&c, 1);
	assert(1 == m_fileTargetColl.tellp());

	// set up root node
	assert(GetNumCounts() == 1);
	vector<float> counts(GetNumCounts());
	counts[0] = DEFAULT_COUNT;
	m_rootSourceNode = new PhraseNode();
	m_rootSourceNode->AddCounts(counts);

	return true;
}

void OnDiskWrapper::EndSave()
{
	assert(m_rootSourceNode->Saved());

	GetVocab().Save(*this);
	
	SaveMisc();

	m_fileMisc.close();
	m_fileVocab.close();
	m_fileSource.close();
	m_fileTarget.close();
	m_fileTargetInd.close();
	m_fileTargetColl.close();
}

void OnDiskWrapper::SaveMisc()
{
	m_fileMisc << "Version 3" << endl;
	m_fileMisc << "NumSourceFactors " << m_numSourceFactors << endl;
	m_fileMisc << "NumTargetFactors " << m_numTargetFactors << endl;
	m_fileMisc << "NumScores " << m_numScores << endl;
	m_fileMisc << "RootNodeOffset " << m_rootSourceNode->GetFilePos() << endl;
}

size_t OnDiskWrapper::GetSourceWordSize() const
{
	return m_numSourceFactors * sizeof(UINT64) + sizeof(char);
}

size_t OnDiskWrapper::GetTargetWordSize() const
{
	return m_numTargetFactors * sizeof(UINT64) + sizeof(char);
}

UINT64 OnDiskWrapper::GetMisc(const std::string &key) const
{
	std::map<std::string, UINT64>::const_iterator iter;
	iter = m_miscInfo.find(key);
	assert(iter != m_miscInfo.end());
	
	return iter->second;
}

PhraseNode &OnDiskWrapper::GetRootSourceNode()
{ return *m_rootSourceNode; }

Word *OnDiskWrapper::ConvertFromMoses(Moses::FactorDirection direction
																	, const std::vector<Moses::FactorType> &factorsVec
																	, const Moses::Word &origWord) const
{
	bool isNonTerminal = origWord.IsNonTerminal();
	Word *newWord = new Word(1, isNonTerminal); // TODO - num of factors
	
	for (size_t ind = 0 ; ind < factorsVec.size() ; ++ind)
	{
		size_t factorType = factorsVec[ind];
		
		const Moses::Factor *factor = origWord.GetFactor(factorType);
		assert(factor);
		
		string str = factor->GetString();
		if (isNonTerminal)
		{
			str = "[" + str + "]";
		}
		
		bool found;
		UINT64 vocabId = m_vocab.GetVocabId(str, found);
		if (!found)
		{ // factor not in phrase table -> phrse definately not in. exit
			delete newWord;
			return NULL;
		}
		else
		{
			newWord->SetVocabId(ind, vocabId);
		}
	} // for (size_t factorType
		
	return newWord;
	
}

	
}
