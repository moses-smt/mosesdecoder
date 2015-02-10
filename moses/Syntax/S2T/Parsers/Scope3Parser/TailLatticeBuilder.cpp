#include "TailLatticeBuilder.h"

#include "moses/Syntax/S2T/RuleTrieScope3.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

void TailLatticeBuilder::Build(
  const std::vector<const PatternApplicationTrie *> &key,
  const std::vector<SymbolRange> &ranges,
  TailLattice &lattice,
  std::vector<std::vector<bool> > &checkTable)
{
  assert(key.size() == ranges.size());
  assert(key.size() > 0);

  ExtendAndClear(key, ranges, lattice, checkTable);

  const int spanStart = ranges.front().minStart;

  const RuleTrieScope3::Node *utrieNode = key.back()->m_node;

  const RuleTrieScope3::Node::LabelTable &labelTable =
    utrieNode->GetLabelTable();

  std::size_t nonTermIndex = 0;

  for (std::size_t i = 0; i < ranges.size(); ++i) {
    const SymbolRange &range = ranges[i];
    const PatternApplicationTrie &patNode = *(key[i]);
    if (patNode.IsTerminalNode()) {
      std::size_t offset = range.minStart - spanStart;
      std::size_t width = range.minEnd - range.minStart + 1;
      assert(lattice[offset][0][width].empty());
      lattice[offset][0][width].push_back(patNode.m_pvertex);
      continue;
    }
    const std::vector<Word> &labelVec = labelTable[nonTermIndex];
    assert(checkTable[nonTermIndex].size() == labelVec.size());
    for (int s = range.minStart; s <= range.maxStart; ++s) {
      for (int e = std::max(s, range.minEnd); e <= range.maxEnd; ++e) {
        assert(e-s >= 0);
        std::size_t offset = s - spanStart;
        std::size_t width = e - s + 1;
        assert(lattice[offset][nonTermIndex+1][width].empty());
        std::vector<bool>::iterator q = checkTable[nonTermIndex].begin();
        for (std::vector<Word>::const_iterator p = labelVec.begin();
             p != labelVec.end(); ++p, ++q) {
          const Word &label = *p;
          const PVertex *v =
            m_chart.GetCell(s, e).nonTerminalVertices.Find(label);
          lattice[offset][nonTermIndex+1][width].push_back(v);
          *q = (*q || static_cast<bool>(v));
        }
      }
    }
    ++nonTermIndex;
  }
}

// Extend the lattice if necessary and clear the innermost vectors.
void TailLatticeBuilder::ExtendAndClear(
  const std::vector<const PatternApplicationTrie *> &key,
  const std::vector<SymbolRange> &ranges,
  TailLattice &lattice,
  std::vector<std::vector<bool> > &checkTable)
{
  const int spanStart = ranges.front().minStart;
  const int spanEnd = ranges.back().maxEnd;

  const std::size_t span = spanEnd - spanStart + 1;

  // Extend the outermost vector.
  if (lattice.size() < span) {
    lattice.resize(span);
  }

  const RuleTrieScope3::Node *utrieNode = key.back()->m_node;
  const RuleTrieScope3::Node::LabelTable &labelTable =
    utrieNode->GetLabelTable();

  std::size_t nonTermIndex = 0;

  for (std::size_t i = 0; i < ranges.size(); ++i) {
    const SymbolRange &range = ranges[i];
    const PatternApplicationTrie &patNode = *(key[i]);
    if (patNode.IsTerminalNode()) {
      std::size_t offset = range.minStart - spanStart;
      std::size_t width = range.minEnd - range.minStart + 1;
      if (lattice[offset].size() < 1) {
        lattice[offset].resize(1);
      }
      if (lattice[offset][0].size() < width+1) {
        lattice[offset][0].resize(width+1);
      }
      lattice[offset][0][width].clear();
      continue;
    }
    const std::vector<Word> &labelVec = labelTable[nonTermIndex];
    for (int s = range.minStart; s <= range.maxStart; ++s) {
      for (int e = std::max(s, range.minEnd); e <= range.maxEnd; ++e) {
        assert(e-s >= 0);
        std::size_t offset = s - spanStart;
        std::size_t width = e - s + 1;
        if (lattice[offset].size() < nonTermIndex+2) {
          lattice[offset].resize(nonTermIndex+2);
        }
        if (lattice[offset][nonTermIndex+1].size() < width+1) {
          lattice[offset][nonTermIndex+1].resize(width+1);
        }
        lattice[offset][nonTermIndex+1][width].clear();
        lattice[offset][nonTermIndex+1][width].reserve(labelVec.size());
      }
    }
    if (checkTable.size() < nonTermIndex+1) {
      checkTable.resize(nonTermIndex+1);
    }
    // Unlike the lattice itself, the check table must contain initial
    // values prior to the main build procedure (and the values must be false).
    checkTable[nonTermIndex].assign(labelVec.size(), false);
    ++nonTermIndex;
  }
}

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses
