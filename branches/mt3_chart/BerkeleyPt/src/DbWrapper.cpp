/*
 *  Db.cpp
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include "DbWrapper.h"
#include "Vocab.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Global.h"

using namespace std;

namespace MosesBerkeleyPt
{

DbWrapper::DbWrapper()
:m_dbMisc(0, 0)
,m_dbVocab(0, 0)
,m_dbSource(0, 0)
,m_dbTarget(0, 0)
,m_dbTargetInd(0, 0)
,m_nextSourceId(1)
,m_nextTargetId(1)
{}

DbWrapper::~DbWrapper()
{
	m_dbVocab.close(0);
}

// helper callback fn for target secondary db
int
GetIdFromTargetPhrase(Db *sdbp,          // secondary db handle
              const Dbt *pkey,   // primary db record's key
              const Dbt *pdata,  // primary db record's data
              Dbt *skey)         // secondary db record's key
{
	// First, extract the structure contained in the primary's data
	memset(skey, 0, sizeof(DBT));
	
	// Now set the secondary key's data to be the representative's name
	long targetId = *(long*) pdata->get_data();
	skey->set_data(pdata->get_data());
	skey->set_size(sizeof(long));
	
	// Return 0 to indicate that the record can be created/updated.
	return (0);
} 

void DbWrapper::Open(const string &filePath)
{
	m_dbMisc.set_error_stream(&cerr);
	m_dbMisc.set_errpfx("SequenceExample");
	m_dbMisc.open(NULL, (filePath + "/Misc.db").c_str(), NULL, DB_BTREE, DB_CREATE, 0664);

	m_dbVocab.set_error_stream(&cerr);
	m_dbVocab.set_errpfx("SequenceExample");
	m_dbVocab.open(NULL, (filePath + "/Vocab.db").c_str(), NULL, DB_BTREE, DB_CREATE, 0664);

	m_dbSource.set_error_stream(&cerr);
	m_dbSource.set_errpfx("SequenceExample");
	m_dbSource.open(NULL, (filePath + "/Source.db").c_str(), NULL, DB_BTREE, DB_CREATE, 0664);

	// store target phrase -> id
	m_dbTarget.set_error_stream(&cerr);
	m_dbTarget.set_errpfx("SequenceExample");
	m_dbTarget.open(NULL, (filePath + "/Target.db").c_str(), NULL, DB_BTREE, DB_CREATE, 0664);
	
	// store id -> target phrase
	m_dbTargetInd.open(NULL, (filePath + "/TargetInd.db").c_str(), NULL, DB_BTREE, DB_CREATE, 0664);
	
	m_dbTarget.associate(NULL, &m_dbTargetInd, GetIdFromTargetPhrase, 0);

}

void DbWrapper::Save(const Vocab &vocab)
{
	Vocab::const_iterator iterVocab;
	for (iterVocab = vocab.begin(); iterVocab != vocab.end(); ++iterVocab)
	{
		const string &word = iterVocab->first;
		char *wordChar = (char*) malloc(word.size() + 1);
		strcpy(wordChar, word.c_str());
		VocabId vocabId = iterVocab->second;
		
		cerr << word << " = " << vocabId << endl;
		
		Dbt key(wordChar, word.size() + 1);
		Dbt data(&vocabId, sizeof(VocabId));
		
		int ret = m_dbVocab.put(NULL, &key, &data, DB_NOOVERWRITE);
		if (ret == DB_KEYEXIST) 
		{
			m_dbVocab.err(ret, "Put failed because key %f already exists", wordChar);
		}
		
		free(wordChar);
	}
	
}

void DbWrapper::GetAllVocab()
{
	Dbt key, data;
	
	// search
	char *c = "Less";
	key.set_data(c);
	key.set_size(5);
	
	m_dbVocab.get(NULL, &key, &data, 0);
	
	VocabId *id = (VocabId*) data.get_data();	
	cerr << *id << endl;
	
	// cursors
	Dbc *cursorp;
	m_dbVocab.cursor(NULL, &cursorp, 0); 
	
	int ret;
	
	// Iterate over the database, retrieving each record in turn.
	while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0)
	{
		VocabId *id = (VocabId*) data.get_data();
		cerr << (char*) key.get_data() << " = " << *id << endl;
	}
	if (ret != DB_NOTFOUND) {
		// ret should be DB_NOTFOUND upon exiting the loop.
		// Dbc::get() will by default throw an exception if any
		// significant errors occur, so by default this if block
		// can never be reached. 
		cerr << "baar";
	}
		
}

void DbWrapper::SaveSource(const Phrase &source, const TargetPhrase &target)
{
	long currSourceId = 0;
	
	// SOURCE
	for (size_t pos = 0; pos < source.GetSize(); ++pos)
	{
		const Word &word = source.GetWord(pos);
		currSourceId = SaveSourceWord(currSourceId, word);
									 		
		if (word.IsNonTerminal())
		{ // store the TARGET non-term label straight after source non-term label
			size_t targetPos = target.GetAlign(pos);
			const Word &targetWord = target.GetWord(targetPos);
			currSourceId = SaveSourceWord(currSourceId, targetWord);
		}
	}
}

long DbWrapper::SaveSourceWord(long currSourceId, const Word &word)
{
	long retSourceId;
	
	// create db data
	SourceKey sourceKey(currSourceId, word.GetFactor(0));
	long nextSourceId = m_nextSourceId;
	
	Dbt key(&sourceKey, sizeof(SourceKey));
	Dbt data(&nextSourceId, sizeof(long));
	
	// save
	int ret = m_dbSource.put(NULL, &key, &data, DB_NOOVERWRITE);
	if (ret == DB_KEYEXIST) 
	{ // already exist. get node id
		m_dbSource.get(NULL, &key, &data, 0);
		
		long *sourceId = (long*) data.get_data();
		retSourceId = *sourceId;
	}
	else
	{
		retSourceId = m_nextSourceId;
		++m_nextSourceId;
	}
	
	return retSourceId;
}

void DbWrapper::SaveTarget(const TargetPhrase &phrase)
{
	phrase.SaveTargetPhrase(m_dbTarget, m_nextTargetId);	
}

}; // namespace


