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
,m_nextSourceNodeId(1)
,m_nextTargetNodeId(1)
,m_initNode(0)
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

void DbWrapper::Load(const string &filePath)
{
	OpenFiles(filePath);
	m_vocab.Load(m_dbVocab);

	m_numSourceFactors = GetMisc("NumSourceFactors");
	m_numTargetFactors = GetMisc("NumTargetFactors");
	m_numScores = GetMisc("NumScores");	
	
}

void DbWrapper::BeginSave(const string &filePath
													, int numSourceFactors, int	numTargetFactors, int numScores)
{
	OpenFiles(filePath);	
	m_numSourceFactors = numSourceFactors;
	m_numTargetFactors = numTargetFactors;
	m_numScores = numScores;
}
	
void DbWrapper::EndSave()
{
	SaveVocab();
	SaveMisc();
}

void DbWrapper::OpenFiles(const std::string &filePath)
{
	m_dbMisc.set_error_stream(&cerr);
	m_dbMisc.set_errpfx("SequenceExample");
	m_dbMisc.open(NULL, (filePath + "/Misc.db").c_str(), NULL, DB_BTREE, DB_CREATE, 0664);
	
	// store string -> vocab id
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
	
	// store source id -> target phrase coll
	m_dbTargetColl.set_error_stream(&cerr);
	m_dbTargetColl.set_errpfx("SequenceExample");
	m_dbTargetColl.open(NULL, (filePath + "/TargetPhraseColl.db").c_str(), NULL, DB_BTREE, DB_CREATE, 0664);
	
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
	
	free(keyData);
	
	int &value = *(int*) valueDb.get_data();
	return value;
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

long DbWrapper::SaveSource(const Phrase &source, const TargetPhrase &target)
{
	long currSourceNodeId = 0;
	
	// SOURCE
	for (size_t pos = 0; pos < source.GetSize(); ++pos)
	{
		const Word &word = source.GetWord(pos);
		currSourceNodeId = SaveSourceWord(currSourceNodeId, word);
									 		
		if (word.IsNonTerminal())
		{ // store the TARGET non-term label straight after source non-term label
			size_t targetPos = target.GetAlign(pos);
			const Word &targetWord = target.GetWord(targetPos);
			currSourceNodeId = SaveSourceWord(currSourceNodeId, targetWord);
		}
	}

	return currSourceNodeId;
}

long DbWrapper::SaveSourceWord(long currSourceNodeId, const Word &word)
{
	long retSourceNodeId;
	
	// create db data
	SourceKey sourceKey(m_nextSourceNodeId, word.GetVocabId(0));
	long nextSourceId = m_nextSourceNodeId;
	
	Dbt key(&sourceKey, sizeof(SourceKey));
	Dbt data(&nextSourceId, sizeof(long));
	
	// save
	int ret = m_dbSource.put(NULL, &key, &data, DB_NOOVERWRITE);
	if (ret == DB_KEYEXIST) 
	{ // already exist. get node id
		m_dbSource.get(NULL, &key, &data, 0);
		
		long *sourceId = (long*) data.get_data();
		retSourceNodeId = *sourceId;
	}
	else
	{
		retSourceNodeId = m_nextSourceNodeId;
		++m_nextSourceNodeId;
	}
	
	return retSourceNodeId;
}

void DbWrapper::SaveTarget(TargetPhrase &phrase)
{
	phrase.SaveTargetPhrase(m_dbTarget, m_nextTargetNodeId
													, m_numScores, GetSourceWordSize(), GetTargetWordSize());	
}

void DbWrapper::SaveTargetPhraseCollection(long sourceNodeId, const TargetPhraseCollection &tpColl)
{
	tpColl.Save(m_dbTargetColl, sourceNodeId, m_numScores, GetSourceWordSize(), GetTargetWordSize());	
}

const SourcePhraseNode *DbWrapper::GetChild(const SourcePhraseNode &parentNode, const Word &word) const
{	
	// create db data
	SourceKey sourceKey(m_nextSourceNodeId, word.GetVocabId(0));
	long nextSourceId = m_nextSourceNodeId;
	
	Dbt key(&sourceKey, sizeof(SourceKey));
	Dbt data(&nextSourceId, sizeof(long));
	
	// get from db
	// need to get rid of const
	int ret = const_cast<Db&>(m_dbSource).get(NULL, &key, &data, 0);
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

const TargetPhraseCollection *DbWrapper::GetTargetPhraseCollection(const SourcePhraseNode &node) const
{
	TargetPhraseCollection *ret = new TargetPhraseCollection;;

	long sourceNodeId = node.GetSourceNodeId();
	Dbt key(&sourceNodeId, sizeof(long));
	Dbt data;

	// get from db
	// need to get rid of const
	int dbRet = const_cast<Db&>(m_dbTargetColl).get(NULL, &key, &data, 0);
	assert(dbRet == 0);

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
		
		tp->Load(m_dbTargetColl, m_numTargetFactors);

		ret->AddTargetPhrase(tp);
	}

	return ret;
}

const Moses::TargetPhraseCollection *DbWrapper::ConvertToMoses(const TargetPhraseCollection &tpColl
																															 , const std::vector<Moses::FactorType> &factors
																															 , const Moses::ScoreProducer &phraseDict
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
		Moses::TargetPhrase *mosePhrase = tp.ConvertToMoses(factors
																											, m_vocab
																											, phraseDict
																											, weightT
																											, weightWP
																											, lmList
																											, sourcePhrase);
		
		ret->Add(mosePhrase);
	}

	return ret;
}

Word *DbWrapper::ConvertFromMosesSource(const std::vector<Moses::FactorType> &inputFactorsVec
																				, const Moses::Word &origWord) const
{
	Word *newWord = CreateSouceWord();
	
	for (size_t ind = 0 ; ind < inputFactorsVec.size() ; ++ind)
	{
		size_t factorType = inputFactorsVec[ind];
		
		const Moses::Factor *factor = origWord.GetFactor(factorType);
		if (factor != NULL)
		{
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
		} // if (factor
	} // for (size_t factorType
	
	return newWord;
	
}

}; // namespace


