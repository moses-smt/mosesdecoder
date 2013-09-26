//-*- c++ -*-

#ifndef __ug_bitext_h
#define __ug_bitext_h
// Implementations of word-aligned bitext.
// Written by Ulrich Germann
// 
// mmBitext: static, memory-mapped bitext
// imBitext: dynamic, in-memory bitext
//

// things we can do to speed up things:
// - set up threads at startup time that force the 
//   data in to memory sequentially
//
// - use multiple agendas for better load balancing and to avoid 
//   competition for locks

#include <string>
#include <vector>
#include <cassert>
#include <iomanip>
#include <algorithm>

#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>

#include "moses/generic/sorting/VectorIndexSorter.h"
#include "moses/generic/sampling/Sampling.h"
#include "moses/generic/file_io/ug_stream.h"
#include "moses/Util.h"

#include "util/exception.hh"
#include "util/check.hh"

#include "ug_typedefs.h"
#include "ug_mm_ttrack.h"
#include "ug_im_ttrack.h"
#include "ug_mm_tsa.h"
#include "ug_im_tsa.h"
#include "tpt_tokenindex.h"
#include "ug_corpus_token.h"
#include "tpt_pickler.h"
#include "ug_lexical_phrase_scorer2.h"

using namespace ugdiss;
using namespace std;
namespace Moses {

  namespace bitext
  {
    using namespace ugdiss;

    template<typename TKN> class Bitext;

    template<typename sid_t, typename off_t, typename len_t>
    void 
    parse_pid(uint64_t const pid, sid_t & sid, 
	      off_t & off, len_t& len)
    {
      static uint64_t two32 = uint64_t(1)<<32;
      static uint64_t two16 = uint64_t(1)<<16;
      len = pid%two16;
      off = (pid%two32)>>16;
      sid = pid>>32;
    }

    float 
    lbop(size_t const tries, size_t const succ, 
	 float const confidence);

    // "joint" (i.e., phrase pair) statistics
    class
    jstats
    {
      boost::mutex lock;
      uint32_t my_rcnt; // unweighted count
      float    my_wcnt; // weighted count 
      uint32_t my_cnt2;
      vector<pair<size_t, vector<uchar> > > my_aln; 
    public:
      jstats();
      jstats(jstats const& other);
      uint32_t rcnt() const;
      uint32_t cnt2() const; // raw target phrase occurrence count
      float    wcnt() const;
      
      vector<pair<size_t, vector<uchar> > > const & aln() const;
      void add(float w, vector<uchar> const& a, uint32_t const cnt2);
    };

    struct 
    pstats
    {
      boost::mutex lock;               // for parallel gathering of stats
      boost::condition_variable ready; // consumers can wait for this data structure to be ready.
      
      size_t raw_cnt;    // (approximate) raw occurrence count 
      size_t sample_cnt; // number of instances selected during sampling
      size_t good;       // number of selected instances with valid word alignments
      size_t sum_pairs;
      size_t in_progress; // keeps track of how many threads are currently working on this
      typename boost::unordered_map<uint64_t, jstats> trg;
      pstats(); 
      void release();
      void register_worker();
      size_t count_workers() { return in_progress; } 

      void add(uint64_t const pid, float const w, 
	       vector<uchar> const& a, uint32_t const cnt2);
    };
    
    class 
    PhrasePair
    {
    public:
      uint64_t p1, p2;
      uint32_t raw1,raw2,sample1,sample2,good1,good2,joint;
      uint32_t mono,swap,left,right;
      vector<float> fvals;
      vector<uchar> aln;
      // float    avlex12,avlex21; // average lexical probs (Moses std)
      // float    znlex1,znlex2;   // zens-ney lexical smoothing
      // float    colex1,colex2;   // based on raw lexical occurrences
      float score;
      PhrasePair();
      bool operator<(PhrasePair const& other) const;
      bool operator>(PhrasePair const& other) const;
      void init(uint64_t const pid1, pstats const& ps, 
		size_t const numfeats);
      void update(uint64_t const pid2, jstats const& js);
      float eval(vector<float> const& w);
    };

    template<typename Token>
    class
    PhraseScorer
    {
    protected:
      int index;
      int num_feats;
    public:
      virtual 
 
      void 
      operator()(Bitext<Token> const& pt, PhrasePair& pp) const = 0;

      int 
      fcnt() const { return num_feats; }
    };

    template<typename Token>
    class
    PScorePfwd : public PhraseScorer<Token>
    {
      float conf;
    public:
      PScorePfwd() 
      {
	this->num_feats = 1;
      }

      int 
      init(int const i, float const c) 
      { 
	conf = c; 
	this->index = i;
	return i + this->num_feats;
      }

      void 
      operator()(Bitext<Token> const& bt, PhrasePair& pp) const
      {
	if (pp.joint > pp.good1) 
	  {
	    cerr << bt.toString(pp.p1,0) << " ::: " << bt.toString(pp.p2,1) << endl;
	    cerr << pp.joint << "/" << pp.good1 << "/" << pp.raw2 << endl;
	  }
	pp.fvals[this->index] = log(lbop(pp.good1, pp.joint, conf));
      }
    };

    template<typename Token>
    class
    PScorePbwd : public PhraseScorer<Token>
    {
      float conf;
    public:
      PScorePbwd() 
      {
	this->num_feats = 1;
      }

      int 
      init(int const i, float const c) 
      { 
	conf = c; 
	this->index = i;
	return i + this->num_feats;
      }

      void 
      operator()(Bitext<Token> const& pt, PhrasePair& pp) const
      {
	pp.fvals[this->index] = log(lbop(max(pp.raw2,pp.joint), pp.joint, conf));
      }
    };

    template<typename Token>
    class
    PScoreLex : public PhraseScorer<Token>
    {
      LexicalPhraseScorer2<Token> scorer;
    public:

      PScoreLex() { this->num_feats = 2; }

      int 
      init(int const i, string const& fname) 
      { 
	scorer.open(fname); 
	this->index = i;
	return i + this->num_feats;
      }

      void 
      operator()(Bitext<Token> const& bt, PhrasePair& pp) const
      {
	uint32_t sid1=0,sid2=0,off1=0,off2=0,len1=0,len2=0;
	parse_pid(pp.p1, sid1, off1, len1);
	parse_pid(pp.p2, sid2, off2, len2);

#if 0
	Token const* t1 = bt.T1->sntStart(sid1);
	for (size_t i = off1; i < off1 + len1; ++i)
	  cout << (*bt.V1)[t1[i].id()] << " "; 
	cout << __FILE__ << ":" << __LINE__ << endl;

	Token const* t2 = bt.T2->sntStart(sid2);
	for (size_t i = off2; i < off2 + len2; ++i)
	  cout << (*bt.V2)[t2[i].id()] << " "; 
	cout << __FILE__ << ":" << __LINE__ << endl;

	BOOST_FOREACH (int a, pp.aln)
	  cout << a << " " ;
	cout << __FILE__ << ":" << __LINE__ << "\n" << endl;
#endif
	scorer.score(bt.T1->sntStart(sid1)+off1,0,len1,
		     bt.T2->sntStart(sid2)+off2,0,len2,
		     pp.aln, pp.fvals[this->index],
		     pp.fvals[this->index+1]);
      }
      
    };

    /// Word penalty
    template<typename Token>
    class
    PScoreWP : public PhraseScorer<Token>
    {
    public:

      PScoreWP() { this->num_feats = 1; }

      int 
      init(int const i) 
      {
	this->index = i;
	return i + this->num_feats;
      }

      void 
      operator()(Bitext<Token> const& bt, PhrasePair& pp) const
      {
	uint32_t sid2=0,off2=0,len2=0;
	parse_pid(pp.p2, sid2, off2, len2);
	pp.fvals[this->index] = len2;
      }
      
    };

    /// Phrase penalty
    template<typename Token>
    class
    PScorePP : public PhraseScorer<Token>
    {
    public:

      PScorePP() { this->num_feats = 1; }

      int 
      init(int const i) 
      {
	this->index = i;
	return i + this->num_feats;
      }

      void 
      operator()(Bitext<Token> const& bt, PhrasePair& pp) const
      {
	pp.fvals[this->index] = 1;
      }
      
    };

    template<typename TKN>
    class Bitext 
    {
      mutable boost::mutex lock;
    public:
      typedef TKN Token;
      typedef typename TSA<Token>::tree_iterator iter;

      class agenda;
      // stores the list of unfinished jobs;
      // maintains a pool of workers and assigns the jobs to them
      
      // to be done: work with multiple agendas for faster lookup
      // (multiplex jobs); not sure if an agenda having more than 
      // four or so workers is efficient, because workers get into 
      // each other's way. 
      mutable sptr<agenda> ag; 
      
      sptr<Ttrack<char> >  const Tx; // word alignments
      sptr<Ttrack<Token> > const T1; // token track
      sptr<Ttrack<Token> > const T2; // token track
      sptr<TokenIndex>     const V1; // vocab
      sptr<TokenIndex>     const V2; // vocab
      sptr<TSA<Token> >    const I1; // indices
      sptr<TSA<Token> >    const I2; // indices
      
      /// given the source phrase sid[start:stop]
      //  find the possible start (s1 .. s2) and end (e1 .. e2) 
      //  points of the target phrase; if non-NULL, store word
      //  alignments in *core_alignment. If /flip/, source phrase is 
      //  L2.
      bool 
      find_trg_phr_bounds
      (size_t const sid, size_t const start, size_t const stop, 
       size_t & s1, size_t & s2, size_t & e1, size_t & e2, 
       vector<uchar> * core_alignment, bool const flip) const;
      
      mutable boost::unordered_map<uint64_t,sptr<pstats> > cache1,cache2;
    private:
      size_t default_sample_size;
      sptr<pstats> 
	prep2(iter const& phrase, size_t const max_sample) const;
    public:
      Bitext(Ttrack<Token>* const t1, 
	     Ttrack<Token>* const t2, 
	     Ttrack<char>*  const tx,
	     TokenIndex*    const v1, 
	     TokenIndex*    const v2,
	     TSA<Token>* const i1, 
	     TSA<Token>* const i2,
	     size_t const max_sample=5000);
	     
      virtual void open(string const base, string const L1, string const L2) = 0;
      
      sptr<pstats> lookup(iter const& phrase) const;
      sptr<pstats> lookup(iter const& phrase, size_t const max_sample) const;
      void prep(iter const& phrase) const;
      void setDefaultSampleSize(size_t const max_samples);
      size_t getDefaultSampleSize() const;
      
      string toString(uint64_t pid, int isL2) const;
    };

    template<typename Token>
    string
    Bitext<Token>::
    toString(uint64_t pid, int isL2) const
    {
      ostringstream buf;
      uint32_t sid,off,len; parse_pid(pid,sid,off,len);
      Token const* t = (isL2 ? T2 : T1)->sntStart(sid) + off;
      Token const* x = t + len;
      TokenIndex const& V = isL2 ? *V2 : *V1;
      while (t < x) 
	{
	  buf << V[t->id()];
	  if (++t < x) buf << " ";
	}
      return buf.str();
    }
    
    

    template<typename Token>
    size_t 
    Bitext<Token>::
    getDefaultSampleSize() const 
    { 
      return default_sample_size; 
    }
    template<typename Token>
    void 
    Bitext<Token>::
    setDefaultSampleSize(size_t const max_samples)
    { 
      if (max_samples != default_sample_size) 
	{
	  cache1.clear();
	  cache2.clear();
	  default_sample_size = max_samples; 
	}
    }

    template<typename Token>
    Bitext<Token>::
    Bitext(Ttrack<Token>* const t1, 
	   Ttrack<Token>* const t2, 
	   Ttrack<char>*  const tx,
	   TokenIndex*    const v1, 
	   TokenIndex*    const v2,
	   TSA<Token>* const i1, 
	   TSA<Token>* const i2,
	   size_t const max_sample)
      : Tx(tx), T1(t1), T2(t2), V1(v1), V2(v2), I1(i1), I2(i2)
      , default_sample_size(max_sample)
    { }

    // agenda is a pool of jobs 
    template<typename Token>
    class 
    Bitext<Token>::
    agenda
    {
      boost::mutex lock; 
      class job 
      {
	boost::mutex      lock; 
	friend class agenda;
      public:
	size_t         workers; // how many workers are working on this job?
	sptr<TSA<Token> const> root; // root of the underlying suffix array
	char const*       next; // next position to read from 
	char const*       stop; // end of index range
	size_t     max_samples; // how many samples to extract at most
	size_t             ctr; /* # of phrase occurrences considered so far
				 * # of samples chosen is stored in stats->good */
	size_t             len; // phrase length
	bool               fwd; // if true, source phrase is L1 
	sptr<pstats>     stats; // stores statistics collected during sampling
	bool step(uint64_t & sid, uint64_t & offset); // select another occurrence
	bool done() const;
	job(typename TSA<Token>::tree_iterator const& m, 
	    sptr<TSA<Token> > const& r, size_t maxsmpl, bool isfwd);
      };
      
      class 
      worker
      {
	agenda& ag;
      public:
	worker(agenda& a) : ag(a) {}
	void operator()();
      };
      
      list<sptr<job> > joblist;
      vector<sptr<boost::thread> > workers;
      bool shutdown;
      size_t doomed;
    public:
      Bitext<Token>   const& bt;
      agenda(Bitext<Token> const& bitext);
      ~agenda();
      void add_workers(int n);

      sptr<pstats> 
      add_job(typename TSA<Token>::tree_iterator const& phrase, 
	      size_t const max_samples);
      sptr<job> get_job();
    };
    
    template<typename Token>
    bool
    Bitext<Token>::
    agenda::
    job::
    step(uint64_t & sid, uint64_t & offset)
    {
      boost::lock_guard<boost::mutex> jguard(lock);
      if ((max_samples == 0) && (next < stop))
	{
	  next = root->readSid(next,stop,sid);
	  next = root->readOffset(next,stop,offset);
	  boost::lock_guard<boost::mutex> sguard(stats->lock);
	  if (stats->raw_cnt == ctr) ++stats->raw_cnt;
	  stats->sample_cnt++;
	  return true;
	}
      else 
	{
	  while (next < stop && stats->good < max_samples)
	    {
	      next = root->readSid(next,stop,sid);
	      next = root->readOffset(next,stop,offset);
	      {
		boost::lock_guard<boost::mutex> sguard(stats->lock);
		if (stats->raw_cnt == ctr) ++stats->raw_cnt;
		size_t rnum = randInt(stats->raw_cnt - ctr++);
		if (rnum < max_samples - stats->good)
		  {
		    stats->sample_cnt++;
		    return true;
		  }
	      }
	    }
	  return false;
	}
    }

    template<typename Token>
    void
    Bitext<Token>::
    agenda::
    add_workers(int n)
    {
      static boost::posix_time::time_duration nodelay(0,0,0,0); 
      boost::lock_guard<boost::mutex> guard(this->lock);

      int target  = max(1, int(n + workers.size() - this->doomed));
      // house keeping: remove all workers that have finished
      for (size_t i = 0; i < workers.size(); )
	{
	  if (workers[i]->timed_join(nodelay))
	    {
	      if (i + 1 < workers.size())
		workers[i].swap(workers.back());
	      workers.pop_back();
	    }
	  else ++i;
	}
      // cerr << workers.size() << "/" << target << " active" << endl;
      if (int(workers.size()) > target)
	this->doomed = workers.size() - target;
      else 
	while (int(workers.size()) < target)
	  {
	    sptr<boost::thread> w(new boost::thread(worker(*this)));
	    workers.push_back(w);
	  }
    }

    template<typename Token>
    void
    Bitext<Token>::
    agenda::
    worker::
    operator()()
    {
      size_t s1=0, s2=0, e1=0, e2=0;
      uint64_t sid=0, offset=0; // of the source phrase
      while(sptr<job> j = ag.get_job())
	{
	  j->stats->register_worker();
	  vector<uchar> aln;
	  while (j->step(sid,offset))
	    {
	      aln.clear();
	      if (!ag.bt.find_trg_phr_bounds
		  (sid, offset, offset + j->len, s1, s2, e1, e2, 
		   j->fwd?&aln:NULL, !j->fwd)) 
		continue;
	      j->stats->lock.lock(); 
	      j->stats->good += 1; 
	      j->stats->sum_pairs += (s2-s1+1)*(e2-e1+1);
	      j->stats->lock.unlock();
	      for (size_t k = j->fwd ? 1 : 0; k < aln.size(); k += 2) 
		aln[k] += s2 - s1;
	      Token const* o = (j->fwd ? ag.bt.T2 : ag.bt.T1)->sntStart(sid);
	      float sample_weight = 1./((s2-s1+1)*(e2-e1+1));
	      for (size_t s = s1; s <= s2; ++s)
		{
		  sptr<iter> b = (j->fwd ? ag.bt.I2 : ag.bt.I1)->find(o+s,e1-s);
		  if (!b || b->size() < e1 -s)
		    UTIL_THROW(util::Exception, "target phrase not found");
		  // assert(b);
		  for (size_t i = e1; i <= e2; ++i)
		    {
		      
		      j->stats->add(b->getPid(),sample_weight,aln,b->approxOccurrenceCount());
		      if (i < e2)
			{
#ifndef NDEBUG
			  bool ok = b->extend(o[i].id());
			  assert(ok);
#else
			  b->extend(o[i].id());
			  // cerr << "boo" << endl;
#endif 
			}
		    }
		  if (j->fwd && s < s2) 
		    for (size_t k = j->fwd ? 1 : 0; k < aln.size(); k += 2) 
		      --aln[k];
		}
	      // j->stats->lock.unlock();
	    }
	  j->stats->release();
	}
    }

    template<typename Token>
    Bitext<Token>::
    agenda::
    job::
    job(typename TSA<Token>::tree_iterator const& m, 
	sptr<TSA<Token> > const& r, size_t maxsmpl, bool isfwd)
      : workers(0)
      , root(r)
      , next(m.lower_bound(-1))
      , stop(m.upper_bound(-1))
      , max_samples(maxsmpl)
      , ctr(0)
      , len(m.size())
      , fwd(isfwd)
    {
      stats.reset(new pstats());
      stats->raw_cnt = m.approxOccurrenceCount();
    }

    template<typename Token>
    sptr<pstats> 
    Bitext<Token>::
    agenda::
    add_job(typename TSA<Token>::tree_iterator const& phrase, 
	    size_t const max_samples)
    {
      static boost::posix_time::time_duration nodelay(0,0,0,0); 
      bool fwd = phrase.root == bt.I1.get();
      sptr<job> j(new job(phrase, fwd ? bt.I2 : bt.I1, max_samples, fwd));
      j->stats->register_worker();
      
      boost::unique_lock<boost::mutex> lk(this->lock);
      joblist.push_back(j);
      if (joblist.size() == 1)
	{
	  size_t i = 0;
	  while (i < workers.size())
	    {
	      if (workers[i]->timed_join(nodelay))
		{
		  if (doomed)
		    {
		      if (i+1 < workers.size())
			workers[i].swap(workers.back());
		      workers.pop_back();
		      --doomed;
		    }
		  else
		    workers[i++] = sptr<boost::thread>(new boost::thread(worker(*this)));
		}
	      else ++i;
	    }
	}
      return j->stats;
    }
    
    template<typename Token>
    sptr<typename Bitext<Token>::agenda::job>
    Bitext<Token>::
    agenda::
    get_job()
    {
      // cerr << workers.size() << " workers on record" << endl;
      sptr<job> ret;
      if (this->shutdown) return ret;
      // add_workers(0);
      boost::unique_lock<boost::mutex> lock(this->lock);
      if (this->doomed) 
	{
	  --this->doomed;
	  return ret;
	}
      typename list<sptr<job> >::iterator j = joblist.begin();
      while (j != joblist.end())
	{
	  if ((*j)->done()) 
	    {
	      (*j)->stats->release();
	      joblist.erase(j++);
	    } 
	  else if ((*j)->workers >= 4) 
	    {
	      ++j;
	    }
	  else break;
	}
      if (joblist.size())
	{
	  ret = j == joblist.end() ? joblist.front() : *j;
	  boost::lock_guard<boost::mutex> jguard(ret->lock);
	  ++ret->workers;
	}
      return ret;
    }

   
    template<typename TKN>
    class mmBitext : public Bitext<TKN>
    {
    public:
      void open(string const base, string const L1, string L2);
      mmBitext();
    };

    template<typename TKN>
    mmBitext<TKN>::
    mmBitext()
      : Bitext<TKN>(new mmTtrack<TKN>(),
		    new mmTtrack<TKN>(),
		    new mmTtrack<char>(),
		    new TokenIndex(),
		    new TokenIndex(),
		    new mmTSA<TKN>(),
		    new mmTSA<TKN>())
    {};
    
    template<typename TKN>
    void
    mmBitext<TKN>::
    open(string const base, string const L1, string L2)
    {
      mmTtrack<TKN>& t1 = *reinterpret_cast<mmTtrack<TKN>*>(this->T1.get());
      mmTtrack<TKN>& t2 = *reinterpret_cast<mmTtrack<TKN>*>(this->T2.get());
      mmTtrack<char>& tx = *reinterpret_cast<mmTtrack<char>*>(this->Tx.get());
      t1.open(base+L1+".mct");
      t2.open(base+L2+".mct");
      tx.open(base+L1+"-"+L2+".mam");
      this->V1->open(base+L1+".tdx"); this->V1->iniReverseIndex();
      this->V2->open(base+L2+".tdx"); this->V2->iniReverseIndex();
      mmTSA<TKN>& i1 = *reinterpret_cast<mmTSA<TKN>*>(this->I1.get());
      mmTSA<TKN>& i2 = *reinterpret_cast<mmTSA<TKN>*>(this->I2.get());
      i1.open(base+L1+".sfa", this->T1.get());
      i2.open(base+L2+".sfa", this->T2.get());
      assert(this->T1->size() == this->T2->size());
    }
    
    template<typename TKN>
    class imBitext : public Bitext<TKN>
    {
    public:
      void open(string const base, string const L1, string L2);
      imBitext();
    };

    template<typename TKN>
    imBitext<TKN>::
    imBitext()
      : Bitext<TKN>(new imTtrack<TKN>(),
		    new imTtrack<TKN>(),
		    new imTtrack<char>(),
		    new TokenIndex(),
		    new TokenIndex(),
		    new imTSA<TKN>(),
		    new imTSA<TKN>())
      {}
    

    // template<typename TKN>
    // void
    // imBitext<TKN>::
    // open(string const base, string const L1, string L2)
    // {
    //   mmTtrack<TKN>& t1 = *reinterpret_cast<mmTtracuk<TKN>*>(this->T1.get());
    //   mmTtrack<TKN>& t2 = *reinterpret_cast<mmTtrack<TKN>*>(this->T2.get());
    //   mmTtrack<char>& tx = *reinterpret_cast<mmTtrack<char>*>(this->Tx.get());
    //   t1.open(base+L1+".mct");
    //   t2.open(base+L2+".mct");
    //   tx.open(base+L1+"-"+L2+".mam");
    //   cerr << "DADA" << endl;
    //   this->V1->open(base+L1+".tdx"); this->V1->iniReverseIndex();
    //   this->V2->open(base+L2+".tdx"); this->V2->iniReverseIndex();
    //   mmTSA<TKN>& i1 = *reinterpret_cast<mmTSA<TKN>*>(this->I1.get());
    //   mmTSA<TKN>& i2 = *reinterpret_cast<mmTSA<TKN>*>(this->I2.get());
    //   i1.open(base+L1+".sfa", this->T1.get());
    //   i2.open(base+L2+".sfa", this->T2.get());
    //   assert(this->T1->size() == this->T2->size());
    // }

    template<typename Token>
    bool
    Bitext<Token>::
    find_trg_phr_bounds(size_t const sid, size_t const start, size_t const stop,
			size_t & s1, size_t & s2, size_t & e1, size_t & e2,
			vector<uchar>* core_alignment, bool const flip) const
    {
      // if (core_alignment) cout << "HAVE CORE ALIGNMENT" << endl;
      // a word on the core_alignment:
      // since fringe words ([s1,...,s2),[e1,..,e2) if s1 < s2, or e1 < e2, respectively)
      // are be definition unaligned, we store only the core alignment in *core_alignment
      // it is up to the calling function to shift alignment points over for start positions
      // of extracted phrases that start with a fringe word
      bitvector forbidden((flip ? T1 : T2)->sntLen(sid));
      size_t src,trg;
      size_t lft = forbidden.size();
      size_t rgt = 0;
      vector<vector<ushort> > aln((*T1).sntLen(sid));
      char const* p = Tx->sntStart(sid);
      char const* x = Tx->sntEnd(sid);

      // cerr << "flip = " << flip << " " << __FILE__ << ":" << __LINE__ << endl;

      while (p < x)
	{
	  if (flip) { p = binread(p,trg); assert(p<x); p = binread(p,src); }
	  else      { p = binread(p,src); assert(p<x); p = binread(p,trg); }
	  if (src < start || src >= stop) 
	    forbidden.set(trg);
	  else
	    {
	      lft = min(lft,trg);
	      rgt = max(rgt,trg);
	      if (core_alignment) 
		{
		  if (flip) aln[trg].push_back(src);
		  else      aln[src].push_back(trg);
		}
	    }
	}
      
      for (size_t i = lft; i <= rgt; ++i)
	if (forbidden[i]) 
	  return false;
      
      s2 = lft;   for (s1 = s2; s1 && !forbidden[s1-1]; --s1);
      e1 = rgt+1; for (e2 = e1; e2 < forbidden.size() && !forbidden[e2]; ++e2);
      
      if (lft > rgt) return false;
      if (core_alignment) 
	{
	  core_alignment->clear();
	  if (flip)
	    {
	      for (size_t i = lft; i <= rgt; ++i)
		{
		  sort(aln[i].begin(),aln[i].end());
		  BOOST_FOREACH(ushort x, aln[i])
		    {
		      core_alignment->push_back(i-lft);
		      core_alignment->push_back(x-start);
		    }
		}
	    }
	  else
	    {
	      for (size_t i = start; i < stop; ++i)
		{
		  BOOST_FOREACH(ushort x, aln[i])
		    {
		      core_alignment->push_back(i-start);
		      core_alignment->push_back(x-lft);
		    }
		}
	    }
	  
#if 0
	  // if (e1 - s1 > 3)
	    {
	      lock_guard<mutex> guard(this->lock);
	      Token const* t1 = T1->sntStart(sid);
	      Token const* t2 = T2->sntStart(sid);
	      cout << "[" << start << ":" << stop << "] => [" 
		   << s1 << ":" << s2 << ":" 
		   << e1 << ":" << e2 << "]" << endl;
	      for (size_t k = start; k < stop; ++k) 
		cout << k-start << "." << (*V1)[t1[k].id()] << " "; 
	      cout << endl;
	      for (size_t k = s1; k < e2;) 
		{
		  if (k == s2) cout << "[";
		  cout << int(k)-int(s2) << "." << (*V2)[t2[k].id()];
		  if (++k == e1) cout << "] ";
		  else cout << " ";
		}
	      cout << endl;
	      for (size_t k = 0; k < core_alignment->size(); k += 2)
		cout << int((*core_alignment)[k]) << "-" << int((*core_alignment)[k+1]) << " ";
	      cout << "\n" << __FILE__ << ":" << __LINE__ << endl;

	    }
#endif
	}
      return lft <= rgt;
    }

    template<typename Token>
    void
    Bitext<Token>::
    prep(iter const& phrase) const
    {
      prep2(phrase, this->default_sample_size);
    }

    template<typename Token>
    sptr<pstats> 
    Bitext<Token>::
      prep2(iter const& phrase, size_t const max_sample) const
    {
      // boost::lock_guard<boost::mutex>(this->lock);
      if (!ag) 
	{
	  // boost::lock_guard<boost::mutex>(this->lock);
	  if (!ag) 
	    {
	      ag.reset(new agenda(*this));
	      ag->add_workers(20);
	    }
	}
      typedef boost::unordered_map<uint64_t,sptr<pstats> > pcache_t;
      sptr<pstats> ret;
      if (max_sample == this->default_sample_size)
      	{
      	  uint64_t pid = phrase.getPid();
      	  pcache_t & cache(phrase.root == &(*this->I1) ? cache1 : cache2);
      	  pcache_t::value_type entry(pid,sptr<pstats>());
	  pair<pcache_t::iterator,bool> foo;
	  {
	    // boost::lock_guard<boost::mutex>(this->lock);
	    foo = cache.emplace(entry);
	  }
      	  if (foo.second) foo.first->second = ag->add_job(phrase, max_sample);
	  ret = foo.first->second;
      	}
      else ret = ag->add_job(phrase, max_sample);
      return ret;
    }
    
    template<typename Token>
    sptr<pstats> 
    Bitext<Token>::
    lookup(iter const& phrase) const
    {
      boost::lock_guard<boost::mutex>(this->lock);
      sptr<pstats> ret;
      ret = prep2(phrase, this->default_sample_size);
      assert(ret);
      boost::unique_lock<boost::mutex> lock(ret->lock);
      while (ret->in_progress)
	ret->ready.wait(lock);
      return ret;
    }

    template<typename Token>
    sptr<pstats> 
    Bitext<Token>::
    lookup(iter const& phrase, size_t const max_sample) const
    {
      boost::lock_guard<boost::mutex>(this->lock);
      sptr<pstats> ret = prep2(phrase, max_sample);
      boost::unique_lock<boost::mutex> lock(ret->lock);
      while (ret->in_progress)
	ret->ready.wait(lock);
      return ret;
    }

    template<typename Token>
    Bitext<Token>::
    agenda::
    ~agenda()
    {
      this->lock.lock();
      this->shutdown = true;
      this->lock.unlock();
      for (size_t i = 0; i < workers.size(); ++i)
	workers[i]->join();
    }
    
    template<typename Token>
    Bitext<Token>::
    agenda::
    agenda(Bitext<Token> const& thebitext)
      : shutdown(false), doomed(0), bt(thebitext)
    { }
    
    template<typename Token>
    bool
    Bitext<Token>::
    agenda::
    job::
    done() const
    { 
      return (max_samples && stats->good >= max_samples) || next == stop; 
    }

  } // end of namespace bitext
} // end of namespace moses
#endif

