/*
 * TargetPhrases.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once
#include <vector>
#include "TargetPhrase.h"

class TargetPhrases {
public:
	TargetPhrases();
	virtual ~TargetPhrases();

	void AddTargetPhrase(const TargetPhrase &targetPhrase)
	{
		m_coll.push_back(&targetPhrase);
	}
protected:
	std::vector<const TargetPhrase*> m_coll;

};

