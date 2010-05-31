/*
 *  Vocab.cpp
 *  CreateOnDisk
 *
 *  Created by Hieu Hoang on 31/12/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include <string>
#include <fstream>
#include "OnDiskWrapper.h"
#include "Vocab.h"
#include "../../moses/src/FactorCollection.h"

using namespace std;

namespace OnDiskPt
{
	
bool Vocab::Load(OnDiskWrapper &onDiskWrapper)
{
	fstream &file = onDiskWrapper.GetFileVocab();
	
	string line;
	while(getline(file, line))
	{
		vector<string> tokens;
		Moses::Tokenize(tokens, line);
		assert(tokens.size() == 2);
		const string &key = tokens[0];
		m_vocabColl[key] =  Moses::Scan<UINT64>(tokens[1]);
	}
	
	// create lookup
	// assume contiguous vocab id
	m_lookup.resize(m_vocabColl.size() + 1);
	
	CollType::const_iterator iter;
	for (iter = m_vocabColl.begin(); iter != m_vocabColl.end(); ++iter)
	{
		UINT32 vocabId = iter->second;
		const std::string &word = iter->first;
		
		m_lookup[vocabId] = word;
	}
	
	return true;
}

void Vocab::Save(OnDiskWrapper &onDiskWrapper)
{
	fstream &file = onDiskWrapper.GetFileVocab();
	CollType::const_iterator iterVocab;
	for (iterVocab = m_vocabColl.begin(); iterVocab != m_vocabColl.end(); ++iterVocab)
	{
		const string &word = iterVocab->first;
		UINT32 vocabId = iterVocab->second;
				
		file << word << " " << vocabId << endl;
	}
}

UINT64 Vocab::AddVocabId(const std::string &factorString)
{
	// find string id
	CollType::const_iterator iter = m_vocabColl.find(factorString);
	if (iter == m_vocabColl.end())
	{ // add new vocab entry
		m_vocabColl[factorString] = m_nextId;
		return m_nextId++;
	}
	else
	{ // return existing entry
		return iter->second;
	}
}

UINT64 Vocab::GetVocabId(const std::string &factorString, bool &found) const
{
	// find string id
	CollType::const_iterator iter = m_vocabColl.find(factorString);
	if (iter == m_vocabColl.end())
	{ 
		found = false;
		return 0; //return whatever
	}
	else
	{ // return existing entry
		found = true;
		return iter->second;
	}
}
	
const Moses::Factor *Vocab::GetFactor(UINT32 vocabId, Moses::FactorType factorType, Moses::FactorDirection direction, bool isNonTerminal) const
{
	string str = GetString(vocabId);
	if (isNonTerminal)
	{
		str = str.substr(1, str.size() - 2);
	}
	const Moses::Factor *factor = Moses::FactorCollection::Instance().AddFactor(direction, factorType, str);
	return factor;	
}

}
