// build a phrase table for the given input
// #include "ug_lexical_phrase_scorer2.h"

#include <stdint.h>
#include <string>
#include <vector>
#include <cassert>
#include <iomanip>
#include <algorithm>

#include "moses/generic/sorting/VectorIndexSorter.h"
#include "moses/generic/sampling/Sampling.h"
#include "moses/generic/file_io/ug_stream.h"

#include <boost/math/distributions/binomial.hpp>
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>

#include "ug_mm_ttrack.h"
#include "ug_mm_tsa.h"
#include "tpt_tokenindex.h"
#include "ug_corpus_token.h"
#include "ug_typedefs.h"
#include "tpt_pickler.h"
#include "ug_bitext.h"
#include "ug_lexical_phrase_scorer2.h"

using namespace std;
using namespace ugdiss;
using namespace Moses;
using namespace Moses::bitext;

#define CACHING_THRESHOLD 1000
#define lbop boost::math::binomial_distribution<>::find_lower_bound_on_p 
size_t mctr=0,xctr=0;

typedef L2R_Token<SimpleWordId> Token;
typedef mmBitext<Token> mmbitext;
mmbitext bt;


float lbsmooth = .005;


PScorePfwd<Token> calc_pfwd;
PScorePbwd<Token> calc_pbwd;
PScoreLex<Token>  calc_lex;
PScoreWP<Token>   apply_wp;
vector<float> fweights;

void
nbest_phrasepairs(uint64_t const  pid1, 
		  pstats   const& ps, 
		  vector<PhrasePair> & nbest)
{
  boost::unordered_map<uint64_t,jstats>::const_iterator m;
  vector<size_t> idx(nbest.size());
  size_t i=0;
  for (m  = ps.trg.begin(); 
       m != ps.trg.end() && i < nbest.size(); 
       ++m)
    {
      // cout << m->second.rcnt() << " " << ps.good << endl;
      if ((m->second.rcnt() < 3) && (m->second.rcnt() * 100 < ps.good))
	continue;
      nbest[i].init(pid1,ps,5);
      nbest[i].update(m->first,m->second);
      calc_pfwd(bt, nbest[i]);
      calc_pbwd(bt, nbest[i]);
      calc_lex(bt, nbest[i]);
      apply_wp(bt, nbest[i]);
      nbest[i].eval(fweights);
      idx[i] = i;
      ++i;
    }
  // cout << i << " " << nbest.size() << endl;
  if (i < nbest.size()) 
    {
      // cout << "Resizing from " << nbest.size() << " to " << i << endl;
      nbest.resize(i);
      idx.resize(i);
    }
  VectorIndexSorter<PhrasePair> sorter(nbest,greater<PhrasePair>());
  if (m != ps.trg.end()) 
    {
      make_heap(idx.begin(),idx.end(),sorter);
      PhrasePair cand; 
      cand.init(pid1,ps,5);
      for (; m != ps.trg.end(); ++m)
	{
	  if ((m->second.rcnt() < 3) && (m->second.rcnt() * 100 < ps.good))
	    continue;
	  cand.update(m->first,m->second);
	  calc_pfwd(bt, cand);
	  calc_pbwd(bt, cand);
	  calc_lex(bt, cand);
	  apply_wp(bt, cand);
	  cand.eval(fweights);
	  if (cand < nbest[idx[0]]) continue;
	  pop_heap(idx.begin(),idx.end(),sorter);
	  nbest[idx.back()] = cand;
	  push_heap(idx.begin(),idx.end(),sorter);
	}
    }
  sort(nbest.begin(),nbest.end(),greater<PhrasePair>());
}
  
int main(int argc, char* argv[])
{
  // assert(argc == 4);
#if 0
  string base = argv[1];
  string L1   = argv[2];
  string L2   = argv[3];
  size_t max_samples = argc > 4 ? atoi(argv[4]) : 0;
#else
  string base = "/fs/syn5/germann/exp/sapt/crp/trn/mm/";
  string L1 = "de";
  string L2 = "en";
  size_t max_samples = argc > 1 ? atoi(argv[1]) : 1000;
#endif
  char c = *base.rbegin(); 
  if (c != '/' && c != '.') 
    base += ".";

  fweights.resize(5,.25);
  fweights[0] = 1;
  bt.open(base,L1,L2);
  bt.setDefaultSampleSize(max_samples);

  size_t i;
  i = calc_pfwd.init(0,.05);
  i = calc_pbwd.init(i,.05);
  i = calc_lex.init(i,base+L1+"-"+L2+".lex");
  i = apply_wp.init(i);

  string line;
  while (getline(cin,line))
    {
      vector<id_type> snt; 
      bt.V1->fillIdSeq(line,snt);
      for (size_t i = 0; i < snt.size(); ++i)
  	{
  	  TSA<Token>::tree_iterator m(bt.I1.get());
	  for (size_t k = i; k < snt.size() && m.extend(snt[k]); ++k)
	    bt.prep(m);
	}
      // continue;
      for (size_t i = 0; i < snt.size(); ++i)
      	{
      	  TSA<Token>::tree_iterator m(bt.I1.get());
      	  for (size_t k = i; k < snt.size() && m.extend(snt[k]); ++k)
      	    {
	      uint64_t spid = m.getPid();
      	      sptr<pstats> s = bt.lookup(m);
      	      for (size_t j = i; j <= k; ++j)
      		cout << (*bt.V1)[snt[j]] << " ";
      	      cout << s->good << "/" 
		   << s->sample_cnt << "/" 
		   << s->raw_cnt << endl;
	      // vector<PhrasePair> nbest(min(s->trg.size(),size_t(20)));
	      vector<PhrasePair> nbest(s->trg.size());
	      nbest_phrasepairs(spid, *s, nbest);
	      BOOST_FOREACH(PhrasePair const& pp, nbest)
		{
		  uint32_t sid,off,len;
		  parse_pid(pp.p2,sid,off,len);
		  uint32_t stop = off + len;
		  // cout << sid << " " << off << " " << len << endl;
		  Token const* o = bt.T2->sntStart(sid);
		  cout << "   " << setw(6) << pp.score << " ";
		  for (uint32_t i = off; i < stop; ++i)
		    cout << (*bt.V2)[o[i].id()] << " ";
		  cout << pp.joint << "/" 
		       << pp.raw1  << "/"
		       << pp.raw2  << " |";
		  BOOST_FOREACH(float f, pp.fvals) 
		    cout << " " << f;
		  cout << endl;
		}
      	    }
      	}
    }
  
    exit(0);
}

