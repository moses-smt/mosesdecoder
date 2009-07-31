/*
 *  Vocab.cpp
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Vocab.h"

namespace MosesBerkeleyPt
{

Vocab Vocab::s_instance;

Vocab::Vocab()
:m_nextId(1)
{}


VocabId Vocab::AddFactor(const std::string &factorString)
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
}; // namespace
