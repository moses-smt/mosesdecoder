/*
 * ConsistentPhrases.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */

#include "ConsistentPhrases.h"

ConsistentPhrases::ConsistentPhrases() {
	// TODO Auto-generated constructor stub

}

ConsistentPhrases::~ConsistentPhrases() {
	// TODO Auto-generated destructor stub
}

void ConsistentPhrases::Add(ConsistentPhrase &phrasePair)
{
	m_coll.push_back(phrasePair);
}
