#pragma once

#include <memory>
#include <vector>

#include "moses/ChartParser.h"
#include "moses/ChartTranslationOptionList.h"
#include "moses/InputType.h"
#include "moses/NonTerminal.h"
#include "moses/StaticData.h"
#include "moses/Syntax/S2T/Parsers/Parser.h"
#include "moses/Syntax/S2T/PChart.h"

#include "TailLatticeSearcher.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

template<typename Callback>
Scope3Parser<Callback>::Scope3Parser(PChart &chart, const RuleTrie &trie,
                                     std::size_t maxChartSpan)
  : Parser<Callback>(chart)
  , m_ruleTable(trie)
  , m_maxChartSpan(maxChartSpan)
  , m_latticeBuilder(chart)
{
  Init();
}

template<typename Callback>
Scope3Parser<Callback>::~Scope3Parser()
{
  delete m_patRoot;
}

template<typename Callback>
void Scope3Parser<Callback>::
EnumerateHyperedges(const Range &range, Callback &callback)
{
  const std::size_t start = range.GetStartPos();
  const std::size_t end = range.GetEndPos();

  const std::vector<const PatternApplicationTrie *> &patNodes =
    m_patSpans[start][end-start+1];

  for (std::vector<const PatternApplicationTrie *>::const_iterator
       p = patNodes.begin(); p != patNodes.end(); ++p) {
    const PatternApplicationTrie *patNode = *p;

    // Read off the sequence of PAT nodes ending at patNode.
    patNode->ReadOffPatternApplicationKey(m_patKey);

    // Calculate the start and end ranges for each symbol in the PAT key.
    m_symbolRangeCalculator.Calc(m_patKey, start, end, m_symbolRanges);

    // Build a lattice that encodes the set of PHyperedge tails that can be
    // generated from this pattern + span.
    m_latticeBuilder.Build(m_patKey, m_symbolRanges, m_lattice,
                           m_quickCheckTable);

    // Ask the grammar for the mapping from label sequences to target phrase
    // collections for this pattern.
    const RuleTrie::Node::LabelMap &labelMap = patNode->m_node->GetLabelMap();

    // For each label sequence, search the lattice for the set of PHyperedge
    // tails.
    TailLatticeSearcher<Callback> searcher(m_lattice, m_patKey, m_symbolRanges);
    RuleTrie::Node::LabelMap::const_iterator q = labelMap.begin();
    for (; q != labelMap.end(); ++q) {
      const std::vector<int> &labelSeq = q->first;
      TargetPhraseCollection::shared_ptr tpc = q->second;
      // For many label sequences there won't be any corresponding paths through
      // the lattice.  As an optimisation, we use m_quickCheckTable to test
      // for this and we don't begin a search if there are no paths to find.
      bool failCheck = false;
      std::size_t nonTermIndex = 0;
      for (std::size_t i = 0; i < m_patKey.size(); ++i) {
        if (m_patKey[i]->IsTerminalNode()) {
          continue;
        }
        if (!m_quickCheckTable[nonTermIndex][labelSeq[nonTermIndex]]) {
          failCheck = true;
          break;
        }
        ++nonTermIndex;
      }
      if (failCheck) {
        continue;
      }
      searcher.Search(labelSeq, tpc, callback);
    }
  }
}

template<typename Callback>
void Scope3Parser<Callback>::Init()
{
  // Build a map from Words to PVertex sets.
  SentenceMap sentMap;
  FillSentenceMap(sentMap);

  // Build the pattern application trie (PAT) for this input sentence.
  const RuleTrie::Node &root = m_ruleTable.GetRootNode();
  m_patRoot = new PatternApplicationTrie(-1, -1, root, 0, 0);
  m_patRoot->Extend(root, -1, sentMap, false);

  // Generate per-span lists of PAT node pointers.
  InitRuleApplicationVector();
  RecordPatternApplicationSpans(*m_patRoot);
}

/* TODO Rename */
template<typename Callback>
void Scope3Parser<Callback>::InitRuleApplicationVector()
{
  std::size_t length = Base::m_chart.GetWidth();
  m_patSpans.resize(length);
  for (std::size_t start = 0; start < length; ++start) {
    std::size_t maxSpan = length-start;
    m_patSpans[start].resize(maxSpan+1);
  }
}

template<typename Callback>
void Scope3Parser<Callback>::FillSentenceMap(SentenceMap &sentMap)
{
  typedef PChart::Cell Cell;

  const std::size_t width = Base::m_chart.GetWidth();
  for (std::size_t i = 0; i < width; ++i) {
    for (std::size_t j = i; j < width; ++j) {
      const Cell::TMap &map = Base::m_chart.GetCell(i, j).terminalVertices;
      for (Cell::TMap::const_iterator p = map.begin(); p != map.end(); ++p) {
        const Word &terminal = p->first;
        const PVertex &v = p->second;
        sentMap[terminal].push_back(&v);
      }
    }
  }
}

template<typename Callback>
void Scope3Parser<Callback>::RecordPatternApplicationSpans(
  const PatternApplicationTrie &patNode)
{
  if (patNode.m_node->HasRules()) {
    int s1 = -1;
    int s2 = -1;
    int e1 = -1;
    int e2 = -1;
    patNode.DetermineStartRange(Base::m_chart.GetWidth(), s1, s2);
    patNode.DetermineEndRange(Base::m_chart.GetWidth(), e1, e2);

    int minSpan = patNode.Depth();

    // Add a PAT node pointer for each valid span in the range.
    for (int i = s1; i <= s2; ++i) {
      for (int j = std::max(e1, i+minSpan-1); j <= e2; ++j) {
        std::size_t span = j-i+1;
        assert(span >= 1);
        if (span < std::size_t(minSpan)) {
          continue;
        }
        if (m_maxChartSpan && span > m_maxChartSpan) {
          break;
        }
        m_patSpans[i][span].push_back(&patNode);
      }
    }
  }

  for (std::vector<PatternApplicationTrie*>::const_iterator p =
         patNode.m_children.begin(); p != patNode.m_children.end(); ++p) {
    RecordPatternApplicationSpans(**p);
  }
}

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses
