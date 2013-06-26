#include "moses/TranslationModel/WordCoocTable.h"
using namespace std;
namespace Moses
{

WordCoocTable::
WordCoocTable()
{
  m_cooc.reserve(1000000);
  m_marg1.reserve(1000000);
  m_marg2.reserve(1000000);
}

WordCoocTable::
WordCoocTable(wordID_t const VocabSize1, wordID_t const VocabSize2)
  : m_cooc(VocabSize1), m_marg1(VocabSize1,0), m_marg2(VocabSize2, 0)
{}

void
WordCoocTable::
Count(size_t const a, size_t const b)
{
  while (a >= m_marg1.size()) {
    m_cooc.push_back(my_map_t());
    m_marg1.push_back(0);
  }
  while (b >= m_marg2.size())
    m_marg2.push_back(0);
  ++m_marg1[a];
  ++m_marg2[b];
  ++m_cooc[a][b];
}

uint32_t
WordCoocTable::
GetJoint(size_t const a, size_t const b) const
{
  if (a >= m_marg1.size() || b >= m_marg2.size()) return 0;
  my_map_t::const_iterator m = m_cooc.at(a).find(b);
  if (m == m_cooc[a].end()) return 0;
  return m->second;
}

uint32_t
WordCoocTable::
GetMarg1(size_t const x) const
{
  return x >= m_marg1.size() ? 0 : m_marg1[x];
}

uint32_t
WordCoocTable::
GetMarg2(size_t const x) const
{
  return x >= m_marg2.size() ? 0 : m_marg2[x];
}

float
WordCoocTable::
pfwd(size_t const a, size_t const b) const
{
  return float(GetJoint(a,b))/GetMarg1(a);
}

float
WordCoocTable::
pbwd(size_t const a, size_t const b) const
{
  // cerr << "at " << __FILE__ << ":" << __LINE__ << endl;
  return float(GetJoint(a,b))/GetMarg2(b);
}
}
