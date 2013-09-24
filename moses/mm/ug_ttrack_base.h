// -*- c++ -*-
// Base class for corpus tracks. mmTtrack (memory-mapped Ttrack) and imTtrack (in-memory Ttrack)
// are derived from this class.

// This code is part of a refactorization of the earlier Ttrack class as a template class for 
// tokens of arbitrary fixed-length size.
// (c) 2007-2009 Ulrich Germann. All rights reserved.

#ifndef __ug_ttrack_base
#define __ug_ttrack_base

#include <string>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "ug_ttrack_position.h"
#include "tpt_typedefs.h"
#include "tpt_tokenindex.h"
// #include "ug_vocab.h"

namespace ugdiss
{
  using namespace std;

  typedef boost::dynamic_bitset<uint64_t> bdBitset;

  template<typename TKN=id_type>
  class Ttrack
  {
  protected:
    id_type numSent;
    id_type numWords;

  public:

    virtual ~Ttrack() {};
    typedef typename ttrack::Position Position;
    typedef TKN Token;

    /** @return a pointer to beginning of sentence /sid/ */
    virtual 
    TKN const* 
    sntStart(size_t sid) const = 0; 

    /** @return end point of sentence /sid/ */
    virtual 
    TKN const* 
    sntEnd(size_t sid) const = 0; 

    TKN const*
    getToken(Position const& p) const;

    template<typename T>
    T const* 
    getTokenAs(Position const& p) const  
    { return reinterpret_cast<T const*>(getToken(p));  }

    template<typename T>
    T const*
    sntStartAs(id_type sid) const 
    { return reinterpret_cast<T const*>(sntStart(sid)); }

    template<typename T>
    T const*
    sntEndAs(id_type sid) const 
    { return reinterpret_cast<T const*>(sntEnd(sid));  }

    /** @return length of sentence /sid/ */
    size_t sntLen(size_t sid) const { return sntEnd(sid) - sntStart(sid); }

    size_t 
    startPos(id_type sid) const { return sntStart(sid)-sntStart(0); }
    
    size_t 
    endPos(id_type sid) const { return sntEnd(sid)-sntStart(0); }

    /** Don't use this unless you want a copy of the sentence */
    vector<TKN> 
    operator[](id_type sid) const 
    { 
      return vector<TKN>(sntStart(sid),sntEnd(sid)); 
    }

    /** @return size of corpus in number of sentences */
    virtual size_t size() const = 0;      

    /** @return size of corpus in number of words/tokens */
    virtual size_t numTokens() const = 0; 

    /** @return string representation of sentence /sid/ 
     *  Currently only defined for Ttrack<id_type> */
    string str(id_type sid, TokenIndex const& T) const;

    // /** @return string representation of sentence /sid/ 
    //  *  Currently only defined for Ttrack<id_type> */
    // string str(id_type sid, Vocab const& V) const;
    
    /** counts the tokens in the corpus; used for example in the construction of 
     *  token sequence arrays */
    count_type count_tokens(vector<count_type>& cnt, bdBitset const& filter,
                            int lengthCutoff=0, ostream* log=NULL) const;

    // static id_type toID(TKN const& t);

    int cmp(Position const& A, Position const& B, int keyLength) const;
    int cmp(Position const& A, TKN const* keyStart, int keyLength=-1, 
            int depth=0) const;

    virtual id_type findSid(TKN const* t) const = 0; // find the sentence id of a given token
    // virtual id_type findSid(id_type TokenOffset) const = 0; // find the sentence id of a given token


    // the following three functions are currently not used by any program ... (deprecate?)
    TKN const*
    find_next_within_sentence(TKN const* startKey, 
                              int         keyLength, 
                              Position    startHere) const;

    Position
    find_first(TKN const* startKey, int keyLength, 
               bdBitset const* filter=NULL) const;

    Position
    find_next(TKN const* startKey, int keyLength, Position startAfter, 
              bdBitset const* filter=NULL) const;
   

    virtual size_t offset(TKN const* t) const { return t-sntStart(0); }
  };

  // ---------------------------------------------------------------------------

  template<typename TKN>
  TKN const*
  Ttrack<TKN>::
  getToken(Position const& p) const
  {
    TKN const* ret = sntStart(p.sid)+p.offset;
    return (ret < sntEnd(p.sid)) ? ret : NULL;
  }

  // ---------------------------------------------------------------------------

  template<typename TKN>
  count_type
  Ttrack<TKN>::
  count_tokens(vector<count_type>& cnt, bdBitset const& filter,
               int lengthCutoff, ostream* log) const
  {
    cnt.clear();
    cnt.reserve(500000);
    count_type totalCount=0;
    int64_t expectedTotal=numTokens();
    for (size_t sid = filter.find_first();
	 sid < filter.size();
	 sid = filter.find_next(sid))
      {
	TKN const* k = sntStart(sid);
	TKN const* const stop = sntEnd(sid);
        if (lengthCutoff && stop-k >= lengthCutoff) 
          {
            if (log) 
              *log << "WARNING: skipping sentence #" << sid 
                   << " with more than 65536 tokens" << endl;
            expectedTotal -= stop-k;
          }
        else
          {
            totalCount += stop-k;
            for (; k < stop; ++k)
              {
		// cout << sid << " " << stop-k << " " << k->lemma << " " << k->id() << " " << sizeof(*k) << endl;
                id_type wid = k->id();
                while (wid >= cnt.size()) cnt.push_back(0);
                cnt[wid]++;
              }
          }
      }
    if (this->size() == filter.count())
      {
        if (totalCount != expectedTotal)
          cerr << "OOPS: expected " << expectedTotal 
               << " tokens but counted " << totalCount << endl;
        assert(totalCount == expectedTotal);
      }
    return totalCount;
  }

  template<typename TKN>
  int
  Ttrack<TKN>::
  cmp(Position const& A, Position const& B, int keyLength) const
  {
    if (keyLength==0) return 2;
    assert(A.sid < this->size());
    assert(B.sid < this->size());

    TKN const* a    = getToken(A);
    TKN const* bosA = sntStart(A.sid);
    TKN const* eosA = sntEnd(A.sid);

    TKN const* b    = getToken(B);
    TKN const* bosB = sntStart(B.sid);
    TKN const* eosB = sntEnd(B.sid);

    int ret=-1;

#if 0
    cerr << "A: "; for (TKN const* x = a; x; x = next(x)) cerr << x->lemma << " "; cerr << endl;
    cerr << "B: "; for (TKN const* x = b; x; x = next(x)) cerr << x->lemma << " "; cerr << endl;
#endif

    while (a >= bosA && a < eosA)
      {
        // cerr << keyLength << "a. " << (a ? a->lemma : 0) << " " << (b ? b->lemma : 0) << endl;
	if (*a < *b) {          break; } // return -1;
        if (*a > *b) { ret = 2; break; } // return  2;
        a = next(a);
        b = next(b);
        // cerr << keyLength << "b. " << (a ? a->lemma : 0) << " " << (b ? b->lemma : 0) << endl;
        if (--keyLength==0 || b < bosB || b >= eosB) 
          { 
            ret = (a < bosA || a >= eosA) ? 0 : 1;
            break;
          }
      }
    // cerr << "RETURNING " << ret << endl;
    return ret; 
  }
  
  template<typename TKN>
  int
  Ttrack<TKN>::
  cmp(Position const& A, TKN const* key, int keyLength, int depth) const
  {
    if (keyLength==0 || !key) return 2;
    assert(A.sid < this->size());
    TKN const* x   = getToken(A);
    TKN const* stopx = x->stop(*this,A.sid);
    for (int i = 0; i < depth; ++i)
      {
        x = x->next();
        if (x == stopx) return -1;
        // assert(x != stopx);
      }
    while (x != stopx)
      {
	if (*x < *key) return -1;
        if (*x > *key) return  2;
        key  = key->next();
        x    = x->next();
        if (--keyLength==0) //  || !key) 
          return (x == stopx) ? 0 : 1;
        assert(key);
      }
    return -1; 
  }

  template<typename TKN>
  TKN const* 
  Ttrack<TKN>::
  find_next_within_sentence(TKN const* startKey, int keyLength, 
                            Position startHere) const
  {
    for (TKN const* t = getToken(startHere); t; t = getToken(startHere))
      {
#if 0
        int foo = cmp(startHere,startKey,1);
        if (foo == 0 || foo ==1)
          {
            TKN const* k = startKey->next();
            TKN const* t2 = t->next();
            if (t2) 
              {
                cout << t2->lemma << "." << int(t2->minpos) << " " 
                     << k->lemma << "." << int(k->minpos) << " "
                     << t2->cmp(*k) << endl;
              }            
          }
#endif
        int x = cmp(startHere,startKey,keyLength,0);
        if (x == 0 || x == 1) return t;
        startHere.offset++;
      }
    return NULL;
  }

  template<typename TKN>
  typename Ttrack<TKN>::Position
  Ttrack<TKN>::
  find_first(TKN const* startKey, int keyLength, bdBitset const* filter) const
  {
    if (filter)
      {
        for (size_t sid = filter->find_first(); 
             sid < filter->size(); 
             sid = filter->find_next(sid))
          {
            TKN const* x = find_next_within_sentence(startKey,keyLength,Position(sid,0));
            if (x) return Position(sid,x-sntStart(sid));
          }
      }
    else
      {
        for (size_t sid = 0; sid < this->size(); ++sid)
          {
            TKN const* x = find_next_within_sentence(startKey,keyLength,Position(sid,0));
            if (x) return Position(sid,x-sntStart(sid));
          }
      }
    return Position(this->size(),0);
  }
  
  template<typename TKN>
  typename Ttrack<TKN>::Position
  Ttrack<TKN>::
  find_next(TKN const* startKey, int keyLength, Position startAfter, bdBitset const* filter) const
  {
    id_type sid = startAfter.sid;
    startAfter.offset++;
    if (filter) assert(filter->test(sid));
    TKN const* x = find_next_within_sentence(startKey,keyLength,startAfter);
    if (x) return Position(sid,x -sntStart(sid));
    if (filter)
      {
        for (sid = filter->find_next(sid); sid < filter->size(); sid = filter->find_next(sid))
          {
            x = find_next_within_sentence(startKey,keyLength,Position(sid,0));
            if (x) break;
          }
      }
    else
      {
        for (++sid; sid < this->size(); sid++)
          {
            x = find_next_within_sentence(startKey,keyLength,Position(sid,0));
            if (x) break;
          }
      }
    if (x)
      return Position(sid,x-sntStart(sid));
    else
      return Position(this->size(),0);
  }

}
#endif
