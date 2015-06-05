// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; -*-
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

using namespace std;
using namespace Moses;
using namespace Moses::bitext;
namespace po=boost::program_options;
using namespace boost::algorithm;
typedef L2R_Token<SimpleWordId> Token;
typedef mmBitext<Token> mmbitext;
typedef Bitext<Token>::tsa tsa;
// typedef TSA<Token>::tree_iterator iter;
typedef Bitext<Token>::iter iter;
typedef imTtrack<Token> imttrack;
typedef imTSA<Token> imtsa;

string bname, bname1, bname2, ifile, L1, L2, Q1, Q2;
size_t maxhits;
void interpret_args(int ac, char* av[]);

sptr<imttrack>
read_input(TokenIndex& V)
{
  sptr<vector<vector<Token> > > crp(new vector<vector<Token> >);
  crp->reserve(1000);
  string line;
  istream* in = &cin;
  ifstream inputfile;
  if (ifile.size()) 
    {
      inputfile.open(ifile.c_str());
      in = & inputfile;
    }
  while (getline(*in,line)) 
    {
      crp->push_back(vector<Token>());
      fill_token_seq(V, line, crp->back());
    }
  sptr<imttrack> ret(new imttrack (crp));
  return ret;
}

int main(int argc, char* argv[])
{
  interpret_args(argc, argv);
  iptr<mmbitext> Bptr(new mmbitext); 
  mmbitext& B = *Bptr;
  B.open(bname, L1, L2);
  sptr<imttrack> icrp = read_input(*B.V1);
  imtsa newIdx(icrp,NULL);
  sptr<SentenceBias> bias = prime_sampling1(*B.I1, newIdx, 5000);
  cerr << "primed" << endl;
  typedef vector<PhrasePair<Token> > pplist_t;
#define WITH_CACHE 1
#if WITH_CACHE
  // map<uint64_t, sptr<pstats const> > CACHE;
  lru_cache::LRU_Cache<uint64_t, pstats> CACHE(1000);
#endif
  for (size_t s = 0; s < icrp->size(); ++s)
    {
      size_t stop = icrp->sntLen(s);
      Token const* t = icrp->sntStart(s);
      cout << string(80,'-') << "\n" << toString(*B.V1, t, stop) << endl;
      for (size_t i = 0; i < stop; ++i)
        {
          iter r(B.I1.get());
          for (size_t k = i; k < stop && r.extend(t[k].id()); ++k)
            {
              // cerr << k << "/" << i << endl;
              cout << "\n" << r.str(B.V1.get()) 
                   << " [" << r.ca() << "]" << endl;
              // sptr<pplist_t> pplist;
              sptr<pstats> stats;
#if WITH_CACHE
              // if (r.ca() > 1000) pplist = CACHE.get(r.getPid()); 
              stats = CACHE.get(r.getPid()); 
#endif
              vector<PhrasePair<Token> > pplist;
              if (!stats)
                {
                  bitext::BitextSampler<Token> 
                    sampler(&B, r, bias, 1000, ranked_sampling);
                  sampler();
                  stats = sampler.stats();
                  // pplist->resize(pplist->size());
#if WITH_CACHE

                  // if (r.ca() > 1000) 
                  CACHE.set(r.getPid(), stats); 
#endif
                }
              expand(r, B, *stats, pplist, NULL);
              cout << pplist.size() << " " << sizeof(PhrasePair<Token>) << "; "
                // << pstats::s_instance_count << " instances of pstats live. "
                // << PhrasePair<Token>::s_instances << " instances of PhrasePair live."
                   << endl;
              // BOOST_FOREACH(PhrasePair<Token> const& pp, *pplist)
              //   {
              //     if (pp.joint == 1) continue;
              //     cout << "   " << setw(6) << pp.joint << " " 
              //          << toString(*B.V2, pp.start2, pp.len2) << endl;
              //   }
            }
        }
    }
  // cout << pstats::s_instance_count << " instances of pstats live. "
  //      << PhrasePair<Token>::s_instances << " instances of PhrasePair live."
  //      << endl;
}

  // vector<float> score(hits->size());
  // VectorIndexSorter<float> sorter(score);
  // for (size_t s = 0; s < icrp->size(); ++s)
  //   {
  //     size_t stop = icrp->sntLen(s);
  //     Token const* t = icrp->sntStart(s);
  //     cout << string(80,'-') << "\n" << toString(V1, t, stop) << endl;
  //     for (size_t i = 0; i < stop; ++i)
  //       {
  //         TSA<Token>::tree_iterator r(&I1);
  //         for (size_t k = i; k < stop && r.extend(t[k].id()); ++k)
  //           {
  //             if (r.ca() < 3) continue;
  //             cout << "\n" << r.str(&V1) << " " << int(r.ca()) << endl;
  //             if (r.ca() > 10000) continue;
  //             sptr<NBestList<uint32_t, VectorIndexSorter<float> > > top;
  //             top = nbest(r, *hits, score, sorter, 5);
  //             for (size_t n = 0; n < top->size(); ++n)
  //               {
  //                 cout << "[" << n << ": " << score[(*top)[n]] 
  //                      << " (" << (*hits)[(*top)[n]] << "/" << C1->sntLen((*top)[n]) << ")]\n"
  //                      << toString(V1, C1->sntStart((*top)[n]), C1->sntLen((*top)[n])) << "\n"; 
  //                 if (C2) cout << toString(V2, C2->sntStart((*top)[n]), C2->sntLen((*top)[n])) << "\n"; 
  //                 cout << endl;
  //               }
  //           }
  //       }
      
  //   }
  //}

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()

    ("help,h",  "print this message")
    ("maxhits,n", po::value<size_t>(&maxhits)->default_value(25),
     "max. number of hits")
    ("q1", po::value<string>(&Q1), "query in L1")
    ("q2", po::value<string>(&Q2), "query in L2")
    ;

  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name of corpus")
    ("L1", po::value<string>(&L1), "L1 tag")
    ("L2", po::value<string>(&L2), "L2 tag")
    ("ifile", po::value<string>(&ifile), "input file")
    ;

  h.add(o);
  po::positional_options_description a;
  a.add("bname",1);
  a.add("L1",1);
  a.add("L2",1);
  a.add("ifile",1);

  po::store(po::command_line_parser(ac,av)
            .options(h)
            .positional(a)
            .run(),vm);
  po::notify(vm);
  if (vm.count("help"))
    {
      cout << "\nusage:\n\t" << av[0]
           << " [options] [--q1=<L1string>] [--q2=<L2string>]" << endl;
      cout << o << endl;
      exit(0);
    }
}
