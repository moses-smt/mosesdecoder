/*
 *  Global.cpp
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 30/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "/usr/local/BerkeleyDB.4.7/include/db_cxx.h"
#include "Global.h"
#include "Vocab.h"

using namespace std;

namespace MosesBerkeleyPt
{
	Global Global::s_instance;

void Global::Save(Db &dbMisc)
{
	s_instance.Save(dbMisc, "NumSourceFactors", 1);
	s_instance.Save(dbMisc, "NumTargetFactors", 1);
	s_instance.Save(dbMisc, "NumScores", 5);
}

void Global::Save(Db &dbMisc, const std::string &key, int value)
{
	char *keyData = (char*) malloc(key.size() + 1);
	strcpy(keyData, key.c_str());
	Dbt keyDb(keyData, key.size() + 1);

	Dbt valueDb(&value, sizeof(int));
	
	int ret = dbMisc.put(NULL, &keyDb, &valueDb, DB_NOOVERWRITE);
	if (ret == DB_KEYEXIST) 
	{
		dbMisc.err(ret, "Put failed because key %f already exists", keyData);
	}	
}

int Global::Load(Db &dbMisc, const std::string &key)
{
	char *keyData = (char*) malloc(key.size() + 1);
	strcpy(keyData, key.c_str());
	Dbt keyDb(keyData, key.size() + 1);
	
	Dbt valueDb;
	
	dbMisc.get(NULL, &keyDb, &valueDb, 0);
	int *value = (int*) valueDb.get_data();
	return *value;
}

void Global::Load(Db &dbMisc)
{
	s_instance.m_numSourceFactors = s_instance.Load(dbMisc, "NumSourceFactors");
	s_instance.m_numTargetFactors = s_instance.Load(dbMisc, "NumTargetFactors");
	s_instance.m_numScores = s_instance.Load(dbMisc, "NumScores");	
}

size_t Global::GetSourceWordSize() const
{
	return m_numSourceFactors * sizeof(VocabId);
}
size_t Global::GetTargetWordSize() const
{
	return m_numTargetFactors * sizeof(VocabId);
}
}; // namespace

