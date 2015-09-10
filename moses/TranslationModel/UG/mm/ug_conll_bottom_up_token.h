// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// (c) 2007-2012 Ulrich Germann
// Token class for dependency trees, where the linear order
// of tokens is defined as going up a dependency chain
#pragma once

#include "ug_typedefs.h"
namespace sapt
{

  template<typename T>
  class ConllBottomUpToken : public T
  {
  public:
    typedef T Token;
    ConllBottomUpToken() : T() {};
    ConllBottomUpToken(id_type id) : T(id) {};

    ConllBottomUpToken const* next(int length=1) const;

    template<typename TTRACK_TYPE>
    ConllBottomUpToken const* stop(TTRACK_TYPE const& C, id_type sid) const
    {
      return NULL;
    };

    ConllBottomUpToken const*
    stop(ConllBottomUpToken const* seqStart,
         ConllBottomUpToken const* seqEnd) const
    {
      return NULL;
    };

    bool operator<(T const& other)  const { return this->cmp(other)  < 0; }
    bool operator>(T const& other)  const { return this->cmp(other)  > 0; }
    bool operator==(T const& other) const { return this->cmp(other) == 0; }
    bool operator!=(T const& other) const { return this->cmp(other) != 0; }

    bool reachable(T const* o)
    {
      for (T const* x = this; x; x = reinterpret_cast<T const*>(x->up()))
        if (x == o) return true;
      return false;
    }
  };

  template<typename T>
  ConllBottomUpToken<T> const*
  ConllBottomUpToken<T>::
  next(int length) const
  {
    return reinterpret_cast<ConllBottomUpToken<T> const*>(this->up(length));
  }

} // end of namespace ugdiss

