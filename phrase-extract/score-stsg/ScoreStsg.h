#pragma once

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "ExtractionPhrasePair.h"
#include "OutputFileStream.h"

#include "syntax-common/tool.h"

#include "LexicalTable.h"
#include "Options.h"
#include "RuleSymbol.h"
#include "TokenizedRuleHalf.h"
#include "Vocabulary.h"

namespace MosesTraining
{
namespace Syntax
{
namespace ScoreStsg
{

class RuleGroup;
class RuleTableWriter;

class ScoreStsg : public Tool
{
public:
  ScoreStsg();

  virtual int Main(int argc, char *argv[]);

private:
  static const int kCountOfCountsMax;

  double ComputeLexProb(const std::vector<RuleSymbol> &,
                        const std::vector<RuleSymbol> &,
                        const ALIGNMENT &);

  void ParseAlignmentString(const std::string &, int,
                            ALIGNMENT &);

  void ProcessOptions(int, char *[], Options &) const;

  void ProcessRuleGroup(const RuleGroup &, RuleTableWriter &);

  void ProcessRuleGroupOrDie(const RuleGroup &, RuleTableWriter &,
                             std::size_t, std::size_t);

  void TokenizeRuleHalf(const std::string &, TokenizedRuleHalf &);

  Options m_options;
  Vocabulary m_srcVocab;
  Vocabulary m_tgtVocab;
  LexicalTable m_lexTable;
  std::vector<int> m_countOfCounts;
  int m_totalDistinct;
  TokenizedRuleHalf m_sourceHalf;
  TokenizedRuleHalf m_targetHalf;
  ALIGNMENT m_tgtToSrc;
};

}  // namespace ScoreStsg
}  // namespace Syntax
}  // namespace MosesTraining
