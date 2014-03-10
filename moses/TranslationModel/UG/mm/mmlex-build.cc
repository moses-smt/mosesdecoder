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
#include <boost/unordered_map.hpp> 
#include <boost/unordered_set.hpp> 

#include "moses/TranslationModel/UG/generic/program_options/ug_get_options.h"
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

id_type first_rare_id=500;
vector<vector<uint32_t> >      JFREQ; // joint count table for frequent L1 words
vector<map<id_type,uint32_t> > JRARE; // joint count table for rare     L1 words
vector<vector<uint32_t> >      CFREQ; // cooc count table for frequent L1 words
vector<map<id_type,uint32_t> > CRARE; // cooc count table for rare     L1 words

mmTtrack<Token> T1,T2;
mmTtrack<char>     Tx;
TokenIndex      V1,V2;

string bname,cfgFile,L1,L2,oname,cooc;

// DECLARATIONS 
void interpret_args(int ac, char* av[]);

void
processSentence(id_type sid)
{
  Token const* s1 = T1.sntStart(sid);
  Token const* e1 = T1.sntEnd(sid);
  Token const* s2 = T2.sntStart(sid);
  Token const* e2 = T2.sntEnd(sid);
  char const* p  = Tx.sntStart(sid);
  char const* q  = Tx.sntEnd(sid);
  ushort r,c;
  bitvector check1(T1.sntLen(sid)), check2(T2.sntLen(sid));
  check1.set();
  check2.set();
  vector<ushort> cnt1(V1.ksize(),0);
  vector<ushort> cnt2(V2.ksize(),0);
  boost::unordered_set<pair<id_type,id_type> > mycoocs;

  for (Token const* x = s1; x < e1; ++x) ++cnt1[x->id()];
  for (Token const* x = s2; x < e2; ++x) ++cnt2[x->id()];

  // count links
  while (p < q)
    {
      p = binread(p,r);
      p = binread(p,c);
      check1.reset(r);
      check2.reset(c);
      id_type id1 = (s1+r)->id();
      id_type id2 = (s2+c)->id();
      if (id1 < first_rare_id) 
	JFREQ[id1][id2]++;
      else                     
	JRARE[id1][id2]++;
      if (cooc.size())
	mycoocs.insert(pair<id_type,id_type>(id1,id2));
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

  if (cooc.size())
    {
      typedef boost::unordered_set<pair<id_type,id_type> >::iterator iter;
      for (iter m = mycoocs.begin(); m != mycoocs.end(); ++m)
	if (m->first < first_rare_id)
	  CFREQ[m->first][m->second] += cnt1[m->first] * cnt2[m->second];
	else
	  CRARE[m->first][m->second] += cnt1[m->first] * cnt2[m->second];
    }
}

// void
// count_coocs(id_type sid)
// {
//   Token const* s1 = T1.sntStart(sid);
//   Token const* e1 = T1.sntEnd(sid);

//   Token const* s2 = T2.sntStart(sid);
//   Token const* e2 = T2.sntEnd(sid);

//   for (Token const* x = s1; x < e1; ++x)
//     {
//       if (x->id() < first_rare_id)
// 	{
// 	  vector<uint32_t>& v = CFREQ[x->id()];
// 	  for (Token const* y = s2; y < e2; ++y) 
// 	    ++v[y->id()];
// 	}
//       else
// 	{
// 	  map<id_type,uint32_t>& m = CRARE[x->id()]; 
// 	  for (Token const* y = s2; y < e2; ++y) 
// 	    ++m[y->id()];
// 	}
//     }
// }


void
writeTable(string ofname, vector<vector<uint32_t> >& FREQ,
	   vector<map<id_type,uint32_t> >& RARE)
{
  ofstream out(ofname.c_str());
  filepos_type idxOffset=0;

  vector<uint32_t> m1; // marginals L1
  vector<uint32_t> m2; // marginals L2
  m1.resize(max(first_rare_id,V1.getNumTokens()),0);
  m2.resize(V2.getNumTokens(),0);
  vector<id_type> index(V1.getNumTokens()+1,0);
  numwrite(out,idxOffset); // blank for the time being
  numwrite(out,id_type(m1.size()));
  numwrite(out,id_type(m2.size()));

  id_type cellCount=0;
  id_type stop = min(first_rare_id,id_type(m1.size()));
  for (id_type id1 = 0; id1 < stop; ++id1)
    {
      index[id1]  = cellCount;
      vector<uint32_t> const& v = FREQ[id1];
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
      map<id_type,uint32_t> const& M = RARE[id1];
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

  JFREQ.resize(first_rare_id,vector<uint32_t>(V2.ksize(),0));
  JRARE.resize(V1.ksize());

  CFREQ.resize(first_rare_id,vector<uint32_t>(V2.ksize(),0));
  CRARE.resize(V1.ksize());

  for (size_t sid = 0; sid < T1.size(); ++sid)
    {
      if (sid%10000 == 0) cerr << sid << endl; 
      processSentence(sid);
    }
  
  if (oname.size()) writeTable(oname,JFREQ,JRARE);
  if (cooc.size())  writeTable(cooc,CFREQ,CRARE);
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
    ("cooc,c", po::value<string>(&cooc),
     "file name for raw co-occurrence counts")
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

  if (vm.count("help") || bname.empty() || (oname.empty() && cooc.empty()))
    {
      cout << "usage:\n\t" << av[0] << " <basename> <L1 tag> <L2 tag> [-o <output file>] [-c <output file>]\n" << endl;
      cout << "at least one of -o / -c must be specified." << endl;
      cout << o << endl;
      exit(0);
    }
}


