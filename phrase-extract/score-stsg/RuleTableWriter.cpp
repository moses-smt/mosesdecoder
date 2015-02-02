#include "RuleTableWriter.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <vector>

#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"

#include "InputFileStream.h"
#include "LexicalTable.h"
#include "OutputFileStream.h"
#include "Options.h"
#include "RuleGroup.h"

namespace MosesTraining
{
namespace Syntax
{
namespace ScoreStsg
{

void RuleTableWriter::WriteLine(const TokenizedRuleHalf &source,
                                const TokenizedRuleHalf &target,
                                const std::string &bestAlignment,
                                double lexScore, double treeScore, int count,
                                int totalCount, int distinctCount)
{
  if (m_options.inverse) {
    WriteRuleHalf(target);
    m_out << " ||| ";
    WriteRuleHalf(source);
  } else {
    WriteRuleHalf(source);
    m_out << " ||| ";
    WriteRuleHalf(target);
  }

  m_out << " |||" << bestAlignment << "||| ";

  if (!m_options.noLex) {
    m_out << MaybeLog(lexScore);
  }

  if (m_options.treeScore && !m_options.inverse) {
    m_out << " " << MaybeLog(treeScore);
  }

  m_out << " ||| " << totalCount << " " << count;
  if (m_options.kneserNey) {
    m_out << " " << distinctCount;
  }
  m_out << " |||";
  m_out << std::endl;
}

void RuleTableWriter::WriteRuleHalf(const TokenizedRuleHalf &half)
{
  if (half.IsTree()) {
    m_out << half.string;
    return;
  }

  for (std::vector<RuleSymbol>::const_iterator p = half.frontierSymbols.begin();
       p != half.frontierSymbols.end(); ++p) {
    if (p->isNonTerminal) {
      m_out << "[" << p->value << "][" << p->value << "] ";
    } else {
      m_out << p->value << " ";
    }
  }
  m_out << "[X]";
}

}  // namespace ScoreStsg
}  // namespace Syntax
}  // namespace MosesTraining
