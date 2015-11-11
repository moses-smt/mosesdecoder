// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include "ug_im_bitext.h"

namespace sapt
{
  using namespace tpt;
  template<>
  SPTR<imBitext<L2R_Token<SimpleWordId> > >
  imBitext<L2R_Token<SimpleWordId> >::
  add(std::vector<std::string> const& s1,
      std::vector<std::string> const& s2,
      std::vector<std::string> const& aln) const
  {
    typedef L2R_Token<SimpleWordId> TKN;
    assert(s1.size() == s2.size() && s1.size() == aln.size());

#ifndef NDEBUG
    size_t first_new_snt = this->T1 ? this->T1->size() : 0;
#endif

    SPTR<imBitext<TKN> > ret;
    {
      boost::unique_lock<boost::shared_mutex> guard(m_lock);
      ret.reset(new imBitext<TKN>(*this));
    }

    // we add the sentences in separate threads (so it's faster)
    boost::thread thread1(snt_adder<TKN>(s1,*ret->V1,ret->myT1,ret->myI1));
    // thread1.join(); // for debugging
    boost::thread thread2(snt_adder<TKN>(s2,*ret->V2,ret->myT2,ret->myI2));
    BOOST_FOREACH(std::string const& a, aln)
      {
        std::istringstream ibuf(a);
        std::ostringstream obuf;
        uint32_t row,col; char c;
        while (ibuf >> row >> c >> col)
          {
            UTIL_THROW_IF2(c != '-', "[" << HERE << "] "
                           << "Error in alignment information:\n" << a);
            binwrite(obuf,row);
            binwrite(obuf,col);
          }
        // important: DO NOT replace the two lines below this comment by
        // char const* x = obuf.str().c_str(), as the memory x is pointing
        // to is freed immediately upon deconstruction of the string object.
        std::string foo = obuf.str();
        char const* x = foo.c_str();
        std::vector<char> v(x,x+foo.size());
        ret->myTx = append(ret->myTx, v);
      }
    
    thread1.join();
    thread2.join();
    
    ret->Tx = ret->myTx;
    ret->T1 = ret->myT1;
    ret->T2 = ret->myT2;
    ret->I1 = ret->myI1;
    ret->I2 = ret->myI2;
    
#ifndef NDEBUG
    // sanity check
    for (size_t i = first_new_snt; i < ret->T1->size(); ++i)
      {
        size_t slen1  = ret->T1->sntLen(i);
        size_t slen2  = ret->T2->sntLen(i);
        char const* p = ret->Tx->sntStart(i);
        char const* q = ret->Tx->sntEnd(i);
        size_t k;
        while (p < q)
          {
            p = binread(p,k);
            assert(p);
            assert(p < q);
            assert(k < slen1);
            p = binread(p,k);
            assert(p);
            assert(k < slen2);
          }
      }
#endif
    return ret;
  }
  
}

