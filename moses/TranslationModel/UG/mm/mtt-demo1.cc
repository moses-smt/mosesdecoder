// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// Demo program for use of single-track suffix arrays

#include <boost/program_options.hpp>
#include <iomanip>

#include "tpt_typedefs.h"
#include "ug_mm_ttrack.h"
#include "ug_mm_tsa.h"
#include "tpt_tokenindex.h"
#include "ug_deptree.h"
#include "ug_corpus_token.h"

using namespace Moses;
using namespace std;
using namespace boost;
using namespace sapt;
typedef L2R_Token < SimpleWordId >  Token;
int main(int argc, char* argv[])
{
  using namespace std;
  if (argc < 3)
    {
      cerr << "usage: " << argv[0] << " <track base name> lookup word sequence"
	   << endl;
    }
  string base = argv[1];
  TokenIndex V;
  V.open(base+".tdx");
  boost::shared_ptr<mmTtrack<Token> > T(new mmTtrack<Token>());
  T->open(base+".mct");
  mmTSA<Token> I; I.open(base+".sfa",T);
  mmTSA<Token>::tree_iterator m(&I);

  // look up the search string m.extend() returns true upon success
  for (int i = 2; i < argc && m.extend(V[argv[i]]); ++i);
  if (int(m.size() + 2) < argc)
    {
      cerr << "NOT FOUND" << endl;
      exit(1);
    }

  tsa::ArrayEntry e(m.lower_bound(-1));
  char const* stop = m.upper_bound(-1);
  do
    {
      m.root->readEntry(e.next,e);
      Token const* t = T->sntStart(e.sid) + e.offset;
      Token const* z = T->sntEnd(e.sid);
      for (;t != z; t = t->next()) cout << V[t->id()] << " ";
      cout << endl;
    } while (e.next != stop);

}
