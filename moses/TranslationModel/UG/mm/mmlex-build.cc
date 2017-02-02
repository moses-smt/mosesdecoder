// -*- c++ -*-
// Program to extract word cooccurrence counts from a memory-mapped
// word-aligned bitext stores the counts lexicon in the format for
// mm2dTable<uint32_t> (ug_mm_2d_table.h)
//
// (c) 2010-2012 Ulrich Germann

// to do: multi-threading

#include <queue>
#include <iomanip>
#include <vector>
#include <iterator>
#include <sstream>
#include <algorithm>

#include <boost/program_options.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "moses/TranslationModel/UG/generic/program_options/ug_get_options.h"
#include "moses/Util.h"
#include "ug_mm_2d_table.h"
#include "ug_mm_ttrack.h"
#include "ug_corpus_token.h"

using namespace std;
using namespace sapt;
using namespace ugdiss;
using namespace boost::math;

typedef mm2dTable<id_type,id_type,uint32_t,uint32_t> LEX_t;
typedef SimpleWordId Token;

// DECLARATIONS
void interpret_args(int ac, char* av[]);

mmTtrack<Token> T1,T2;
mmTtrack<char>     Tx;
TokenIndex      V1,V2;

typedef pair<id_type,id_type> wpair;
struct Count
{
  uint32_t a;
  uint32_t c;
  Count() : a(0), c(0) {};
  Count(uint32_t ax, uint32_t cx) : a(ax), c(cx) {}
};

bool
operator<(pair<id_type,Count> const& a,
	  pair<id_type,Count> const& b)
{
  return a.first < b.first;
}


typedef boost::unordered_map<wpair,Count> countmap_t;
typedef vector<vector<pair<id_type,Count> > > countlist_t;

vector<countlist_t> XLEX;

class Counter
{
public:
  countmap_t  CNT;
  countlist_t & LEX;
  size_t  offset;
  size_t    skip;
  Counter(countlist_t& lex, size_t o, size_t s)
    : LEX(lex), offset(o), skip(s) {}
  void processSentence(id_type sid);
  void operator()();
};

string bname,cfgFile,L1,L2,oname,cooc;
int    verbose;
size_t truncat;
size_t num_threads;

void
Counter::
operator()()
{
  for (size_t sid = offset; sid < min(truncat,T1.size()); sid += skip)
    processSentence(sid);

  LEX.resize(V1.ksize());
  for (countmap_t::const_iterator c = CNT.begin(); c != CNT.end(); ++c)
    {
      pair<id_type,Count> foo(c->first.second,c->second);
      LEX.at(c->first.first).push_back(foo);
    }
  typedef vector<pair<id_type,Count> > v_t;
  BOOST_FOREACH(v_t& v, LEX)
    sort(v.begin(),v.end());
}

struct lexsorter
{
  vector<countlist_t> const& v;
  id_type wid;
  lexsorter(vector<countlist_t> const& vx, id_type widx)
    : v(vx),wid(widx) {}
  bool operator()(pair<uint32_t,uint32_t> const& a,
		  pair<uint32_t,uint32_t> const& b) const
  {
    return (v.at(a.first).at(wid).at(a.second).first >
	    v.at(b.first).at(wid).at(b.second).first);
  }
};

void
writeTableHeader(ostream& out)
{
  filepos_type idxOffset=0;
  tpt::numwrite(out,idxOffset); // blank for the time being
  tpt::numwrite(out,id_type(V1.ksize()));
  tpt::numwrite(out,id_type(V2.ksize()));
}

void writeTable(ostream* aln_out, ostream* coc_out)
{
  vector<uint32_t> m1a(V1.ksize(),0); // marginals L1
  vector<uint32_t> m2a(V2.ksize(),0); // marginals L2
  vector<uint32_t> m1c(V1.ksize(),0); // marginals L1
  vector<uint32_t> m2c(V2.ksize(),0); // marginals L2
  vector<id_type> idxa(V1.ksize()+1,0);
  vector<id_type> idxc(V1.ksize()+1,0);
  if (aln_out) writeTableHeader(*aln_out);
  if (coc_out) writeTableHeader(*coc_out);
  size_t CellCountA=0,CellCountC=0;
  for (size_t id1 = 0; id1 < V1.ksize(); ++id1)
    {
      idxa[id1] = CellCountA;
      idxc[id1] = CellCountC;
      lexsorter sorter(XLEX,id1);
      vector<pair<uint32_t,uint32_t> > H; H.reserve(num_threads);
      for (size_t i = 0; i < num_threads; ++i)
	{
	  if (id1 < XLEX.at(i).size() && XLEX[i][id1].size())
	    H.push_back(pair<uint32_t,uint32_t>(i,0));
	}
      if (!H.size()) continue;
      make_heap(H.begin(),H.end(),sorter);
      while (H.size())
	{
	  id_type  id2 = XLEX[H[0].first][id1][H[0].second].first;
	  uint32_t aln = XLEX[H[0].first][id1][H[0].second].second.a;
	  uint32_t coc = XLEX[H[0].first][id1][H[0].second].second.c;
	  pop_heap(H.begin(),H.end(),sorter);
	  ++H.back().second;
	  if (H.back().second == XLEX[H.back().first][id1].size())
	    H.pop_back();
	  else
	    push_heap(H.begin(),H.end(),sorter);
	  while (H.size() &&
		 XLEX[H[0].first][id1].at(H[0].second).first == id2)
	    {
	      aln += XLEX[H[0].first][id1][H[0].second].second.a;
	      coc += XLEX[H[0].first][id1][H[0].second].second.c;
	      pop_heap(H.begin(),H.end(),sorter);
	      ++H.back().second;
	      if (H.back().second == XLEX[H.back().first][id1].size())
		H.pop_back();
	      else
		push_heap(H.begin(),H.end(),sorter);
	    }
	  if (aln_out)
	    {
	      ++CellCountA;
	      tpt::numwrite(*aln_out,id2);
	      tpt::numwrite(*aln_out,aln);
	      m1a[id1] += aln;
	      m2a[id2] += aln;
	    }
	  if (coc_out && coc)
	    {
	      ++CellCountC;
	      tpt::numwrite(*coc_out,id2);
	      tpt::numwrite(*coc_out,coc);
	      m1c[id1] += coc;
	      m2c[id2] += coc;
	    }
	}
    }
  idxa.back() = CellCountA;
  idxc.back() = CellCountC;
  if (aln_out)
    {
      filepos_type idxOffsetA = aln_out->tellp();
      BOOST_FOREACH(id_type foo, idxa)
	tpt::numwrite(*aln_out,foo);
      aln_out->write(reinterpret_cast<char const*>(&m1a[0]),m1a.size()*4);
      aln_out->write(reinterpret_cast<char const*>(&m2a[0]),m2a.size()*4);
      aln_out->seekp(0);
      tpt::numwrite(*aln_out,idxOffsetA);
    }
  if (coc_out)
    {
      filepos_type idxOffsetC = coc_out->tellp();
      BOOST_FOREACH(id_type foo, idxc)
	tpt::numwrite(*coc_out,foo);
      coc_out->write(reinterpret_cast<char const*>(&m1c[0]),m1c.size()*4);
      coc_out->write(reinterpret_cast<char const*>(&m2c[0]),m2c.size()*4);
      coc_out->seekp(0);
      tpt::numwrite(*coc_out,idxOffsetC);
    }
}

void
Counter::
processSentence(id_type sid)
{
  Token const* s1 = T1.sntStart(sid);
  Token const* e1 = T1.sntEnd(sid);
  Token const* s2 = T2.sntStart(sid);
  Token const* e2 = T2.sntEnd(sid);
  // vector<ushort> cnt1(V1.ksize(),0);
  // vector<ushort> cnt2(V2.ksize(),0);
  // for (Token const* x = s1; x < e1; ++x)
  // ++cnt1.at(x->id());
  // for (Token const* x = s2; x < e2; ++x)
  // ++cnt2.at(x->id());

  // boost::unordered_set<wpair> seen;
  bitvector check1(T1.sntLen(sid)); check1.set();
  bitvector check2(T2.sntLen(sid)); check2.set();

  // count links
  char const*   p = Tx.sntStart(sid);
  char const*   q = Tx.sntEnd(sid);
  ushort r,c;
  if (verbose && sid % 1000000 == 0)
    cerr << sid/1000000 << " M sentences processed" << endl;
  while (p < q)
    {
      p = tpt::binread(p,r);
      p = tpt::binread(p,c);
      // cout << sid << " " << r << "-" << c << endl;
      UTIL_THROW_IF2(r >= check1.size(), "out of bounds at line " << sid);
      UTIL_THROW_IF2(c >= check2.size(), "out of bounds at line " << sid);
      // assert(r < check1.size());
      // assert(c < check2.size());
      UTIL_THROW_IF2(s1+r >= e1, "out of bounds at line " << sid);
      UTIL_THROW_IF2(s2+c >= e2, "out of bounds at line " << sid);
      // assert(s1+r < e1);
      // assert(s2+c < e2);
      check1.reset(r);
      check2.reset(c);
      id_type id1 = (s1+r)->id();
      id_type id2 = (s2+c)->id();
      wpair k(id1,id2);
      Count& cnt = CNT[k];
      cnt.a++;
      // if (seen.insert(k).second)
      // cnt.c += cnt1[id1] * cnt2[id2];
    }
  // count unaliged words
  for (size_t i = check1.find_first();
       i < check1.size();
       i = check1.find_next(i))
    CNT[wpair((s1+i)->id(),0)].a++;
  for (size_t i = check2.find_first();
       i < check2.size();
       i = check2.find_next(i))
    CNT[wpair(0,(s2+i)->id())].a++;
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
  if (!truncat) truncat = T1.size();
  XLEX.resize(num_threads);
  vector<boost::shared_ptr<boost::thread> > workers(num_threads);
  for (size_t i = 0; i < num_threads; ++i)
    workers[i].reset(new boost::thread(Counter(XLEX[i],i,num_threads)));
  for (size_t i = 0; i < workers.size(); ++i)
    workers[i]->join();
  // cerr << "done counting" << endl;
  ofstream aln_out,coc_out;
  if (oname.size()) aln_out.open(oname.c_str());
  // if (cooc.size())  coc_out.open(cooc.c_str());
  writeTable(oname.size() ? &aln_out : NULL,
	     cooc.size()  ? &coc_out : NULL);
  if (oname.size()) aln_out.close();
  // if (cooc.size())  coc_out.close();
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
    // ("cooc,c", po::value<string>(&cooc),
    // "file name for raw co-occurrence counts")
    ("verbose,v", po::value<int>(&verbose)->default_value(0)->implicit_value(1),
     "verbosity level")
    ("threads,t", po::value<size_t>(&num_threads)->default_value(4),
     "count in <N> parallel threads")
    ("truncate,n", po::value<size_t>(&truncat)->default_value(0),
     "truncate corpus to <N> sentences (for debugging)")
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
  size_t num_cores = boost::thread::hardware_concurrency();
  num_threads = min(num_threads,num_cores);
}


