// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// (c) 2007-2009 Ulrich Germann. All rights reserved.
#ifndef _ug_im_tsa_h
#define _ug_im_tsa_h

// TO DO:
// - multi-threaded sorting during TSA construction (currently painfully slow!)

#include <iostream>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>

#include "tpt_tightindex.h"
#include "tpt_tokenindex.h"
#include "ug_tsa_base.h"
#include "tpt_pickler.h"

#include "moses/TranslationModel/UG/generic/threading/ug_thread_pool.h"
#include "util/usage.hh"

namespace sapt
{
  namespace bio=boost::iostreams;

  template<typename TOKEN, typename SORTER>
  class TsaSorter
  {
  public:
    typedef typename Ttrack<TOKEN>::Position cpos;
    typedef typename std::vector<cpos>::iterator iter;
  private:
    SORTER m_sorter;
    iter m_begin;
    iter m_end;
  public:
    TsaSorter(SORTER sorter, iter& begin, iter& end)
      : m_sorter(sorter),
        m_begin(begin),
        m_end(end) { }
    
    bool 
    operator()()
    {
      std::sort(m_begin, m_end, m_sorter);
      return true;
    }
    
  };


 //-----------------------------------------------------------------------
  template<typename TOKEN>
  class imTSA : public TSA<TOKEN>
  {
    typedef typename Ttrack<TOKEN>::Position cpos;

  public:
    class tree_iterator;
    friend class tree_iterator;

  private:
    std::vector<cpos>          sufa; // stores the actual array
    std::vector<filepos_type> index; /* top-level index into regions in sufa
                                 * (for faster access) */
  private:
    char const*
    index_jump(char const* a, char const* z, float ratio) const;

    char const*
    getLowerBound(id_type id) const;

    char const*
    getUpperBound(id_type id) const;

  public:
    imTSA();
    imTSA(boost::shared_ptr<Ttrack<TOKEN> const> c, bdBitset const* filt, 
	  std::ostream* log = NULL, size_t threads = 0);

    imTSA(imTSA<TOKEN> const& prior,
	  boost::shared_ptr<imTtrack<TOKEN> const> const&   crp,
	  std::vector<id_type> const& newsids, size_t const vsize);

    count_type
    sntCnt(char const* p, char const * const q) const;

    count_type
    rawCnt(char const* p, char const * const q) const;

    void
    getCounts(char const* p, char const * const q,
              count_type& sids, count_type& raw) const;

    char const*
    readSid(char const* p, char const* q, id_type& sid) const;

    char const*
    readSid(char const* p, char const* q, ::uint64_t& sid) const;

    char const*
    readOffset(char const* p, char const* q, uint16_t& offset) const;

    char const*
    readOffset(char const* p, char const* q, ::uint64_t& offset) const;

    void
    sanityCheck() const;

    void
    save_as_mm_tsa(std::string fname) const;

    /// add a sentence to the database
    // shared_ptr<imTSA<TOKEN> > add(vector<TOKEN> const& snt) const;

  };

  template<typename TOKEN>
  class
  imTSA<TOKEN>::
  tree_iterator : public TSA<TOKEN>::tree_iterator
  {
  public:
    tree_iterator(imTSA<TOKEN> const* s);
  };

  template<typename TOKEN>
  imTSA<TOKEN>::
  tree_iterator::
  tree_iterator(imTSA<TOKEN> const* s)
    : TSA<TOKEN>::tree_iterator::tree_iterator(reinterpret_cast<TSA<TOKEN> const*>(s))
  {};

  /** jump to the point 1/ratio in a tightly packed index
   *  assumes that keys are flagged with '1', values with '0'
   */
  template<typename TOKEN>
  char const*
  imTSA<TOKEN>::
  index_jump(char const* a, char const* z, float ratio) const
  {
    typedef cpos cpos;
    assert(ratio >= 0 && ratio < 1);
    cpos const* xa = reinterpret_cast<cpos const*>(a);
    cpos const* xz = reinterpret_cast<cpos const*>(z);
    return reinterpret_cast<char const*>(xa+int(ratio*(xz-xa)));
  }

  template<typename TOKEN>
  imTSA<TOKEN>::
  imTSA()
  {
    this->indexSize  = 0;
    // this->data       = NULL;
    this->startArray = NULL;
    this->endArray   = NULL;
    this->corpusSize = 0;
    this->BitSetCachingThreshold=4096;
  };

  // build an array from all the tokens in the sentences in *c that are
  // specified in filter
  template<typename TOKEN>
  imTSA<TOKEN>::
  imTSA(boost::shared_ptr<Ttrack<TOKEN> const> c, 
	bdBitset const* filter,	std::ostream* log, size_t threads)
  {
    if (threads == 0) 
      threads = boost::thread::hardware_concurrency();
    assert(c);
    this->corpus = c;
    bdBitset  filter2;
    if (!filter)
      {
        filter2.resize(c->size());
        filter2.set();
        filter = &filter2;
      }
    assert(filter);
    // In the first iteration over the corpus, we obtain word counts.
    // They allows us to
    //    a. allocate the exact amount of memory we need
    //    b. place tokens into the right 'section' in the array, based on
    //       the ID of the first token in the sequence. We can then sort
    //       each section separately.

    if (log) *log << "counting tokens ... ";
    int slimit = 65536;
    // slimit=65536 is the upper bound of what we can fit into a ushort which
    // we currently use for the offset. Actually, due to (memory) word
    // alignment in the memory, using a ushort instead of a uint32_t might not
    // even make a difference.

    std::vector<count_type> wcnt; // word counts
    sufa.resize(c->count_tokens(wcnt,filter,slimit,log));

    if (log) *log << sufa.size() << "." << std::endl;
    // exit(1);
    // we use a second std::vector that keeps track for each ID of the current insertion
    // position in the array
    std::vector<count_type> tmp(wcnt.size(),0);
    for (size_t i = 1; i < wcnt.size(); ++i)
      tmp[i] = tmp[i-1] + wcnt[i-1];

    // Now dump all token positions into the right place in sufa
    this->corpusSize = 0;
    for (id_type sid = filter->find_first();
	 sid < filter->size();
	 sid = filter->find_next(sid))
      {
	TOKEN const* k = c->sntStart(sid);
	TOKEN const* const stop = c->sntEnd(sid);
	if (stop - k >= slimit) continue;
        this->corpusSize++;
	for (ushort p=0; k < stop; ++p,++k)
	  {
	    id_type wid = k->id();
            cpos& cpos = sufa[tmp[wid]++];
            cpos.sid    = sid;
            cpos.offset = p;
            assert(p < c->sntLen(sid));
	  }
      }

    // Now sort the array
    if (log) *log << "sorting .... with " << threads << " threads." << std::endl;
#ifndef NO_MOSES
    double start_time = util::WallTime();
#endif
    boost::scoped_ptr<ug::ThreadPool> tpool;
    tpool.reset(new ug::ThreadPool(threads));
    
    index.resize(wcnt.size()+1,0);
    typedef typename ttrack::Position::LESS<Ttrack<TOKEN> > sorter_t;
    sorter_t sorter(c.get());
    for (size_t i = 0; i < wcnt.size(); i++)
      {
        // if (log && wcnt[i] > 5000)
        //   *log << "sorting " << wcnt[i]
        //        << " entries starting with id " << i << "." << std::endl;
        index[i+1] = index[i]+wcnt[i];
        assert(index[i+1]==tmp[i]); // sanity check
        if (wcnt[i]>1)
	  {
	    typename std::vector<cpos>::iterator b,e;
	    b = sufa.begin()+index[i];
	    e = sufa.begin()+index[i+1];
	    TsaSorter<TOKEN,sorter_t> foo(sorter,b,e);
	    tpool->add(foo);
	    // sort(sufa.begin()+index[i],sufa.begin()+index[i+1],sorter);
	  }
      }
    tpool.reset();
#ifndef NO_MOSES
    if (log) *log << "Done sorting after " << util::WallTime() - start_time
		  << " seconds." << std::endl;
#endif
    this->startArray = reinterpret_cast<char const*>(&(*sufa.begin()));
    this->endArray   = reinterpret_cast<char const*>(&(*sufa.end()));
    this->numTokens  = sufa.size();
    this->indexSize  = this->index.size();
#if 1
    // Sanity check during code development. Can be removed once the thing is stable.
    typename std::vector<cpos>::iterator m = sufa.begin();
    for (size_t i = 0; i < wcnt.size(); i++)
      {
        for (size_t k = 0; k < wcnt[i]; ++k,++m)
          {
            assert(c->getToken(*m)->id()==i);
            assert(m->offset < c->sntLen(m->sid));
          }
      }
#endif
  } // end of imTSA constructor (corpus,filter,quiet)

  // ----------------------------------------------------------------------

  template<typename TOKEN>
  char const*
  imTSA<TOKEN>::
  getLowerBound(id_type id) const
  {
    if (id >= this->index.size())
      return NULL;
    assert(index[id] <= this->sufa.size());
    return reinterpret_cast<char const*>(&(this->sufa.front()) + index[id]);
  }

  template<typename TOKEN>
  char const*
  imTSA<TOKEN>::
  getUpperBound(id_type id) const
  {
    if (++id >= this->index.size())
      return NULL;
    assert(index[id] <= this->sufa.size());
    return reinterpret_cast<char const*>(&(this->sufa.front()) + index[id]);
  }

  template<typename TOKEN>
  char const*
  imTSA<TOKEN>::
  readSid(char const* p, char const* q, id_type& sid) const
  {
    assert(reinterpret_cast<cpos const*>(p) >= &(this->sufa.front()));
    assert(reinterpret_cast<cpos const*>(p) <= &(this->sufa.back()));
    sid = reinterpret_cast<cpos const*>(p)->sid;
    return p;
  }

  template<typename TOKEN>
  char const*
  imTSA<TOKEN>::
  readSid(char const* p, char const* q, ::uint64_t& sid) const
  {
    assert(reinterpret_cast<cpos const*>(p) >= &(this->sufa.front()));
    assert(reinterpret_cast<cpos const*>(p) <= &(this->sufa.back()));
    sid = reinterpret_cast<cpos const*>(p)->sid;
    return p;
  }

  template<typename TOKEN>
  char const*
  imTSA<TOKEN>::
  readOffset(char const* p, char const* q, uint16_t& offset) const
  {
    assert(reinterpret_cast<cpos const*>(p) >= &(this->sufa.front()));
    assert(reinterpret_cast<cpos const*>(p) <= &(this->sufa.back()));
    offset = reinterpret_cast<cpos const*>(p)->offset;
    return p+sizeof(cpos);
  }

  template<typename TOKEN>
  char const*
  imTSA<TOKEN>::
  readOffset(char const* p, char const* q, ::uint64_t& offset) const
  {
    assert(reinterpret_cast<cpos const*>(p) >= &(this->sufa.front()));
    assert(reinterpret_cast<cpos const*>(p) <= &(this->sufa.back()));
    offset = reinterpret_cast<cpos const*>(p)->offset;
    return p+sizeof(cpos);
  }

  template<typename TOKEN>
  count_type
  imTSA<TOKEN>::
  rawCnt(char const* p, char const* const q) const
  {
    cpos const* xp = reinterpret_cast<cpos const*>(p);
    cpos const* xq = reinterpret_cast<cpos const*>(q);
    return xq-xp;
  }

  template<typename TOKEN>
  void
  imTSA<TOKEN>::
  getCounts(char const* p, char const* const q,
	    count_type& sids, count_type& raw) const
  {
    id_type sid; // uint16_t off;
    bdBitset check(this->corpus->size());
    cpos const* xp = reinterpret_cast<cpos const*>(p);
    cpos const* xq = reinterpret_cast<cpos const*>(q);
    raw = xq-xp;
    for (;xp < xq;xp++)
      {
	sid = xp->sid;
	// off = xp->offset;
	check.set(sid);
      }
    sids = check.count();
  }

  template<typename TOKEN>
  void
  imTSA<TOKEN>::
  save_as_mm_tsa(std::string fname) const
  {
    std::ofstream out(fname.c_str());
    filepos_type idxStart(0);
    id_type idxSize(index.size());
    tpt::numwrite(out,idxStart);
    tpt::numwrite(out,idxSize);
    std::vector<filepos_type> mmIndex;
    for (size_t i = 1; i < this->index.size(); i++)
      {
        mmIndex.push_back(out.tellp());
        for (size_t k = this->index[i-1]; k < this->index[i]; ++k)
          {
            tpt::tightwrite(out,sufa[k].sid,0);
            tpt::tightwrite(out,sufa[k].offset,1);
          }
      }
    mmIndex.push_back(out.tellp());
    idxStart = out.tellp();
    for (size_t i = 0; i < mmIndex.size(); i++)
      tpt::numwrite(out,mmIndex[i]-mmIndex[0]);
    out.seekp(0);
    tpt::numwrite(out,idxStart);
    out.close();
  }

  template<typename TOKEN>
  imTSA<TOKEN>::
  imTSA(imTSA<TOKEN> const& prior,
        boost::shared_ptr<imTtrack<TOKEN> const> const&   crp,
        std::vector<id_type> const& newsids, size_t const vsize)
  {
    typename ttrack::Position::LESS<Ttrack<TOKEN> > sorter(crp.get());

    // count how many tokens will be added to the TSA
    // and index the new additions to the corpus
    size_t newToks = 0;
    BOOST_FOREACH(id_type sid, newsids)
      newToks += crp->sntLen(sid);
    std::vector<cpos> nidx(newToks); // new array entries

    size_t n = 0;
    BOOST_FOREACH(id_type sid, newsids)
      {
	assert(sid < crp->size());
  	for (size_t o = 0; o < (*crp)[sid].size(); ++o, ++n)
  	  { nidx[n].offset = o; nidx[n].sid  = sid; }
      }
    sort(nidx.begin(),nidx.end(),sorter);

    // create the new suffix array
    this->numTokens = newToks + prior.sufa.size();
    this->sufa.resize(this->numTokens);
    this->startArray = reinterpret_cast<char const*>(&(*this->sufa.begin()));
    this->endArray   = reinterpret_cast<char const*>(&(*this->sufa.end()));
    this->corpusSize = crp->size();
    this->corpus     = crp;
    this->index.resize(vsize+1);

    size_t i = 0;
    typename std::vector<cpos>::iterator k = this->sufa.begin();
    // cerr << newToks << " new items at "
    // << __FILE__ << ":" << __LINE__ << std::endl;
    for (size_t n = 0; n < nidx.size();)
      {
  	id_type nid = crp->getToken(nidx[n])->id();
  	assert(nid >= i);
  	while (i < nid)
  	  {
  	    this->index[i] = k - this->sufa.begin();
  	    if (++i < prior.index.size() && prior.index[i-1] < prior.index[i])
  	      {
  		k = copy(prior.sufa.begin() + prior.index[i-1],
  			 prior.sufa.begin() + prior.index[i], k);
  	      }
  	  }
	this->index[i] = k - this->sufa.begin();
  	if (++i < prior.index.size() && prior.index[i] > prior.index[i-1])
  	  {
  	    size_t j = prior.index[i-1];
  	    while (j < prior.index[i] && n < nidx.size()
  		   && crp->getToken(nidx[n])->id() < i)
  	      {
  		assert(k < this->sufa.end());
  		if (sorter(prior.sufa[j],nidx[n]))
  		  *k++ = prior.sufa[j++];
  		else
  		  *k++ = nidx[n++];
  	      }
  	    while (j < prior.index[i])
  	      {
  		assert(k < this->sufa.end());
  		*k++ = prior.sufa[j++];
  	      }
  	  }
  	while (n < nidx.size() && this->corpus->getToken(nidx[n])->id() < i)
  	  {
  	    assert(k < this->sufa.end());
  	    *k++ = nidx[n++];
  	  }
  	this->index[i] = k - this->sufa.begin();
      }
    this->index[i] = k - this->sufa.begin();
    while (++i < this->index.size())
      {
  	if (i < prior.index.size() && prior.index[i-1] < prior.index[i])
  	  k = copy(prior.sufa.begin() + prior.index[i-1],
  		   prior.sufa.begin() + prior.index[i], k);
  	this->index[i] = k - this->sufa.begin();
      }
#if 0
    // sanity checks
    assert(this->sufa.size() == this->index.back());
    BOOST_FOREACH(cpos const& x, this->sufa)
      {
	assert(x.sid < this->corpusSize);
	assert(x.offset < this->corpus->sntLen(x.sid));
      }
    for (size_t i = 1; i < index.size(); ++i)
      {
	assert(index[i-1] <= index[i]);
	assert(index[i] <= sufa.size());
	for (size_t k = index[i-1]; k < index[i]; ++k)
	  assert(this->corpus->getToken(sufa[k])->id() == i-1);
      }
    assert(index[0] == 0);
    assert(this->startArray == reinterpret_cast<char const*>(&(*this->sufa.begin())));
    assert(this->endArray == reinterpret_cast<char const*>(&(*this->sufa.end())));
#endif
  }

}

#endif
