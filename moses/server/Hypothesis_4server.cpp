// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include <boost/foreach.hpp>
namespace Moses {
  void
  Hypothesis::
  OutputLocalWordAlignment(std::vector<xmlrpc_c::value>& dest) const
  {
    using namespace std;
    Range const& src = this->GetCurrSourceWordsRange();
    Range const& trg = this->GetCurrTargetWordsRange();

    WordAlignmentSort waso = m_manager.options()->output.WA_SortOrder;
    vector<pair<size_t,size_t> const* > a
      = this->GetCurrTargetPhrase().GetAlignTerm().GetSortedAlignments(waso);
    typedef pair<size_t,size_t> item;
    BOOST_FOREACH(item const* p, a) {
      map<string, xmlrpc_c::value> M;
      M["source-word"] = xmlrpc_c::value_int(src.GetStartPos() + p->first);
      M["target-word"] = xmlrpc_c::value_int(trg.GetStartPos() + p->second);
      dest.push_back(xmlrpc_c::value_struct(M));
    }
  }

  void
  Hypothesis::
  OutputWordAlignment(std::vector<xmlrpc_c::value>& out) const
  {
    std::vector<Hypothesis const*> tmp;
    for (Hypothesis const* h = this; h; h = h->GetPrevHypo())
      tmp.push_back(h);
    for (size_t i = tmp.size(); i-- > 0;)
      tmp[i]->OutputLocalWordAlignment(out);
  }

}
