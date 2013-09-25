#ifndef __vector_index_sorter_h
#define __vector_index_sorter_h
#include <boost/shared_ptr.hpp>
#include <stdint.h>
#include <vector>
// VectorIndexSorter; (c) 2007-2012 Ulrich Germann

// A VectorIndexSorter is a function object for sorting indices into a vector
// of objects (instead of sorting the vector itself).
//
// typcial use:
// vector<thingy> my_vector;
// VectorIndexSorter<thingy,less<thingy>,int> sorter(my_vector);
// vector<int> order;
// sorter.get_order(order);

namespace Moses
{
  using namespace std;
  template<typename VAL, 
	   typename COMP = greater<VAL>,
	   typename IDX_T=size_t>
  class
  VectorIndexSorter 
    : public binary_function<IDX_T const&, IDX_T const&, bool>
  {
    vector<VAL> const&    m_vecref;
    boost::shared_ptr<COMP> m_comp;
  public:
    
    COMP const& Compare;
    VectorIndexSorter(vector<VAL> const& v, COMP const& comp)
      : m_vecref(v), Compare(comp) {
    }
    
    VectorIndexSorter(vector<VAL> const& v)
      : m_vecref(v), m_comp(new COMP()), Compare(*m_comp) {
    }
    
    bool operator()(IDX_T const & a, IDX_T const & b) const {
      bool fwd = Compare(m_vecref.at(a) ,m_vecref.at(b));
      bool bwd = Compare(m_vecref[b],    m_vecref[a]);
      return (fwd == bwd ? a < b : fwd);
    }
    
    boost::shared_ptr<vector<IDX_T> >
    GetOrder() const;
    
    void
    GetOrder(vector<IDX_T> & order) const;
    
  };
  
  template<typename VAL, typename COMP, typename IDX_T>
  boost::shared_ptr<vector<IDX_T> >
  VectorIndexSorter<VAL,COMP,IDX_T>::
  GetOrder() const
  {
    boost::shared_ptr<vector<IDX_T> > ret(new vector<IDX_T>(m_vecref.size()));
    get_order(*ret);
    return ret;
  }
  
  template<typename VAL, typename COMP, typename IDX_T>
  void
  VectorIndexSorter<VAL,COMP,IDX_T>::
  GetOrder(vector<IDX_T> & order) const
  {
    order.resize(m_vecref.size());
    for (IDX_T i = 0; i < IDX_T(m_vecref.size()); ++i) order[i] = i;
    sort(order.begin(), order.end(), *this);
  }
  
}
#endif
