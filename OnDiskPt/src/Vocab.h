#pragma once
/*
 *  Vocab.h
 *  CreateOnDisk
 *
 *  Created by Hieu Hoang on 31/12/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include <string>
#include <map>
#include "../../moses/src/TypeDef.h"

namespace Moses
{
	class Factor;
}

namespace OnDiskPt
{

class OnDiskWrapper;

class Vocab
{
protected:	
	typedef std::map<std::string, UINT64> CollType;
	CollType m_vocabColl;

	std::vector<std::string> m_lookup; // opposite of m_vocabColl	
	UINT64 m_nextId; // starts @ 1
	
	const std::string &GetString(UINT32 vocabId) const
	{ return m_lookup[vocabId]; }

public:
	Vocab()
	:m_nextId(1)
	{}
	UINT64 AddVocabId(const std::string &factorString);
	UINT64 GetVocabId(const std::string &factorString, bool &found) const;
	const Moses::Factor *GetFactor(UINT32 vocabId, Moses::FactorType factorType, Moses::FactorDirection direction, bool isNonTerminal) const;

	bool Load(OnDiskWrapper &onDiskWrapper);
	void Save(OnDiskWrapper &onDiskWrapper);
};

}

