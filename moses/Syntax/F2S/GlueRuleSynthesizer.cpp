#include "GlueRuleSynthesizer.h"

#include <sstream>

#include "moses/FF/UnknownWordPenaltyProducer.h"
#include "moses/StaticData.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

GlueRuleSynthesizer::GlueRuleSynthesizer(HyperTree &trie)
    : m_hyperTree(trie)
{
  const std::vector<FactorType> &inputFactorOrder =
      StaticData::Instance().GetInputFactorOrder();
  Word *lhs = NULL;
  m_dummySourcePhrase.CreateFromString(Input, inputFactorOrder, "hello", &lhs);
  delete lhs;
}

void GlueRuleSynthesizer::SynthesizeRule(const Forest::Hyperedge &e)
{
  HyperPath source;
  SynthesizeHyperPath(e, source);
  TargetPhrase *tp = SynthesizeTargetPhrase(e);
  TargetPhraseCollection &tpc = GetOrCreateTargetPhraseCollection(m_hyperTree,
                                                                  source);
  tpc.Add(tp);
}

void GlueRuleSynthesizer::SynthesizeHyperPath(const Forest::Hyperedge &e,
                                              HyperPath &path)
{
  path.nodeSeqs.clear();
  path.nodeSeqs.resize(2);
  path.nodeSeqs[0].push_back(e.head->pvertex.symbol[0]->GetId());
  for (std::vector<Forest::Vertex*>::const_iterator p = e.tail.begin();
       p != e.tail.end(); ++p) {
    const Forest::Vertex &child = **p;
    path.nodeSeqs[1].push_back(child.pvertex.symbol[0]->GetId());
  }
}

TargetPhrase *GlueRuleSynthesizer::SynthesizeTargetPhrase(
    const Forest::Hyperedge &e)
{
  const StaticData &staticData = StaticData::Instance();

  const UnknownWordPenaltyProducer &unknownWordPenaltyProducer =
      UnknownWordPenaltyProducer::Instance();

  TargetPhrase *targetPhrase = new TargetPhrase();

  std::ostringstream alignmentSS;
  for (std::size_t i = 0; i < e.tail.size(); ++i) {
    const Word &symbol = e.tail[i]->pvertex.symbol;
    if (symbol.IsNonTerminal()) {
      targetPhrase->AddWord(staticData.GetOutputDefaultNonTerminal());
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
  targetPhrase->EvaluateInIsolation(m_dummySourcePhrase);
  Word *targetLhs = new Word(staticData.GetOutputDefaultNonTerminal());
  targetPhrase->SetTargetLHS(targetLhs);
  targetPhrase->SetAlignmentInfo(alignmentSS.str());

  return targetPhrase;
}

}  // F2S
}  // Syntax
}  // Moses
