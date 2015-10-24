/*
 * Manager.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "Manager.h"
#include "PhraseTable.h"
#include "StaticData.h"

Manager::Manager(const StaticData &staticData, const std::string &inputStr)
:m_staticData(staticData)
{
	m_input = Phrase::CreateFromString(m_pool, inputStr);
	m_inputPaths.Init(*m_input, staticData);

	const std::vector<const PhraseTable*> &pts = staticData.GetPhraseTables();
	for (size_t i = 0; i < pts.size(); ++i) {
		const PhraseTable &pt = *pts[i];
		pt.Lookups(m_inputPaths);
	}
}

void Manager::Decode()
{

}

Manager::~Manager() {
	// TODO Auto-generated destructor stub
}

