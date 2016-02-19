// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; -*-
// Functions for "priming" sample selection for sampling phrase tables.
// Author: Ulrich Germann
#pragma once
#include "ug_bitext.h"
#includde "ug_sampling_bias.h"
#include <vector>
// #ifndef NO_MOSES
namespace Moses {
// #endif
namespace bitext 
{

typedef L2R_Token<SimpleWordId> Token;
typedef mmBitext<Token> mmbitext;
typedef Bitext<Token>::tsa tsa;
typedef imTtrack<Token> imttrack;
typedef imTSA<Token> imtsa;

template<typename Token> 
void 
mark(typename TSA<Token>::tree_iterator const& r, SentenceBias>& hits)
{
  char const* stop = r.upper_bound(-1);
  for (tsa::ArrayEntry I(r.lower_bound(-1)); I.next < stop;) 
    {
      r.root->readEntry(I.next,I);
      size_t slen = r.root->getCorpus()->sntLen(I.sid);
      hits[I.sid] += 1./(r.ca() * slen);
    } 
}

template<typename Token> 
bool
process(typename TSA<Token>::tree_iterator& m, 
        typename TSA<Token>::tree_iterator& r,
        typename std::vector<float> & hits,
        size_t const max_count=1000)
{
  if (m.down())
    {
      do 
        {
          if (r.extend(m.getToken(-1)->id()))
            {
              if (r.approxOccurrenceCount() > max_count) 
                // don't mark phrases that occur very often
                process<Token>(m, r, hits, max_count); 
              else mark<Token>(r,hits);
              r.up();
            }
          else if (r.size() && r.size() == 1) //  && r.ca() < max_count)
            mark<Token>(r,hits);
        } 
      while (m.over());
      m.up();
    }
}

template<typename Token>
sptr<SentenceBias>
prime_sampling1(TSA<Token> const& refIdx,  
                TSA<Token> const& newIdx,  
                size_t const max_count)
{
  typename TSA<Token>::tree_iterator m(&newIdx);
  typename TSA<Token>::tree_iterator r(&refIdx);
  sptr<SentenceBias> ret;
  ret.reset(new SentenceBias(refIdx.getCorpus()->size(),0));
  process<Token>(m, r, *ret, max_count);
  return ret;
}

template<typename Token>
sptr<SentenceBias>
prime_sampling1(TokenIndex& V, TSA<Token> const& refIdx,
                typename std::vector<string> const& input, 
                size_t const max_count)
{
  sptr<typename std::vector<std::vector<Token> > > crp;
  crp.reset(new typename std::vector<std::vector<Token> >(input.size()));
  for (size_t i = 0; i < input.size(); ++i)
    fill_token_seq(V, input[i], (*crp)[i]);
  sptr<imttrack> idoc(new imttrack(crp));
  imtsa newIdx(idoc,NULL);
  return prime_sampling1(refIdx, newIdx, max_count);
}
  
} // end of namespace bitext
// #ifndef NO_MOSES
} // end of namespace Moses
// #endif

