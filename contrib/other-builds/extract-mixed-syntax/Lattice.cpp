/*
 * Lattice.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */

#include "Lattice.h"
#include "AlignedSentence.h"
#include "Word.h"
#include "ConsistentPhrase.h"

Lattice::Lattice(const AlignedSentence &alignedSentence)
:m_coll(alignedSentence.GetPhrase(Moses::Input).size())
{
	// add all words
	const Phrase &sourcePhrase = alignedSentence.GetPhrase(Moses::Input);

	for (size_t i = 0; i < sourcePhrase.size(); ++i) {
		const Word *word = sourcePhrase[i];
		Node &node = m_coll[i];
		node.push_back(word);
	}

	// add non-terms
	const std::vector<ConsistentPhrase> &consistentPhrases = alignedSentence.GetConsistentPhrases();
	for (size_t i = 0; i < consistentPhrases.size(); ++i) {
		const ConsistentPhrase &consistentPhrase = consistentPhrases[i];
		const ConsistentRange &nonTerm = consistentPhrase.GetConsistentRange(Moses::Input);

		int start = nonTerm.GetStart();
		Node &node = m_coll[start];
		node.push_back(&nonTerm);

	}
}

Lattice::~Lattice() {
	// TODO Auto-generated destructor stub
}

