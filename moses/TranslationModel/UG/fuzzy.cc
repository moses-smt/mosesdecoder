// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; -*-
#include <boost/program_options.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>
#include "mm/ug_bitext.h"
#include "mm/tpt_typedefs.h"
#include "mm/ug_prime_sampling1.h"
#include "generic/sorting/VectorIndexSorter.h"
#include "generic/sorting/NBestList.h"
#include <string>

using namespace std;
using namespace Moses;
using namespace Moses::bitext;
namespace po=boost::program_options;
using namespace boost::algorithm;
typedef L2R_Token<SimpleWordId> Token;
typedef mmBitext<Token> mmbitext;
typedef Bitext<Token>::tsa tsa;
typedef imTtrack<Token> imttrack;
typedef imTSA<Token> imtsa;

string bname, bname1, bname2, L1, L2, Q1, Q2;
size_t maxhits;
void interpret_args(int ac, char* av[]);

TokenIndex V1; 
TokenIndex V2; 
sptr<mmTtrack<Token> > C1;
sptr<mmTtrack<Token> > C2;
mmTSA<Token> I1; 

void 
open_bitext()
{
  C1.reset(new mmTtrack<Token>); 
  if (L2.size())
    {
      bname1 = bname + L1 + ".";
      bname2 = bname + L2 + ".";
    }
  else if (L1.size())
    {
      bname1 = bname;
      bname2 = L1;
    }
  else bname1 = bname;
  
  if (bname2.size()) C2.reset(new mmTtrack<Token>); 
  
  C1->open(bname1+"mct");
  I1.open(bname1+"sfa", C1);
  V1.open(bname1+"tdx");
  V1.setDynamic(true);
  
  if (bname2.size())
    {
      C2->open(bname2+"mct");
      V2.open(bname2+"tdx");
    }

}

sptr<imttrack>
read_input()
{
  sptr<vector<vector<Token> > > crp(new vector<vector<Token> >);
  crp->reserve(1000);
  string line;
  while (getline(cin,line)) 
    {
      crp->push_back(vector<Token>());
      fill_token_seq(V1, line, crp->back());
    }
  sptr<imttrack> ret(new imttrack (crp));
  return ret;
}

sptr<NBestList<uint32_t, VectorIndexSorter<float> > > 
nbest(TSA<Token>::tree_iterator const& r, vector<float> const& hits, 
      vector<float>& score, VectorIndexSorter<float>& sorter,
      size_t const nbest_size)
{
  typedef NBestList<uint32_t, VectorIndexSorter<float> > nbest_list_t;
  sptr<nbest_list_t> ret(new nbest_list_t(nbest_size, sorter));
  bitvector mycheck(hits.size());
  tsa::ArrayEntry I(r.lower_bound(-1));
  char const* stop = r.upper_bound(-1);
  while (I.next < stop) 
    {
      r.root->readEntry(I.next,I);
      if (mycheck[I.sid]) continue;
      score[I.sid] = hits[I.sid] / r.root->getCorpus()->sntLen(I.sid);
      ret->add(I.sid);
      mycheck.set(I.sid);
    }
  return ret;
}

int main(int argc, char* argv[])
{
  interpret_args(argc, argv);
  open_bitext(); 
  sptr<imttrack> icrp = read_input();
  imtsa newIdx(icrp,NULL);
  sptr<SentenceBias> hits = prime_sampling1(I1, newIdx, 1000);
  vector<float> score(hits->size());
  VectorIndexSorter<float> sorter(score);
  for (size_t s = 0; s < icrp->size(); ++s)
    {
      size_t stop = icrp->sntLen(s);
      Token const* t = icrp->sntStart(s);
      cout << string(80,'-') << "\n" << toString(V1, t, stop) << endl;
      for (size_t i = 0; i < stop; ++i)
        {
          TSA<Token>::tree_iterator r(&I1);
          for (size_t k = i; k < stop && r.extend(t[k].id()); ++k)
            {
              if (r.ca() < 3) continue;
              cout << "\n" << r.str(&V1) << " " << int(r.ca()) << endl;
              if (r.ca() > 10000) continue;
              sptr<NBestList<uint32_t, VectorIndexSorter<float> > > top;
              top = nbest(r, *hits, score, sorter, 5);
              for (size_t n = 0; n < top->size(); ++n)
                {
                  cout << "[" << n << ": " << score[(*top)[n]] 
                       << " (" << (*hits)[(*top)[n]] << "/" << C1->sntLen((*top)[n]) << ")]\n"
                       << toString(V1, C1->sntStart((*top)[n]), C1->sntLen((*top)[n])) << "\n"; 
                  if (C2) cout << toString(V2, C2->sntStart((*top)[n]), C2->sntLen((*top)[n])) << "\n"; 
                  cout << endl;
                }
            }
        }
      
    }
}

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
           << " [options] [--q1=<L1string>] [--q2=<L2string>]" << endl;
      cout << o << endl;
      exit(0);
    }
}
