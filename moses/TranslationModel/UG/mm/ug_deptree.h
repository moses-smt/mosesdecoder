// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// (c) 2007-2012 Ulrich Germann
// Stuff related to dependency trees

#ifndef __ug_deptree_h
#define __ug_deptree_h

#include <string>
#include <iostream>

#include "tpt_tokenindex.h"
#include "ug_ttrack_base.h"

#include "ug_conll_record.h"
#include "ug_conll_bottom_up_token.h"
#include "ug_typedefs.h"

namespace sapt
{

  // Fills the std::vector v with pointers to the internal root r_x for the
  // stretch [start,x] for all x: start <= x < stop. If the stretch
  // is incoherent, r_x is NULL
  template<typename T>
  void
  fill_L2R_roots(T const* start,T const* stop, std::vector<T const*>& v)
  {
    assert(stop>start);
    v.resize(stop-start);
    v[0] = start;
    bitvector isR(v.size());
    std::vector<T const*> root(v.size());
    isR.set(0);
    root[0] = start+start->parent;
    for (T const* x = start+1; x < stop; ++x)
      {
        size_t p = x-start;
        root[p] = x+x->parent;
        for (size_t i = isR.find_first(); i < isR.size(); i = isR.find_next(i))
          if (root[i]==x)
            isR.reset(i);
        if (root[p] < start || root[p] >= stop)
          isR.set(x-start);
        v[p] = (isR.count()==1) ? start+isR.find_first() : NULL;
      }
  }

  // return the root of the tree if the span [start,stop) constitutes a
  // tree, NULL otherwise
  template<typename T>
  T const*
  findInternalRoot(T const* start, T const* stop)
  {
    int outOfRange=0;
    T const* root = NULL;
    for (T const* t = start; t < stop && outOfRange <= 1; t++)
      {
	T const* n = reinterpret_cast<T const*>(t->up());
	if (!n || n < start || n >=stop)
	  {
	    outOfRange++;
	    root = t;
	  }
      }
    assert(outOfRange);
    return outOfRange == 1 ? root : NULL;
  }

  // return the governor of the tree given by [start,stop) if the span
  // constitutes a tree, NULL otherwise
  template<typename T>
  T const*
  findExternalRoot(T const* start, T const* stop)
  {
    int numRoots=0;
    T const* root = NULL;
    for (T const* t = start; t < stop && numRoots <= 1; t++)
      {
	T const* n = reinterpret_cast<T const*>(t->up());
	if (!n || n < start || n >=stop)
	  {
	    if (root && n != root)
	      numRoots++;
	    else
                {
                  root = n;
                  if (!numRoots) numRoots++;
                }
	  }
      }
    assert(numRoots);
    return numRoots == 1 ? root : NULL;
  }

  template<typename T>
  T const*
  findInternalRoot(std::vector<T> const& v)
  {
    T const* a = as<T>(&(*v.begin()));
    T const* b = as<T>(&(*v.end()));
    return (a==b) ? NULL : findInternalRoot<T>(a,b);
  }

#if 1
  class DTNode
  {
  public:
    Conll_Record const*        rec; // pointer to the record (see below) for this node
    DTNode*           parent; // pointer to my parent
    std::vector<DTNode*> children; // children (in the order they appear in the sentence)
    DTNode(Conll_Record const* p);
  };

  /** A parsed sentence */
  class
  DependencyTree
  {
  public:
    std::vector<DTNode> w;
    DependencyTree(Conll_Record const* first, Conll_Record const* last);
  };
#endif

  class
  Conll_Lemma : public Conll_Record
  {
  public:
    Conll_Lemma();
    Conll_Lemma(id_type _id);
    id_type id() const;
    int cmp(Conll_Record const& other) const;
  };

  class
  Conll_Sform : public Conll_Record
  {
  public:
    Conll_Sform();
    Conll_Sform(id_type _id);
    id_type id() const;
    int cmp(Conll_Record const& other) const;
  };

  class
  Conll_MajPos : public Conll_Record
  {
  public:
    Conll_MajPos();
    Conll_MajPos(id_type _id);
    id_type id() const;
    int cmp(Conll_Record const& other) const;
  };


  class
  Conll_MinPos : public Conll_Record
  {
  public:
    Conll_MinPos();
    Conll_MinPos(id_type _id);
    id_type id() const;
    int cmp(Conll_Record const& other) const;
  };

  class
  Conll_MinPos_Lemma : public Conll_Record
  {
  public:
    Conll_MinPos_Lemma();
    id_type id() const;
    int cmp(Conll_Record const& other) const;
  };

  class
  Conll_AllFields : public Conll_Record
  {
  public:
    Conll_AllFields();
    int cmp(Conll_Record const& other) const;
    bool operator==(Conll_AllFields const& other) const;
  };

  class
  Conll_WildCard : public Conll_Record
  {
  public:
    Conll_WildCard();
    int cmp(Conll_Record const& other) const;
  };

  /** @return true if the linear sequence of /Conll_Record/s is coherent,
   *  i.e., a proper connected tree structure */
  bool
  isCoherent(Conll_Record const* start, Conll_Record const* const stop);


  /** @return the root node of the tree covering the span [start,stop), if the span is coherent;
   *  NULL otherwise */
  template<typename T>
  T const* topNode(T const* start , T const* stop)
  {
    T const* ret = NULL;
    for (T const* x = start; x < stop; ++x)
      {
        T const* n = reinterpret_cast<T const*>(x->up());
        if (!n || n < start || n >= stop)
          {
            if (ret) return NULL;
            else ret = x;
          }
      }
    return ret;
  }

}
#endif
