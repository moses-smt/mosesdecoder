#include "GlueRuleSynthesizer.h"

#include <sstream>

#include "moses/FF/UnknownWordPenaltyProducer.h"
#include <boost/scoped_ptr.hpp>

namespace Moses
{
namespace Syntax
{
namespace T2S
{

void
GlueRuleSynthesizer::
SynthesizeRule(const InputTree::Node &node)
{
  const Word &sourceLhs = node.pvertex.symbol;
  boost::scoped_ptr<Phrase> sourceRhs(SynthesizeSourcePhrase(node));
  TargetPhrase *tp = SynthesizeTargetPhrase(node, *sourceRhs);
  TargetPhraseCollection::shared_ptr tpc
  = GetOrCreateTargetPhraseCollection(m_ruleTrie, sourceLhs, *sourceRhs);
  tpc->Add(tp);
}

Phrase*
GlueRuleSynthesizer::
SynthesizeSourcePhrase(const InputTree::Node &node)
{
  Phrase *phrase = new Phrase(node.children.size());
  for (std::vector<InputTree::Node*>::const_iterator p = node.children.begin();
       p != node.children.end(); ++p) {
    phrase->AddWord((*p)->pvertex.symbol);
  }
  /*
  TODO What counts as an OOV?
    phrase->AddWord() = sourceWord;
    phrase->GetWord(0).SetIsOOV(true);
  */
  return phrase;
}

TargetPhrase*
GlueRuleSynthesizer::
SynthesizeTargetPhrase(const InputTree::Node &node, const Phrase &sourceRhs)
{
  const UnknownWordPenaltyProducer &unknownWordPenaltyProducer =
    UnknownWordPenaltyProducer::Instance();

  TargetPhrase *targetPhrase = new TargetPhrase();

  util::StringStream alignmentSS;
  for (std::size_t i = 0; i < node.children.size(); ++i) {
    const Word &symbol = node.children[i]->pvertex.symbol;
    if (symbol.IsNonTerminal()) {
      targetPhrase->AddWord(m_output_default_nonterminal);
    } else {
      // TODO Check this
      Word &targetWord = targetPhrase->AddWord();
      targetWord.CreateUnknownWord(symbol);
    }
    alignmentSS << i << "-" << i << " ";
  }

  // Assign the lowest possible score so that glue rules are only used when
  // absolutely required.
  float score = LOWEST_SCORE;
  targetPhrase->GetScoreBreakdown().Assign(&unknownWordPenaltyProducer, score);
  targetPhrase->EvaluateInIsolation(sourceRhs);
  Word *targetLhs = new Word(m_output_default_nonterminal);
  targetPhrase->SetTargetLHS(targetLhs);
  targetPhrase->SetAlignmentInfo(alignmentSS.str());

  return targetPhrase;
}

}  // T2S
}  // Syntax
}  // Moses
