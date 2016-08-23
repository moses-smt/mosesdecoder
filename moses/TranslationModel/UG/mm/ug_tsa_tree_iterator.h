// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// (c) 2007 - 2010 Ulrich Germann. All rights reserved.
#ifndef __ug_tsa_tree_iterator_h
#define __ug_tsa_tree_iterator_h

#include "ug_tsa_array_entry.h"
#include "ug_typedefs.h"
#include "tpt_tokenindex.h"
#include <iostream>
#include "util/exception.hh"
#include "moses/Util.h"
#include "util/random.hh"

namespace sapt
{

#ifndef _DISPLAY_CHAIN
#define _DISPLAY_CHAIN
  // for debugging only
  template<typename T>
  void display(T const* x, std::string label)
  {
    std::cout << label << ":";
    for (;x;x=next(x)) std::cout << " " << x->lemma;
    std::cout << std::endl;
  }
#endif

  template<typename T> class TSA;

  // CLASS DEFINITION
  // The TSA_tree_iterator allows traversal of a Token Sequence Array
  // as if it was a trie.
  //
  // down(): go to first child
  // over(): go to next sibling
  // up():   go to parent
  // extend(id): go to a specific child node
  // all four functions return true if successful, false otherwise
  // lower_bound() and upper_bound() give the range of entries in the
  // array covered by the "virtual trie node".
  template<typename TKN>
  class
  TSA_tree_iterator
  {
  protected:
    std::vector<char const*> lower;
    std::vector<char const*> upper;

    // for debugging ...
    void showBounds(std::ostream& out) const;
  public:
    typedef TKN Token;

    virtual ~TSA_tree_iterator() {};

    TSA<Token> const* root;
    // TO BE DONE: make the pointer private and add a const function
    // to return the pointer

    // TSA_tree_iterator(TSA_tree_iterator const& other);
    TSA_tree_iterator(TSA<Token> const* s);
    TSA_tree_iterator(TSA<Token> const* s, TSA_tree_iterator<Token> const& other);
    TSA_tree_iterator(TSA<Token> const* r, id_type const* s, size_t const len);
    // TSA_tree_iterator(TSA<Token> const* s, Token const& t);
    TSA_tree_iterator(TSA<Token> const* s,
		      Token const* kstart,
		      size_t const len,
		      bool full_match_only=true);
    TSA_tree_iterator(TSA<Token> const* s,
		      Token const* kstart,
		      Token const* kend,
		      bool full_match_only=true);
    TSA_tree_iterator(TSA<Token> const* s,
    		      TokenIndex const& V,
     		      std::string const& key);

    char const* lower_bound(int p) const;
    char const* upper_bound(int p) const;

    size_t size() const;
    // Token const& wid(int p) const;
    Token const* getToken(int p) const;
    id_type getSid() const;
    ushort getOffset(int p) const;
    size_t sntCnt(int p=-1) const;
    size_t rawCnt(int p=-1) const;
    uint64_t getPid(int p=-1) const; // get phrase id

    virtual bool extend(Token const& id);
    virtual bool extend(id_type id);
    virtual bool down();
    virtual bool over();
    virtual bool up();

    std::string str(TokenIndex const* V=NULL, int start=0, int stop=0) const;

    // checks if the sentence [start,stop) contains the given sequence.
    bool match(Token const* start, Token const* stop) const;
    // checks if the sentence /sid/ contains the given sequence.
    bool match(id_type sid) const;

    // fillBitSet: deprecated; use markSentences() instead
    count_type
    fillBitSet(boost::dynamic_bitset<uint64_t>& bitset) const;

    count_type
    markEndOfSequence(Token const*  start, Token const*  stop,
		      boost::dynamic_bitset<uint64_t>& dest) const;
    count_type
    markSequence(Token const* start, Token const* stop, bitvector& dest) const;

    count_type
    markSentences(boost::dynamic_bitset<uint64_t>& bitset) const;

    count_type
    markOccurrences(boost::dynamic_bitset<uint64_t>& bitset,
		    bool markOnlyStartPosition=false) const;

    count_type
    markOccurrences(std::vector<ushort>& dest) const;

    ::uint64_t
    getSequenceId() const;

    // equivalent but more efficient than
    // bitvector tmp; markSentences(tmp); foo &= tmp;
    bitvector& filterSentences(bitvector& foo) const;

    /// a special auxiliary function for finding trees
    void
    tfAndRoot(bitvector const& ref, // reference root positions
              bitvector const& snt, // relevant sentences
              bitvector& dest) const;

    size_t arrayByteSpanSize(int p = -1) const
    {
      if (lower.size()==0) return 0; // or endArray-startArray???
      if (p < 0) p = lower.size()+p;
      assert(p >=0 && p < int(lower.size()));
      return lower.size() ? upper[p]-lower[p] : 0;
    }

    struct SortByApproximateCount
    {
      bool operator()(TSA_tree_iterator const& a,
                      TSA_tree_iterator const& b) const
      {
        if (a.size()==0) return b.size() ? true : false;
        if (b.size()==0) return false;
        return a.arrayByteSpanSize() < b.arrayByteSpanSize();
      }
    };

    double 
    ca(int p=-1) const // approximate occurrence count
    {
      assert(root);
      if (p < 0) p += lower.size();
      double ret = arrayByteSpanSize(p)/root->aveIndexEntrySize();
      // for larger numbers, the estimate is reasonably accurate.
      // if the estimate is small, scan the index range and perform
      // an exact count
      if (ret < 25) ret = rawCnt(p);
      UTIL_THROW_IF2(ret > root->corpus->numTokens(), "[" << HERE << "] "
		     << "Word count mismatch.");
      assert(ret <= root->corpus->numTokens());
      return ret;
    }

    inline
    double 
    approxOccurrenceCount(int p=-1) const // deprecated, use ca()
    {
      return ca();
    }

    size_t grow(Token const* t, Token const* stop)
    {
      while ((t != stop) && extend(*t)) t = t->next();
      return this->size();
    }

    size_t grow(Token const* snt, bitvector const& cov)
    {
      size_t x = cov.find_first();
      while (x < cov.size() && extend(snt[x]))
        x = cov.find_next(x);
      return this->size();
    }

    SPTR<std::vector<typename ttrack::Position> >
    randomSample(int level, size_t N) const;

  };

  //---------------------------------------------------------------------------
  // DOWN
  //---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  bool
  TSA_tree_iterator<TSA_TYPE>::
  down()
  {
    assert(root);
    if (lower.size() == 0)
      {
	char const* lo = root->arrayStart();
        assert(lo < root->arrayEnd());
	if (lo == root->arrayEnd()) return false; // array is empty, can't go down
        tsa::ArrayEntry A(root,lo);
        assert(root->corpus->getToken(A));
        assert(lo < root->getUpperBound(root->corpus->getToken(A)->id()));
        lower.push_back(lo);
        Token const* foo = this->getToken(0);
        upper.push_back(root->upper_bound(foo,lower.size()));
        return lower.size();
      }
    else
      {
        char const* lo = lower.back();
        tsa::ArrayEntry A(root,lo);
        Token const* a = root->corpus->getToken(A); assert(a);
        Token const* z = next(a);
        for (size_t i = 1; i < size(); ++i) z = next(z);
        if (z < root->corpus->sntStart(A.sid) || z >= root->corpus->sntEnd(A.sid))
          {
            char const* up = upper.back();
            lo = root->find_longer(lo,up,a,lower.size(),0);
            if (!lo) return false;
            root->readEntry(lo,A);
            a = root->corpus->getToken(A); assert(a);
            z = next(a);
            assert(z >= root->corpus->sntStart(A.sid) && z < root->corpus->sntEnd(A.sid));
          }
        lower.push_back(lo);
        char const* up = root->getUpperBound(a->id());
        char const* u  = root->find_end(lo,up,a,lower.size(),0);
        assert(u);
        upper.push_back(u);
        return true;
      }
  }

  // ---------------------------------------------------------------------------
  // OVER
  //---------------------------------------------------------------------------

  template<typename Token>
  bool
  TSA_tree_iterator<Token>::
  over()
  {
    if (lower.size() == 0)
      return false;
    if (lower.size() == 1)
      {
        Token const* t = this->getToken(0);
	id_type wid = t->id();
        char const* hi = root->getUpperBound(wid);
        if (upper[0] < hi)
          {
            lower[0] = upper[0];
            Token const* foo = this->getToken(0);
            upper.back() = root->upper_bound(foo,lower.size());
          }
        else
          {
            for (++wid; wid < root->indexSize; ++wid)
              {
                char const* lo = root->getLowerBound(wid);
                if (lo == root->endArray) return false;
                char const* hi = root->getUpperBound(wid);
                if (!hi) return false;
                if (lo == hi) continue;
                assert(lo);
                lower[0] = lo;
                Token const* foo = this->getToken(0);
                upper.back() = root->upper_bound(foo,lower.size());
                break;
              }
          }
        return wid < root->indexSize;
      }
    else
      {
        if (upper.back() == root->arrayEnd())
          return false;
        tsa::ArrayEntry L(root,lower.back());
        tsa::ArrayEntry U(root,upper.back());

        // display(root->corpus->getToken(L),"L1");
        // display(root->corpus->getToken(U),"U1");

	int x = root->corpus->cmp(U,L,lower.size()-1);
	// cerr << "x=" << x << std::endl;
        if (x != 1)
          return false;
        lower.back() = upper.back();

        // display(root->corpus->getToken(U),"L2");

        Token const* foo = this->getToken(0);
        // display(foo,"F!");
        upper.back() = root->upper_bound(foo,lower.size());
        return true;
      }
  }

  // ---------------------------------------------------------------------------
  // UP
  //---------------------------------------------------------------------------

  template<typename Token>
  bool
  TSA_tree_iterator<Token>::
  up()
  {
    if (lower.size())
      {
        lower.pop_back();
        upper.pop_back();
        return true;
      }
    else
      return false;
  }


  // ---------------------------------------------------------------------------
  // CONSTRUCTORS
  //----------------------------------------------------------------------------
  template<typename Token>
  TSA_tree_iterator<Token>::
  TSA_tree_iterator(TSA<Token> const* s)
    : root(s)
  {};

  template<typename Token>
  TSA_tree_iterator<Token>::
  TSA_tree_iterator(TSA<Token> const* s, TSA_tree_iterator<Token> const& other)
    : root(s)
  {
    Token const* x = other.getToken(0);
    for (size_t i = 0; i < other.size() && this->extend(x->id()); ++i)
      x = x->next();
  };



  template<typename Token>
  TSA_tree_iterator<Token>::
  TSA_tree_iterator
  (TSA<Token> const* r,
   id_type    const* s,
   size_t     const  len)
    : root(r)
  {
    for (id_type const* e = s + len; s < e && extend(*s); ++s);
  };

  // ---------------------------------------------------------------------------

#if 1
  template<typename Token>
  TSA_tree_iterator<Token>::
  TSA_tree_iterator(TSA<Token> const* s,
		    TokenIndex const& V,
		    std::string const& key)
    : root(s)
  {
    std::istringstream buf(key); std::string w;
    while (buf >> w)
      {
	if (this->extend(V[w]))
	  continue;
	else
	  {
	    lower.clear();
	    upper.clear();
	    break;
	  }
      }
  };
#endif

#if 0
  // ---------------------------------------------------------------------------

  template<typename Token>
  TSA_tree_iterator<Token>::
  TSA_tree_iterator(TSA_tree_iterator<Token> const& other)
    : root(other.root)
  {
    lower = other.lower;
    upper = other.upper;
  };

  // ---------------------------------------------------------------------------

  template<typename Token>
  TSA_tree_iterator<Token>::
  TSA_tree_iterator(TSA<Token> const* s, Token const& t)
    : root(s)
  {
    if (!root) return;
    char const* up = root->getUpperBound(t.id());
    if (!up) return;
    lower.push_back(root->getLowerBound(t.id()));
    upper.push_back(up);
  };

  // ---------------------------------------------------------------------------

#endif

  template<typename Token>
  TSA_tree_iterator<Token>::
  TSA_tree_iterator(TSA<Token> const* s, Token const* kstart,
		    size_t const len, bool full_match_only)
    : root(s)
  {
    if (!root) return;
    size_t i = 0;
    for (; i < len && kstart && extend(*kstart); ++i)
      kstart = kstart->next();
    if (full_match_only && i != len)
      {
        lower.clear();
        upper.clear();
      }
  };

  // DEPRECATED: DO NOT USE. Use the one that takes the length
  // instead of kend.
  template<typename Token>
  TSA_tree_iterator<Token>::
  TSA_tree_iterator(TSA<Token> const* s, Token const* kstart,
		    Token const* kend, bool full_match_only)
    : root(s)
  {
    for (;kstart != kend; kstart = kstart->next())
      if (!extend(*kstart))
        break;
    if (full_match_only && kstart != kend)
      {
        lower.clear();
        upper.clear();
      }
  };

  // ---------------------------------------------------------------------------
  // EXTEND
  // ---------------------------------------------------------------------------

  template<typename Token>
  bool
  TSA_tree_iterator<Token>::
  extend(id_type const id)
  {
    return extend(Token(id));
  }


  template<typename Token>
  bool
  TSA_tree_iterator<Token>::
  extend(Token const& t)
  {
    if (lower.size())
      {
        char const* lo = lower.back();
        char const* hi = upper.back();
        lo = root->find_start(lo, hi, &t, 1, lower.size());
        if (!lo) return false;
        lower.push_back(lo);
        hi = root->find_end(lo, hi, getToken(-1), 1, lower.size()-1);
        upper.push_back(hi);
      }
    else
      {
        char const* lo = root->getLowerBound(t.id());
        char const* hi = root->getUpperBound(t.id());

        if (lo==hi) return false;
        lo = root->find_start(lo, hi, &t, 1, lower.size());
        if (!lo) return false;
        lower.push_back(lo);
#if 0
        tsa::ArrayEntry I;
        root->readEntry(lo,I);
        cout << I.sid << " " << I.offset << std::endl;
        cout << root->corpus->sntLen(I.sid) << std::endl;
#endif
        hi = root->find_end(lo, hi, getToken(0), 1, 0);
        upper.push_back(hi);
      }
    return true;
  };

  // ---------------------------------------------------------------------------

  template<typename Token>
  size_t
  TSA_tree_iterator<Token>::
  size() const
  {
    return lower.size();
  }

  // ---------------------------------------------------------------------------

  template<typename Token>
  id_type
  TSA_tree_iterator<Token>::
  getSid() const
  {
    char const* p = (lower.size() ? lower.back() : root->startArray);
    char const* q = (upper.size() ? upper.back() : root->endArray);
    id_type sid;
    root->readSid(p,q,sid);
    return sid;
  }

  // ---------------------------------------------------------------------------

  template<typename Token>
  ::uint64_t
  TSA_tree_iterator<Token>::
  getPid(int p) const
  {
    if (this->size() == 0) return 0;
    if (p < 0) p += upper.size();
    char const* lb = lower_bound(p);
    char const* ub = upper_bound(p);
    ::uint64_t sid,off;
    root->readOffset(root->readSid(lb,ub,sid),ub,off);
    ::uint64_t ret = (sid<<32) + (off<<16) + ::uint64_t(p+1);
    return ret;
  }

  // ---------------------------------------------------------------------------

  template<typename Token>
  char const*
  TSA_tree_iterator<Token>::
  lower_bound(int p) const
  {
    if (p < 0) p += lower.size();
    assert(p >= 0 && p < int(lower.size()));
    return lower[p];
  }

  // ---------------------------------------------------------------------------

  template<typename Token>
  char const*
  TSA_tree_iterator<Token>::
  upper_bound(int p) const
  {
    if (p < 0) p += upper.size();
    assert(p >= 0 && p < int(upper.size()));
    return upper[p];
  }

  // ---------------------------------------------------------------------------

  /* @return a pointer to the position in the corpus
   * where this->wid(p) is read from
   */
  template<typename Token>
  Token const*
  TSA_tree_iterator<Token>::
  getToken(int p) const
  {
    if (lower.size()==0) return NULL;
    tsa::ArrayEntry A(root,lower.back());
    Token const* t   = root->corpus->getToken(A); assert(t);
#ifndef NDEBUG
    Token const* bos = root->corpus->sntStart(A.sid);
    Token const* eos = root->corpus->sntEnd(A.sid);
#endif
    if (p < 0) p += lower.size();
    // cerr << p << ". " << t->id() << std::endl;
    while (p-- > 0)
      {
        t = next(t);
	// if (t) cerr << p << ". " << t->id() << std::endl;
        assert(t >= bos && t < eos);
      }
    return t;
  }

  // ---------------------------------------------------------------------------

  template<typename Token>
  size_t
  TSA_tree_iterator<Token>::
  sntCnt(int p) const
  {
    if (p < 0)
      p = lower.size()+p;
    assert(p>=0);
    if (lower.size() == 0) return root->getCorpusSize();
    return reinterpret_cast<TSA<Token> const* const>(root)->sntCnt(lower[p],upper[p]);
  }

  // ---------------------------------------------------------------------------

  template<typename Token>
  size_t
  TSA_tree_iterator<Token>::
  rawCnt(int p) const
  {
    if (p < 0) p += lower.size();
    assert(p>=0);
    if (lower.size() == 0) return root->getCorpusSize();
    return root->rawCnt(lower[p],upper[p]);
  }

  //---------------------------------------------------------------------------

  template<typename Token>
  count_type
  TSA_tree_iterator<Token>::
  fillBitSet(boost::dynamic_bitset<uint64_t>& bitset) const
  {
    return markSentences(bitset);
  }

  //---------------------------------------------------------------------------

  template<typename Token>
  count_type
  TSA_tree_iterator<Token>::
  markSentences(boost::dynamic_bitset<uint64_t>& bitset) const
  {
    assert(root && root->corpus);
    bitset.resize(root->corpus->size());
    bitset.reset();
    if (lower.size()==0) return 0;
    char const* lo = lower.back();
    char const* up = upper.back();
    char const* p = lo;
    id_type sid;
    ushort  off;
    count_type wcount=0;
    while (p < up)
      {
        p = root->readSid(p,up,sid);
        p = root->readOffset(p,up,off);
        bitset.set(sid);
        wcount++;
      }
    return wcount;
  }

  //---------------------------------------------------------------------------

  template<typename Token>
  count_type
  TSA_tree_iterator<Token>::
  markOccurrences(boost::dynamic_bitset<uint64_t>& bitset, bool markOnlyStartPosition) const
  {
    assert(root && root->corpus);
    if (bitset.size() != root->corpus->numTokens())
      bitset.resize(root->corpus->numTokens());
    bitset.reset();
    if (lower.size()==0) return 0;
    char const* lo = lower.back();
    char const* up = upper.back();
    return root->markOccurrences(lo,up,lower.size(),bitset,markOnlyStartPosition);
  }
  //---------------------------------------------------------------------------

  template<typename Token>
  count_type
  TSA_tree_iterator<Token>::
  markOccurrences(std::vector<ushort>& dest) const
  {
    assert(root && root->corpus);
    assert(dest.size() == root->corpus->numTokens());
    if (lower.size()==0) return 0;
    char const* lo = lower.back();
    char const* up = upper.back();
    char const* p = lo;
    id_type sid;
    ushort  off;
    count_type wcount=0;
    Token const* crpStart = root->corpus->sntStart(0);
    while (p < up)
      {
        p = root->readSid(p,up,sid);
        p = root->readOffset(p,up,off);
        Token const* t = root->corpus->sntStart(sid)+off;
        for (size_t i = 1; i < lower.size(); ++i, t = t->next());
        dest[t-crpStart]++;
        wcount++;
      }
    return wcount;
  }
  //---------------------------------------------------------------------------

  // mark all endpoints of instances of the path represented by this
  // iterator in the sentence [start,stop)
  template<typename Token>
  count_type
  TSA_tree_iterator<Token>::
  markEndOfSequence(Token const*  start, Token const*  stop,
                    boost::dynamic_bitset<uint64_t>& dest) const
  {
    count_type matchCount=0;
    Token const* a = getToken(0);
    for (Token const* x = start; x < stop; ++x)
      {
        if (*x != *a) continue;
        Token const* y = x;
        Token const* b = a;
        size_t i;
        for (i = 0; *b==*y && ++i < this->size();)
          {
            b = b->next();
            y = y->next();
            if (y < start || y >= stop) break;
          }
        if (i == this->size())
          {
            dest.set(y-start);
            ++matchCount;
          }
      }
    return matchCount;
  }
  //---------------------------------------------------------------------------

  // mark all occurrences of the sequence represented by this
  // iterator in the sentence [start,stop)
  template<typename Token>
  count_type
  TSA_tree_iterator<Token>::
  markSequence(Token const*  start,
               Token const*  stop,
               bitvector& dest) const
  {
    count_type numMatches=0;
    Token const* a = getToken(0);
    for (Token const* x = start; x < stop; ++x)
      {
        if (*x != *a) continue;
        Token const* y = x;
        Token const* b = a;
        size_t i;
        for (i = 0; *b==*y && i++ < this->size();)
          {
            dest.set(y-start);
            b = b->next();
            y = y->next();
            if (y < start || y >= stop) break;
          }
        if (i == this->size()) ++numMatches;
      }
    return numMatches;
  }
  //---------------------------------------------------------------------------

  template<typename Token>
  ::uint64_t
  TSA_tree_iterator<Token>::
  getSequenceId() const
  {
    if (this->size() == 0) return 0;
    char const* p = this->lower_bound(-1);
    typename Token::ArrayEntry I;
    root->readEntry(p,I);
    return (::uint64_t(I.sid)<<32)+(I.offset<<16)+this->size();
  }

  template<typename Token>
  std::string
  TSA_tree_iterator<Token>::
  str(TokenIndex const* V, int start, int stop) const
  {
    if (this->size()==0) return "";
    if (start < 0) start = this->size()+start;
    if (stop <= 0) stop  = this->size()+stop;
    assert(start>=0 && start < int(this->size()));
    assert(stop > 0 && stop <= int(this->size()));
    Token const* x = this->getToken(0);
    std::ostringstream buf;
    for (int i = start; i < stop; ++i, x = x->next())
      {
        assert(x);
        buf << (i > start ? " " : "");
        if (V) buf << (*V)[x->id()];
        else   buf << x->id();
      }
    return buf.str();
  }

#if 0
  template<typename Token>
  string
  TSA_tree_iterator<Token>::
  str(Vocab const& V, int start, int stop) const
  {
    if (this->size()==0) return "";
    if (start < 0) start = this->size()+start;
    if (stop <= 0) stop  = this->size()+stop;
    assert(start>=0 && start < int(this->size()));
    assert(stop > 0 && stop <= int(this->size()));
    Token const* x = this->getToken(0);
    std::ostringstream buf;
    for (int i = start; i < stop; ++i, x = x->next())
      {
        assert(x);
        buf << (i > start ? " " : "");
        buf << V[x->id()].str;
      }
    return buf.str();
  }
#endif

  /// @return true if the sentence [start,stop) contains the sequence
  template<typename Token>
  bool
  TSA_tree_iterator<Token>::
  match(Token const* start, Token const* stop) const
  {
    Token const* a = getToken(0);
    for (Token const* t = start; t < stop; ++t)
      {
        if (*t != *a) continue;
        Token const* b = a;
        Token const* y = t;
        size_t i;
        for (i = 1; i < lower.size(); ++i)
          {
            y = y->next();
            if (y < start || y >= stop) break;
            b = b->next();
            if (*b != *y) break;
          }
        if (i == lower.size()) return true;
      }
    return false;
  }

  /// @return true if the sentence /sid/ contains the sequence
  template<typename Token>
  bool
  TSA_tree_iterator<Token>::
  match(id_type sid) const
  {
    return match(root->corpus->sntStart(sid),root->corpus->sntEnd(sid));
  }

  /// a special auxiliary function for finding trees
  // @param sntcheck: number of roots in the respective sentence
  // @param dest:     bitvector to keep track of the exact root location
  template<typename Token>
  void
  TSA_tree_iterator<Token>::
  tfAndRoot(bitvector const& ref, // reference root positions
            bitvector const& snt, // relevant sentences
            bitvector& dest) const
  {
    tsa::ArrayEntry I(lower.back());
    Token const* crpStart = root->corpus->sntStart(0);
    do
      {
        root->readEntry(I.next,I);
        if (!snt.test(I.sid)) continue; // skip, no root there
        // find my endpoint:
        Token const* t = root->corpus->getToken(I)->next(lower.size()-1);
        assert(t >= crpStart);
        size_t p = t-crpStart;
        if (ref.test(p)) // it's a valid root
          dest.set(p);
      } while (I.next != upper.back());
  }

  // @param bv: bitvector with bits set for selected sentences
  // @return: reference to bv
  template<typename Token>
  bitvector&
  TSA_tree_iterator<Token>::
  filterSentences(bitvector& bv) const
  {
    float  aveSntLen    = root->corpus->numTokens()/root->corpus->size();
    size_t ANDcost      = bv.size()/8; // cost of dest&=ref;
    float  aveEntrySize = ((root->endArray-root->startArray)
                           /root->corpus->numTokens());
    if (arrayByteSpanSize()+ANDcost < aveEntrySize*aveSntLen*bv.count())
      {
        bitvector tmp(bv.size());
        markSentences(tmp);
        bv &= tmp;
      }
    else
      {
        for (size_t i = bv.find_first(); i < bv.size(); i = bv.find_next(i))
          if (!match(i)) bv.reset(i);
      }
    return bv;
  }

  /// randomly select up to N occurrences of the sequence
  template<typename Token>
  SPTR<std::vector<typename ttrack::Position> >
  TSA_tree_iterator<Token>::
  randomSample(int level, size_t N) const
  {
    if (level < 0) level += lower.size();
    assert(level >=0);

    SPTR<std::vector<typename ttrack::Position> >
      ret(new std::vector<typename ttrack::Position>(N));

    size_t m=0; // number of samples selected so far
    typename Token::ArrayEntry I(lower.at(level));

    char const* stop = upper.at(level);
    while (m < N && (I.next) < stop)
      {
        root->readEntry(I.next,I);

        // t: expected number of remaining samples
        const double t = (stop - I.pos)/root->aveIndexEntrySize();
        const double r = util::rand_excl(t);
        if (r < N-m)
          {
            ret->at(m).offset = I.offset;
            ret->at(m++).sid  = I.sid;
          }
      }
    ret->resize(m);

    return ret;
  }

} // end of namespace ugdiss
#endif
