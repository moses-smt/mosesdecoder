/*
 * Manager.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "Manager.h"
#include "PhraseTable.h"
#include "System.h"
#include "SearchNormal.h"

using namespace std;

Manager::Manager(System &system, const std::string &inputStr)
:m_pool(system.GetManagerPool())
,m_system(system)
,m_initRange(NOT_FOUND, NOT_FOUND)
,m_initPhrase(system.GetManagerPool(), system, 0)
{
	Moses::FactorCollection &vocab = system.GetVocab();

	m_input = Phrase::CreateFromString(m_pool, vocab, inputStr);
	m_inputPaths.Init(*m_input, system);

	const std::vector<const PhraseTable*> &pts = system.GetPhraseTables();
	for (size_t i = 0; i < pts.size(); ++i) {
		const PhraseTable &pt = *pts[i];
		pt.Lookups(m_inputPaths);
	}

	m_stacks.resize(m_input->GetSize() + 1);
	m_bitmaps = new Moses::Bitmaps(m_input->GetSize(), vector<bool>(0));
	m_search = new SearchNormal(*this, m_stacks);
}

Manager::~Manager() {
	delete m_bitmaps;
	delete m_search;
}

const Hypothesis *Manager::GetBestHypothesis() const
{
	return m_search->GetBestHypothesis();
}

void Manager::Decode()
{
	const Moses::Bitmap &initBitmap = m_bitmaps->GetInitialBitmap();
	Hypothesis *initHypo = new (GetPool().Allocate<Hypothesis>()) Hypothesis(*this, m_initPhrase, m_initRange, initBitmap);
	StackAdd stackAdded = m_stacks[0].Add(initHypo);
	assert(stackAdded.added);

	for (size_t i = 0; i < m_stacks.size(); ++i) {
		m_search->Decode(i);
	}
}
