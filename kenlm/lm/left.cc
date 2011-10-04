
#include <iostream>
#include "left.hh"
#include "../../moses/src/Phrase.h"
#include "../../moses/src/ChartHypothesis.h"
#include "../../moses/src/ChartManager.h"

using namespace std;
using namespace Moses;

namespace lm {
namespace ngram {

std::vector<int> ChartState::recombCount(8,0);
std::vector<size_t> left_revisit_count(5,0);
std::vector<double> left_revisit_change(5,0);
std::vector<size_t> left_revisit_count_partial(5,0);
std::vector<double> left_revisit_change_partial(5,0);

Counters global_left_counters = Counters();
  
ChartState::~ChartState()
{
	//delete prefix;
	//delete suffix;
}

int ChartState::Compare(const ChartState &other) const {
  int comparePre = prefix->Compare(*other.prefix);
	int compareSuf = suffix->Compare(*other.suffix);

  if (hypo->GetCurrSourceRange().GetStartPos() == 0) comparePre = 0;
  if (hypo->GetCurrSourceRange().GetEndPos() >= hypo->GetManager().GetSource().GetSize() - 1) compareSuf = 0;
	
	int lres = left.Compare(other.left);
	int fres = (int)full - (int)other.full;
	int rres = right.Compare(other.right);

  if (!comparePre) {
    assert(!lres);
    assert(!fres);
  }
  if (!compareSuf) assert(!rres);

  if (lres) return lres;
  if (fres) return fres;
  if (rres) return rres;

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

  return 0;
}

void ChartState::CreatePreAndSuffices(const ChartHypothesis &hyp) 
{
  prefix = &hyp.GetPrefix();
  suffix = &hyp.GetSuffix();
  hypo = &hyp;
}

}
}
