#pragma once

#include "moses/Syntax/S2T/PChart.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

template<typename Callback>
RecursiveCYKPlusParser<Callback>::RecursiveCYKPlusParser(
  PChart &chart,
  const RuleTrie &trie,
  std::size_t maxChartSpan)
  : Parser<Callback>(chart)
  , m_ruleTable(trie)
  , m_maxChartSpan(maxChartSpan)
  , m_callback(NULL)
{
  m_hyperedge.head = 0;
}

template<typename Callback>
void RecursiveCYKPlusParser<Callback>::EnumerateHyperedges(
  const WordsRange &range,
  Callback &callback)
{
  const std::size_t start = range.GetStartPos();
  const std::size_t end = range.GetEndPos();
  m_callback = &callback;
  const RuleTrie::Node &rootNode = m_ruleTable.GetRootNode();
  m_maxEnd = std::min(Base::m_chart.GetWidth()-1, start+m_maxChartSpan-1);
  m_hyperedge.tail.clear();

  // Find all hyperedges where the first incoming vertex is a terminal covering
  // [start,end].
  GetTerminalExtension(rootNode, start, end);

  // Find all hyperedges where the first incoming vertex is a non-terminal
  // covering [start,end-1].
  if (end > start) {
    GetNonTerminalExtensions(rootNode, start, end-1, end-1);
  }
}

// Search for all extensions of a partial rule (pointed at by node) that begin
// with a non-terminal over a span between [start,minEnd] and [start,maxEnd].
template<typename Callback>
void RecursiveCYKPlusParser<Callback>::GetNonTerminalExtensions(
  const RuleTrie::Node &node,
  std::size_t start,
  std::size_t minEnd,
  std::size_t maxEnd)
{
  // Non-terminal labels in node's outgoing edge set.
  const RuleTrie::Node::SymbolMap &nonTermMap = node.GetNonTerminalMap();

  // Compressed matrix from PChart.
  const PChart::CompressedMatrix &matrix =
    Base::m_chart.GetCompressedMatrix(start);

  // Loop over possible expansions of the rule.
  RuleTrie::Node::SymbolMap::const_iterator p;
  RuleTrie::Node::SymbolMap::const_iterator p_end = nonTermMap.end();
  for (p = nonTermMap.begin(); p != p_end; ++p) {
    const Word &nonTerm = p->first;
    const std::vector<PChart::CompressedItem> &items =
      matrix[nonTerm[0]->GetId()];
    for (std::vector<PChart::CompressedItem>::const_iterator q = items.begin();
         q != items.end(); ++q) {
      if (q->end >= minEnd && q->end <= maxEnd) {
        const RuleTrie::Node &child = p->second;
        AddAndExtend(child, q->end, *(q->vertex));
      }
    }
  }
}

// Search for all extensions of a partial rule (pointed at by node) that begin
// with a terminal over span [start,end].
template<typename Callback>
void RecursiveCYKPlusParser<Callback>::GetTerminalExtension(
  const RuleTrie::Node &node,
  std::size_t start,
  std::size_t end)
{

  const PChart::Cell::TMap &vertexMap =
    Base::m_chart.GetCell(start, end).terminalVertices;
  if (vertexMap.empty()) {
    return;
  }

  const RuleTrie::Node::SymbolMap &terminals = node.GetTerminalMap();

  for (PChart::Cell::TMap::const_iterator p = vertexMap.begin();
       p != vertexMap.end(); ++p) {
    const Word &terminal = p->first;
    const PVertex &vertex = p->second;

    // if node has small number of terminal edges, test word equality for each.
    if (terminals.size() < 5) {
      for (RuleTrie::Node::SymbolMap::const_iterator iter = terminals.begin();
           iter != terminals.end(); ++iter) {
        const Word &word = iter->first;
        if (word == terminal) {
          const RuleTrie::Node *child = & iter->second;
          AddAndExtend(*child, end, vertex);
          break;
        }
      }
    } else { // else, do hash lookup
      const RuleTrie::Node *child = node.GetChild(terminal);
      if (child != NULL) {
        AddAndExtend(*child, end, vertex);
      }
    }
  }
}

// If a (partial) rule matches, pass it to the callback (if non-unary and
// non-empty), and try to find expansions that have this partial rule as prefix.
template<typename Callback>
void RecursiveCYKPlusParser<Callback>::AddAndExtend(
  const RuleTrie::Node &node,
  std::size_t end,
  const PVertex &vertex)
{
  // FIXME Sort out const-ness.
  m_hyperedge.tail.push_back(const_cast<PVertex *>(&vertex));

  // Add target phrase collection (except if rule is empty or unary).
  const TargetPhraseCollection &tpc = node.GetTargetPhraseCollection();
  if (!tpc.IsEmpty() && !IsNonLexicalUnary(m_hyperedge)) {
    m_hyperedge.label.translations = &tpc;
    (*m_callback)(m_hyperedge, end);
  }

  // Get all further extensions of rule (until reaching end of sentence or
  // max-chart-span).
  if (end < m_maxEnd) {
    if (!node.GetTerminalMap().empty()) {
      for (std::size_t newEndPos = end+1; newEndPos <= m_maxEnd; newEndPos++) {
        GetTerminalExtension(node, end+1, newEndPos);
      }
    }
    if (!node.GetNonTerminalMap().empty()) {
      GetNonTerminalExtensions(node, end+1, end+1, m_maxEnd);
    }
  }

  m_hyperedge.tail.pop_back();
}

template<typename Callback>
bool RecursiveCYKPlusParser<Callback>::IsNonLexicalUnary(
  const PHyperedge &hyperedge) const
{
  return hyperedge.tail.size() == 1 &&
         hyperedge.tail[0]->symbol.IsNonTerminal();
}

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses
