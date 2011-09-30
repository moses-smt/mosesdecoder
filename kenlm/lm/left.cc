
#include <iostream>
#include "left.hh"
#include "../../moses/src/Phrase.h"
#include "../../moses/src/ChartHypothesis.h"

using namespace std;
using namespace Moses;

namespace lm {
namespace ngram {

std::vector<int> ChartState::recombCount(8,0);
  
ChartState::~ChartState()
{
	//delete prefix;
	//delete suffix;
}

int ChartState::Compare(const ChartState &other) const {
	assert(prefix);
	assert(suffix);
	assert(prefix->GetSize() > 0);
	assert(suffix->GetSize() > 0);
	
	int lres = left.Compare(other.left);
	if (lres) return lres;
	int rres = right.Compare(other.right);
	if (rres) return rres;
	int ret = (int)full - (int)other.full;

	if (lres == 0 && rres != 0)
	    recombCount[4]++;
	else if (lres == 0 && rres == 0)
	    recombCount[5]++;
	else if (lres != 0 && rres == 0)
	    recombCount[6]++;
	else
	{
	    assert(lres);
	    assert(rres);
	    recombCount[7]++;
	}

	if (ret == 0)
	{
		int comparePre = prefix->Compare(*other.prefix);
		int compareSuf = suffix->Compare(*other.suffix);

    if (comparePre)
    {
      if (compareSuf)
        recombCount[1]++; // both. middle
      else
        recombCount[0]++; // only prefix. left
    }
    else if (compareSuf)
      recombCount[2]++; // only suffix. right
    else
      recombCount[3]++; // nothing. last
    
    /*
		cerr << "recomb_stats " << comparePre << " " << compareSuf << endl
				<< *prefix << " ||| " << *other.prefix << endl
				<< *suffix << " ||| " << *other.suffix << endl;
     */
	}
	
	return ret;
}

void ChartState::CreatePreAndSuffices(const ChartHypothesis &hypo) const 
{
  prefix = &hypo.GetPrefix();
  suffix = &hypo.GetSuffix();
}

}
}
