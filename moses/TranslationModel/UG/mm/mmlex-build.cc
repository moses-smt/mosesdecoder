// -*- c++ -*-
// Program to extract word cooccurrence counts from a memory-mapped word-aligned bitext
// stores the counts lexicon in the format for mm2dTable<uint32_t> (ug_mm_2d_table.h)
// (c) 2010-2012 Ulrich Germann

#include <queue>
#include <iomanip>
#include <vector>
#include <iterator>
#include <sstream>

#include <boost/program_options.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/math/distributions/binomial.hpp>

#include "moses/generic/program_options/ug_get_options.h"
// #include "ug_translation_finder.h"
// #include "ug_sorters.h"
// #include "ug_corpus_sampling.h"
#include "ug_mm_2d_table.h"
#include "ug_mm_ttrack.h"
#include  "ug_corpus_token.h"

using namespace std;
using namespace ugdiss;
using namespace boost::math;

typedef mm2dTable<id_type,id_type,uint32_t,uint32_t> LEX_t;
typedef SimpleWordId Token;

vector<uint32_t> m1; // marginals L1
vector<uint32_t> m2; // marginals L2

id_type first_rare_id=500;
vector<vector<uint32_t> >      JFREQ; // joint count table for frequent L1 words
vector<map<id_type,uint32_t> > JRARE; // joint count table for rare     L1 words

mmTtrack<Token> T1,T2;
mmTtrack<char>     Tx;
TokenIndex      V1,V2;

string bname,cfgFile,L1,L2,oname;

// DECLARATIONS 
void interpret_args(int ac, char* av[]);

void
processSentence(id_type sid)
{
  Token const* s1 = T1.sntStart(sid);
  Token const* s2 = T2.sntStart(sid);
  char const* p  = Tx.sntStart(sid);
  char const* q  = Tx.sntEnd(sid);
  ushort r,c;
  bitvector check1(T1.sntLen(sid)), check2(T2.sntLen(sid));
  check1.set();
  check2.set();
  
  // count links
  while (p < q)
    {
      p = binread(p,r);
      p = binread(p,c);
      check1.reset(r);
      check2.reset(c);
      id_type id1 = (s1+r)->id();
      if (id1 < first_rare_id) JFREQ[id1][(s2+c)->id()]++;
      else                     JRARE[id1][(s2+c)->id()]++;
    }
  
  // count unaliged words
  for (size_t i = check1.find_first(); i < check1.size(); i = check1.find_next(i))
    {
      id_type id1 = (s1+i)->id();
      if (id1 < first_rare_id) JFREQ[id1][0]++;
      else                     JRARE[id1][0]++;
    }
  for (size_t i = check2.find_first(); i < check2.size(); i = check2.find_next(i))
    JFREQ[0][(s2+i)->id()]++;
}

void
makeTable(string ofname)
{
  ofstream out(ofname.c_str());
  filepos_type idxOffset=0;
  m1.resize(max(first_rare_id,V1.getNumTokens()),0);
  m2.resize(V2.getNumTokens(),0);
  JFREQ.resize(first_rare_id,vector<uint32_t>(m2.size(),0));
  JRARE.resize(m1.size());
  for (size_t sid = 0; sid < T1.size(); ++sid)
    processSentence(sid);

  vector<id_type> index(V1.getNumTokens()+1,0);
  numwrite(out,idxOffset); // blank for the time being
  numwrite(out,id_type(m1.size()));
  numwrite(out,id_type(m2.size()));

  id_type cellCount=0;
  id_type stop = min(first_rare_id,id_type(m1.size()));
  for (id_type id1 = 0; id1 < stop; ++id1)
    {
      index[id1]  = cellCount;
      vector<uint32_t> const& v = JFREQ[id1];
      for (id_type id2 = 0; id2 < id_type(v.size()); ++id2)
        {
          if (!v[id2]) continue;
          cellCount++;
          numwrite(out,id2);
          out.write(reinterpret_cast<char const*>(&v[id2]),sizeof(uint32_t));
          m1[id1] += v[id2];
          m2[id2] += v[id2];
        }
    }
  for (id_type id1 = stop; id1 < id_type(m1.size()); ++id1)
    {
      index[id1]  = cellCount;
      map<id_type,uint32_t> const& M = JRARE[id1];
      for (map<id_type,uint32_t>::const_iterator m = M.begin(); m != M.end(); ++m)
        {
          if (m->second == 0) continue;
          cellCount++;
          numwrite(out,m->first);
          out.write(reinterpret_cast<char const*>(&m->second),sizeof(float));
          m1[id1] += m->second;
          m2[m->first] += m->second;
        }
    }
  index[m1.size()] = cellCount;
  idxOffset    = out.tellp();
  for (size_t i = 0; i < index.size(); ++i)
    numwrite(out,index[i]);
  out.write(reinterpret_cast<char const*>(&m1[0]),m1.size()*sizeof(float));
  out.write(reinterpret_cast<char const*>(&m2[0]),m2.size()*sizeof(float));
  
  // re-write the file header
  out.seekp(0);
  numwrite(out,idxOffset);
  out.close();
}

int 
main(int argc, char* argv[])
{
  interpret_args(argc,argv);
  char c = *bname.rbegin();
  if (c != '/' && c != '.') bname += '.';
  T1.open(bname+L1+".mct");
  T2.open(bname+L2+".mct");
  Tx.open(bname+L1+"-"+L2+".mam");
  V1.open(bname+L1+".tdx");
  V2.open(bname+L2+".tdx");
  makeTable(oname);
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
    ("cfg,f", po::value<string>(&cfgFile),"config file")
    ("oname,o", po::value<string>(&oname),"output file name")
    ;
  
  h.add_options()
    ("bname", po::value<string>(&bname), "base name")
    ("L1",    po::value<string>(&L1),"L1 tag")
    ("L2",    po::value<string>(&L2),"L2 tag")
    ;
  a.add("bname",1);
  a.add("L1",1);
  a.add("L2",1);
  get_options(ac,av,h.add(o),a,vm,"cfg");

  if (vm.count("help") || bname.empty() || oname.empty())
    {
      cout << "usage:\n\t" << av[0] << " <basename> <L1 tag> <L2 tag> -o <output file>\n" << endl;
      cout << o << endl;
      exit(0);
    }
}


