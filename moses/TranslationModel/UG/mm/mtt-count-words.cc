// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// count words in a memory-mapped corpus
#include "ug_mm_ttrack.h"
#include "tpt_tokenindex.h"
#include "ug_corpus_token.h"
#include <string>
#include <vector>
#include <cassert>
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include <iomanip>
#include "ug_typedefs.h"
#include "tpt_pickler.h"
// #include "moses/TranslationModel/UG/generic/sorting/VectorIndexSorter.h"
// #include "moses/TranslationModel/UG/generic/sampling/Sampling.h"
// #include "moses/TranslationModel/UG/generic/file_io/ug_stream.h"
#include <algorithm>
#include "moses/TranslationModel/UG/generic/program_options/ug_get_options.h"

using namespace std;
using namespace sapt;
using namespace ugdiss;
using namespace Moses;
typedef L2R_Token<SimpleWordId> Token;
// typedef mmTSA<Token>::tree_iterator iter;
typedef boost::unordered_map<pair<size_t,size_t>,size_t> phrase_counter_t;

#define CACHING_THRESHOLD 1000

mmTtrack<Token> T; // token tracks
TokenIndex      V; // vocabs
// mmTSA<Token>    I; // suffix arrays

void interpret_args(int ac, char* av[]);
string bname;
bool   echo;
int main(int argc, char* argv[])
{
  interpret_args(argc,argv);
  T.open(bname+".mct");
  V.open(bname+".tdx");
  vector<size_t> cnt(V.ksize(),0);
  for (size_t sid = 0; sid < T.size(); ++sid)
    {
      Token const* stop = T.sntEnd(sid);
      for (Token const* t = T.sntStart(sid); t < stop; ++cnt[(t++)->id()]);
    }
  for (size_t wid = 2; wid < V.ksize(); ++wid)
    cout << V[wid] << " " << cnt[wid] << endl;
  exit(0);
}

void
interpret_args(int ac, char* av[])
{
  namespace po=boost::program_options;
  po::variables_map vm;
  po::options_description o("Options");
  po::options_description h("Hidden Options");
  po::positional_options_description a;

  o.add_options()
    ("help,h",    "print this message")
    ;

  h.add_options()
    ("bname", po::value<string>(&bname), "base name")
    ;
  a.add("bname",1);
  get_options(ac,av,h.add(o),a,vm);
}
