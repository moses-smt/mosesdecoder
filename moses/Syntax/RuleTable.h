#pragma once

namespace Moses
{
namespace Syntax
{

class RuleTableFF;

// Base class for any data structure representing a synchronous
// grammar, like a trie (for S2T) or a DFA (for T2S).
class RuleTable
{
public:
  RuleTable(const RuleTableFF *ff) : m_ff(ff) {}

  virtual ~RuleTable() {}

protected:
  const RuleTableFF *m_ff;
};

}  // Syntax
}  // Moses
