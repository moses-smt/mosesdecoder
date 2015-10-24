/*
 * StaticData.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "StaticData.h"
#include "PhraseTable.h"
#include "moses/Util.h"

StaticData::StaticData() {
	PhraseTable *pt = new PhraseTable();
	pt->Load(*this);
	m_featureFunctions.push_back(pt);
}

StaticData::~StaticData() {
	Moses::RemoveAllInColl(m_featureFunctions);
}

