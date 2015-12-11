#pragma once

#include "moses/Phrase.h"
#include "moses/TargetPhrase.h"

#include "HyperTree.h"
#include "HyperTreeCreator.h"
#include "Forest.h"

namespace Moses
{
class AllOptions;
namespace Syntax
{
namespace F2S
{

class GlueRuleSynthesizer : public HyperTreeCreator
{
  Word m_input_default_nonterminal;
  Word m_output_default_nonterminal;
public:
  GlueRuleSynthesizer(Moses::AllOptions const& opts, HyperTree &);

  // Synthesize the minimal, monotone rule that can be applied to the given
  // hyperedge and add it to the rule trie.
  void SynthesizeRule(const Forest::Hyperedge &);

private:
  void SynthesizeHyperPath(const Forest::Hyperedge &, HyperPath &);

  TargetPhrase *SynthesizeTargetPhrase(const Forest::Hyperedge &);

  HyperTree &m_hyperTree;
  Phrase m_dummySourcePhrase;
};

}  // F2S
}  // Syntax
}  // Moses
