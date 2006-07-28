#ifndef _SENTENCE_STATS_H_
#define _SENTENCE_STATS_H_

#include <iostream>

struct SentenceStats
{
	SentenceStats() : numRecombinations(0), numPruned(0) {};
	unsigned int numRecombinations;
	unsigned int numPruned;

	void ZeroAll() { numRecombinations = 0; numPruned = 0; }
};

inline std::ostream& operator<<(std::ostream& os, const SentenceStats& ss)
{
  return os << "number of hypotheses recombined=" << ss.numRecombinations << std::endl
						<< "  \"           \"          pruned=" << ss.numPruned << std::endl;
}

#endif
