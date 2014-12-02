#pragma once

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "ExtractionPhrasePair.h"

#include "LexicalTable.h"
#include "Options.h"
#include "RuleSymbol.h"
#include "TokenizedRuleHalf.h"
#include "Vocabulary.h"

namespace Moses
{

class OutputFileStream;

namespace ScoreStsg
{

class RuleGroup;
class RuleTableWriter;

class ScoreStsg
{
public:
  ScoreStsg();

  const std::string &GetName() const {
    return m_name;
  }

  int Main(int argc, char *argv[]);

private:
  static const int kCountOfCountsMax;

  double ComputeLexProb(const std::vector<RuleSymbol> &,
                        const std::vector<RuleSymbol> &,
                        const MosesTraining::ALIGNMENT &);

  void Error(const std::string &) const;

  void OpenOutputFileOrDie(const std::string &, OutputFileStream &);

  void ParseAlignmentString(const std::string &, int,
                            MosesTraining::ALIGNMENT &);

  void ProcessOptions(int, char *[], Options &) const;

  void ProcessRuleGroup(const RuleGroup &, RuleTableWriter &);

  void ProcessRuleGroupOrDie(const RuleGroup &, RuleTableWriter &,
                             std::size_t, std::size_t);

  void TokenizeRuleHalf(const std::string &, TokenizedRuleHalf &);

  std::string m_name;
  Options m_options;
  Vocabulary m_srcVocab;
  Vocabulary m_tgtVocab;
  LexicalTable m_lexTable;
  std::vector<int> m_countOfCounts;
  int m_totalDistinct;
  TokenizedRuleHalf m_sourceHalf;
  TokenizedRuleHalf m_targetHalf;
  MosesTraining::ALIGNMENT m_tgtToSrc;
};

}  // namespace ScoreStsg
}  // namespace Moses
