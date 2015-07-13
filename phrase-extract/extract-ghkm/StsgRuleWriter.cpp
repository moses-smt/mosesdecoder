#include "StsgRuleWriter.h"

#include <cassert>
#include <cmath>
#include <ostream>
#include <map>
#include <sstream>
#include <vector>

#include "Alignment.h"
#include "Options.h"
#include "StsgRule.h"

namespace MosesTraining
{
namespace Syntax
{
namespace GHKM
{

void StsgRuleWriter::Write(const StsgRule &rule)
{
  std::ostringstream sourceSS;
  std::ostringstream targetSS;

  // Write the source side of the rule to sourceSS.
  const std::vector<Symbol> &sourceSide = rule.GetSourceSide();
  for (std::size_t i = 0; i < sourceSide.size(); ++i) {
    const Symbol &symbol = sourceSide[i];
    if (i > 0) {
      sourceSS << " ";
    }
    if (symbol.GetType() == NonTerminal) {
      sourceSS << "[X]";
    } else {
      sourceSS << symbol.GetValue();
    }
  }

  // Write the target side of the rule to targetSS.
  rule.GetTargetSide().PrintTree(targetSS);

  // Write the rule to the forward and inverse extract files.
  if (m_options.t2s) {
    // If model is tree-to-string then flip the source and target.
    m_fwd << targetSS.str() << " ||| " << sourceSS.str() << " |||";
    m_inv << sourceSS.str() << " ||| " << targetSS.str() << " |||";
  } else {
    m_fwd << sourceSS.str() << " ||| " << targetSS.str() << " |||";
    m_inv << targetSS.str() << " ||| " << sourceSS.str() << " |||";
  }

  // Write the non-terminal alignments.
  const std::vector<int> &nonTermAlignment = rule.GetNonTermAlignment();
  for (int srcIndex = 0; srcIndex <  nonTermAlignment.size(); ++srcIndex) {
    int tgtIndex = nonTermAlignment[srcIndex];
    if (m_options.t2s) {
      // If model is tree-to-string then flip the source and target.
      m_fwd << " " << tgtIndex << "-" << srcIndex;
      m_inv << " " << srcIndex << "-" << tgtIndex;
    } else {
      m_fwd << " " << srcIndex << "-" << tgtIndex;
      m_inv << " " << tgtIndex << "-" << srcIndex;
    }
  }
  m_fwd << " |||";
  m_inv << " |||";

  // Write the symbol alignments.
  const Alignment &alignment = rule.GetAlignment();
  for (Alignment::const_iterator p = alignment.begin();
       p != alignment.end(); ++p) {
    if (m_options.t2s) {
      // If model is tree-to-string then flip the source and target.
      m_fwd << " " << p->second << "-" << p->first;
      m_inv << " " << p->first << "-" << p->second;
    } else {
      m_fwd << " " << p->first << "-" << p->second;
      m_inv << " " << p->second << "-" << p->first;
    }
  }

  // Write a count of 1.
  m_fwd << " ||| 1";
  m_inv << " ||| 1";

  // Write the PCFG score (if requested).
  if (m_options.pcfg) {
    m_fwd << " ||| " << std::exp(rule.GetTargetSide().GetPcfgScore());
  }

  m_fwd << std::endl;
  m_inv << std::endl;
}

}  // namespace GHKM
}  // namespace Syntax
}  // namespace MosesTraining
