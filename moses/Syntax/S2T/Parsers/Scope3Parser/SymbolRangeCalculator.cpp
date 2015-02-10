#include "SymbolRangeCalculator.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

void SymbolRangeCalculator::Calc(const PatternApplicationKey &key,
                                 int spanStart, int spanEnd,
                                 std::vector<SymbolRange> &ranges)
{
  FillInTerminalRanges(key, ranges);
  FillInAuxSymbolInfo(ranges);
  FillInGapRanges(key, spanStart, spanEnd, ranges);
}

// Fill in ranges for terminals and set ranges to -1 for non-terminals.
void SymbolRangeCalculator::FillInTerminalRanges(
  const PatternApplicationKey &key, std::vector<SymbolRange> &ranges)
{
  ranges.resize(key.size());
  for (std::size_t i = 0; i < key.size(); ++i) {
    const PatternApplicationTrie *patNode = key[i];
    if (patNode->IsTerminalNode()) {
      ranges[i].minStart = ranges[i].maxStart = patNode->m_start;
      ranges[i].minEnd = ranges[i].maxEnd = patNode->m_end;
    } else {
      ranges[i].minStart = ranges[i].maxStart = -1;
      ranges[i].minEnd = ranges[i].maxEnd = -1;
    }
  }
}

void SymbolRangeCalculator::FillInAuxSymbolInfo(
  const std::vector<SymbolRange> &ranges)
{
  m_auxSymbolInfo.resize(ranges.size());

  // Forward pass: set distanceToPrevTerminal.
  int distanceToPrevTerminal = -1;
  for (std::size_t i = 0; i < ranges.size(); ++i) {
    const SymbolRange &range = ranges[i];
    AuxSymbolInfo &auxInfo = m_auxSymbolInfo[i];
    if (range.minStart != -1) {
      // Symbol i is a terminal.
      assert(range.maxStart == range.minStart);
      distanceToPrevTerminal = 1;
      // Distances are not used for terminals so set auxInfo value to -1.
      auxInfo.distanceToPrevTerminal = -1;
    } else if (distanceToPrevTerminal == -1) {
      // Symbol i is a non-terminal and there are no preceding terminals.
      auxInfo.distanceToPrevTerminal = -1;
    } else {
      // Symbol i is a non-terminal and there is a preceding terminal.
      auxInfo.distanceToPrevTerminal = distanceToPrevTerminal++;
    }
  }

  // Backward pass: set distanceToNextTerminal
  int distanceToNextTerminal = -1;
  for (std::size_t j = ranges.size(); j > 0; --j) {
    std::size_t i = j-1;
    const SymbolRange &range = ranges[i];
    AuxSymbolInfo &auxInfo = m_auxSymbolInfo[i];
    if (range.minStart != -1) {
      // Symbol i is a terminal.
      assert(range.maxStart == range.minStart);
      distanceToNextTerminal = 1;
      // Distances are not used for terminals so set auxInfo value to -1.
      auxInfo.distanceToNextTerminal = -1;
    } else if (distanceToNextTerminal == -1) {
      // Symbol i is a non-terminal and there are no succeeding terminals.
      auxInfo.distanceToNextTerminal = -1;
    } else {
      // Symbol i is a non-terminal and there is a succeeding terminal.
      auxInfo.distanceToNextTerminal = distanceToNextTerminal++;
    }
  }
}

void SymbolRangeCalculator::FillInGapRanges(const PatternApplicationKey &key,
    int spanStart, int spanEnd,
    std::vector<SymbolRange> &ranges)
{
  for (std::size_t i = 0; i < key.size(); ++i) {
    const PatternApplicationTrie *patNode = key[i];

    if (patNode->IsTerminalNode()) {
      continue;
    }

    SymbolRange &range = ranges[i];
    AuxSymbolInfo &auxInfo = m_auxSymbolInfo[i];

    // Determine minimum start position.
    if (auxInfo.distanceToPrevTerminal == -1) {
      // There are no preceding terminals in pattern.
      range.minStart = spanStart + i;
    } else {
      // There is at least one preceeding terminal in the pattern.
      int j = i - auxInfo.distanceToPrevTerminal;
      assert(ranges[j].minEnd == ranges[j].maxEnd);
      range.minStart = ranges[j].maxEnd + auxInfo.distanceToPrevTerminal;
    }

    // Determine maximum start position.
    if (i == 0) {
      // Gap is leftmost symbol in pattern.
      range.maxStart = spanStart;
    } else if (auxInfo.distanceToPrevTerminal == 1) {
      // Gap follows terminal so start position is fixed.
      range.maxStart = ranges[i-1].maxEnd + 1;
    } else if (auxInfo.distanceToNextTerminal == -1) {
      // There are no succeeding terminals in the pattern.
      int numFollowingGaps = (ranges.size()-1) - i;
      range.maxStart = spanEnd - numFollowingGaps;
    } else {
      // There is at least one succeeding terminal in the pattern.
      int j = i + auxInfo.distanceToNextTerminal;
      range.maxStart = ranges[j].minStart - auxInfo.distanceToNextTerminal;
    }

    // Determine minimum end position.
    if (i+1 == key.size()) {
      // Gap is rightmost symbol in pattern.
      range.minEnd = spanEnd;
    } else if (auxInfo.distanceToNextTerminal == 1) {
      // Gap immediately precedes terminal.
      range.minEnd = ranges[i+1].minStart - 1;
    } else if (auxInfo.distanceToPrevTerminal == -1) {
      // There are no preceding terminals in pattern.
      range.minEnd = spanStart + i;
    } else {
      // There is at least one preceeding terminal in the pattern.
      int j = i - auxInfo.distanceToPrevTerminal;
      assert(ranges[j].minEnd == ranges[j].maxEnd);
      range.minEnd = ranges[j].maxEnd + auxInfo.distanceToPrevTerminal;
    }

    // Determine maximum end position.
    if (i+1 == key.size()) {
      // Gap is rightmost symbol in pattern.
      range.maxEnd = spanEnd;
    } else if (auxInfo.distanceToNextTerminal == -1) {
      // There are no succeeding terminals in the pattern.
      int numFollowingGaps = (ranges.size()-1) - i;
      range.maxEnd = spanEnd - numFollowingGaps;
    } else {
      // There is at least one succeeding terminal in the pattern.
      int j = i + auxInfo.distanceToNextTerminal;
      range.maxEnd = ranges[j].minStart - auxInfo.distanceToNextTerminal;
    }
  }
}

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses
