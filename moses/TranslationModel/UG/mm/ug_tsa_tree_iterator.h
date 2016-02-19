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
    		      TokenIndex const& V,
     		      std::string const& key);

    char const* lower_bound(int p) const;
    char const* upper_bound(int p) const;

    size_t size() const;
    // Token const& wid(int p) const;
    Token const* getToken(int p) const;
    size_t rawCnt(int p=-1) const;
    uint64_t getPid(int p=-1) const; // get phrase id

    virtual bool extend(Token const& id);
    virtual bool extend(id_type id);
    virtual bool down();
    virtual bool over();
    virtual bool up();

    // checks if the sentence [start,stop) contains the given sequence.
    bool match(Token const* start, Token const* stop) const;
    // checks if the sentence /sid/ contains the given sequence.
    bool match(id_type sid) const;

    // only used by bitext-find.cc
    count_type
    markSentences(boost::dynamic_bitset<uint64_t>& bitset) const;

    size_t arrayByteSpanSize(int p = -1) const
    {
      if (lower.size()==0) return 0; // or endArray-startArray???
      if (p < 0) p = lower.size()+p;
      assert(p >=0 && p < int(lower.size()));
      return lower.size() ? upper[p]-lower[p] : 0;
    }

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

} // end of namespace ugdiss
#endif
