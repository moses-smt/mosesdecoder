// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// (c) 2010 Ulrich Germann. All rights reserved.

#ifndef __ug_tsa_bitset_cache_h
#define __ug_tsa_bitset_cache_h
//#include "ug_tsa_base.h"
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/dynamic_bitset.hpp>
#include <stdint.h>
#include <iostream>
// A simple mechanism for caching bit std::vectors representing occurrences of token
// sequences in a corpus. Useful for very frequent items for which the bit
// std::vector is expensive to create on the fly. The variable threshold determines
// when bit std::vectors are cached and when they are created on the fly, using the
// size of the range of entries in the TSA's index in bytes to determine
// whether or not to store the respective bit std::vector in the cache.

namespace sapt
{

  template<typename TSA>
  class
  BitSetCache
  {
  public:
    typedef boost::dynamic_bitset<uint64_t>         BitSet;
    typedef boost::shared_ptr<BitSet>                bsptr;
    typedef std::map<std::pair<char const*,ushort>,bsptr> myMap;
    typedef myMap::iterator                      myMapIter;
  private:
    TSA const*    tsa;
    myMap      cached1,cached2;
    int     threshold;
  public:

    BitSetCache() : tsa(NULL), threshold(0) {};
    BitSetCache(TSA const* t, size_t th=4194304)
    {
      init(t,th);
    };

    void
    init(TSA const* t, size_t th=4194304)
    {
      tsa       = t;
      threshold = th;
    }

    bsptr
    get(typename TSA::Token const* keyStart, size_t keyLen)
    {
      bsptr ret;
      char const* lo = tsa->lower_bound(keyStart,keyLen);
      char const* up = tsa->upper_bound(keyStart,keyLen);
      if (!lo) return ret;
      if (up-lo > threshold)
        {
          std::pair<char const*,ushort> k(lo,keyLen);
          myMapIter m = cached1.find(k);
          if (m != cached1.end())
            ret = m->second;
          else
            {
              ret.reset(new BitSet(tsa->getCorpus()->size()));
              cached1[k] = ret;
            }
        }
      else if (ret == NULL)
        ret.reset(new BitSet(tsa->getCorpus()->size()));
      if (ret->count() == 0)
        tsa->setBits(lo,up,*ret);
      return ret;
    }

    // get bitvector with the path occurrences marked
    bsptr
    get2(typename TSA::Token const* keyStart, size_t keyLen, bool onlyEndpoint=true)
    {
      bsptr ret;
      char const* lo = tsa->lower_bound(keyStart,keyLen);
      char const* up = tsa->upper_bound(keyStart,keyLen);
      if (!lo) return ret;
      if (up-lo > threshold)
        {
          std::pair<char const*,ushort> k(lo,keyLen);
          // cout << "bla " << keyStart->id() << " "
	  // << cached2.size() << " " << up-lo << " " << k.second << std::endl;
          myMapIter m = cached2.find(k);
          if (m != cached2.end())
            ret = m->second;
          else
            {
              ret.reset(new BitSet(tsa->getCorpus()->numTokens()));
              cached2[k] = ret;
            }
        }
      else if (ret == NULL)
        ret.reset(new BitSet(tsa->getCorpus()->numTokens()));
      if (ret->count() == 0)
        {
          if (onlyEndpoint)
            tsa->setTokenBits(lo,up,keyLen,*ret);
          else
            tsa->markOccurrences(lo,up,keyLen,*ret,false);
        }
      return ret;
    }

    void clear()
    {
      cached1.clear();
      cached2.clear();
    }

  };
}
#endif
