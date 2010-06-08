// $Id:  $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "Ngram.h"

using namespace Josiah;
using namespace std;

  
  
Josiah::NgramCollector::NgramCollector(size_t order) : m_order(order) {
    FactorCollection &factorCollection = FactorCollection::Instance();
    m_start = factorCollection.AddFactor(Output, 0, BOS_);
    m_end = factorCollection.AddFactor(Output, 1, BOS_);
}
  
string Josiah::NgramCollector::ToString(const vector<const Factor*>& ws) const {
  ostringstream os;
  for (vector<const Factor*>::const_iterator i = ws.begin(); i != ws.end(); ++i)
    os << (*i)->GetString() << " ";
  return os.str();
}

void Josiah::NgramCollector::collect(Sample& sample) {
  const Hypothesis* h = sample.GetSampleHypothesis();
  vector<const Factor*> trans;
  h->GetTranslation(&trans, 0);
  for (int ngramstart = -(m_order-1); ngramstart < (int)trans.size(); ++ngramstart) {
    vector<const Factor*> ngram(m_order);
    for (int i = ngramstart; i < ngramstart+(int)m_order; ++i) {
      if (i < 0) {
        ngram[i-ngramstart] = m_start;
      } else if (i >= (int)trans.size()) {
        ngram[i-ngramstart] = m_end;
      } else {
        ngram[i-ngramstart] = trans[i];
      }
    }
    m_counts.insert(ngram);
    m_ngrams.insert(ngram);
  }
}
  
void Josiah::NgramCollector::dump( std::ostream & out ) const {
  //maps n-1-gram prefixes to suffix-count map
  map<vector<const Factor*>, multimap<size_t, const Factor*, greater<size_t> > >sortedCounts;
  for (set<vector<const Factor*> >::const_iterator i = m_ngrams.begin(); i != m_ngrams.end(); ++i) {
    vector<const Factor*> prefix(m_order-1);
    copy(i->begin(),i->end()-1,prefix.begin());
    const Factor* suffix = i->at(i->size()-1);
    cerr << "ngram: " << ToString(*i) << " prefix: " << ToString(prefix) << " suffix: " << suffix->GetString() << endl;
    sortedCounts[prefix].insert(pair<size_t,const Factor* >(m_counts.count(*i),suffix));
    
  }
  
  for (map<vector<const Factor*>, multimap<size_t, const Factor*, greater<size_t> > >::const_iterator i = sortedCounts.begin(); 
        i != sortedCounts.end(); ++i) {
          for (multimap<size_t, const Factor*, greater<size_t> >::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
            out << j->first << " ";
            out << ToString(i->first);
            out << j->second->GetString() << endl;
          }
  }
}
  



