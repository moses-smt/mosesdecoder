
#include <iostream>
#include "left.hh"
#include "../../moses/src/Phrase.h"
#include "../../moses/src/ChartHypothesis.h"

using namespace std;
using namespace Moses;

namespace lm {
namespace ngram {

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
	
	if (ret == 0)
	{
		int comparePre = prefix->Compare(*other.prefix);
		int compareSuf = suffix->Compare(*other.suffix);
		cerr << "recomb_stats " << comparePre << " " << compareSuf << endl
				<< *prefix << " ||| " << *other.prefix << endl
				<< *suffix << " ||| " << *other.suffix << endl;
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
