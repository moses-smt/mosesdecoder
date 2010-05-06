
#include <algorithm>
#include <iostream>
#include "../../moses/src/Util.h"
#include "../../moses/src/TargetPhraseCollection.h"
#include "../../moses/src/PhraseDictionary.h"
#include "TargetPhraseCollection.h"
#include "Vocab.h"
#include "OnDiskWrapper.h"

using namespace std;

namespace OnDiskPt
{

size_t TargetPhraseCollection::s_sortScoreInd;

TargetPhraseCollection::TargetPhraseCollection()
:m_filePos(777)
{}
	
TargetPhraseCollection::TargetPhraseCollection(const TargetPhraseCollection &copy)
:m_filePos(copy.m_filePos)
,m_debugStr(copy.m_debugStr)
{
}

TargetPhraseCollection::~TargetPhraseCollection()
{
	Moses::RemoveAllInColl(m_coll);	
}

void TargetPhraseCollection::AddTargetPhrase(TargetPhrase *targetPhrase)
{
	m_coll.push_back(targetPhrase);
}

void TargetPhraseCollection::Sort(size_t tableLimit)
{
	std::sort(m_coll.begin(), m_coll.end(), TargetPhraseOrderByScore());
	
	if (m_coll.size() > tableLimit)
	{
		CollType::iterator iter;
		for (iter = m_coll.begin() + tableLimit ; iter != m_coll.end(); ++iter)
		{
			delete *iter;
		}
		m_coll.resize(tableLimit);
	}
}
	
void TargetPhraseCollection::Save(OnDiskWrapper &onDiskWrapper)
{
	std::fstream &file = onDiskWrapper.GetFileTargetColl();

	size_t memUsed = sizeof(UINT64);
	char *mem = (char*) malloc(memUsed); 

	// size of coll
	UINT64 numPhrases = GetSize();
	((UINT64*)mem)[0] = numPhrases;
	
	// MAIN LOOP
	CollType::iterator iter;
	for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
	{
		// save phrase
		TargetPhrase &targetPhrase = **iter;
		targetPhrase.Save(onDiskWrapper);
		
		// save coll
		size_t memUsedTPOtherInfo;
		char *memTPOtherInfo = targetPhrase.WriteOtherInfoToMemory(onDiskWrapper, memUsedTPOtherInfo);

		// expand existing mem
		mem = (char*) realloc(mem, memUsed + memUsedTPOtherInfo);
		memcpy(mem + memUsed, memTPOtherInfo, memUsedTPOtherInfo); 
		memUsed += memUsedTPOtherInfo;
		
		free(memTPOtherInfo);
	}

	// total number of bytes
	//((UINT64*)mem)[0] = (UINT64) memUsed;
	
	UINT64 startPos = file.tellp();	
	file.seekp(0, ios::end);
	file.write((char*) mem, memUsed);

	free(mem);
	
	UINT64 endPos = file.tellp();
	assert(startPos + memUsed == endPos);
	
	m_filePos = startPos;

}
	
Moses::TargetPhraseCollection *TargetPhraseCollection::ConvertToMoses(const std::vector<Moses::FactorType> &inputFactors
																							, const std::vector<Moses::FactorType> &outputFactors
																							, const Moses::PhraseDictionary &phraseDict
																							, const std::vector<float> &weightT
																							, float weightWP
																							, const Moses::LMList &lmList
																							, const Moses::Phrase &sourcePhrase
																							, const std::string &filePath
																							, Vocab &vocab) const
{
	Moses::TargetPhraseCollection *ret = new Moses::TargetPhraseCollection();
	
	CollType::const_iterator iter;
	for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
	{
		const TargetPhrase &tp = **iter;
		Moses::TargetPhrase *mosesPhrase = tp.ConvertToMoses(inputFactors, outputFactors
																												 , vocab
																												 , phraseDict
																												 , weightT
																												 , weightWP
																												 , lmList
																												 , sourcePhrase);
		
		/*
		// debugging output
		stringstream strme;
		strme << filePath << " " << *mosesPhrase;
		mosesPhrase->SetDebugOutput(strme.str());
		*/
		
		ret->Add(mosesPhrase);
	}
	
	ret->Prune(true, phraseDict.GetTableLimit());
	
	return ret;
	
}

void TargetPhraseCollection::ReadFromFile(size_t tableLimit, UINT64 filePos, OnDiskWrapper &onDiskWrapper)
{
	fstream &fileTPColl = onDiskWrapper.GetFileTargetColl();
	fstream &fileTP = onDiskWrapper.GetFileTargetInd();
	
	size_t numScores = onDiskWrapper.GetNumScores();
	size_t numTargetFactors = onDiskWrapper.GetNumTargetFactors();
	
	UINT64 numPhrases;

	UINT64 currFilePos = filePos;
	fileTPColl.seekg(filePos);	
	fileTPColl.read((char*) &numPhrases, sizeof(UINT64));
	
	// table limit
	numPhrases = std::min(numPhrases, (UINT64) tableLimit);
	
	currFilePos += sizeof(UINT64);
	
	for (size_t ind = 0; ind < numPhrases; ++ind)
	{
		TargetPhrase *tp = new TargetPhrase(numScores);
		
		UINT64 sizeOtherInfo = tp->ReadOtherInfoFromFile(currFilePos, fileTPColl);
		tp->ReadFromFile(fileTP, numTargetFactors);
		
		currFilePos += sizeOtherInfo;
		
		m_coll.push_back(tp);
	}
}

UINT64 TargetPhraseCollection::GetFilePos() const
{ return m_filePos; }

const std::string TargetPhraseCollection::GetDebugStr() const
{ return m_debugStr; }
	
void TargetPhraseCollection::SetDebugStr(const std::string &str)
{ m_debugStr = str; }

}


