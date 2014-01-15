// -*- c++ -*-
#ifndef _ug_mm_tsa_h
#define _ug_mm_tsa_h

// (c) 2007-2009 Ulrich Germann. All rights reserved.

#include <iostream>
#include <stdexcept>
#include <sstream>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/dynamic_bitset.hpp>

#include "tpt_tightindex.h"
#include "tpt_tokenindex.h"
#include "tpt_pickler.h"
#include "ug_tsa_base.h"

namespace ugdiss
{
  using namespace std;
  namespace bio=boost::iostreams;

  template<typename TOKEN>
  class mmTSA : public TSA<TOKEN>
  {
  public:
    typedef typename TSA<TOKEN>::tree_iterator tree_iterator;
    friend class TSA_tree_iterator<TOKEN>;
  private:
    bio::mapped_file_source file;

  public: // temporarily for debugging

    filepos_type const* index; // random access to top-level sufa ranges

  private:

    char const* index_jump(char const* a, char const* z, float ratio) const;
    char const* getLowerBound(id_type t) const;
    char const* getUpperBound(id_type t) const;
		   
  public:
    mmTSA();
    mmTSA(string fname, Ttrack<TOKEN> const* c);
    void open(string fname, Ttrack<TOKEN> const* c);

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

    void sanityCheck() const;

  }; 

  // ======================================================================

  /** jump to the point 1/ratio in a tightly packed index
   *  assumes that keys are flagged with '1', values with '0'
   */
  template<typename TOKEN>
  char const* 
  mmTSA<TOKEN>::
  index_jump(char const* a, char const* z, float ratio) const
  {
    assert(ratio >= 0 && ratio < 1);
    char const* m = a+int(ratio*(z-a));
    if (m > a) 
      {
	while (m > a && *m <  0) --m;
	while (m > a && *m >= 0) --m;
	if (*m < 0) ++m;
      }
    assert(*m >= 0);
    return m;
  }

  // ======================================================================

  template<typename TOKEN>
  mmTSA<TOKEN>::
  mmTSA() 
  {
    this->corpus       = NULL;
    this->startArray   = NULL;
    this->endArray     = NULL;
    this->BitSetCachingThreshold=4096;
  };

  // ======================================================================

  template<typename TOKEN>
  mmTSA<TOKEN>::
  mmTSA(string fname, Ttrack<TOKEN> const* c)
  {
    open(fname,c);
  }

  // ======================================================================

  template<typename TOKEN>
  void
  mmTSA<TOKEN>::
  open(string fname, Ttrack<TOKEN> const* c)
  {
    this->bsc.reset(new BitSetCache<TSA<TOKEN> >(this));
    if (access(fname.c_str(),F_OK))
      {
        ostringstream msg;
        msg << "mmTSA<>::open: File '" << fname << "' does not exist.";
        throw std::runtime_error(msg.str().c_str());
      }
    assert(c);
    this->corpus = c;
    file.open(fname);
    Moses::prime(file);
    char const* p = file.data();
    filepos_type idxOffset;
    p = numread(p,idxOffset);
    p = numread(p,this->indexSize);
    
    // cerr << fname << ": " << idxOffset << " " << this->indexSize << endl;
    
    this->startArray = p;
    this->index      = reinterpret_cast<filepos_type const*>(file.data()+idxOffset);
    this->endArray   = reinterpret_cast<char const*>(index);
    this->corpusSize = c->size();
    this->numTokens  = c->numTokens();
  }

  // ======================================================================

  template<typename TOKEN>
  char const*
  mmTSA<TOKEN>::
  getLowerBound(id_type id) const
  {
    if (id >= this->indexSize) 
      return NULL;
    return this->startArray + this->index[id];
  }

  // ======================================================================

  template<typename TOKEN>
  char const*
  mmTSA<TOKEN>::
  getUpperBound(id_type id) const
  {
    if (id >= this->indexSize) 
      return NULL;
    // if (index[id] == index[id+1])
    // return NULL;
    else
      return this->startArray + this->index[id+1];
  }

  // ======================================================================

  template<typename TOKEN>
  char const*
  mmTSA<TOKEN>::
  readSid(char const* p, char const* q, id_type& sid) const
  {
    return tightread(p,q,sid);
  }

  // ======================================================================

  template<typename TOKEN>
  char const*
  mmTSA<TOKEN>::
  readSid(char const* p, char const* q, uint64_t& sid) const
  {
    return tightread(p,q,sid);
  }

  // ======================================================================

  template<typename TOKEN>
  inline
  char const*
  mmTSA<TOKEN>::
  readOffset(char const* p, char const* q, uint16_t& offset) const
  {
    return tightread(p,q,offset);
  }

  // ======================================================================

  template<typename TOKEN>
  inline
  char const*
  mmTSA<TOKEN>::
  readOffset(char const* p, char const* q, uint64_t& offset) const
  {
    return tightread(p,q,offset);
  }

  // ======================================================================

  template<typename TOKEN>
  count_type
  mmTSA<TOKEN>::
  rawCnt(char const* p, char const* const q) const
  {
    id_type sid; uint16_t off;
    size_t ret=0;
    while (p < q)
      {
	p = tightread(p,q,sid);
	p = tightread(p,q,off);
	ret++;
      }
    return ret;
  }
  
  // ======================================================================

  template<typename TOKEN>
  void 
  mmTSA<TOKEN>::
  getCounts(char const* p, char const* const q, 
	    count_type& sids, count_type& raw) const
  {
    raw = 0;
    id_type sid; uint16_t off;
    boost::dynamic_bitset<uint64_t> check(this->corpus->size());
    while (p < q)
      {
	p = tightread(p,q,sid);
	p = tightread(p,q,off);
	check.set(sid);
	raw++;
      }
    sids = check.count();
  }

  // ======================================================================

} // end of namespace ugdiss

// #include "ug_mm_tsa_extra.h"
#endif
