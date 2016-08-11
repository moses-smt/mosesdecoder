// $Id$
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

#include <cstdio>
#include "ChartManager.h"
#include "ChartCell.h"
#include "ChartHypothesis.h"
#include "ChartKBestExtractor.h"
#include "ChartTranslationOptions.h"
#include "HypergraphOutput.h"
#include "StaticData.h"
#include "DecodeStep.h"
#include "TreeInput.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/WordPenaltyProducer.h"
#include "moses/OutputCollector.h"
#include "moses/ChartKBestExtractor.h"
#include "moses/HypergraphOutput.h"
#include "moses/TranslationTask.h"

using namespace std;

namespace Moses
{

/* constructor. Initialize everything prior to decoding a particular sentence.
 * \param source the sentence to be decoded
 * \param system which particular set of models to use.
 */
ChartManager::ChartManager(ttasksptr const& ttask)
  : BaseManager(ttask)
  , m_hypoStackColl(m_source, *this)
  , m_start(clock())
  , m_hypothesisId(0)
  , m_parser(ttask, m_hypoStackColl)
  , m_translationOptionList(ttask->options()->syntax.rule_limit, m_source)
{ }

ChartManager::~ChartManager()
{
  clock_t end = clock();
  float et = (end - m_start);
  et /= (float)CLOCKS_PER_SEC;
  VERBOSE(1, "Translation took " << et << " seconds" << endl);

}

//! decode the sentence. This contains the main laps. Basically, the CKY++ algorithm
void ChartManager::Decode()
{

  VERBOSE(1,"Translating: " << m_source << endl);

  ResetSentenceStats(m_source);

  VERBOSE(2,"Decoding: " << endl);
  //ChartHypothesis::ResetHypoCount();

  AddXmlChartOptions();

  // MAIN LOOP
  size_t size = m_source.GetSize();
  for (int startPos = size-1; startPos >= 0; --startPos) {
    for (size_t width = 1; width <= size-startPos; ++width) {
      size_t endPos = startPos + width - 1;
      Range range(startPos, endPos);

      // create trans opt
      m_translationOptionList.Clear();
      m_parser.Create(range, m_translationOptionList);
      m_translationOptionList.ApplyThreshold(options()->search.trans_opt_threshold);

      const InputPath &inputPath = m_parser.GetInputPath(range);
      m_translationOptionList.EvaluateWithSourceContext(m_source, inputPath);

      // decode
      ChartCell &cell = m_hypoStackColl.Get(range);
      cell.Decode(m_translationOptionList, m_hypoStackColl);

      m_translationOptionList.Clear();
      cell.PruneToSize();
      cell.CleanupArcList();
      cell.SortHypotheses();
    }
  }

  IFVERBOSE(1) {

    for (size_t startPos = 0; startPos < size; ++startPos) {
      cerr.width(3);
      cerr << startPos << " ";
    }
    cerr << endl;
    for (size_t width = 1; width <= size; width++) {
      for( size_t space = 0; space < width-1; space++ ) {
        cerr << "  ";
      }
      for (size_t startPos = 0; startPos <= size-width; ++startPos) {
        Range range(startPos, startPos+width-1);
        cerr.width(3);
        cerr << m_hypoStackColl.Get(range).GetSize() << " ";
      }
      cerr << endl;
    }
  }
}

/** add specific translation options and hypotheses according to the XML override translation scheme.
 *  Doesn't seem to do anything about walls and zones.
 *  @todo check walls & zones. Check that the implementation doesn't leak, xml options sometimes does if you're not careful
 */
void ChartManager::AddXmlChartOptions()
{
  const std::vector <ChartTranslationOptions*> xmlChartOptionsList
  = m_source.GetXmlChartTranslationOptions();
  IFVERBOSE(2) {
    cerr << "AddXmlChartOptions " << xmlChartOptionsList.size() << endl;
  }
  if (xmlChartOptionsList.size() == 0) return;

  typedef std::vector<ChartTranslationOptions*>::const_iterator citer;
  for(citer i = xmlChartOptionsList.begin(); i != xmlChartOptionsList.end(); ++i) {
    ChartTranslationOptions* opt = *i;

    const Range &range = opt->GetSourceWordsRange();

    RuleCubeItem* item = new RuleCubeItem( *opt, m_hypoStackColl );
    ChartHypothesis* hypo = new ChartHypothesis(*opt, *item, *this);
    hypo->EvaluateWhenApplied();


    ChartCell &cell = m_hypoStackColl.Get(range);
    cell.AddHypothesis(hypo);
  }
}

//! get best complete translation from the top chart cell.
const ChartHypothesis *ChartManager::GetBestHypothesis() const
{
  size_t size = m_source.GetSize();

  if (size == 0) // empty source
    return NULL;
  else {
    Range range(0, size-1);
    const ChartCell &lastCell = m_hypoStackColl.Get(range);
    return lastCell.GetBestHypothesis();
  }
}

/** Calculate the n-best paths through the output hypergraph.
 * Return the list of paths with the variable ret
 * \param n how may paths to return
 * \param ret return argument
 * \param onlyDistinct whether to check for distinct output sentence or not (default - don't check, just return top n-paths)
 */
void ChartManager::CalcNBest(
  std::size_t n,
  std::vector<boost::shared_ptr<ChartKBestExtractor::Derivation> > &nBestList,
  bool onlyDistinct) const
{
  nBestList.clear();
  if (n == 0 || m_source.GetSize() == 0) {
    return;
  }

  // Get the list of top-level hypotheses, sorted by score.
  Range range(0, m_source.GetSize()-1);
  const ChartCell &lastCell = m_hypoStackColl.Get(range);
  boost::scoped_ptr<const std::vector<const ChartHypothesis*> > topLevelHypos(
    lastCell.GetAllSortedHypotheses());
  if (!topLevelHypos) {
    return;
  }

  ChartKBestExtractor extractor;

  if (!onlyDistinct) {
    // Return the n-best list as is, including duplicate translations.
    extractor.Extract(*topLevelHypos, n, nBestList);
    return;
  }

  // Determine how many derivations to extract.  If the n-best list is
  // restricted to distinct translations then this limit should be bigger
  // than n.  The n-best factor determines how much bigger the limit should be,
  // with 0 being 'unlimited.'  This actually sets a large-ish limit in case
  // too many translations are identical.
  const std::size_t nBestFactor = options()->nbest.factor;
  std::size_t numDerivations = (nBestFactor == 0) ? n*1000 : n*nBestFactor;

  // Extract the derivations.
  ChartKBestExtractor::KBestVec bigList;
  bigList.reserve(numDerivations);
  extractor.Extract(*topLevelHypos, numDerivations, bigList);

  // Copy derivations into nBestList, skipping ones with repeated translations.
  std::set<Phrase> distinct;
  for (ChartKBestExtractor::KBestVec::const_iterator p = bigList.begin();
       nBestList.size() < n && p != bigList.end(); ++p) {
    boost::shared_ptr<ChartKBestExtractor::Derivation> derivation = *p;
    Phrase translation = ChartKBestExtractor::GetOutputPhrase(*derivation);
    if (distinct.insert(translation).second) {
      nBestList.push_back(derivation);
    }
  }
}

void ChartManager::WriteSearchGraph(const ChartSearchGraphWriter& writer) const
{

  size_t size = m_source.GetSize();

  // which hypotheses are reachable?
  std::map<unsigned,bool> reachable;
  Range fullRange(0, size-1);
  const ChartCell &lastCell = m_hypoStackColl.Get(fullRange);
  const ChartHypothesis *hypo = lastCell.GetBestHypothesis();

  if (hypo == NULL) {
    // no hypothesis
    return;
  }
  size_t winners = 0;
  size_t losers = 0;

  FindReachableHypotheses( hypo, reachable, &winners, &losers);
  writer.WriteHeader(winners, losers);

  for (size_t width = 1; width <= size; ++width) {
    for (size_t startPos = 0; startPos <= size-width; ++startPos) {
      size_t endPos = startPos + width - 1;
      Range range(startPos, endPos);
      TRACE_ERR(" " << range << "=");

      const ChartCell &cell = m_hypoStackColl.Get(range);
      cell.WriteSearchGraph(writer, reachable);
    }
  }
}

void ChartManager::FindReachableHypotheses(
  const ChartHypothesis *hypo, std::map<unsigned,bool> &reachable, size_t* winners, size_t* losers) const
{
  // do not recurse, if already visited
  if (reachable.find(hypo->GetId()) != reachable.end()) {
    return;
  }

  // recurse
  reachable[ hypo->GetId() ] = true;
  if (hypo->GetWinningHypothesis() == hypo) {
    (*winners)++;
  } else {
    (*losers)++;
  }
  const std::vector<const ChartHypothesis*> &previous = hypo->GetPrevHypos();
  for(std::vector<const ChartHypothesis*>::const_iterator i = previous.begin(); i != previous.end(); ++i) {
    FindReachableHypotheses( *i, reachable, winners, losers );
  }

  // also loop over recombined hypotheses (arcs)
  const ChartArcList *arcList = hypo->GetArcList();
  if (arcList) {
    ChartArcList::const_iterator iterArc;
    for (iterArc = arcList->begin(); iterArc != arcList->end(); ++iterArc) {
      const ChartHypothesis &arc = **iterArc;
      FindReachableHypotheses( &arc, reachable, winners, losers );
    }
  }
}

void
ChartManager::
OutputSearchGraphAsHypergraph(std::ostream& out) const
{
  ChartSearchGraphWriterHypergraph writer(options(), &out);
  WriteSearchGraph(writer);
}

void ChartManager::OutputSearchGraphMoses(std::ostream &outputSearchGraphStream) const
{
  ChartSearchGraphWriterMoses writer(options(), &outputSearchGraphStream,
                                     m_source.GetTranslationId());
  WriteSearchGraph(writer);
}

void ChartManager::OutputBest(OutputCollector *collector) const
{
  const ChartHypothesis *bestHypo = GetBestHypothesis();
  if (collector && bestHypo) {
    const size_t translationId = m_source.GetTranslationId();
    const ChartHypothesis *bestHypo = GetBestHypothesis();
    OutputBestHypo(collector, bestHypo, translationId);
  }
}

void ChartManager::OutputNBest(OutputCollector *collector) const
{
  size_t nBestSize = options()->nbest.nbest_size;
  if (nBestSize > 0) {
    const size_t translationId = m_source.GetTranslationId();

    VERBOSE(2,"WRITING " << nBestSize << " TRANSLATION ALTERNATIVES TO "
            << options()->nbest.output_file_path << endl);
    std::vector<boost::shared_ptr<ChartKBestExtractor::Derivation> > nBestList;
    CalcNBest(nBestSize, nBestList, options()->nbest.only_distinct);
    OutputNBestList(collector, nBestList, translationId);
    IFVERBOSE(2) {
      PrintUserTime("N-Best Hypotheses Generation Time:");
    }
  }

}

void ChartManager::OutputNBestList(OutputCollector *collector,
                                   const ChartKBestExtractor::KBestVec &nBestList,
                                   long translationId) const
{
  std::ostringstream out;

  if (collector->OutputIsCout()) {
    // Set precision only if we're writing the n-best list to cout.  This is to
    // preserve existing behaviour, but should probably be done either way.
    FixPrecision(out);
  }

  NBestOptions const& nbo = options()->nbest;
  bool includeWordAlignment = nbo.include_alignment_info;
  bool PrintNBestTrees = nbo.print_trees;

  for (ChartKBestExtractor::KBestVec::const_iterator p = nBestList.begin();
       p != nBestList.end(); ++p) {
    const ChartKBestExtractor::Derivation &derivation = **p;

    // get the derivation's target-side yield
    Phrase outputPhrase = ChartKBestExtractor::GetOutputPhrase(derivation);

    // delete <s> and </s>
    UTIL_THROW_IF2(outputPhrase.GetSize() < 2,
                   "Output phrase should have contained at least 2 words (beginning and end-of-sentence)");
    outputPhrase.RemoveWord(0);
    outputPhrase.RemoveWord(outputPhrase.GetSize() - 1);

    // print the translation ID, surface factors, and scores
    out << translationId << " ||| ";
    OutputSurface(out, outputPhrase); // , outputFactorOrder, false);
    out << " ||| ";
    boost::shared_ptr<ScoreComponentCollection> scoreBreakdown = ChartKBestExtractor::GetOutputScoreBreakdown(derivation);
    bool with_labels = options()->nbest.include_feature_labels;
    scoreBreakdown->OutputAllFeatureScores(out, with_labels);
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
      TreePointer tree = ChartKBestExtractor::GetOutputTree(derivation);
      out << " ||| " << tree->GetString();
    }

    out << std::endl;
  }

  assert(collector);
  collector->Write(translationId, out.str());
}

size_t ChartManager::CalcSourceSize(const Moses::ChartHypothesis *hypo) const
{
  size_t ret = hypo->GetCurrSourceRange().GetNumWordsCovered();
  const std::vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();
  for (size_t i = 0; i < prevHypos.size(); ++i) {
    size_t childSize = prevHypos[i]->GetCurrSourceRange().GetNumWordsCovered();
    ret -= (childSize - 1);
  }
  return ret;
}

size_t ChartManager::OutputAlignmentNBest(
  Alignments &retAlign,
  const Moses::ChartKBestExtractor::Derivation &derivation,
  size_t startTarget) const
{
  const ChartHypothesis &hypo = derivation.edge.head->hypothesis;

  size_t totalTargetSize = 0;
  size_t startSource = hypo.GetCurrSourceRange().GetStartPos();

  const TargetPhrase &tp = hypo.GetCurrTargetPhrase();

  size_t thisSourceSize = CalcSourceSize(&hypo);

  // position of each terminal word in translation rule, irrespective of alignment
  // if non-term, number is undefined
  vector<size_t> sourceOffsets(thisSourceSize, 0);
  vector<size_t> targetOffsets(tp.GetSize(), 0);

  const AlignmentInfo &aiNonTerm = hypo.GetCurrTargetPhrase().GetAlignNonTerm();
  vector<size_t> sourceInd2pos = aiNonTerm.GetSourceIndex2PosMap();
  const AlignmentInfo::NonTermIndexMap &targetPos2SourceInd = aiNonTerm.GetNonTermIndexMap();

  UTIL_THROW_IF2(sourceInd2pos.size() != derivation.subderivations.size(),
                 "Error");

  size_t targetInd = 0;
  for (size_t targetPos = 0; targetPos < tp.GetSize(); ++targetPos) {
    if (tp.GetWord(targetPos).IsNonTerminal()) {
      UTIL_THROW_IF2(targetPos >= targetPos2SourceInd.size(), "Error");
      size_t sourceInd = targetPos2SourceInd[targetPos];
      size_t sourcePos = sourceInd2pos[sourceInd];

      const Moses::ChartKBestExtractor::Derivation &subderivation =
        *derivation.subderivations[sourceInd];

      // calc source size
      size_t sourceSize = subderivation.edge.head->hypothesis.GetCurrSourceRange().GetNumWordsCovered();
      sourceOffsets[sourcePos] = sourceSize;

      // calc target size.
      // Recursively look thru child hypos
      size_t currStartTarget = startTarget + totalTargetSize;
      size_t targetSize = OutputAlignmentNBest(retAlign, subderivation,
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
  const AlignmentInfo &aiTerm = hypo.GetCurrTargetPhrase().GetAlignTerm();

  // add to output arg, offsetting by source & target
  AlignmentInfo::const_iterator iter;
  for (iter = aiTerm.begin(); iter != aiTerm.end(); ++iter) {
    const std::pair<size_t,size_t> &align = *iter;
    size_t relSource = align.first;
    size_t relTarget = align.second;
    size_t absSource = sourceOffsets[relSource];
    size_t absTarget = targetOffsets[relTarget];

    pair<size_t, size_t> alignPoint(absSource, absTarget);
    pair<Alignments::iterator, bool> ret = retAlign.insert(alignPoint);
    UTIL_THROW_IF2(!ret.second, "Error");
  }

  return totalTargetSize;
}

void ChartManager::OutputAlignment(OutputCollector *collector) const
{
  if (collector == NULL) {
    return;
  }

  ostringstream out;

  const ChartHypothesis *hypo = GetBestHypothesis();
  if (hypo) {
    Alignments retAlign;
    OutputAlignment(retAlign, hypo, 0);

    // output alignments
    Alignments::const_iterator iter;
    for (iter = retAlign.begin(); iter != retAlign.end(); ++iter) {
      const pair<size_t, size_t> &alignPoint = *iter;
      out << alignPoint.first << "-" << alignPoint.second << " ";
    }
  }
  out << endl;

  collector->Write(m_source.GetTranslationId(), out.str());

}

size_t ChartManager::OutputAlignment(Alignments &retAlign,
                                     const Moses::ChartHypothesis *hypo,
                                     size_t startTarget) const
{
  size_t totalTargetSize = 0;
  size_t startSource = hypo->GetCurrSourceRange().GetStartPos();

  const TargetPhrase &tp = hypo->GetCurrTargetPhrase();

  size_t thisSourceSize = CalcSourceSize(hypo);

  // position of each terminal word in translation rule, irrespective of alignment
  // if non-term, number is undefined
  vector<size_t> sourceOffsets(thisSourceSize, 0);
  vector<size_t> targetOffsets(tp.GetSize(), 0);

  const vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();

  const AlignmentInfo &aiNonTerm = hypo->GetCurrTargetPhrase().GetAlignNonTerm();
  vector<size_t> sourceInd2pos = aiNonTerm.GetSourceIndex2PosMap();
  const AlignmentInfo::NonTermIndexMap &targetPos2SourceInd = aiNonTerm.GetNonTermIndexMap();

  UTIL_THROW_IF2(sourceInd2pos.size() != prevHypos.size(), "Error");

  size_t targetInd = 0;
  for (size_t targetPos = 0; targetPos < tp.GetSize(); ++targetPos) {
    if (tp.GetWord(targetPos).IsNonTerminal()) {
      UTIL_THROW_IF2(targetPos >= targetPos2SourceInd.size(), "Error");
      size_t sourceInd = targetPos2SourceInd[targetPos];
      size_t sourcePos = sourceInd2pos[sourceInd];

      const ChartHypothesis *prevHypo = prevHypos[sourceInd];

      // calc source size
      size_t sourceSize = prevHypo->GetCurrSourceRange().GetNumWordsCovered();
      sourceOffsets[sourcePos] = sourceSize;

      // calc target size.
      // Recursively look thru child hypos
      size_t currStartTarget = startTarget + totalTargetSize;
      size_t targetSize = OutputAlignment(retAlign, prevHypo, currStartTarget);
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
  const AlignmentInfo &aiTerm = hypo->GetCurrTargetPhrase().GetAlignTerm();

  // add to output arg, offsetting by source & target
  AlignmentInfo::const_iterator iter;
  for (iter = aiTerm.begin(); iter != aiTerm.end(); ++iter) {
    const std::pair<size_t,size_t> &align = *iter;
    size_t relSource = align.first;
    size_t relTarget = align.second;
    size_t absSource = sourceOffsets[relSource];
    size_t absTarget = targetOffsets[relTarget];

    pair<size_t, size_t> alignPoint(absSource, absTarget);
    pair<Alignments::iterator, bool> ret = retAlign.insert(alignPoint);
    UTIL_THROW_IF2(!ret.second, "Error");

  }

  return totalTargetSize;
}

void ChartManager::OutputDetailedTranslationReport(OutputCollector *collector) const
{
  if (collector) {
    OutputDetailedTranslationReport(collector,
                                    GetBestHypothesis(),
                                    static_cast<const Sentence&>(m_source),
                                    m_source.GetTranslationId());
  }
}

void ChartManager::OutputDetailedTranslationReport(
  OutputCollector *collector,
  const ChartHypothesis *hypo,
  const Sentence &sentence,
  long translationId) const
{
  if (hypo == NULL) {
    return;
  }
  std::ostringstream out;
  ApplicationContext applicationContext;

  OutputTranslationOptions(out, applicationContext, hypo, sentence, translationId);
  collector->Write(translationId, out.str());

  //DIMw
  if (options()->output.detailed_all_transrep_filepath.size()) {
    const Sentence &sentence = static_cast<const Sentence &>(m_source);
    size_t nBestSize = options()->nbest.nbest_size;
    std::vector<boost::shared_ptr<ChartKBestExtractor::Derivation> > nBestList;
    CalcNBest(nBestSize, nBestList, options()->nbest.only_distinct);
    OutputDetailedAllTranslationReport(collector, nBestList, sentence, translationId);
  }

}

void ChartManager::OutputTranslationOptions(std::ostream &out,
    ApplicationContext &applicationContext,
    const ChartHypothesis *hypo,
    const Sentence &sentence,
    long translationId) const
{
  if (hypo != NULL) {
    OutputTranslationOption(out, applicationContext, hypo, sentence, translationId);
    out << std::endl;
  }

  // recursive
  const std::vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();
  std::vector<const ChartHypothesis*>::const_iterator iter;
  for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter) {
    const ChartHypothesis *prevHypo = *iter;
    OutputTranslationOptions(out, applicationContext, prevHypo, sentence, translationId);
  }
}

void ChartManager::OutputTranslationOption(std::ostream &out,
    ApplicationContext &applicationContext,
    const ChartHypothesis *hypo,
    const Sentence &sentence,
    long translationId) const
{
  ReconstructApplicationContext(*hypo, sentence, applicationContext);
  out << "Trans Opt " << translationId
      << " " << hypo->GetCurrSourceRange()
      << ": ";
  WriteApplicationContext(out, applicationContext);
  out << ": " << hypo->GetCurrTargetPhrase().GetTargetLHS()
      << "->" << hypo->GetCurrTargetPhrase()
      << " " << hypo->GetFutureScore() << hypo->GetScoreBreakdown();
}

// Given a hypothesis and sentence, reconstructs the 'application context' --
// the source RHS symbols of the SCFG rule that was applied, plus their spans.
void ChartManager::ReconstructApplicationContext(const ChartHypothesis &hypo,
    const Sentence &sentence,
    ApplicationContext &context) const
{
  context.clear();
  const std::vector<const ChartHypothesis*> &prevHypos = hypo.GetPrevHypos();
  std::vector<const ChartHypothesis*>::const_iterator p = prevHypos.begin();
  std::vector<const ChartHypothesis*>::const_iterator end = prevHypos.end();
  const Range &span = hypo.GetCurrSourceRange();
  size_t i = span.GetStartPos();
  while (i <= span.GetEndPos()) {
    if (p == end || i < (*p)->GetCurrSourceRange().GetStartPos()) {
      // Symbol is a terminal.
      const Word &symbol = sentence.GetWord(i);
      context.push_back(std::make_pair(symbol, Range(i, i)));
      ++i;
    } else {
      // Symbol is a non-terminal.
      const Word &symbol = (*p)->GetTargetLHS();
      const Range &range = (*p)->GetCurrSourceRange();
      context.push_back(std::make_pair(symbol, range));
      i = range.GetEndPos()+1;
      ++p;
    }
  }
}

void ChartManager::OutputUnknowns(OutputCollector *collector) const
{
  if (collector) {
    long translationId = m_source.GetTranslationId();
    const std::vector<Phrase*> &oovs = GetParser().GetUnknownSources();

    std::ostringstream out;
    for (std::vector<Phrase*>::const_iterator p = oovs.begin();
         p != oovs.end(); ++p) {
      out << **p;
    }
    out << std::endl;
    collector->Write(translationId, out.str());
  }

}

void ChartManager::OutputDetailedTreeFragmentsTranslationReport(OutputCollector *collector) const
{
  const ChartHypothesis *hypo = GetBestHypothesis();
  if (collector == NULL || hypo == NULL) {
    return;
  }

  std::ostringstream out;
  ApplicationContext applicationContext;

  const Sentence &sentence = static_cast<const Sentence &>(m_source);
  const size_t translationId = m_source.GetTranslationId();

  OutputTreeFragmentsTranslationOptions(out, applicationContext, hypo, sentence, translationId);

  //Tree of full sentence
  const StatefulFeatureFunction* treeStructure;
  treeStructure = StaticData::Instance().GetTreeStructure();
  if (treeStructure != NULL) {
    const vector<const StatefulFeatureFunction*>& sff = StatefulFeatureFunction::GetStatefulFeatureFunctions();
    for( size_t i=0; i<sff.size(); i++ ) {
      if (sff[i] == treeStructure) {
        const TreeState* tree = static_cast<const TreeState*>(hypo->GetFFState(i));
        out << "Full Tree " << translationId << ": " << tree->GetTree()->GetString() << "\n";
        break;
      }
    }
  }

  collector->Write(translationId, out.str());

}

void ChartManager::OutputTreeFragmentsTranslationOptions(std::ostream &out,
    ApplicationContext &applicationContext,
    const ChartHypothesis *hypo,
    const Sentence &sentence,
    long translationId) const
{

  if (hypo != NULL) {
    OutputTranslationOption(out, applicationContext, hypo, sentence, translationId);

    const TargetPhrase &currTarPhr = hypo->GetCurrTargetPhrase();

    out << " ||| ";
    if (const PhraseProperty *property = currTarPhr.GetProperty("Tree")) {
      out << " " << *property->GetValueString();
    } else {
      out << " " << "noTreeInfo";
    }
    out << std::endl;
  }

  // recursive
  const std::vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();
  std::vector<const ChartHypothesis*>::const_iterator iter;
  for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter) {
    const ChartHypothesis *prevHypo = *iter;
    OutputTreeFragmentsTranslationOptions(out, applicationContext, prevHypo, sentence, translationId);
  }
}

void ChartManager::OutputSearchGraph(OutputCollector *collector) const
{
  if (collector) {
    long translationId = m_source.GetTranslationId();
    std::ostringstream out;
    OutputSearchGraphMoses( out);
    collector->Write(translationId, out.str());
  }
}

//DIMw
void ChartManager::OutputDetailedAllTranslationReport(
  OutputCollector *collector,
  const std::vector<boost::shared_ptr<Moses::ChartKBestExtractor::Derivation> > &nBestList,
  const Sentence &sentence,
  long translationId) const
{
  std::ostringstream out;
  ApplicationContext applicationContext;

  const ChartCellCollection& cells = GetChartCellCollection();
  size_t size = GetSource().GetSize();
  for (size_t width = 1; width <= size; ++width) {
    for (size_t startPos = 0; startPos <= size-width; ++startPos) {
      size_t endPos = startPos + width - 1;
      Range range(startPos, endPos);
      const ChartCell& cell = cells.Get(range);
      const HypoList* hyps = cell.GetAllSortedHypotheses();
      out << "Chart Cell [" << startPos << ".." << endPos << "]" << endl;
      HypoList::const_iterator iter;
      size_t c = 1;
      for (iter = hyps->begin(); iter != hyps->end(); ++iter) {
        out << "----------------Item " << c++ << " ---------------------"
            << endl;
        OutputTranslationOptions(out, applicationContext, *iter,
                                 sentence, translationId);
      }
    }
  }
  collector->Write(translationId, out.str());
}

void ChartManager::OutputBestHypo(OutputCollector *collector, const ChartHypothesis *hypo, long translationId) const
{
  if (!collector)
    return;
  std::ostringstream out;
  FixPrecision(out);
  if (hypo != NULL) {
    VERBOSE(1,"BEST TRANSLATION: " << *hypo << endl);
    VERBOSE(3,"Best path: ");
    Backtrack(hypo);
    VERBOSE(3,"0" << std::endl);

    if (options()->output.ReportHypoScore) {
      out << hypo->GetFutureScore() << " ";
    }

    if (options()->output.RecoverPath) {
      out << "||| ";
    }
    Phrase outPhrase(ARRAY_SIZE_INCR);
    hypo->GetOutputPhrase(outPhrase);

    // delete 1st & last
    UTIL_THROW_IF2(outPhrase.GetSize() < 2,
                   "Output phrase should have contained at least 2 words (beginning and end-of-sentence)");

    outPhrase.RemoveWord(0);
    outPhrase.RemoveWord(outPhrase.GetSize() - 1);

    string output = outPhrase.GetStringRep(options()->output.factor_order);
    out << output << endl;
  } else {
    VERBOSE(1, "NO BEST TRANSLATION" << endl);

    if (options()->output.ReportHypoScore) {
      out << "0 ";
    }

    out << endl;
  }
  collector->Write(translationId, out.str());
}

void ChartManager::Backtrack(const ChartHypothesis *hypo) const
{
  const vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();

  vector<const ChartHypothesis*>::const_iterator iter;
  for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter) {
    const ChartHypothesis *prevHypo = *iter;

    VERBOSE(3,prevHypo->GetId() << " <= ");
    Backtrack(prevHypo);
  }
}

} // namespace Moses
