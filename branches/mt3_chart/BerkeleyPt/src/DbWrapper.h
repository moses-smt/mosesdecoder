#pragma once

/*
 *  Db.h
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <string>
#include <db.h>
#include <db_cxx.h>

namespace MosesBerkeleyPt
{

class Vocab;
class Phrase;
class TargetPhrase;
class Word;
	
class SourceKey
{
public:
	SourceKey(long sourceId, int vocabId)
	:m_sourceId(sourceId)
	,m_vocabId(vocabId)
	{	}
	
	long m_sourceId;
	int m_vocabId;
	
};

class TargetfirKey
{

};


class DbWrapper
{
	Db m_dbMisc, m_dbVocab, m_dbSource, m_dbTarget, m_dbTargetInd;
	long m_nextSourceId, m_nextTargetId;
	
	long SaveSourceWord(long currSourceId, const Word &word);
	
public:
	DbWrapper();
	~DbWrapper();
	void Open(const std::string &filePath);

	void Save(const Vocab &vocab);
	void SaveSource(const Phrase &phrase, const TargetPhrase &target);
	void SaveTarget(const TargetPhrase &phrase);

	void GetAllVocab();
	
	Db &GetSDbMisc()
	{ return m_dbMisc; }
};

}; // namespace

