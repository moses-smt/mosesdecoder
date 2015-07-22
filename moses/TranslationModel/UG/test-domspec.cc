// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; -*-
// test domain specificity
// Written by Ulrich Germann

#include <boost/program_options.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>
#include "mm/ug_bitext.h"
#include "mm/tpt_typedefs.h"
#include "mm/ug_prime_sampling1.h"
#include "mm/ug_bitext_sampler.h"
#include "mm/ug_phrasepair.h"
#include "mm/ug_lru_cache.h"
#include "generic/sorting/VectorIndexSorter.h"
#include "generic/sorting/NBestList.h"
#include <string>
#include <boost/unordered_map.hpp>
#include "moses/thread_safe_container.h"
#include "mm/ug_prep_phrases.h"

using namespace std;
using namespace Moses;
using namespace Moses::bitext;
namespace po=boost::program_options;
using namespace boost::algorithm;
typedef L2R_Token<SimpleWordId> Token;
typedef mmBitext<Token> mmbitext;
typedef Bitext<Token>::tsa tsa;
typedef Bitext<Token>::iter iter;
typedef imTtrack<Token> imttrack;
typedef imTSA<Token> imtsa;
typedef vector<PhrasePair<Token> > pplist_t;

string bname, bname1, bname2, ifile, L1, L2, Q1, Q2;
size_t maxhits;
size_t cache_size;
void interpret_args(int ac, char* av[]);

typedef PhrasePair<Token>::SortDescendingByJointCount sorter_t;
sorter_t sorter;

void 
show(Bitext<Token> const& B, iter const& m, pstats& stats)
{
  pplist_t pplist;
  expand(m, B, stats, pplist, NULL);
  if (pplist.empty()) return;
  cout << "\n" << m.str(B.V1.get()) << " [" << m.ca() << "]" << endl;
  VectorIndexSorter<PhrasePair<Token>, sorter_t> viso(pplist, sorter);
  sptr<vector<size_t> > ranked = viso.GetOrder();
  size_t ctr=0;
  BOOST_FOREACH(size_t const i, *ranked)
    {
      PhrasePair<Token> const& pp = pplist[i];
      cout << boost::format("   %6d | ") % pp.joint  
           << toString(*B.V2, pp.start2, pp.len2) << endl; 
      typedef map<uint32_t, uint32_t>::value_type entry_t;
      BOOST_FOREACH(entry_t const& e, pp.indoc)
        {
          cout << float(pp.joint)/pp.raw1 * stats.indoc[e.first]
               << "/" << e.second << "/" << stats.indoc[e.first] << endl;
        }
    }
}


void 
process(Bitext<Token> const* bitext, TSA<Token>::tree_iterator& m)
{
  if (m.approxOccurrenceCount() <= 5000) return;
  boost::shared_ptr<SamplingBias> nil;
  Moses::bitext::sampling_method random = Moses::bitext::random_sampling;
  Moses::bitext::BitextSampler<Token> s(bitext, m, nil, 10000, random);
  s();
  show(*bitext, m, *s.stats());
  if (m.down())
    {
      do { process(bitext, m); } while (m.over());
      m.up();
    }
}

int main(int argc, char* argv[])
{
  interpret_args(argc, argv);
  mmbitext B; 
  B.open(bname, L1, L2);
  TSA<Token>::tree_iterator m(B.I1.get());
  process(&B, m);
}

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",  "print this message")
    ;
  
  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name of corpus")
    ("L1", po::value<string>(&L1), "L1 tag")
    ("L2", po::value<string>(&L2), "L2 tag")
    ;

  h.add(o);
  po::positional_options_description a;
  a.add("bname",1);
  a.add("L1",1);
  a.add("L2",1);

  po::store(po::command_line_parser(ac,av)
            .options(h)
            .positional(a)
            .run(),vm);
  po::notify(vm);
  if (vm.count("help"))
    {
      cout << "\nusage:\n\t" << av[0]
           << " <bname> <L1> <L2>" << endl;
      cout << o << endl;
      exit(0);
    }
}
