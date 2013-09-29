// -*- c++ -*-
// Base class for Token Sequence Arrays
// (c) 2007-2010 Ulrich Germann. All rights reserved.
#ifndef _ug_tsa_base_h
#define _ug_tsa_base_h

#include <iostream>
#include <string>

#include <boost/iostreams/device/mapped_file.hpp>

#include "tpt_tokenindex.h"
#include "ug_ttrack_base.h"
#include "ug_corpus_token.h"
#include "ug_tsa_tree_iterator.h"
#include "ug_tsa_array_entry.h"
#include "ug_tsa_bitset_cache.h"
#include "ug_typedefs.h"

namespace ugdiss
{

  using namespace std;
  using namespace boost;
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
    typedef shared_ptr<bitvector>         bitset_pointer;
    typedef TKN                                        Token;
    typedef BitSetCache<TSA<TKN> >                     BSC_t; 
    /* to allow caching of bit vectors that are expensive to create on
     * the fly */

    friend class TSA_tree_iterator<TKN>;

  protected:
    Ttrack<TKN> const* corpus; // pointer to the underlying corpus
    char const*    startArray; // beginning ...
    char const*      endArray; // ... and end ...
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

    size_t BitSetCachingThreshold;
    
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
    shared_ptr<BSC_t> bsc;
    
    char const* arrayStart() const { return startArray; }
    char const* arrayEnd()   const { return endArray;   }

    /** @return a pointer to the beginning of the index entry range covering 
     *  [keyStart,keyStop)
     */
    char const* 
    lower_bound(typename vector<TKN>::const_iterator const& keyStart,
                typename vector<TKN>::const_iterator const& keyStop) const;
    char const* 
    lower_bound(TKN const* keyStart, TKN const* keyStop) const;

    char const* 
    lower_bound(TKN const* keyStart, int keyLen) const;

    /** @return a pointer to the end point of the index entry range covering 
     *  [keyStart,keyStop)
     */
    char const* 
    upper_bound(typename vector<TKN>::const_iterator const& keyStart, 
                typename vector<TKN>::const_iterator const& keyStop) const;

    char const* 
    upper_bound(TKN const* keyStart, int keyLength) const;


    /** dump all suffixes in order to /out/ */
    void dump(ostream& out, TokenIndex const& T) const;
    
    /** fill the dynamic bit set with true for all sentences that contain 
     *  /phrase/.
     *  @return the raw number of occurrences.
     */
    count_type
    fillBitSet(vector<TKN> const& phrase, bdBitset& dest) const;

    count_type
    fillBitSet(TKN const* key, size_t keyLen, bdBitset& dest) const;

    count_type
    setBits(char const* startRange, char const* endRange,
            boost::dynamic_bitset<uint64_t>& bs) const;

    void
    setTokenBits(char const* startRange, char const* endRange, size_t len,
                 bitvector& bs) const;

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
    readSid(char const* p, char const* q, uint64_t& sid) const = 0;

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
    readOffset(char const* p, char const* q, uint64_t& offset) const = 0;

    /** @return sentence count 
     */
    count_type
    sntCnt(char const* p, char const* const q) const; 
    
    count_type
    rawCnt2(TKN const* keyStart, size_t keyLen) const; 

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

    string 
    suffixAt(char const* p, TokenIndex const* V=NULL, size_t maxlen=0) 
      const;

    string 
    suffixAt(ArrayEntry const& I, TokenIndex const* V=NULL, size_t maxlen=0) 
      const;

    tsa::ArrayEntry& readEntry(char const* p, tsa::ArrayEntry& I) const;

    /** return pointer to the end of the data block */
    char const* dataEnd() const;

    bool sanityCheck1() const;
    
    /** Return an ID that represents a given phrase; 
        This should NEVER be 0!
        Structure of a phrase ID: 
        leftmost 32 bits: sentence ID in the corpus
        next 16 bits: offset from the start of the sentence
        next 16 bits: length of the phrase
    */
    uint64_t 
    getSequenceId(typename vector<TKN>::const_iterator const& pstart,
                  typename vector<TKN>::const_iterator const& pstop) const;
    
    uint64_t 
    getSequenceId(TKN const* t, ushort plen) const;
    
    /** Return the phrase represented by phrase ID pid_ */
    string
    getSequence(uint64_t pid, TokenIndex const& V) const;
    
    /** Return the phrase represented by phrase ID pid_ */
    vector<TKN>
    getSequence(uint64_t pid) const;

    TKN const* 
    getSequenceStart(uint64_t) const;

    ushort
    getSequenceLength(uint64_t) const;

    size_t 
    getCorpusSize() const;

    Ttrack<TKN> const*
    getCorpus() const;

    bitset_pointer
    getBitSet(TKN const* startKey, size_t keyLen) const;
    
    shared_ptr<bitvector>
    findTree(TKN const* treeStart, TKN const* treeEnd, 
             bitvector const* filter) const;
    
    size_t markOccurrences(char const* lo, char const* up, size_t len,
                           bitvector& bitset, 
                           bool markOnlyStartPosition) const;

    bool
    findBranches(TKN const* base, bitvector const& terminals,
                 vector<tree_iterator>& dest) const;

    double aveIndexEntrySize() const
    { 
      return (endArray-startArray)/double(numTokens); 
    }

  public:
    // virtual 
    sptr<TSA_tree_iterator<TKN> > 
    find(TKN const* start, size_t len) const
    {
      typedef TSA_tree_iterator<TKN> iter;
      sptr<iter> ret(new iter(this));
      size_t i = 0;
      while (i < len && ret->extend(start[i])) ++i;
      if (i < len) ret.reset();
      return ret;
    }

  };

  // ======================================================================

  // template<typename TOKEN>
  // sptr<TSA_tree_iterator<TOKEN> > 
  // TSA<TOKEN>::
  // find(TOKEN const* start, size_t len) const
  // {
  //   typedef TSA_tree_iterator<TOKEN> iter;
  //   sptr<iter> ret(new iter(this));
  //   size_t i = 0;
  //   while (i < len && ret->extend(start[i])) ++i;
  //   if (i < len) ret.reset();
  //   return ret;
  // }


  // ---------------------------------------------------------------------------


  /** fill the dynamic bitset with information as to which sentences
   *  the phrase occurs in
   * @return number of total occurrences of the phrase in the corpus
   */
  template<typename TKN>
  count_type 
  TSA<TKN>::
  fillBitSet(vector<TKN> const& key,
             bitvector& bitset) const
  {
    if (!key.size()) return 0;
    return fillBitset(&(key[0]),key.size(),bitset);
  }
  
  // ---------------------------------------------------------------------------

  /** fill the dynamic bitset with information as to which sentences
   *  the phrase occurs in
   * @return number of total occurrences of the phrase in the corpus
   */
  template<typename TKN>
  count_type 
  TSA<TKN>::
  fillBitSet(TKN const* key, size_t keyLen,
             bitvector& bitset) const
  {
    char const* lo = lower_bound(key,keyLen);
    char const* up = upper_bound(key,keyLen);
    bitset.resize(corpus->size());
    bitset.reset();
    return setBits(lo,up,bitset);
  }

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

  // ---------------------------------------------------------------------------

  template<typename TKN>
  void
  TSA<TKN>::
  setTokenBits(char const* startRange, char const* endRange, size_t len,
          bitvector& bs) const
  {
    ArrayEntry I;
    I.next = startRange;
    do {
        readEntry(I.next,I);
        Token const*    t = corpus->getToken(I);
        Token const* stop = t->stop(*corpus,I.sid);
        for (size_t i = 1; i < len; ++i)
          {
            assert(t != stop);
            t = t->next();
          }
        assert(t != stop);
        bs.set(t - corpus->sntStart(0));
    } while (I.next != endRange);
  }

  // ---------------------------------------------------------------------------

  template<typename TKN>
  count_type
  TSA<TKN>::
  sntCnt(char const* p, char const* const q) const
  {
    id_type sid; uint16_t off;
    bitvector check(corpus->size());
    while (p < q)
      {
	p = readSid(p,q,sid);
	p = readOffset(p,q,off);
	check.set(sid);
      }
    return check.count();
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
  lower_bound(typename vector<TKN>::const_iterator const& keyStart,
              typename vector<TKN>::const_iterator const& keyStop) const
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
  upper_bound(typename vector<TKN>::const_iterator const& keyStart,
              typename vector<TKN>::const_iterator const& keyStop) const
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
  count_type
  TSA<TKN>::
  rawCnt2(TKN const* keyStart, size_t keyLen) const
  {
    char const* lo = lower_bound(keyStart,keyLen);
    char const* up = upper_bound(keyStart,keyLen);
    // cerr << up-lo << endl;
    return rawCnt(lo,up);
  }

  //---------------------------------------------------------------------------

  template<typename TKN>
  uint64_t
  TSA<TKN>::
  getSequenceId(typename vector<TKN>::const_iterator const& pstart,
                typename vector<TKN>::const_iterator const& pstop) const
  {
    return getSequenceId(&(*pstart),pstop-pstart);
  }
  
  //---------------------------------------------------------------------------

  template<typename TKN>
  uint64_t
  TSA<TKN>::
  getSequenceId(TKN const* pstart, ushort plen) const
  {
    char const* p = lower_bound(pstart,plen);
    if (!p) return 0; // not found!
    ArrayEntry I;
    readEntry(p,I);
    uint64_t ret = I.sid;
    ret <<= 16;
    ret += I.offset;
    ret <<= 16;
    ret += plen;
    return ret;
  }

  //---------------------------------------------------------------------------

  template<typename TKN> 
  vector<TKN>
  TSA<TKN>::
  getSequence(uint64_t pid) const
  {
    size_t   plen = pid % 65536;
    size_t offset = (pid >> 16) % 65536;
    TKN const* w = corpus->sntStart(pid >> 32)+offset;    
    vector<TKN> ret(plen);
    for (size_t i = 0; i < plen; i++, w = w->next())
      {
        assert(w);
        ret[i] = *w;
      }
    return ret;
  }

  template<typename TKN> 
  string
  TSA<TKN>::
  getSequence(uint64_t pid, TokenIndex const& V) const
  {
    ostringstream buf;
    TKN const* a = getSequenceStart(pid);
    buf << V[a->id()];
    size_t len = getSequenceLength(pid);
    for (a = a->next(); --len>0; a = a->next())
      buf << " " << V[a->id()];
    return buf.str();
  }

  
  //---------------------------------------------------------------------------

  template<typename TKN> 
  TKN const*
  TSA<TKN>::
  getSequenceStart(uint64_t pid) const
  {
    size_t offset = (pid >> 16) % 65536;
    return corpus->sntStart(pid >> 32)+offset;    
  }
  
  //---------------------------------------------------------------------------

  template<typename TKN> 
  ushort
  TSA<TKN>::
  getSequenceLength(uint64_t pid) const
  {
    return (pid % 65536);
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
    return corpus;
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
  
  /// find all instances of the tree described by [treeStart, treeEnd)
  template<typename TKN>
  typename TSA<TKN>::bitset_pointer
  TSA<TKN>::
  getBitSet(TKN const* startKey, size_t keyLen) const
  {
    bitset_pointer ret;
    if (bsc != NULL) 
      ret =  bsc->get(startKey,keyLen);
    else
      {
        ret.reset(new bitvector(corpus->size()));
        fillBitSet(startKey,keyLen,*ret);
      }
    return ret;
  }
	    
  //---------------------------------------------------------------------------

  template<typename TKN>
  size_t
  TSA<TKN>::
  markOccurrences(char const* lo, char const* up, size_t len,
                  bitvector& bitset, bool markOnlyStartPosition) const
  {
    id_type sid;
    ushort  off;
    count_type wcount=0;
    TKN const* crpStart = corpus->sntStart(0);
    char const* p = lo;
    while (p < up)
      {
        p = readSid(p,up,sid);
        p = readOffset(p,up,off);
        TKN const* t = corpus->sntStart(sid)+off;
        if (markOnlyStartPosition)
          bitset.set(t-crpStart);
        else
          for (size_t i = 0; i < len; ++i, t = t->next())
            bitset.set(t-crpStart);
        wcount++;
      }
    return wcount;
  }
#if 1
  template<typename TKN>
  bool
  TSA<TKN>::
  findBranches(TKN const* base, bitvector const& terminals,
               vector<tree_iterator>& dest) const
  {
    dest.assign(terminals.count(),tree_iterator(this));
    for (size_t i = terminals.find_first(), k = 0; 
         i < terminals.size(); 
         i = terminals.find_next(i),++k)
      {
        for (TKN const* x = base+i; x && x->id(); x = x->next())
          if (!dest[k].extend(x->id())) 
            return false;
      }
    typename tree_iterator::SortByApproximateCount sorter;
    sort(dest.begin(),dest.end(),sorter);
    return true;
  }
#endif

}
#endif
