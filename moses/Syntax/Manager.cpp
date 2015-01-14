#include "Manager.h"

#include <sstream>

#include "moses/OutputCollector.h"
#include "moses/StaticData.h"

#include "PVertex.h"

namespace Moses
{
namespace Syntax
{

Manager::Manager(const InputType &source)
  : Moses::BaseManager(source)
{
}

void Manager::OutputBest(OutputCollector *collector) const
{
  if (!collector) {
    return;
  }
  std::ostringstream out;
  FixPrecision(out);
  const SHyperedge *best = GetBestSHyperedge();
  if (best == NULL) {
    VERBOSE(1, "NO BEST TRANSLATION" << std::endl);
    if (StaticData::Instance().GetOutputHypoScore()) {
      out << "0 ";
    }
  } else {
    if (StaticData::Instance().GetOutputHypoScore()) {
      out << best->score << " ";
    }
    Phrase yield = GetOneBestTargetYield(*best);
    // delete 1st & last
    UTIL_THROW_IF2(yield.GetSize() < 2,
                   "Output phrase should have contained at least 2 words (beginning and end-of-sentence)");
    yield.RemoveWord(0);
    yield.RemoveWord(yield.GetSize()-1);
    out << yield.GetStringRep(StaticData::Instance().GetOutputFactorOrder());
    out << '\n';
  }
  collector->Write(m_source.GetTranslationId(), out.str());
}

void Manager::OutputNBest(OutputCollector *collector) const
{
  if (collector) {
    const StaticData &staticData = StaticData::Instance();
    long translationId = m_source.GetTranslationId();

    KBestExtractor::KBestVec nBestList;
    ExtractKBest(staticData.GetNBestSize(), nBestList,
                 staticData.GetDistinctNBest());
    OutputNBestList(collector, nBestList, translationId);
  }
}

void Manager::OutputUnknowns(OutputCollector *collector) const
{
  if (collector) {
    long translationId = m_source.GetTranslationId();

    std::ostringstream out;
    for (std::set<Moses::Word>::const_iterator p = m_oovs.begin();
         p != m_oovs.end(); ++p) {
      out << *p;
    }
    out << std::endl;
    collector->Write(translationId, out.str());
  }
}

void Manager::OutputNBestList(OutputCollector *collector,
                              const KBestExtractor::KBestVec &nBestList,
                              long translationId) const
{
  const StaticData &staticData = StaticData::Instance();

  const std::vector<FactorType> &outputFactorOrder =
    staticData.GetOutputFactorOrder();

  std::ostringstream out;

  if (collector->OutputIsCout()) {
    // Set precision only if we're writing the n-best list to cout.  This is to
    // preserve existing behaviour, but should probably be done either way.
    FixPrecision(out);
  }

  bool includeWordAlignment = staticData.PrintAlignmentInfoInNbest();
  bool PrintNBestTrees = staticData.PrintNBestTrees();

  for (KBestExtractor::KBestVec::const_iterator p = nBestList.begin();
       p != nBestList.end(); ++p) {
    const KBestExtractor::Derivation &derivation = **p;

    // get the derivation's target-side yield
    Phrase outputPhrase = KBestExtractor::GetOutputPhrase(derivation);

    // delete <s> and </s>
    UTIL_THROW_IF2(outputPhrase.GetSize() < 2,
                   "Output phrase should have contained at least 2 words (beginning and end-of-sentence)");
    outputPhrase.RemoveWord(0);
    outputPhrase.RemoveWord(outputPhrase.GetSize() - 1);

    // print the translation ID, surface factors, and scores
    out << translationId << " ||| ";
    OutputSurface(out, outputPhrase, outputFactorOrder, false);
    out << " ||| ";
    derivation.scoreBreakdown.OutputAllFeatureScores(out);
    out << " ||| " << derivation.score;

    // optionally, print word alignments
    if (includeWordAlignment) {
      out << " ||| ";
      Alignments align;
      OutputAlignmentNBest(align, derivation, 0);
      for (Alignments::const_iterator q = align.begin(); q != align.end();
           ++q) {
        out << q->first << "-" << q->second << " ";
      }
    }

    // optionally, print tree
    if (PrintNBestTrees) {
      TreePointer tree = KBestExtractor::GetOutputTree(derivation);
      out << " ||| " << tree->GetString();
    }

    out << std::endl;
  }

  assert(collector);
  collector->Write(translationId, out.str());
}

std::size_t Manager::OutputAlignmentNBest(
  Alignments &retAlign,
  const KBestExtractor::Derivation &derivation,
  std::size_t startTarget) const
{
  const SHyperedge &shyperedge = derivation.edge->shyperedge;

  std::size_t totalTargetSize = 0;
  std::size_t startSource = shyperedge.head->pvertex->span.GetStartPos();

  const TargetPhrase &tp = *(shyperedge.translation);

  std::size_t thisSourceSize = CalcSourceSize(derivation);

  // position of each terminal word in translation rule, irrespective of
  // alignment if non-term, number is undefined
  std::vector<std::size_t> sourceOffsets(thisSourceSize, 0);
  std::vector<std::size_t> targetOffsets(tp.GetSize(), 0);

  const AlignmentInfo &aiNonTerm = shyperedge.translation->GetAlignNonTerm();
  std::vector<std::size_t> sourceInd2pos = aiNonTerm.GetSourceIndex2PosMap();
  const AlignmentInfo::NonTermIndexMap &targetPos2SourceInd =
    aiNonTerm.GetNonTermIndexMap();

  UTIL_THROW_IF2(sourceInd2pos.size() != derivation.subderivations.size(),
                 "Error");

  std::size_t targetInd = 0;
  for (std::size_t targetPos = 0; targetPos < tp.GetSize(); ++targetPos) {
    if (tp.GetWord(targetPos).IsNonTerminal()) {
      UTIL_THROW_IF2(targetPos >= targetPos2SourceInd.size(), "Error");
      std::size_t sourceInd = targetPos2SourceInd[targetPos];
      std::size_t sourcePos = sourceInd2pos[sourceInd];

      const KBestExtractor::Derivation &subderivation =
        *derivation.subderivations[sourceInd];

      // calc source size
      std::size_t sourceSize =
        subderivation.edge->head->svertex.pvertex->span.GetNumWordsCovered();
      sourceOffsets[sourcePos] = sourceSize;

      // calc target size.
      // Recursively look thru child hypos
      std::size_t currStartTarget = startTarget + totalTargetSize;
      std::size_t targetSize = OutputAlignmentNBest(retAlign, subderivation,
                               currStartTarget);
      targetOffsets[targetPos] = targetSize;

      totalTargetSize += targetSize;
      ++targetInd;
    } else {
      ++totalTargetSize;
    }
  }

  // convert position within translation rule to absolute position within
  // source sentence / output sentence
  ShiftOffsets(sourceOffsets, startSource);
  ShiftOffsets(targetOffsets, startTarget);

  // get alignments from this hypo
  const AlignmentInfo &aiTerm = shyperedge.translation->GetAlignTerm();

  // add to output arg, offsetting by source & target
  AlignmentInfo::const_iterator iter;
  for (iter = aiTerm.begin(); iter != aiTerm.end(); ++iter) {
    const std::pair<std::size_t, std::size_t> &align = *iter;
    std::size_t relSource = align.first;
    std::size_t relTarget = align.second;
    std::size_t absSource = sourceOffsets[relSource];
    std::size_t absTarget = targetOffsets[relTarget];

    std::pair<std::size_t, std::size_t> alignPoint(absSource, absTarget);
    std::pair<Alignments::iterator, bool> ret = retAlign.insert(alignPoint);
    UTIL_THROW_IF2(!ret.second, "Error");
  }

  return totalTargetSize;
}

std::size_t Manager::CalcSourceSize(const KBestExtractor::Derivation &d) const
{
  const SHyperedge &shyperedge = d.edge->shyperedge;
  std::size_t ret = shyperedge.head->pvertex->span.GetNumWordsCovered();
  for (std::size_t i = 0; i < shyperedge.tail.size(); ++i) {
    std::size_t childSize =
      shyperedge.tail[i]->pvertex->span.GetNumWordsCovered();
    ret -= (childSize - 1);
  }
  return ret;
}

}  // Syntax
}  // Moses
