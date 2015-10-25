/*
 * Manager.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "Manager.h"
#include "PhraseTable.h"
#include "StaticData.h"
#include "SearchNormal.h"

using namespace std;

Manager::Manager(const StaticData &staticData, const std::string &inputStr)
:m_staticData(staticData)
,m_initRange(NOT_FOUND, NOT_FOUND)
{
	m_input = Phrase::CreateFromString(m_pool, inputStr);
	m_inputPaths.Init(*m_input, staticData);

	const std::vector<const PhraseTable*> &pts = staticData.GetPhraseTables();
	for (size_t i = 0; i < pts.size(); ++i) {
		const PhraseTable &pt = *pts[i];
		pt.Lookups(m_inputPaths);
	}

	m_stacks.resize(m_input->GetSize());
	m_bitmaps = new Moses::Bitmaps(m_input->GetSize(), vector<bool>(0));
	m_search = new SearchNormal(*this, m_stacks);
}

void Manager::Decode()
{
	const Moses::WordsBitmap &initBitmap = m_bitmaps->GetInitialBitmap();
	Hypothesis *iniHypo = new (GetPool().Allocate<Hypothesis>()) Hypothesis(*this, initBitmap, m_initRange);

	for (size_t i = 0; i < m_stacks.size(); ++i) {
		m_search->Decode(i);
	}
}

Manager::~Manager() {
	delete m_bitmaps;
	delete m_search;
}

