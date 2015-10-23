/*
 * Manager.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "Manager.h"

Manager::Manager(const StaticData &staticData, Phrase &input)
:m_staticData(staticData)
,m_input(input)
,m_inputPaths(input)
{
	m_pt.Load();

}

Manager::~Manager() {
	// TODO Auto-generated destructor stub
}

