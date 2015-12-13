#ifndef moses_LVoc_h
#define moses_LVoc_h

#include<map>
#include<vector>
#include<iostream>
#include<fstream>
#include <sstream>

typedef unsigned LabelId;
extern const LabelId InvalidLabelId;
extern const LabelId Epsilon;

typedef std::vector<LabelId> IPhrase;

/** class used in phrase-based binary phrase-table.
 *  @todo vocab?
 *  A = type of things to numberize, ie, std::string
 *  B = map type to use, might consider using hash_map for better performance
 */
template<typename A,typename B=std::map<A,LabelId> >
class LVoc
{
  typedef A Key;
  typedef B M;
  typedef std::vector<Key> V;
  M m;
  V data;
public:
  LVoc() {}

  bool isKnown(const Key& k) const {
    return m.find(k)!=m.end();
  }
  LabelId index(const Key& k) const {
    typename M::const_iterator i=m.find(k);
    return i!=m.end()? i->second : InvalidLabelId;
  }
  LabelId add(const Key& k) {
    std::pair<typename M::iterator,bool> p
    =m.insert(std::make_pair(k,data.size()));
    if(p.second) data.push_back(k);
    assert(static_cast<size_t>(p.first->second)<data.size());
    return p.first->second;
  }
  Key const& symbol(LabelId i) const {
    assert(static_cast<size_t>(i)<data.size());
    return data[i];
  }

  typedef typename V::const_iterator const_iterator;
  const_iterator begin() const {
    return data.begin();
  }
  const_iterator end() const {
    return data.end();
  }

  void Write(const std::string& fname) const {
    std::ofstream out(fname.c_str());
    Write(out);
  }
  void Write(std::ostream& out) const {
    for(int i=data.size()-1; i>=0; --i)
      out<<i<<' '<<data[i]<<'\n';
  }
  void Read(const std::string& fname) {
    std::ifstream in(fname.c_str());
    Read(in);
  }
  void Read(std::istream& in) {
    Key k;
    size_t i;
    std::string line;
    while(getline(in,line)) {
      std::istringstream is(line);
      if(is>>i>>k) {
        if(i>=data.size()) data.resize(i+1);
        data[i]=k;
        m[k]=i;
      }
    }
  }
};

#endif
