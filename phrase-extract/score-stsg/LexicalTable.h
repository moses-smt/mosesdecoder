#pragma once

#include <istream>
#include <string>
#include <vector>

#include <boost/unordered_map.hpp>

#include "Vocabulary.h"

namespace MosesTraining
{
namespace Syntax
{
namespace ScoreStsg
{

class LexicalTable
{
public:
  LexicalTable(Vocabulary &, Vocabulary &);

  void Load(std::istream &);

  double PermissiveLookup(Vocabulary::IdType s, Vocabulary::IdType t) {
    OuterMap::const_iterator p = m_table.find(s);
    if (p == m_table.end()) {
      return 1.0;
    }
    const InnerMap &inner = p->second;
    InnerMap::const_iterator q = inner.find(t);
    return q == inner.end() ? 1.0 : q->second;
  }

private:
  typedef boost::unordered_map<Vocabulary::IdType, double> InnerMap;
  typedef boost::unordered_map<Vocabulary::IdType, InnerMap> OuterMap;

  Vocabulary &m_srcVocab;
  Vocabulary &m_tgtVocab;
  OuterMap m_table;
};

}  // namespace ScoreStsg
}  // namespace Syntax
}  // namespace MosesTraining
