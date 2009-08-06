/*
 *  Vocab.cpp
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <iostream>
#include <cassert>
#include "Vocab.h"
#include "../../moses/src/Util.h"
#include "../../moses/src/FactorCollection.h"

using namespace std;

namespace MosesBerkeleyPt
{

void Vocab::Load(Db &db)
{
	Dbt key, data;
	
	// cursors
	Dbc *cursorp;
	db.cursor(NULL, &cursorp, 0); 
	
	// Iterate over the database, retrieving each record in turn.
	int ret;
	while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0)
	{
		VocabId &vocabId= *(VocabId*) data.get_data();
		char *str = (char*) key.get_data();
		cerr << vocabId << "=" << str << endl;

		m_vocabColl[str] = vocabId;
	}

	assert(ret == DB_NOTFOUND);
		
	// create lookup
	// assume contiguous vocab id
	m_lookup.resize(m_vocabColl.size() + 1);
	
	CollType::const_iterator iter;
	for (iter = m_vocabColl.begin(); iter != m_vocabColl.end(); ++iter)
	{
		VocabId vocabId = iter->second;
		const std::string &word = iter->first;

		m_lookup[vocabId] = word;
	}
}

VocabId Vocab::GetVocabId(const std::string &factorString, bool &found) const
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

VocabId Vocab::AddVocabId(const std::string &factorString)
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
	
const Moses::Factor *Vocab::GetFactor(VocabId vocabId, Moses::FactorType factorType, Moses::FactorDirection direction, bool isNonTerminal) const
{
	const string &str = GetString(vocabId);
	const Moses::Factor *factor = Moses::FactorCollection::Instance().AddFactor(direction, factorType, str, isNonTerminal);
	return factor;
}

}; // namespace
