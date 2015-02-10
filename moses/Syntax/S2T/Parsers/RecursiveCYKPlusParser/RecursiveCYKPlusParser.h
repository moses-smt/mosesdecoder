#pragma once

#include "moses/Syntax/PHyperedge.h"
#include "moses/Syntax/PVertex.h"
#include "moses/Syntax/S2T/Parsers/Parser.h"
#include "moses/Syntax/S2T/RuleTrieCYKPlus.h"
#include "moses/WordsRange.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

// Parser that implements the recursive variant of CYK+ from this paper:
//
//  Rico Sennrich
//  "A CYK+ Variant for SCFG Decoding Without a Dot Chart"
//  In proceedings of SSST-8 2014
//
template<typename Callback>
class RecursiveCYKPlusParser : public Parser<Callback>
{
public:
  typedef Parser<Callback> Base;
  typedef RuleTrieCYKPlus RuleTrie;

  // TODO Make this configurable?
  static bool RequiresCompressedChart() {
    return true;
  }

  RecursiveCYKPlusParser(PChart &, const RuleTrie &, std::size_t);

  ~RecursiveCYKPlusParser() {}

  void EnumerateHyperedges(const WordsRange &, Callback &);

private:

  void GetTerminalExtension(const RuleTrie::Node &, std::size_t, std::size_t);

  void GetNonTerminalExtensions(const RuleTrie::Node &, std::size_t,
                                std::size_t, std::size_t);

  void AddAndExtend(const RuleTrie::Node &, std::size_t, const PVertex &);

  bool IsNonLexicalUnary(const PHyperedge &) const;

  const RuleTrie &m_ruleTable;
  const std::size_t m_maxChartSpan;
  std::size_t m_maxEnd;
  PHyperedge m_hyperedge;
  Callback *m_callback;
};

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses

// Implementation
#include "RecursiveCYKPlusParser-inl.h"
