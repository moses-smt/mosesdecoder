/*
 * Latticea.cpp
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#include "Lattice.h"
#include "AlignedSentence.h"

Lattice::Lattice(const AlignedSentence &alignedSentence)
{
	/*
	const ConsistentPhrases &consistentPhrases = alignedSentence.GetConsistentPhrases();
	ConsistentPhrases::const_iterator iter;
	for (iter = consistentPhrases.begin(); iter != consistentPhrases.end(); ++iter) {
		const ConsistentPhrase &consistentPhrase = *iter;
		const ConsistentRange &nonTerm = consistentPhrase.GetConsistentRange(Moses::Input);

		int start = nonTerm.GetStart();
		Node &node = m_coll[start];
		node.push_back(&nonTerm);
	}
	*/
}

Lattice::~Lattice() {
	// TODO Auto-generated destructor stub
}

