#pragma once

#include <string>
#include <vector>

#include "syntax-common/numbered_set.h"

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"

#include "CfgFilter.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

// Filters a rule table, discarding rules that cannot be applied to a given
// test set.  The rule table must have a CFG source-side and the test sentences
// must be strings.
class StringCfgFilter : public CfgFilter
{
public:
  // Initialize the filter for a given set of test sentences.
  StringCfgFilter(const std::vector<boost::shared_ptr<std::string> > &);

  void Filter(std::istream &in, std::ostream &out);

private:
  // Filtering works by converting the source LHSs of translation rules to
  // patterns containing variable length gaps and then pattern matching
  // against the test set.
  //
  // The algorithm is vaguely similar to Algorithm 1 from Rahman et al. (2006),
  // but with a slightly different definition of a pattern and designed for a
  // text containing sentence boundaries.  Here the text is assumed to be
  // short (a few thousand sentences) and the number of patterns is assumed to
  // be large (tens of millions of rules).
  //
  //   M. Sohel Rahman, Costas S. Iliopoulos, Inbok Lee, Manal Mohamed, and
  //     William F. Smyth
  //   "Finding Patterns with Variable Length Gaps or Don't Cares"
  //   In proceedings of COCOON, 2006

  // Max NGram length.
  static const std::size_t kMaxNGramLength;

  // Maps words from strings to integers.
  typedef NumberedSet<std::string, std::size_t> Vocabulary;

  // A NGram is a sequence of words.
  typedef std::vector<Vocabulary::IdType> NGram;

  // A pattern is an alternating sequence of gaps and NGram subpatterns,
  // starting and ending with a gap.  Every gap has a minimum width, which
  // can be any integer >= 0 (a gap of width 0 is really a non-gap).
  //
  // The source LHSs of translation rules are converted to patterns where each
  // sequence of m consecutive non-terminals is converted to a gap with minimum
  // width m.  For example, if a rule has the source LHS:
  //
  //    [NP] and all the king 's men could n't [VB] [NP] together again
  //
  // and kMaxN is set to 5 then the following pattern is used:
  //
  //    * <and all the king 's> * <men could n't> * <together again> *
  //
  // where the gaps have minimum widths of 1, 0, 2, and 0.
  //
  struct Pattern {
    std::vector<NGram> subpatterns;
    std::vector<int> minGapWidths;
  };

  // A sorted (ascending) sequence of start positions.
  typedef std::vector<int> PositionSeq;

  // A range of start positions.
  typedef std::pair<int, int> Range;

  // A CoordinateTable records the set of sentences in which a single
  // n-gram occurs and for each of those sentences, the start positions
  struct CoordinateTable {
    // Sentences IDs (ascending).  This contains the same values as the key set
    // from intraSentencePositions but sorted into ascending order.
    std::vector<int> sentences;
    // Map from sentence ID to set of intra-sentence start positions.
    boost::unordered_map<int, PositionSeq> intraSentencePositions;
  };

  // NGramCoordinateMap is the main search structure.  It maps a NGram to
  // a CoordinateTable containing the positions that the n-gram occurs at
  // in the test set.
  typedef boost::unordered_map<NGram, CoordinateTable> NGramCoordinateMap;

  // Add all n-grams and coordinates for a single sentence s with index i.
  void AddSentenceNGrams(const std::vector<Vocabulary::IdType> &s,
                         std::size_t i);

  // Calculate the range of possible start positions for subpattern i+1
  // assuming that subpattern i has position x.
  Range CalcNextRange(const Pattern &p, int i, int x, int sentenceLength) const;

  // Generate the pattern corresponding to the given source-side of a rule.
  // This will fail if the rule's source-side contains any terminals that
  // do not occur in the test sentence vocabulary.
  bool GeneratePattern(const std::vector<StringPiece> &, Pattern &) const;

  // Calculate the minimum width of the pattern suffix starting
  // at subpattern i.
  int MinWidth(const Pattern &p, int i) const;

  bool IsNonTerminal(const StringPiece &symbol) const;

  // Try to match the pattern p against any sentence in the test set.
  bool MatchPattern(const Pattern &p) const;

  // Try to match the pattern p against the sentence with the given ID.
  bool MatchPattern(const Pattern &p,
                    std::vector<const CoordinateTable *> &tables,
                    int id) const;

  // The main search structure constructed from the test set sentences.
  NGramCoordinateMap m_ngramCoordinateMap;

  // The lengths of the test sentences.
  std::vector<int> m_sentenceLengths;

  // The maximum length of any test sentence.
  int m_maxSentenceLength;

  // The symbol vocabulary of the test sentences.
  Vocabulary m_testVocab;
};

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining
