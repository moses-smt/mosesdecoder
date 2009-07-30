#pragma once
/*
 *  Global.h
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 30/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include <string>

class Db;

class Global
{
protected:
	static Global s_instance;

	int m_numSourceFactors, m_numTargetFactors, m_numScores;

	void Save(Db &dbMisc, const std::string &key, int value);
	int Load(Db &dbMisc, const std::string &key);
public:
	static const Global &Instance()
	{ return s_instance; }

	static void Save(Db &dbMisc);
	static void Load(Db &dbMisc);

	int GetNumSourceFactors() const
	{ return m_numSourceFactors; }
	int GetNumTargetFactors() const
	{ return m_numTargetFactors; }
	int GetNumScores() const
	{ return m_numScores; }
};
