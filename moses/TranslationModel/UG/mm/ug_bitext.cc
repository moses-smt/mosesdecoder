//-*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-

#include "ug_bitext.h"
#include <algorithm>
#include <boost/math/distributions/binomial.hpp>

namespace sapt
{

  float
  lbop(size_t const tries, size_t const succ, float const confidence)
  {
    return (confidence == 0
            ? float(succ)/tries
            : (boost::math::binomial_distribution<>::
               find_lower_bound_on_p(tries, succ, confidence)));
  }

  void
  snt_adder<L2R_Token<SimpleWordId> >::
  operator()()
  {
    typedef L2R_Token<SimpleWordId> tkn;
    std::vector<id_type> sids; sids.reserve(snt.size());
    BOOST_FOREACH(std::string const& foo, snt)
      {
        sids.push_back(track ? track->size() : 0);
        std::istringstream buf(foo);
        std::string w;
        std::vector<tkn> s; s.reserve(100);
        while (buf >> w) s.push_back(tkn(V[w]));
        track = append(track,s);
      }
    if (index)
      index.reset(new imTSA<tkn>(*index,track,sids,V.tsize()));
    else
      index.reset(new imTSA<tkn>(track,NULL,NULL));
  }
  
  snt_adder<L2R_Token<SimpleWordId> >::
  snt_adder(std::vector<std::string> const& s, TokenIndex& v,
            SPTR<imTtrack<L2R_Token<SimpleWordId> > >& t,
            SPTR<imTSA<L2R_Token<SimpleWordId> > >& i)
    : snt(s), V(v), track(t), index(i)
  { }
  
  bool
  expand_phrase_pair
  (std::vector<std::vector<ushort> >& a1,
   std::vector<std::vector<ushort> >& a2,
   ushort const s2, // next word on in target side
   ushort const L1, ushort const R1, // limits of previous phrase
   ushort & s1, ushort & e1, ushort& e2) // start/end src; end trg
  {
    if (a2[s2].size() == 0)
      {
        std::cout << __FILE__ << ":" << __LINE__ << std::endl;
        return false;
      }
    bitvector done1(a1.size());
    bitvector done2(a2.size());
    std::vector<std::pair<ushort,ushort> > agenda;
    // x.first:  side (1 or 2)
    // x.second: word position
    agenda.reserve(a1.size() + a2.size());
    agenda.push_back(std::pair<ushort,ushort>(2,s2));
    e2 = s2;
    s1 = e1 = a2[s2].front();
    if (s1 >= L1 && s1 < R1)
      {
        std::cout << __FILE__ << ":" << __LINE__ << std::endl;
        return false;
      }
    agenda.push_back(std::pair<ushort,ushort>(2,s2));
    while (agenda.size())
      {
        ushort side = agenda.back().first;
        ushort p    = agenda.back().second;
        agenda.pop_back();
        if (side == 1)
          {
            done1.set(p);
            BOOST_FOREACH(ushort i, a1[p])
              {
                if (i < s2)
                  {
                    // cout << __FILE__ << ":" << __LINE__ << endl;
                    return false;
                  }
                if (done2[i]) continue;
                for (;e2 <= i;++e2)
                  if (!done2[e2])
                    agenda.push_back(std::pair<ushort,ushort>(2,e2));
              }
          }
        else
          {
            done2.set(p);
            BOOST_FOREACH(ushort i, a2[p])
              {
                if ((e1 < L1 && i >= L1) ||
                    (s1 >= R1 && i < R1) ||
                    (i >= L1 && i < R1))
                  {
                    // cout << __FILE__ << ":" << __LINE__ << " "
                    // << L1 << "-" << R1 << " " << i << " "
                    // << s1 << "-" << e1<< endl;
                    return false;
                  }
          
                if (e1 < i)
                  {
                    for (; e1 <= i; ++e1)
                      if (!done1[e1])
                        agenda.push_back(std::pair<ushort,ushort>(1,e1));
                  }
                else if (s1 > i)
                  {
                    for (; i <= s1; ++i)
                      if (!done1[i])
                        agenda.push_back(std::pair<ushort,ushort>(1,i));
                  }
              }
          }
      }
    ++e1;
    ++e2;
    return true;
  }
  
  void
  print_amatrix(std::vector<std::vector<ushort> > a1, uint32_t len2,
                ushort b1, ushort e1, ushort b2, ushort e2)
  {
    using namespace std;
    std::vector<bitvector> M(a1.size(),bitvector(len2));
    for (ushort j = 0; j < a1.size(); ++j)
      {
        BOOST_FOREACH(ushort k, a1[j])
          M[j].set(k);
      }
    cout << b1 << "-" << e1 << " " << b2 << "-" << e2 << endl;
    cout << "   ";
    for (size_t c = 0; c < len2;++c)
      cout << c%10;
    cout << endl;
    for (size_t r = 0; r < M.size(); ++r)
      {
        cout << setw(3) << r << " ";
        for (size_t c = 0; c < M[r].size(); ++c)
          {
            if ((b1 <= r) && (r < e1) && b2 <= c && c < e2)
              cout << (M[r][c] ? 'x' : '-');
            else cout << (M[r][c] ? 'o' : '.');
          }
        cout << endl;
      }
    cout  << std::string(90,'-') << endl;
  }
  
  void
  write_bitvector(bitvector const& v, std::ostream& out)
  {
    for (size_t i = v.find_first(); i < v.size();)
      {
        out << i;
        if ((i = v.find_next(i)) < v.size()) out << ",";
      }
  }
  
}
