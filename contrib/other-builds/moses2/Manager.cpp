/*
 * Manager.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "Manager.h"

Manager::Manager(const StaticData &staticData, const std::string &inputStr)
:m_staticData(staticData)
{
	m_input = Phrase::CreateFromString(m_pool, inputStr);
	m_inputPaths.Init(*m_input);
}

Manager::~Manager() {
	// TODO Auto-generated destructor stub
}

