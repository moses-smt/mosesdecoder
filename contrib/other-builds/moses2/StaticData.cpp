/*
 * StaticData.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "StaticData.h"
#include "PhraseTable.h"
#include "moses/Util.h"

StaticData::StaticData()
:m_ffStartInd(0)
{
	PhraseTable *pt = new PhraseTable(m_ffStartInd);
	pt->SetPtInd(m_phraseTables.size());
	pt->Load(*this);

	m_featureFunctions.push_back(pt);
	m_phraseTables.push_back(pt);
}

StaticData::~StaticData() {
	Moses::RemoveAllInColl(m_featureFunctions);
}

