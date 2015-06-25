#pragma once

#include "moses/FF/UnknownWordPenaltyProducer.h"
#include "moses/StaticData.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

template<typename RuleTrie>
template<typename InputIterator>
boost::shared_ptr<RuleTrie> OovHandler<RuleTrie>::SynthesizeRuleTrie(
  InputIterator first, InputIterator last)
{
  const UnknownLHSList &lhsList = StaticData::Instance().GetUnknownLHS();

  boost::shared_ptr<RuleTrie> trie(new RuleTrie(&m_ruleTableFF));

  while (first != last) {
    const Word &oov = *first++;
    if (ShouldDrop(oov)) {
      continue;
    }
    boost::scoped_ptr<Phrase> srcPhrase(SynthesizeSourcePhrase(oov));
    for (UnknownLHSList::const_iterator p = lhsList.begin();
         p != lhsList.end(); ++p) {
      const std::string &targetLhsStr = p->first;
      float prob = p->second;
// TODO Check ownership and fix any leaks.
      Word *tgtLHS = SynthesizeTargetLhs(targetLhsStr);
      TargetPhrase *tp = SynthesizeTargetPhrase(oov, *srcPhrase, *tgtLHS, prob);
      TargetPhraseCollection &tpc = GetOrCreateTargetPhraseCollection(
                                      *trie, *srcPhrase, *tp, NULL);  // TODO Check NULL is valid argument
      tpc.Add(tp);
    }
  }

  return trie;
}

template<typename RuleTrie>
Phrase *OovHandler<RuleTrie>::SynthesizeSourcePhrase(const Word &sourceWord)
{
  Phrase *phrase = new Phrase(1);
  phrase->AddWord() = sourceWord;
  phrase->GetWord(0).SetIsOOV(true);
  return phrase;
}

template<typename RuleTrie>
Word *OovHandler<RuleTrie>::SynthesizeTargetLhs(const std::string &lhsStr)
{
  Word *targetLhs = new Word(true);
  targetLhs->CreateFromString(Output,
                              StaticData::Instance().GetOutputFactorOrder(),
                              lhsStr, true);
  UTIL_THROW_IF2(targetLhs->GetFactor(0) == NULL, "Null factor for target LHS");
  return targetLhs;
}

template<typename RuleTrie>
TargetPhrase *OovHandler<RuleTrie>::SynthesizeTargetPhrase(
  const Word &oov, const Phrase &srcPhrase, const Word &targetLhs, float prob)
{
  const StaticData &staticData = StaticData::Instance();

  const UnknownWordPenaltyProducer &unknownWordPenaltyProducer =
    UnknownWordPenaltyProducer::Instance();

  TargetPhrase *targetPhrase = new TargetPhrase();
  Word &targetWord = targetPhrase->AddWord();
  targetWord.CreateUnknownWord(oov);

  // scores
  float score = FloorScore(TransformScore(prob));

  targetPhrase->GetScoreBreakdown().Assign(&unknownWordPenaltyProducer, score);
  targetPhrase->EvaluateInIsolation(srcPhrase);
  targetPhrase->SetTargetLHS(&targetLhs);
  targetPhrase->SetAlignmentInfo("0-0");
  if (staticData.IsDetailedTreeFragmentsTranslationReportingEnabled() ||
      staticData.GetTreeStructure() != NULL) {
    std::string value = "[ " + targetLhs[0]->GetString().as_string() + " " +
                        oov[0]->GetString().as_string() + " ]";
    targetPhrase->SetProperty("Tree", value);
  }

  return targetPhrase;
}

template<typename RuleTrie>
bool OovHandler<RuleTrie>::ShouldDrop(const Word &oov)
{
  if (!StaticData::Instance().GetDropUnknown()) {
    return false;
  }
  const Factor *f = oov[0]; // TODO hack. shouldn't know which factor is surface
  const StringPiece s = f->GetString();
  return s.find_first_of("0123456789") != std::string::npos;
}

}  // S2T
}  // Syntax
}  // Moses
