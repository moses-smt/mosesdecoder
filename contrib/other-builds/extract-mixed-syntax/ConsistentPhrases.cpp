/*
 * ConsistentPhrases.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#include <cassert>
#include "ConsistentPhrases.h"

using namespace std;

ConsistentPhrases::ConsistentPhrases() {
	// TODO Auto-generated constructor stub

}

ConsistentPhrases::~ConsistentPhrases() {
	// TODO Auto-generated destructor stub
}

void ConsistentPhrases::Add(ConsistentPhrase &phrasePair)
{
	pair<iterator,bool> success = m_coll.insert(phrasePair);
	assert(success.second);

}
