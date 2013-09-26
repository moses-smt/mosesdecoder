// -*- c++ -*-
// (c) 2007-2009 Ulrich Germann. All rights reserved.
#ifndef _ug_im_tsa_h
#define _ug_im_tsa_h

// TO DO:
// - multi-threaded sorting during TSA construction (currently painfully slow!)

#include <iostream>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/dynamic_bitset.hpp>

#include "tpt_tightindex.h"
#include "tpt_tokenindex.h"
#include "ug_tsa_base.h"
#include "tpt_pickler.h"

namespace ugdiss
{
  using namespace std;
  namespace bio=boost::iostreams;
  
  //-----------------------------------------------------------------------
  template<typename TOKEN>
  class imTSA : public TSA<TOKEN>
  {
    typedef typename Ttrack<TOKEN>::Position cpos;
  public:
    class tree_iterator;
    friend class tree_iterator;
    
  private:
    vector<cpos>          sufa; // stores the actual array
    vector<filepos_type> index; /* top-level index into regions in sufa 
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
    imTSA(Ttrack<TOKEN> const* c, bdBitset const& filt, ostream* log = NULL);
    
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
    readSid(char const* p, char const* q, uint64_t& sid) const;

    char const* 
    readOffset(char const* p, char const* q, uint16_t& offset) const;

    char const* 
    readOffset(char const* p, char const* q, uint64_t& offset) const;
    
    void 
    sanityCheck() const;
    
    void 
    save_as_mm_tsa(string fname) const;
    
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
    this->corpus  = NULL;
    this->indexSize = 0;
    this->data    = NULL;
    this->startArray = NULL;
    this->endArray = NULL;
    this->corpusSize=0;
    this->BitSetCachingThreshold=4096;
  };
  
  // build an array from all the tokens in the sentences in *c that are
  // specified in filter
  template<typename TOKEN>
  imTSA<TOKEN>::
  imTSA(Ttrack<TOKEN> const* c, bdBitset const& filter, ostream* log)
  {
    assert(c);
    this->corpus = c;
    
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

    vector<count_type> wcnt; // word counts
    sufa.resize(c->count_tokens(wcnt,filter,slimit,log));

    if (log) *log << sufa.size() << "." << endl;
    // exit(1);
    // we use a second vector that keeps track for each ID of the current insertion
    // position in the array
    vector<count_type> tmp(wcnt.size(),0);
    for (size_t i = 1; i < wcnt.size(); ++i)
      tmp[i] = tmp[i-1] + wcnt[i-1];
    
    // Now dump all token positions into the right place in sufa
    this->corpusSize = 0;
    for (id_type sid = filter.find_first();
	 sid < filter.size();
	 sid = filter.find_next(sid))
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
    if (log) *log << "sorting ...." << endl;
    index.resize(wcnt.size()+1,0);
    typename ttrack::Position::LESS<Ttrack<TOKEN> > sorter(c);
    for (size_t i = 0; i < wcnt.size(); i++)
      {
        if (log && wcnt[i] > 5000)
          *log << "sorting " << wcnt[i] 
               << " entries starting with id " << i << "." << endl;
        index[i+1] = index[i]+wcnt[i];
        assert(index[i+1]==tmp[i]); // sanity check
        if (wcnt[i]>1)
          sort(sufa.begin()+index[i],sufa.begin()+index[i+1],sorter);
      }
    this->startArray = reinterpret_cast<char const*>(&(*sufa.begin()));
    this->endArray   = reinterpret_cast<char const*>(&(*sufa.end()));
    this->numTokens  = sufa.size();
    this->indexSize  = this->index.size();
#if 1
    // Sanity check during code development. Can be removed once the thing is stable.
    typename vector<cpos>::iterator m = sufa.begin();
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
    return reinterpret_cast<char const*>(&(this->sufa[index[id]]));
  }

  template<typename TOKEN>
  char const*
  imTSA<TOKEN>::
  getUpperBound(id_type id) const
  {
    if (id+1 >= this->index.size()) 
      return NULL;
    return reinterpret_cast<char const*>(&(this->sufa[index[id+1]]));
  }

  template<typename TOKEN>
  char const*
  imTSA<TOKEN>::
  readSid(char const* p, char const* q, id_type& sid) const
  {
    sid = reinterpret_cast<cpos const*>(p)->sid;
    return p;
  }
  
  template<typename TOKEN>
  char const*
  imTSA<TOKEN>::
  readSid(char const* p, char const* q, uint64_t& sid) const
  {
    sid = reinterpret_cast<cpos const*>(p)->sid;
    return p;
  }

  template<typename TOKEN>
  char const*
  imTSA<TOKEN>::
  readOffset(char const* p, char const* q, uint16_t& offset) const
  {
    offset = reinterpret_cast<cpos const*>(p)->offset;
    return p+sizeof(cpos);
  }

  template<typename TOKEN>
  char const*
  imTSA<TOKEN>::
  readOffset(char const* p, char const* q, uint64_t& offset) const
  {
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
    id_type sid; uint16_t off;
    bdBitset check(this->corpus->size());
    cpos const* xp = reinterpret_cast<cpos const*>(p);
    cpos const* xq = reinterpret_cast<cpos const*>(q);
    raw = xq-xp;
    for (;xp < xq;xp++)
      {
	sid = xp->sid;
	off = xp->offset;
	check.set(sid);
      }
    sids = check.count();
  }

  template<typename TOKEN>
  void 
  imTSA<TOKEN>::
  save_as_mm_tsa(string fname) const
  {
    ofstream out(fname.c_str());
    filepos_type idxStart(0);
    id_type idxSize(index.size());
    numwrite(out,idxStart);
    numwrite(out,idxSize);
    vector<filepos_type> mmIndex;
    for (size_t i = 1; i < this->index.size(); i++)
      {
	mmIndex.push_back(out.tellp());
	for (size_t k = this->index[i-1]; k < this->index[i]; ++k)
	  {
	    tightwrite(out,sufa[k].sid,0);
	    tightwrite(out,sufa[k].offset,1);
	  }
      }
    mmIndex.push_back(out.tellp());
    idxStart = out.tellp();
    for (size_t i = 0; i < mmIndex.size(); i++)
      numwrite(out,mmIndex[i]-mmIndex[0]);
    out.seekp(0);
    numwrite(out,idxStart);
    out.close();
  }
}
#endif
