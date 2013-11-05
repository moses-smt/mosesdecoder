/*
 * PhraseTable.cpp
 *
 *  Created on: 5 Oct 2013
 *      Author: hieu
 */

#include "PhraseTable.h"
#include "InputPath.h"

std::vector<PhraseTable*> PhraseTable::s_staticColl;
size_t PhraseTable::s_ptId = 0;

PhraseTable::PhraseTable(const std::string line)
  :StatelessFeatureFunction(line)
  ,m_ptId(s_ptId++)
{
  s_staticColl.push_back(this);

}

PhraseTable::~PhraseTable()
{
  // TODO Auto-generated destructor stub
}

