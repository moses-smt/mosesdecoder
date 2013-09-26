// -*- c++ -*-
// (c) 2007-2010 Ulrich Germann
// implementation of stuff related to ArrayEntries
// this file should only be included via ug_tsa_base.h, 
// never by itself
#ifndef __ug_tsa_array_entry_h
#define __ug_tsa_array_entry_h
#include "ug_ttrack_position.h"

namespace ugdiss 
{
  namespace tsa
  {
    class
    ArrayEntry : public ttrack::Position
    {
    public:
      char const* pos;
      char const* next;
      ArrayEntry();

      ArrayEntry(char const* p);
      
      template<typename TSA_TYPE>
      ArrayEntry(TSA_TYPE const* S, char const* p);

    };

    template<typename TSA_TYPE>
    ArrayEntry::
    ArrayEntry(TSA_TYPE const* S, char const* p)
    {
      S->readEntry(p,*this);
    }

    // template<typename TSA_TYPE>
    // class SamplingArrayEntryIterator 
    //   : public tsa::ArrayEntry
    // {
    //   size_t const          N; // (approximate) total number of occurrences
    //   size_t const samplesize; // how many samples to chose from the range
    //   size_t const    sampled; // how many occurrences we've looked at so far
    //   size_t const     chosen; // how many we have chosen
    //   TSA_TYPE const*    root; // the underlying TSA
    //   char        const* stop; // end of the range
    // public:
    //   SamplingArrayEntryIterator(TSA_TYPE::tree_iterator const& m, size_t const s);
    //   bool step(); // returns false when at end of range
    //   bool done(); // 
    // };

    // template<typename TSA_TYPE>
    // SamplingArrayEntryIterator::
    // SamplingArrayEntryIterator(typename TSA_TYPE::tree_iterator const& m, size_t const s)
    //   : ArrayEntry<TSA_TYPE>(m.lower_bound(-1))
    //   , N(m.approxOccurrenceCount())
    //   , samplesize(min(s,N))
    //   , sampled(0)
    //   , chosen(0)
    //   , root(m.root)
    //   , stop(m.upper_bound(-1))
    // { }
    
    // template<typename TSA_TYPE>
    // bool
    // SamplingArrayEntryIterator::
    // step()
    // {
    //   while (chosen < samplesize && next < stop)
    // 	{
    // 	  root->readEntry(next,*this);
    // 	  if (randInt(N - sampled++) < samplesize - chosen)
    // 	    {
    // 	      ++chosen;
    // 	      return true;
    // 	    }
    // 	}
    //   return false;
    // }

  } // end of namespace tsa
} // end of namespace ugdiss
#endif
