/*
 *  Db.cpp
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include "sys/stat.h"

#include "../../moses/src/PhraseDictionary.h"
#include "DbWrapper.h"
#include "Vocab.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "TargetPhraseCollection.h"

using namespace std;

namespace MosesBerkeleyPt
{

DbWrapper::DbWrapper()
:m_dbMisc(0, 0)
,m_dbVocab(0, 0)
,m_dbSource(0, 0)
,m_dbTarget(0, 0)
,m_dbTargetInd(0, 0)
,m_dbTargetColl(0, 0)
,m_dbTargetCollOtherInfo(0, 0)
,m_nextSourceNodeId(1)
,m_nextTargetNodeId(1)
,m_initNode(0)
{}

DbWrapper::~DbWrapper()
{
	m_dbMisc.close(0);
	m_dbVocab.close(0);
	m_dbSource.close(0);
	m_dbTarget.close(0);
	m_dbTargetInd.close(0);
	m_dbTargetColl.close(0);

	m_dbTargetCollOtherInfo.close(0);
}

// helper callback fn for target secondary db
int GetIdFromTargetPhrase(Db *sdbp,          // secondary db handle
              const Dbt *pkey,   // primary db record's key
              const Dbt *pdata,  // primary db record's data
              Dbt *skey)         // secondary db record's key
{
	// First, extract the structure contained in the primary's data
	memset(skey, 0, sizeof(DBT));
	
	// Now set the secondary key's data to be the representative's name
	long targetId = *(long*) pdata->get_data();
	skey->set_data(&targetId);
	//skey->set_data(pdata->get_data());
	skey->set_size(sizeof(long));
	
	// Return 0 to indicate that the record can be created/updated.
	return (0);
} 

bool DbWrapper::Load(const string &filePath)
{
	if (!OpenForLoad(filePath))
		return false;
	
	m_vocab.Load(m_dbVocab);

	m_numSourceFactors = GetMisc("NumSourceFactors");
	m_numTargetFactors = GetMisc("NumTargetFactors");
	m_numScores = GetMisc("NumScores");	
	
	return true;
}

bool DbWrapper::BeginSave(const string &filePath
													, int numSourceFactors, int	numTargetFactors, int numScores)
{
	if (!OpenForSave(filePath))
		return false;
	
	m_numSourceFactors = numSourceFactors;
	m_numTargetFactors = numTargetFactors;
	m_numScores = numScores;

	return true;
}
	
void DbWrapper::EndSave()
{
	SaveVocab();
	SaveMisc();
}

bool DbWrapper::OpenForSave(const std::string &filePath)
{
	mkdir(filePath.c_str(), 0777);
	
	if (OpenDb(m_dbMisc, filePath + "/Misc.db", DB_BTREE, DB_CREATE | DB_EXCL, 0664)
			&& 
			// store string -> vocab id
			OpenDb(m_dbVocab, filePath + "/Vocab.db", DB_BTREE, DB_CREATE | DB_EXCL, 0664)
			&&
			OpenDb(m_dbSource, filePath + "/Source.db", DB_BTREE, DB_CREATE | DB_EXCL, 0664)
			&&
			// store target phrase -> id
			OpenDb(m_dbTarget, filePath + "/Target.db", DB_BTREE, DB_CREATE | DB_EXCL, 0664)
			&&
			// store id -> target phrase
			OpenDb(m_dbTargetInd, filePath + "/TargetInd.db", DB_BTREE, DB_CREATE | DB_EXCL, 0664)
			&&
			// store source node id -> target phrase coll
			OpenDb(m_dbTargetColl, filePath + "/TargetPhraseColl.db", DB_BTREE, DB_CREATE | DB_EXCL, 0664)
			
			&&
			// store entropy & counts for each node id
			OpenDb(m_dbTargetCollOtherInfo, filePath + "/TargetPhraseCollOtherInfo.db", DB_BTREE, DB_CREATE | DB_EXCL, 0664)
		)
		return true;
	else
		return false;

	//	m_dbTarget.associate(NULL, &m_dbTargetInd, GetIdFromTargetPhrase, 0);
}

bool DbWrapper::OpenForLoad(const std::string &filePath)
{
	if (OpenDb(m_dbMisc, filePath + "/Misc.db", DB_UNKNOWN, 0, 0)
			&&
			OpenDb(m_dbVocab, filePath + "/Vocab.db", DB_UNKNOWN, 0, 0)
			&&
			OpenDb(m_dbSource, filePath + "/Source.db", DB_UNKNOWN, 0, 0)
			&&
			OpenDb(m_dbTargetInd, filePath + "/TargetInd.db", DB_UNKNOWN, 0, 0)
			&&
			OpenDb(m_dbTargetColl, filePath + "/TargetPhraseColl.db", DB_UNKNOWN, 0, 0)
			&&
			OpenDb(m_dbTargetCollOtherInfo, filePath + "/TargetPhraseCollOtherInfo.db", DB_UNKNOWN, 0, 0)
			)
		return true;
	else
		return false;
}

bool DbWrapper::OpenDb(Db &db, const std::string &filePath, DBTYPE type, u_int32_t flags, int mode)
{
	db.set_error_stream(&cerr);
	db.set_errpfx("dbError");
	int retDb = db.open(NULL, filePath.c_str(), NULL, type, flags, mode);	

	return retDb == 0;
}

void DbWrapper::SaveMisc()
{
	SetMisc("Version", 1);

	SetMisc("NumSourceFactors", m_numSourceFactors);
	SetMisc("NumTargetFactors", m_numTargetFactors);
	SetMisc("NumScores", m_numScores);
}

void DbWrapper::SetMisc(const string &key, int value)
{
	char *keyData = (char*) malloc(key.size() + 1);
	strcpy(keyData, key.c_str());
	Dbt keyDb(keyData, key.size() + 1);
	
	Dbt valueDb(&value, sizeof(int));
	
	int retDb = m_dbMisc.put(NULL, &keyDb, &valueDb, DB_NOOVERWRITE);
	assert(retDb == retDb);

	free(keyData);
}

int DbWrapper::GetMisc(const std::string &key)
{
	char *keyData = (char*) malloc(key.size() + 1);
	strcpy(keyData, key.c_str());
	Dbt keyDb(keyData, key.size() + 1);
	
	Dbt valueDb;
	
	m_dbMisc.get(NULL, &keyDb, &valueDb, 0);
	assert(valueDb.get_size() == sizeof(int));
	
	free(keyData);
	
	int &value = *(int*) valueDb.get_data();
	return value;
}
	
long &DbWrapper::GetNextSourceNodeId()
{ 
	return m_nextSourceNodeId; 
}

Word *DbWrapper::CreateSouceWord() const
{
	return new Word(m_numSourceFactors);
}

Word *DbWrapper::CreateTargetWord() const
{
	return new Word(m_numTargetFactors);
}

void DbWrapper::SaveVocab()
{
	Vocab::const_iterator iterVocab;
	for (iterVocab = m_vocab.begin(); iterVocab != m_vocab.end(); ++iterVocab)
	{
		const string &word = iterVocab->first;
		char *wordChar = (char*) malloc(word.size() + 1);
		strcpy(wordChar, word.c_str());
		VocabId vocabId = iterVocab->second;
				
		Dbt key(wordChar, word.size() + 1);
		Dbt data(&vocabId, sizeof(VocabId));
		
		int retDb = m_dbVocab.put(NULL, &key, &data, DB_NOOVERWRITE);
		assert(retDb == 0); 
		
		free(wordChar);
	}
	
}

void DbWrapper::SaveTarget(TargetPhrase &phrase)
{
	phrase.SaveTargetPhrase(m_dbTarget, m_dbTargetInd, m_nextTargetNodeId
													, m_numScores, GetSourceWordSize(), GetTargetWordSize());	
}

void DbWrapper::SaveTargetPhraseCollection(long sourceNodeId, const TargetPhraseCollection &tpColl)
{
	tpColl.Save(m_dbTargetColl, sourceNodeId, m_numScores, GetSourceWordSize(), GetTargetWordSize());	

	// other info
	// calc entropy
	float entropy = 0;
	TargetPhraseCollection::const_iterator iter;
	for (iter = tpColl.begin(); iter != tpColl.end(); ++iter)
	{
		const TargetPhrase &tp = **iter;
		float prob = tp.GetScores()[2];
		
		entropy -= prob * log(prob);
	}
	
	// save
	Moses::TargetPhraseCollectionOtherInfo info(345, entropy);
	Dbt key(&sourceNodeId, sizeof(sourceNodeId));
	Dbt data(&info, sizeof(info));
	
	int retDb = m_dbTargetCollOtherInfo.put(NULL, &key, &data, DB_NOOVERWRITE);
	assert(retDb == 0); 
}

const SourcePhraseNode *DbWrapper::GetChild(const SourcePhraseNode &parentNode, const Word &word)
{	
	// create db data
	long sourceNodeId = parentNode.GetSourceNodeId();
	SourceKey sourceKey(sourceNodeId, word.GetVocabId(0));
	
	Dbt key(&sourceKey, sizeof(SourceKey));
	Dbt data;
	
	// get from db
	// need to get rid of const
	int ret = m_dbSource.get(NULL, &key, &data, 0);
	if (ret == 0) 
	{ // exist. 		
		long sourceNodeId = *(long*) data.get_data();
		SourcePhraseNode *node = new SourcePhraseNode(sourceNodeId);
		return node;
	}
	else
	{
		return NULL;
	}	
}

const TargetPhraseCollection *DbWrapper::GetTargetPhraseCollection(const SourcePhraseNode &node)
{
	TargetPhraseCollection *ret = new TargetPhraseCollection();

	long sourceNodeId = node.GetSourceNodeId();
	Dbt key(&sourceNodeId, sizeof(long));
	Dbt data;

	// get from db
	// need to get rid of const
	int dbRet = m_dbTargetColl.get(NULL, &key, &data, 0);
	//assert(dbRet == 0);

	if (dbRet == 0)
	{
		char *mem = (char*) data.get_data();
		size_t offset = 0;

		// size
		int sizeColl;
		memcpy(&sizeColl, mem, sizeof(int));
		offset += sizeof(int);

		// phrase
		for (size_t ind = 0; ind < sizeColl; ++ind)
		{
			TargetPhrase *tp = new TargetPhrase();

			size_t memUsed = tp->ReadOtherInfoFromMemory(&mem[offset]
																									, m_numSourceFactors, m_numTargetFactors
																									, m_numScores);
			offset += memUsed;
			
			// actual words
			tp->Load(m_dbTargetInd, m_numTargetFactors);

			ret->AddTargetPhrase(tp);
		}

		assert(offset == data.get_size());
	}
	
	// other info	
	dbRet = m_dbTargetCollOtherInfo.get(NULL, &key, &data, 0);
	assert(dbRet == 0); 
	assert(data.get_size() == sizeof(Moses::TargetPhraseCollectionOtherInfo)); 
	
	Moses::TargetPhraseCollectionOtherInfo *info = (Moses::TargetPhraseCollectionOtherInfo*) data.get_data();
	ret->SetOtherInfo(*info);
	
	return ret;
}

Moses::TargetPhraseCollection *DbWrapper::ConvertToMoses(const TargetPhraseCollection &tpColl
																															, const std::vector<Moses::FactorType> &inputFactors
																															, const std::vector<Moses::FactorType> &outputFactors
																															, const Moses::PhraseDictionary &phraseDict
																															, const std::vector<float> &weightT
																															, float weightWP
																															, const Moses::LMList &lmList
																															, const Moses::Phrase &sourcePhrase) const
{
	Moses::TargetPhraseCollection *ret = new Moses::TargetPhraseCollection();

	TargetPhraseCollection::const_iterator iter;
	for (iter = tpColl.begin(); iter != tpColl.end(); ++iter)
	{
		const TargetPhrase &tp = **iter;
		Moses::TargetPhrase *mosesPhrase = tp.ConvertToMoses(inputFactors, outputFactors
																											, m_vocab
																											, phraseDict
																											, weightT
																											, weightWP
																											, lmList
																											, sourcePhrase);
		ret->Add(mosesPhrase);
	}

	ret->NthElement(phraseDict.GetTableLimit());

	// other info
	ret->SetOtherInfo(tpColl.GetOtherInfo());
	
	return ret;
}

Word *DbWrapper::ConvertFromMoses(Moses::FactorDirection direction
																	, const std::vector<Moses::FactorType> &factorsVec
																	, const Moses::Word &origWord) const
{
	Word *newWord = (direction == Moses::Input) ? CreateSouceWord() : CreateTargetWord();
	
	for (size_t ind = 0 ; ind < factorsVec.size() ; ++ind)
	{
		size_t factorType = factorsVec[ind];
		
		const Moses::Factor *factor = origWord.GetFactor(factorType);
		assert(factor);

		const string &str = factor->GetString();
		bool found;
		VocabId vocabId = m_vocab.GetVocabId(str, found);
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

}; // namespace


