// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; -*-
// test domain specificity
// Written by Ulrich Germann

#include <boost/program_options.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>
#include <boost/math/distributions/binomial.hpp>

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
  size_t cumul=0;
  BOOST_FOREACH(size_t const i, *ranked)
    {
      typedef map<uint32_t, uint32_t>::value_type entry_t;

      PhrasePair<Token> const& pp = pplist[i];
      if (pp.joint < pp.good1 * .01) break;
      size_t remarkable = 0;
      float p = float(pp.joint)/pp.good1;
      BOOST_FOREACH(entry_t const& e, pp.indoc)
        {
          boost::math::binomial binomi(stats.indoc[e.first], p);
          float x = boost::math::cdf(binomi, e.second);
          float y = boost::math::cdf(boost::math::complement(binomi, e.second-1));
          if ((x > .01 && y > .01) || e.second < 5) continue;
          remarkable += e.second;
          // cout <<  p * stats.indoc[e.first]
          //      << "/" << e.second << "/" << stats.indoc[e.first] 
          //      << " " << boost::math::cdf(binomi, e.second)
          //      << " " << boost::math::cdf(boost::math::complement(binomi, e.second-1))
          //      << " " << toString(*B.V2, pp.start2, pp.len2) 
          //      << endl;
        }
      if (remarkable*20 > pp.good1)
        {
          cout << boost::format("   %6d | ") % pp.joint  
               << toString(*B.V2, pp.start2, pp.len2) 
               << boost::format(" (%d: %.2f)") % cumul % (float(cumul)/pp.good1)
               << endl; 
          BOOST_FOREACH(entry_t const& e, pp.indoc)
            {
              boost::math::binomial binomi(stats.indoc[e.first], p);
              float x = boost::math::cdf(binomi, e.second);
              float y = boost::math::cdf(boost::math::complement(binomi, e.second-1));
              if ((x > .001 && y > .001) || e.second < 20) continue;
              cout <<  p * stats.indoc[e.first]
                   << "/" << e.second << "/" << stats.indoc[e.first] 
                   << " " << boost::math::cdf(binomi, e.second)
                   << " " << boost::math::cdf(boost::math::complement
                                              (binomi, e.second-1))
                   << " " << toString(*B.V2, pp.start2, pp.len2) 
                   << endl;
            }
        }
    }
}


void 
process(SPTR<Bitext<Token> const> const& bitext, TSA<Token>::tree_iterator& m)
{
  static boost::shared_ptr<SamplingBias> nil(new SamplingBiasAlways(bitext->sid2did()));
  static Moses::bitext::sampling_method random = Moses::bitext::random_sampling;
  // if (m.down())
  if (m.extend((*bitext->V1)["job"]))
    {
      do 
        { 
          if (m.ca() >= 5000) 
            {
              // cout << m.str(bitext->V1.get()) << " [" << m.ca() << "]" << endl;
              Moses::bitext::BitextSampler<Token> s(bitext, m, nil, 10000, random);
              s();
              show(*bitext, m, *s.stats());
              process(bitext, m); 
            } 
        } 
      while (m.over());
      m.up();
    }
}

int main(int argc, char* argv[])
{
  interpret_args(argc, argv);
  SPTR<mmbitext> B(new mmbitext); 
  B->open(bname, L1, L2);
  TSA<Token>::tree_iterator m(B->I1.get());
  // m.extend((*B.V1)["job"]);
  process(B.get(), m);
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
