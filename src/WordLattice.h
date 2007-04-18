#ifndef WORDLATTICE_H_
#define WORDLATTICE_H_

#include <vector>
#include "ConfusionNet.h"

/** General word lattice */
class WordLattice: public ConfusionNet {
private:
	std::vector<std::vector<size_t> > next_nodes;
		 
public:
	WordLattice();
	size_t GetColumnIncrement(size_t i, size_t j) const;
	void Print(std::ostream&) const;
	int Read(std::istream& in,const std::vector<FactorType>& factorOrder);
};

#endif
