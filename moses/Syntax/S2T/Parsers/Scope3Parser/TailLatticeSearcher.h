#pragma once

#include <algorithm>
#include <vector>

#include "moses/Syntax/PHyperedge.h"

#include "TailLattice.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

template<typename Callback>
class TailLatticeSearcher
{
public:
  TailLatticeSearcher(const TailLattice &lattice,
                      const PatternApplicationKey &key,
                      const std::vector<SymbolRange> &ranges)
    : m_lattice(lattice)
    , m_key(key)
    , m_ranges(ranges) {}

  void Search(const std::vector<int> &labels, const TargetPhraseCollection &tpc,
              Callback &callback) {
    m_labels = &labels;
    m_matchCB = &callback;
    m_hyperedge.head = 0;
    m_hyperedge.tail.clear();
    m_hyperedge.translations = &tpc;
    SearchInner(0, 0, 0);
  }

private:
  void SearchInner(int offset, std::size_t i, std::size_t nonTermIndex) {
    assert(m_hyperedge.tail.size() == i);

    const PatternApplicationTrie *patNode = m_key[i];
    const SymbolRange &range = m_ranges[i];

    if (patNode->IsTerminalNode()) {
      const int width = range.minEnd - range.minStart + 1;
      const PVertex *v = m_lattice[offset][0][width][0];
      // FIXME Sort out const-ness
      m_hyperedge.tail.push_back(const_cast<PVertex*>(v));
      if (i == m_key.size()-1) {
        (*m_matchCB)(m_hyperedge);
      } else {
        SearchInner(offset+width, i+1, nonTermIndex);
      }
      m_hyperedge.tail.pop_back();
      return;
    }

    const int absStart = m_ranges[0].minStart + offset;
    const int minWidth = std::max(1, range.minEnd - absStart + 1);
    const int maxWidth = range.maxEnd - absStart + 1;

    const std::vector<std::vector<const PVertex *> > &innerVec =
      m_lattice[offset][nonTermIndex+1];

    std::size_t labelIndex = (*m_labels)[nonTermIndex];

    // Loop over all possible widths for this offset and index.
    for (std::size_t width = minWidth; width <= maxWidth; ++width) {
      const PVertex *v = innerVec[width][labelIndex];
      if (!v) {
        continue;
      }
      // FIXME Sort out const-ness
      m_hyperedge.tail.push_back(const_cast<PVertex*>(v));
      if (i == m_key.size()-1) {
        (*m_matchCB)(m_hyperedge);
      } else {
        SearchInner(offset+width, i+1, nonTermIndex+1);
      }
      m_hyperedge.tail.pop_back();
    }
  }

  const TailLattice &m_lattice;
  const PatternApplicationKey &m_key;
  const std::vector<SymbolRange> &m_ranges;
  const std::vector<int> *m_labels;
  Callback *m_matchCB;
  PHyperedge m_hyperedge;
};

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses
