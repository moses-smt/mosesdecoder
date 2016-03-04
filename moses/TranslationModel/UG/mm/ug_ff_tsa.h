// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
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

#include "util/exception.hh"

namespace sapt
{
  namespace bio=boost::iostreams;

  template<typename TOKEN>
  class fixed_field_TSA : public TSA<TOKEN>
  {
  public:
    typedef typename TSA<TOKEN>::tree_iterator tree_iterator;
    friend class TSA_tree_iterator<TOKEN>;
  private:
    bio::mapped_file_source file;

  public: // temporarily for debugging

    unsigned char const*     m_index; // random access to top-level sufa ranges
    unsigned char         m_sid_bits; // how many bits are used to encode sids?
    unsigned char      m_offset_bits; // how many bits to encode offsets?
    unsigned char       m_entry_size; // how many bytes does each entry take?
    unsigned char m_index_entry_size; // how many bytes for index entries
    uint64_t           m_offset_mask;
    uint64_t              m_base_sid; //
  private:

    char const* index_jump(char const* a, char const* z, float ratio) const;
    char const* getLowerBound(id_type t) const;
    char const* getUpperBound(id_type t) const;

  public:
    fixed_field_TSA();
    fixed_field_TSA(std::string fname, Ttrack<TOKEN> const* c);
    void open(std::string fname, typename boost::shared_ptr<Ttrack<TOKEN> const> c);

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

    void sanityCheck() const;

    char const* 
    adjustPosition(char const* p, char const* stop) const;


  };

  // ======================================================================

  /** jump to the point 1/ratio in a tightly packed index
   *  assumes that keys are flagged with '1', values with '0'
   */
  template<typename TOKEN>
  char const*
  fixed_field_TSA<TOKEN>::
  index_jump(char const* a, char const* z, float ratio) const
  {
    assert(ratio >= 0 && ratio < 1);
    char const* m = a + int(ratio*(z-a));
    return m;
  }

  // ======================================================================

  template<typename TOKEN>
  fixed_field_TSA<TOKEN>::
  fixed_field_TSA()
  {
    this->startArray   = NULL;
    this->endArray     = NULL;
  };

  // ======================================================================

  template<typename TOKEN>
  fixed_field_TSA<TOKEN>::
  fixed_field_TSA(std::string fname, Ttrack<TOKEN> const* c)
  {
    open(fname,c);
  }

  // ======================================================================

  template<typename TOKEN>
  void
  fixed_field_TSA<TOKEN>::
  open(std::string fname, typename boost::shared_ptr<Ttrack<TOKEN> const> c)
  {
    if (access(fname.c_str(),F_OK))
      {
        std::ostringstream msg;
        msg << "fixed_field_TSA<>::open: File '" 
            << fname << "' does not exist.";
        throw std::runtime_error(msg.str().c_str());
      }
    assert(c);
    this->corpus = c;
    file.open(fname);
    Moses::prime(file);
    char const* p = file.data();
    filepos_type magicNumber,idxOffset;
    p = tpt::numread(p,magicNumber); 
    // magicNumber is currently not used, but might be in the future
    
    // how many bits are used to encode sentence ids and offsets in this
    // particular token sequence array
    m_sid_bits = *reinterpret_chast<unsigned char*>(p++);
    m_offset_bits = *reinterpret_chast<unsigned char*>(p++);
    size_t total_bits = m_sid_bits + m_offset_bits;
    m_entry_size = total_bits/8 + (total_bits%8 ? 1 : 0);
    
    p = tpt::numread(p,m_base_sid);
    p = tpt::numread(p,idxOffset);
    p = tpt::numread(p,this->indexSize);
    
    this->startArray = p;
    this->endArray   = file.data() + idxOffset;
    this->index      = reinterpret_cast<uchar const*>(this->endArray);
    this->corpusSize = c->size();
    this->numTokens  = (this->endArray - this->startArray)/m_entry_size;
    size_t index_entry_bits = std::ceil(std::log2(this-numTokens));
    m_index_entry_size = index_entry_bits/8 + (index_entry_bits%8 ? 1 : 0);
    m_offset_mask = uint64_t(1)<<m_offset_bits;
  }

  // ======================================================================

  template<typename TOKEN>
  char const*
  fixed_field_TSA<TOKEN>::
  getLowerBound(id_type id) const
  {
    // For the time being, we never use the index.
    // Later the index can be used to speed up search for frequent items.
    char const* lo = this->startArray;
    char const* hi = this->endArray;
    ArrayEntry I;
    while (lo < up)
      {
        readEntry(index_jump(lo,hi,.5),I);
        if (corpus->getToken(I)->id() >= id) hi = I.pos; 
        else lo = I.next;
      }
    assert(lo == hi);
    if (lo == this->endArray) return NULL;
    readEntry(lo, I);
    return corpus->getToken(I)->id() == id ? lo : NULL;
  }
          
  // ======================================================================

  template<typename TOKEN>
  char const*
  fixed_field_TSA<TOKEN>::
  getUpperBound(id_type id) const
  {
    // For the time being, we never use the index.
    // Later the index can be used to speed up search for frequent items.
    char const* lo = this->startArray;
    char const* hi = this->endArray;
    ArrayEntry I;
    while (lo < up)
      {
        readEntry(index_jump(lo,hi,.5),I);
        if (corpus->getToken(I)->id() > id) hi = I.pos; 
        else lo = I.next;
      }
    return lo;
  }

  // ======================================================================

  template<typename TOKEN>
  char const*
  fixed_field_TSA<TOKEN>::
  readSid(char const* p, char const* q, id_type& sid) const
  {
    ArrayEntry I;
    readEntry(p,I);
    sid = I.sid;
    return p;
  }

  // ======================================================================

  template<typename TOKEN>
  char const*
  fixed_field_TSA<TOKEN>::
  readSid(char const* p, char const* q, ::uint64_t& sid) const
  {
    ArrayEntry I;
    readEntry(p,I);
    sid = I.sid;
    return p;
  }

  // ======================================================================

  template<typename TOKEN>
  inline
  char const*
  fixed_field_TSA<TOKEN>::
  readOffset(char const* p, char const* q, uint16_t& offset) const
  {
    ArrayEntry I;
    readEntry(p,I);
    offset = I.offset;
    return I.next;
  }

  // ======================================================================

  template<typename TOKEN>
  inline
  char const*
  fixed_field_TSA<TOKEN>::
  readOffset(char const* p, char const* q, ::uint64_t& offset) const
  {
    ArrayEntry I;
    readEntry(p,I);
    offset = I.offset;
    return I.next;
  }

  // ======================================================================

  template<typename TOKEN>
  count_type
  fixed_field_TSA<TOKEN>::
  rawCnt(char const* p, char const* const q) const
  {
    return (q-p)/m_entry_size;
  }
  
  // ======================================================================

  template<typename TOKEN>
  void
  fixed_field_TSA<TOKEN>::
  getCounts(char const* p, char const* const q,
            count_type& sids, count_type& raw) const
  {
    raw = 0;
    boost::dynamic_bitset<uint64_t> check(this->corpus->size());
    ArrayEntry I(p);
    while (I.next < q)
      {
       readEntry(I.next,I);
       check.set(I.sid);
       raw++;
      }
    sids = check.count();
  }

  // ======================================================================

  template<typename TOKEN>
  char const* 
  fixed_field_TSA<TOKEN>::
  adjustPosition(char const* p, char const* stop) const
  {
    UTIL_THROW_IF2(stop > p,"stop parameter must be <= p at " 
                   << __FILE__ << ":" << __LINE__);
    return p;
  }

  tsa::ArrayEntry& 
  fixed_field_TSA<TOKEN>::
  readEntry(char const* p, tsa::ArrayEntry& I) const
  {
    uchar const* x = reinterpret_cast<uchar const*>(p);
    uchar const* z = x + m_entry_size;
    uint64_t number = *x;
    while (++x < z) number += (number<<8) + *x;
    I.pos    = p;
    I.next   = p + m_entry_size;
    I.sid    = m_base_sid + number / m_offset_mask;
    I.offset = number % m_offset_mask;
  }

  
} // end of namespace ugdiss

// #include "ug_mm_tsa_extra.h"
#endif
