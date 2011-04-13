#ifndef moses_WordLattice_h
#define moses_WordLattice_h

#include <vector>
#include "ConfusionNet.h"

namespace Moses
{

/** General word lattice */
class WordLattice: public ConfusionNet {
private:
	std::vector<std::vector<size_t> > next_nodes;
	std::vector<std::vector<int> > distances;
		 
public:
	WordLattice();
	size_t GetColumnIncrement(size_t ic, size_t j) const;
	void Print(std::ostream&) const;
	/** Get shortest path between two nodes
	 */
	virtual int ComputeDistortionDistance(const WordsRange& prev, const WordsRange& current) const;
	// is it possible to get from the edge of the previous word range to the current word range
	virtual bool CanIGetFromAToB(size_t start, size_t end) const;
	
	int Read(std::istream& in,const std::vector<FactorType>& factorOrder);

	/** Convert internal representation into an edge matrix
	 * @note edges[1][2] means there is an edge from 1 to 2
	 */
	void GetAsEdgeMatrix(std::vector<std::vector<bool> >& edges) const;
	
    const NonTerminalSet &GetLabelSet(size_t /*startPos*/, size_t /*endPos*/) const
	{
		assert(false);
		return *(new NonTerminalSet());
	}
	
};

}

#endif
