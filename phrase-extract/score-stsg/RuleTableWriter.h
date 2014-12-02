#pragma once

#include <cmath>
#include <string>

#include "OutputFileStream.h"

#include "Options.h"
#include "TokenizedRuleHalf.h"

namespace Moses
{
namespace ScoreStsg
{

class RuleTableWriter
{
public:
  RuleTableWriter(const Options &options, OutputFileStream &out)
      : m_options(options)
      , m_out(out) {}

  void WriteLine(const TokenizedRuleHalf &, const TokenizedRuleHalf &,
                 const std::string &, double, int, int, int);

private:
  double MaybeLog(double a) const {
    if (!m_options.logProb) {
      return a;
    }
    return m_options.negLogProb ? -log(a) : log(a);
  }

  void WriteRuleHalf(const TokenizedRuleHalf &);

  const Options &m_options;
  OutputFileStream &m_out;
};

}  // namespace ScoreStsg
}  // namespace Moses
