// build a phrase table for the given input
#include "ug_mm_ttrack.h"
#include "ug_mm_tsa.h"
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
#include "moses/generic/sorting/VectorIndexSorter.h"
#include "moses/generic/sampling/Sampling.h"
#include "moses/generic/file_io/ug_stream.h"
#include <algorithm>
#include "moses/generic/program_options/ug_get_options.h"

using namespace std;
using namespace ugdiss;
using namespace Moses;
typedef L2R_Token<SimpleWordId> Token;
typedef mmTSA<Token>::tree_iterator iter;
typedef boost::unordered_map<pair<size_t,size_t>,size_t> phrase_counter_t;

#define CACHING_THRESHOLD 1000

mmTtrack<Token> T; // token tracks
TokenIndex      V; // vocabs
mmTSA<Token>    I; // suffix arrays

void interpret_args(int ac, char* av[]);
string bname;
bool   echo;
int main(int argc, char* argv[])
{
  interpret_args(argc,argv);
  
  T.open(bname+".mct");
  V.open(bname+".tdx"); V.iniReverseIndex();
  I.open(bname+".sfa",&T);
  string line;
  while (getline(cin,line))
    {
      vector<id_type> phr; 
      V.fillIdSeq(line,phr);
      TSA<Token>::tree_iterator m(&I);
      size_t i = 0;
      while (i < phr.size() && m.extend(phr[i])) ++i;
      if (echo) cout << line << ": ";
      if (i < phr.size()) cout << 0 << endl;
      else                cout << m.rawCnt() << endl;
    }
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
    ("echo,e", po::bool_switch(&echo), "repeat lookup phrases")
    ;
  
  h.add_options()
    ("bname", po::value<string>(&bname), "base name")
    ;
  a.add("bname",1);
  get_options(ac,av,h.add(o),a,vm);
}
