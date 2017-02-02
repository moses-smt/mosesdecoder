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
// #include "mm/memtrack.h"
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
// typedef TSA<Token>::tree_iterator iter;
typedef Bitext<Token>::iter iter;
typedef imTtrack<Token> imttrack;
typedef imTSA<Token> imtsa;

string bname, bname1, bname2, ifile, L1, L2, Q1, Q2;
size_t maxhits;
size_t cache_size;
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

typedef ThreadSafeContainer<uint64_t, sptr<pstats> > permacache_t;

void dump(iter& m, TokenIndex& V)
{
  if (m.down())
    {
      do 
        { 
          // cout << m.str(&V) << endl;
          dump(m,V); 
        } 
      while (m.over());
      m.up();
    }
}

int main(int argc, char* argv[])
{
  typedef vector<PhrasePair<Token> > pplist_t;
  interpret_args(argc, argv);
  boost_iptr<mmbitext> Bptr(new mmbitext);
  mmbitext& B = *Bptr;// static_cast<mmbitext*>(Bptr.get());
  B.open(bname, L1, L2);
  B.V1->setDynamic(true);
  sptr<imttrack> icrp = read_input(*B.V1);
  imtsa newIdx(icrp,NULL);
  sptr<SentenceBias> bias = prime_sampling1(*B.I1, newIdx, 5000, B.sid2did());
  cerr << "primed " << endl;
  ug::ThreadPool T(1); // boost::thread::hardware_concurrency());
  TSA<Token>::tree_iterator m(&newIdx);
  // dump(m, *B.V1);
  // exit(0);
  TSA<Token>::tree_iterator r(B.I1.get());
  StatsCollector<Token> collect(Bptr, bias);
  // collect.tpool = &T;
  collect.process(m, r);

  typedef PhrasePair<Token>::SortDescendingByJointCount sorter_t;
  sorter_t sorter;
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
              sptr<pstats> stats = (*collect.lcache)[r.getPid()];
              stats->wait();
              pplist_t pplist;
              expand(r, B, *stats, pplist, NULL);
              if (pplist.empty()) continue;
              cout << "\n" << r.str(B.V1.get()) << " [" << r.ca() << "]" << endl;
              VectorIndexSorter<PhrasePair<Token>, sorter_t> viso(pplist, sorter);
              sptr<vector<size_t> > ranked = viso.GetOrder();
              size_t ctr=0;
              BOOST_FOREACH(size_t const i, *ranked)
                {
                  PhrasePair<Token> const& pp = pplist[i];
                  // if (pp.joint == 1) break;
                  cout << boost::format("   %6d %.5f | ") % pp.joint % pp.cum_bias 
                       << toString(*B.V2, pp.start2, pp.len2) 
                       << " [";
                  for (size_t d = 0; d < pp.indoc.size(); ++d)
                    {
                      if (d) cout << ":";
                      cout << pp.indoc[d];
                    }
                  cout << "]" << endl;
                  if (++ctr == 5) break;
                }
           }
        }
    }  
}


//   permacache_t permacache;
//   lru_cache::LRU_Cache<uint64_t, pstats> CACHE(10000);
  
  
//   for (size_t s = 0; s < icrp->size(); ++s)
//     {
//       size_t stop = icrp->sntLen(s);
//       Token const* t = icrp->sntStart(s);
//       cout << string(80,'-') << "\n" << toString(*B.V1, t, stop) << endl;
//       for (size_t i = 0; i < stop; ++i)
//         {
//           iter r(B.I1.get());
//           for (size_t k = i; k < stop && r.extend(t[k].id()); ++k)
//             {
//               // cerr << k << "/" << i << endl;
//               cout << "\n" << r.str(B.V1.get()) 
//                    << " [" << r.ca() << "]" << endl;
//               // sptr<pplist_t> pplist;
//               sptr<pstats> stats;

//               if (cache_size) stats = CACHE.get(r.getPid()); 

//               vector<PhrasePair<Token> > pplist;
//               if (!stats)
//                 {
//                   bitext::BitextSampler<Token> 
//                     sampler(&B, r, bias, 1000, ranked_sampling);
//                   sampler();
//                   stats = sampler.stats();
//                   if (cache_size) CACHE.set(r.getPid(), stats); 
//                 }
//               expand(r, B, *stats, pplist, NULL);
//               typedef PhrasePair<Token>::SortDescendingByJointCount sorter_t;
//               sorter_t sorter;
//               VectorIndexSorter<PhrasePair<Token>, sorter_t> viso(pplist, sorter);
//               sptr<vector<size_t> > ranked = viso.GetOrder();
//               size_t ctr=0;
//               BOOST_FOREACH(size_t const i, *ranked)
//                 {
//                   PhrasePair<Token> const& pp = pplist[i];
//                   // if (pp.joint == 1) break;
//                   cout << "   " << setw(6) << pp.joint << " " 
//                        << boost::format("%.5f ") % pp.cum_bias 
//                        << toString(*B.V2, pp.start2, pp.len2) << endl;
//                   if (++ctr == 5) break;
//                 }
//             }
//         }
//     }
//   // MemTrack::TrackListMemoryUsage();
//   // cout << pstats::s_instance_count << " instances of pstats live. "
//   //      << PhrasePair<Token>::s_instances << " instances of PhrasePair live."
//   //      << endl;
// }

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
    ("cache,C", po::value<size_t>(&cache_size)->default_value(0), 
     "cache size")
    // ("maxhits,n", po::value<size_t>(&maxhits)->default_value(25),
    // "max. number of hits")
    // ("q1", po::value<string>(&Q1), "query in L1")
    // ("q2", po::value<string>(&Q2), "query in L2")
    ;

  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name of corpus")
    ("L1", po::value<string>(&L1), "L1 tag")
    ("L2", po::value<string>(&L2), "L2 tag")
    ("ifile,i", po::value<string>(&ifile), "input file")
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
