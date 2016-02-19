// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// Base class for Token Sequence Arrays
// (c) 2007-2010 Ulrich Germann. All rights reserved.
#ifndef _ug_tsa_base_h
#define _ug_tsa_base_h

#include <iostream>
#include <string>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/shared_ptr.hpp>
#include "tpt_tokenindex.h"
#include "ug_ttrack_base.h"
#include "ug_im_ttrack.h"
#include "ug_corpus_token.h"
#include "ug_tsa_tree_iterator.h"
#include "ug_tsa_array_entry.h"
#include "ug_typedefs.h"

namespace sapt
{

  namespace bio=boost::iostreams;

  template<typename TKN>
  TKN const*
  next(TKN const* x)
  {
    return static_cast<TKN const*>(x ? x->next() : NULL);
  }

  /** Base class for [T]oken [S]equence [A]arrays, a generalization of
   * Suffix arrays.
   *
   * Token types (TKN) must provide a number of functions, see the
   * class SimpleWordId (as a simple example of a "core token base
   * class") and the template class L2R_Token (a class derived from
   * its template parameter (e.g. SimpleWordId) that handles the
   * ordering of sequences. Both are decleared/defined in
   * ug_corpus_token.{h|cc}
   */
  template<typename TKN>
  class TSA
  {
  public:
    virtual ~TSA() {};
    typedef TSA_tree_iterator<TKN>       tree_iterator;
    // allows iteration over the array as if it were a trie
    typedef tsa::ArrayEntry                       ArrayEntry;
    /* an entry in the array, for iteration over all occurrences of a
     * particular sequence */
    // typedef boost::dynamic_bitset<uint64_t>           bitset;
    typedef TKN                                        Token;
    /* to allow caching of bit std::vectors that are expensive to create on
     * the fly */

    friend class TSA_tree_iterator<TKN>;

  protected:
    boost::shared_ptr<Ttrack<TKN> const> corpus; // pointer to the underlying corpus
    char const*               startArray; // beginning ...
    char const*                 endArray; // ... and end ...
    // of memory block storing the actual TSA

    size_t corpusSize;
    /** size of the corpus (in number of sentences) of the corpus
     *  underlying the sequence array.
     *
     * ATTENTION: This number may differ from
     *            corpus->size(), namely when the
     *            suffix array is based on a subset
     *            of the sentences of /corpus/.
     */

    id_type numTokens;
    /** size of the corpus (in number of tokens) of the corpus underlying the
     * sequence array.
     *
     * ATTENTION: This number may differ from corpus->numTokens(), namely when
     *            the suffix array is based on a subset of the sentences of
     *            /corpus/.
     */

    id_type indexSize;
    // (number of entries +1) in the index of root-level nodes

    ////////////////////////////////////////////////////////////////
    // private member functions:

    /** @return an index position approximately /fraction/ between
     *  /startRange/ and /endRange/.
     */
    virtual
    char const*
    index_jump(char const* startRange,
               char const* stopRange,
               float fraction) const = 0;

    /** return the index position of the first item that
     *  is equal to or includes [refStart,refStart+refLen) as a prefix
     */
    char const*
    find_start(char const* lo, char const* const upX,
               TKN const* const refStart, int refLen,
               size_t d) const;

    /** return the index position of the first item that is greater than
     *  [refStart,refStart+refLen) and does not include it as a prefix
     */
    char const*
    find_end(char const* lo, char const* const upX,
             TKN const* const refStart, int refLen,
             size_t d) const;

    /** return the index position of the first item that is longer than
     *  [refStart,refStart+refLen) and includes it as a prefix
     */
    char const*
    find_longer(char const* lo, char const* const upX,
                TKN const* const refStart, int refLen,
                size_t d) const;

    /** Returns a char const* pointing to the position in the data block
     *  where the first item starting with token /id/ is located.
     */
    virtual
    char const*
    getLowerBound(id_type id) const = 0;

    virtual
    char const*
    getUpperBound(id_type id) const = 0;

  public:
    char const* arrayStart() const { return startArray; }
    char const* arrayEnd()   const { return endArray;   }

    /** @return a pointer to the beginning of the index entry range covering
     *  [keyStart,keyStop)
     */
    char const*
    lower_bound(typename std::vector<TKN>::const_iterator const& keyStart,
                typename std::vector<TKN>::const_iterator const& keyStop) const;
    char const*
    lower_bound(TKN const* keyStart, TKN const* keyStop) const;

    char const*
    lower_bound(TKN const* keyStart, int keyLen) const;

    /** @return a pointer to the end point of the index entry range covering
     *  [keyStart,keyStop)
     */
    char const*
    upper_bound(typename std::vector<TKN>::const_iterator const& keyStart,
                typename std::vector<TKN>::const_iterator const& keyStop) const;

    char const*
    upper_bound(TKN const* keyStart, int keyLength) const;

    count_type
    setBits(char const* startRange, char const* endRange,
            boost::dynamic_bitset<uint64_t>& bs) const;

    /** read the sentence ID into /sid/
     *  @return position of associated offset.
     *
     *  The function provides an abstraction that uses the right
     *  interpretation of the position based on the subclass
     *  (memory-mapped or in-memory).
     */
    virtual
    char const*
    readSid(char const* p, char const* q, id_type& sid) const = 0;

    virtual
    char const*
    readSid(char const* p, char const* q, ::uint64_t& sid) const = 0;

    /** read the offset part of the index entry into /offset/
     *  @return position of the next entry in the index.
     *
     *  The function provides an abstraction that uses the right
     *  interpretation of the position based on the subclass
     *  (memory-mapped or in-memory).
     */
    virtual
    char const*
    readOffset(char const* p, char const* q, uint16_t& offset) const = 0;

    virtual
    char const*
    readOffset(char const* p, char const* q, ::uint64_t& offset) const = 0;

    /** @return raw occurrence count
     *
     *  depending on the subclass, this is constant time (imTSA) or
     *  linear in in the number of occurrences (mmTSA).
     */
    virtual
    count_type
    rawCnt(char const* p, char const* const q) const = 0;

    /** get both sentence and word counts.
     *
     *  Avoids having to go over the byte range representing the range
     *  of suffixes in question twice when dealing with memory-mapped
     *  suffix arrays.
     */
    virtual
    void
    getCounts(char const* p, char const* const q,
	      count_type& sids, count_type& raw) const = 0;

    tsa::ArrayEntry& readEntry(char const* p, tsa::ArrayEntry& I) const;

    size_t
    getCorpusSize() const;

    Ttrack<TKN> const*
    getCorpus() const;

    double aveIndexEntrySize() const
    {
      return (endArray-startArray)/double(numTokens);
    }

  public:
    // virtual
    SPTR<TSA_tree_iterator<TKN> >
    find(TKN const* start, size_t len) const
    {
      typedef TSA_tree_iterator<TKN> iter;
      SPTR<iter> ret(new iter(this));
      size_t i = 0;
      while (i < len && ret->extend(start[i])) ++i;
      if (i < len) ret.reset();
      return ret;
    }

  };

  // ---------------------------------------------------------------------------

  template<typename TKN>
  count_type
  TSA<TKN>::
  setBits(char const* startRange, char const* endRange,
          bitvector& bs) const
  {
    count_type wcount=0;
    char const* p = startRange;
    id_type sid;
    ushort  off;
    while (p < endRange)
      {
        p = readSid(p,endRange,sid);
        p = readOffset(p,endRange,off);
        bs.set(sid);
        wcount++;
      }
    return wcount;
  }

  //---------------------------------------------------------------------------

  /** return the lower bound (first matching entry)
   *  of the token range matching [startKey,endKey)
   */
  template<typename TKN>
  char const*
  TSA<TKN>::
  find_start(char const* lo, char const* const upX,
	     TKN const* const refStart, int refLen,
	     size_t d) const
  {
    char const* up = upX;
    if (lo >= up) return NULL;
    int x;
    ArrayEntry I;
    while (lo < up)
      {
        readEntry(index_jump(lo,up,.5),I);
        x = corpus->cmp(I,refStart,refLen,d);
        if   (x >= 0) up = I.pos;
        else          lo = I.next;
      }
    assert(lo==up);
    if (lo < upX)
      {
        readEntry(lo,I);
        x = corpus->cmp(I,refStart,refLen,d);
      }
    // return (x >= 0) ? lo : NULL;
    return (x == 0 || x == 1) ? lo : NULL;
  }

  //---------------------------------------------------------------------------

  /** return the upper bound (first entry beyond)
   *  of the token range matching [startKey,endKey)
   */
  template<typename TKN>
  char const*
  TSA<TKN>::
  find_end(char const* lo, char const* const upX,
           TKN const* const refStart, int refLen,
           size_t d) const

  {
    char const* up = upX;
    if (lo >= up) return NULL;
    int x;
    ArrayEntry I;
    // float ratio = .1;
    while (lo < up)
      {
        readEntry(index_jump(lo,up,.1),I);
        x = corpus->cmp(I,refStart,refLen,d);
        if   (x == 2) up = I.pos;
        else          lo = I.next;
        // ratio = .5;
      }
    assert(lo==up);
    if (lo < upX)
      {
        readEntry(lo,I);
        x = corpus->cmp(I,refStart,refLen,d);
      }
    return (x == 2) ? up : upX;
  }

  //---------------------------------------------------------------------------

  /** return the first entry that has the prefix [refStart,refStart+refLen)
   *  but continues on
   */
  template<typename TKN>
  char const*
  TSA<TKN>::
  find_longer(char const* lo, char const* const upX,
              TKN const* const refStart, int refLen,
              size_t d) const
  {
    char const* up = upX;
    if (lo >= up) return NULL;
    int x;
    ArrayEntry I;
    while (lo < up)
      {
        readEntry(index_jump(lo,up,.5),I);
        x = corpus->cmp(I,refStart,refLen,d);
        if   (x > 0) up = I.pos;
        else         lo = I.next;
      }
    assert(lo==up);
    if (lo < upX)
      {
        readEntry(index_jump(lo,up,.5),I);
        x = corpus->cmp(I,refStart,refLen,d);
      }
    return (x == 1) ? up : NULL;
  }

  //---------------------------------------------------------------------------

  /** returns the start position in the byte array representing
   *  the tightly packed sorted list of corpus positions for the
   *  given search phrase
   */
  template<typename TKN>
  char const*
  TSA<TKN>::
  lower_bound(typename std::vector<TKN>::const_iterator const& keyStart,
              typename std::vector<TKN>::const_iterator const& keyStop) const
  {
    TKN const* const a = &(*keyStart);
    TKN const* const z = &(*keyStop);
    return lower_bound(a,z);
  }

  //---------------------------------------------------------------------------

  /** returns the start position in the byte array representing
   *  the tightly packed sorted list of corpus positions for the
   *  given search phrase
   */
  template<typename TKN>
  char const*
  TSA<TKN>::
  lower_bound(TKN const* const keyStart,
              TKN const* const keyStop) const
  {
    return lower_bound(keyStart,keyStop-keyStart);
  }

  template<typename TKN>
  char const*
  TSA<TKN>::
  lower_bound(TKN const* const keyStart, int keyLen) const
  {
    if (keyLen == 0) return startArray;
    char const* const lower = getLowerBound(keyStart->id());
    char const* const upper = getUpperBound(keyStart->id());
    return find_start(lower,upper,keyStart,keyLen,0);
  }
  //---------------------------------------------------------------------------

  /** returns the upper bound in the byte array representing
   *  the tightly packed sorted list of corpus positions for the
   *  given search phrase (i.e., points just beyond the range)
   */
  template<typename TKN>
  char const*
  TSA<TKN>::
  upper_bound(typename std::vector<TKN>::const_iterator const& keyStart,
              typename std::vector<TKN>::const_iterator const& keyStop) const
  {
    TKN const* const a = &((TKN)*keyStart);
    TKN const* const z = &((TKN)*keyStop);
    return upper_bound(a,z-a);
  }

  //---------------------------------------------------------------------------

  /** returns the upper bound in the byte array representing
   *  the tightly packed sorted list of corpus positions for the
   *  given search phrase (i.e., points just beyond the range)
   */
  template<typename TKN>
  char const*
  TSA<TKN>::
  upper_bound(TKN const* keyStart, int keyLength) const
  {
    if (keyLength == 0) return arrayEnd();
    char const* const lower = getLowerBound(keyStart->id());
    char const* const upper = getUpperBound(keyStart->id());
    return find_end(lower,upper,keyStart,keyLength,0);
  }

  //---------------------------------------------------------------------------

  template<typename TKN>
  size_t
  TSA<TKN>::
  getCorpusSize() const
  {
    return corpusSize;
  }

  //---------------------------------------------------------------------------

  template<typename TKN>
  Ttrack<TKN> const*
  TSA<TKN>::
  getCorpus() const
  {
    return corpus.get();
  }

  //---------------------------------------------------------------------------

  template<typename TKN>
  tsa::ArrayEntry &
  TSA<TKN>::
  readEntry(char const* p, tsa::ArrayEntry& I) const
  {
    I.pos  = p;
    p      = readSid(p,endArray,I.sid);
    I.next = readOffset(p,endArray,I.offset);
    assert(I.sid    < corpus->size());
    assert(I.offset < corpus->sntLen(I.sid));
    return I;
  };

  //---------------------------------------------------------------------------

}
#endif
