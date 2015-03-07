#include "mm/ug_bitext.h"
#include <boost/format.hpp>

using namespace std;
using namespace Moses;
using namespace ugdiss;

typedef L2R_Token<SimpleWordId> Token;
typedef mmTtrack<Token> ttrack_t;
typedef mmTSA<Token>     tsa_t;

void 
getoccs(tsa_t::tree_iterator const& m, 
	vector<size_t>& occs)
{
  occs.clear();
  occs.reserve(m.approxOccurrenceCount()+10);
  tsa::ArrayEntry I(m.lower_bound(-1));
  char const* stop = m.upper_bound(-1);
  do {
    m.root->readEntry(I.next,I);
    occs.push_back(I.sid);
  } while (I.next != stop);
  sort(occs.begin(), occs.end());
}

int main(int argc, char* argv[])
{
  string L = argv[1];

  TokenIndex V;
  boost::shared_ptr<ttrack_t> T(new ttrack_t());
  tsa_t I;
  
  V.open(L + ".tdx");
  T->open(L + ".mct");
  I.open(L + ".sfa", T);

  string line;
  while (getline(cin,line)) {
      vector<id_type> snt;
      V.fillIdSeq(line,snt);

      vector<size_t> occs;

      tsa_t::tree_iterator m(&I);
      for (size_t k = 0; k < snt.size() && m.extend(snt[k]); ++k);

      getoccs(m,occs);
      cout << occs.size() << endl;
      cout << occs[0] << endl;
  }
}

