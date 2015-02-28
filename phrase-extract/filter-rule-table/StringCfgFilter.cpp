#include "StringCfgFilter.h"

#include <algorithm>

#include "util/string_piece_hash.hh"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

const std::size_t StringCfgFilter::kMaxNGramLength = 5;

StringCfgFilter::StringCfgFilter(
  const std::vector<boost::shared_ptr<std::string> > &sentences)
  : m_maxSentenceLength(-1)
{
  // Populate m_ngramCoordinateMap (except for the CoordinateTable's
  // sentence vectors) and record the sentence lengths.
  m_sentenceLengths.reserve(sentences.size());
  const util::AnyCharacter delimiter(" \t");
  std::vector<Vocabulary::IdType> vocabIds;
  for (std::size_t i = 0; i < sentences.size(); ++i) {
    vocabIds.clear();
    for (util::TokenIter<util::AnyCharacter, true> p(*sentences[i], delimiter);
         p; ++p) {
      std::string tmp;
      p->CopyToString(&tmp);
      vocabIds.push_back(m_testVocab.Insert(tmp));
    }
    AddSentenceNGrams(vocabIds, i);
    const int sentenceLength = static_cast<int>(vocabIds.size());
    m_sentenceLengths.push_back(sentenceLength);
    m_maxSentenceLength = std::max(sentenceLength, m_maxSentenceLength);
  }

  // Populate the CoordinateTable's sentence vectors.
  for (NGramCoordinateMap::iterator p = m_ngramCoordinateMap.begin();
       p != m_ngramCoordinateMap.end(); ++p) {
    CoordinateTable &ct = p->second;
    ct.sentences.reserve(ct.intraSentencePositions.size());
    for (boost::unordered_map<int, PositionSeq>::const_iterator
         q = ct.intraSentencePositions.begin();
         q != ct.intraSentencePositions.end(); ++q) {
      ct.sentences.push_back(q->first);
    }
    std::sort(ct.sentences.begin(), ct.sentences.end());
  }
}

void StringCfgFilter::Filter(std::istream &in, std::ostream &out)
{
  const util::MultiCharacter fieldDelimiter("|||");
  const util::AnyCharacter symbolDelimiter(" \t");

  std::string line;
  std::string prevLine;
  StringPiece source;
  std::vector<StringPiece> symbols;
  Pattern pattern;
  bool keep = true;
  int lineNum = 0;

  while (std::getline(in, line)) {
    ++lineNum;

    // Read the source-side of the rule.
    util::TokenIter<util::MultiCharacter> it(line, fieldDelimiter);

    // Check if this rule has the same source-side as the previous rule.  If
    // it does then we already know whether or not to keep the rule.  This
    // optimisation is based on the assumption that the rule table is sorted
    // (which is the case in the standard Moses training pipeline).
    if (*it == source) {
      if (keep) {
        out << line << std::endl;
      }
      continue;
    }

    // The source-side is different from the previous rule's.
    source = *it;

    // Tokenize the source-side.
    symbols.clear();
    for (util::TokenIter<util::AnyCharacter, true> p(source, symbolDelimiter);
         p; ++p) {
      symbols.push_back(*p);
    }

    // Generate a pattern (fails if any source-side terminal is not in the
    // test set vocabulary) and attempt to match it against the test sentences.
    keep = GeneratePattern(symbols, pattern) && MatchPattern(pattern);
    if (keep) {
      out << line << std::endl;
    }

    // Retain line for the next iteration (in order that the source StringPiece
    // remains valid).
    prevLine.swap(line);
  }
}

void StringCfgFilter::AddSentenceNGrams(
  const std::vector<Vocabulary::IdType> &s, std::size_t sentNum)
{
  const std::size_t len = s.size();

  NGram ngram;
  // For each starting position in the sentence:
  for (std::size_t i = 0; i < len; ++i) {
    // For each n-gram length: 1, 2, 3, ... kMaxNGramLength (or less when
    // approaching the end of the sentence):
    for (std::size_t n = 1; n <= std::min(kMaxNGramLength, len-i); ++n) {
      ngram.clear();
      for (std::size_t j = 0; j < n; ++j) {
        ngram.push_back(s[i+j]);
      }
      m_ngramCoordinateMap[ngram].intraSentencePositions[sentNum].push_back(i);
    }
  }
}

bool StringCfgFilter::GeneratePattern(const std::vector<StringPiece> &symbols,
                                      Pattern &pattern) const
{
  pattern.subpatterns.clear();
  pattern.minGapWidths.clear();

  int gapWidth = 0;

  // The first symbol is handled as a special case because there is always a
  // leading gap / non-gap.
  if (IsNonTerminal(symbols[0])) {
    ++gapWidth;
  } else {
    pattern.minGapWidths.push_back(0);
    // Add the symbol to the first n-gram.
    Vocabulary::IdType vocabId =
      m_testVocab.Lookup(symbols[0], StringPieceCompatibleHash(),
                         StringPieceCompatibleEquals());
    if (vocabId == Vocabulary::NullId()) {
      return false;
    }
    pattern.subpatterns.push_back(NGram(1, vocabId));
  }

  // Process the remaining symbols (except the last which is the RHS).
  for (std::size_t i = 1; i < symbols.size()-1; ++i) {
    // Is current symbol a non-terminal?
    if (IsNonTerminal(symbols[i])) {
      ++gapWidth;
      continue;
    }
    // Does the current terminal follow a non-terminal?
    if (gapWidth > 0) {
      pattern.minGapWidths.push_back(gapWidth);
      gapWidth = 0;
      pattern.subpatterns.resize(pattern.subpatterns.size()+1);
      // Is the current n-gram full?
    } else if (pattern.subpatterns.back().size() == kMaxNGramLength) {
      pattern.minGapWidths.push_back(0);
      pattern.subpatterns.resize(pattern.subpatterns.size()+1);
    }
    // Add the symbol to the current n-gram.
    Vocabulary::IdType vocabId =
      m_testVocab.Lookup(symbols[i], StringPieceCompatibleHash(),
                         StringPieceCompatibleEquals());
    if (vocabId == Vocabulary::NullId()) {
      return false;
    }
    pattern.subpatterns.back().push_back(vocabId);
  }

  // Add the final gap width value (0 if the last symbol was a terminal).
  pattern.minGapWidths.push_back(gapWidth);
  return true;
}

bool StringCfgFilter::IsNonTerminal(const StringPiece &symbol) const
{
  return symbol.size() >= 3 && symbol[0] == '[' &&
         symbol[symbol.size()-1] == ']';
}

bool StringCfgFilter::MatchPattern(const Pattern &pattern) const
{
  // Step 0: If the pattern is just a single gap (i.e. the original rule
  //         was fully non-lexical) then the pattern matches unless the
  //         minimum gap width is wider than any sentence.
  if (pattern.subpatterns.empty()) {
    assert(pattern.minGapWidths.size() == 1);
    return pattern.minGapWidths[0] <= m_maxSentenceLength;
  }

  // Step 1: Look up all of the subpatterns in m_ngramCoordinateMap and record
  //         pointers to their CoordinateTables.
  std::vector<const CoordinateTable *> tables;
  for (std::vector<NGram>::const_iterator p = pattern.subpatterns.begin();
       p != pattern.subpatterns.end(); ++p) {
    NGramCoordinateMap::const_iterator q = m_ngramCoordinateMap.find(*p);
    // If a subpattern doesn't appear in m_ngramCoordinateMap then the match
    // has already failed.
    if (q == m_ngramCoordinateMap.end()) {
      return false;
    }
    tables.push_back(&(q->second));
  }

  // Step 2: Intersect the CoordinateTables' sentence sets to find the set of
  //         test set sentences in which all subpatterns occur.
  std::vector<int> intersection = tables[0]->sentences;
  std::vector<int> tmp(intersection.size());
  for (std::size_t i = 1; i < tables.size(); ++i) {
    std::vector<int>::iterator p = std::set_intersection(
                                     intersection.begin(), intersection.end(), tables[i]->sentences.begin(),
                                     tables[i]->sentences.end(), tmp.begin());
    tmp.resize(p-tmp.begin());
    if (tmp.empty()) {
      return false;
    }
    intersection.swap(tmp);
  }

  // Step 3: For each sentence in the intersection, try to find a consistent
  //         sequence of intra-sentence positions (one for each subpattern).
  //         'Consistent' here means that the subpatterns occur in the right
  //         order and are separated by at least the minimum widths required
  //         by the pattern's gaps).
  for (std::vector<int>::const_iterator p = intersection.begin();
       p != intersection.end(); ++p) {
    if (MatchPattern(pattern, tables, *p)) {
      return true;
    }
  }
  return false;
}

bool StringCfgFilter::MatchPattern(
  const Pattern &pattern,
  std::vector<const CoordinateTable *> &tables,
  int sentenceId) const
{
  const int sentenceLength = m_sentenceLengths[sentenceId];

  // In the for loop below, we need to know the set of start position ranges
  // where subpattern i is allowed to occur (rangeSet) and we are generating
  // the ranges for subpattern i+1 (nextRangeSet).
  // TODO Merge ranges if subpattern i follows a non-zero gap.
  std::vector<Range> rangeSet;
  std::vector<Range> nextRangeSet;

  // Calculate the range for the first subpattern.
  int minStart = pattern.minGapWidths[0];
  int maxStart = sentenceLength - MinWidth(pattern, 0);
  rangeSet.push_back(Range(minStart, maxStart));

  // Attempt to match subpatterns.
  for (int i = 0; i < pattern.subpatterns.size(); ++i) {
    // Look-up the intra-sentence position sequence.
    boost::unordered_map<int, PositionSeq>::const_iterator r =
      tables[i]->intraSentencePositions.find(sentenceId);
    assert(r != tables[i]->intraSentencePositions.end());
    const PositionSeq &col = r->second;
    for (PositionSeq::const_iterator p = col.begin(); p != col.end(); ++p) {
      bool inRange = false;
      for (std::vector<Range>::const_iterator q = rangeSet.begin();
           q != rangeSet.end(); ++q) {
        // TODO Use the fact that the ranges are ordered to break early.
        if (*p >= q->first && *p <= q->second) {
          inRange = true;
          break;
        }
      }
      if (!inRange) {
        continue;
      }
      // If this is the last subpattern then we're done.
      if (i+1 == pattern.subpatterns.size()) {
        return true;
      }
      nextRangeSet.push_back(CalcNextRange(pattern, i, *p, sentenceLength));
    }
    if (nextRangeSet.empty()) {
      return false;
    }
    rangeSet.swap(nextRangeSet);
    nextRangeSet.clear();
  }
  return true;
}

StringCfgFilter::Range StringCfgFilter::CalcNextRange(
  const Pattern &pattern, int i, int x, int sentenceLength) const
{
  assert(i+1 < pattern.subpatterns.size());
  Range range;
  if (pattern.minGapWidths[i+1] == 0) {
    // The next subpattern follows this one without a gap.
    range.first = range.second = x + pattern.subpatterns[i].size();
  } else {
    range.first = x + pattern.subpatterns[i].size() + pattern.minGapWidths[i+1];
    // TODO MinWidth should only be computed once per subpattern.
    range.second = sentenceLength - MinWidth(pattern, i+1);
  }
  return range;
}

int StringCfgFilter::MinWidth(const Pattern &pattern, int i) const
{
  int minWidth = 0;
  for (; i < pattern.subpatterns.size(); ++i) {
    minWidth += pattern.subpatterns[i].size();
    minWidth += pattern.minGapWidths[i+1];
  }
  return minWidth;
}

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining
