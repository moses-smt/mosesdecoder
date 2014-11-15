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

#include <stdio.h>
#include "ChartManager.h"
#include "ChartCell.h"
#include "ChartHypothesis.h"
#include "ChartKBestExtractor.h"
#include "ChartTranslationOptions.h"
#include "HypergraphOutput.h"
#include "StaticData.h"
#include "DecodeStep.h"
#include "TreeInput.h"
#include "moses/FF/WordPenaltyProducer.h"

using namespace std;
using namespace Moses;

namespace Moses
{
extern bool g_mosesDebug;

/* constructor. Initialize everything prior to decoding a particular sentence.
 * \param source the sentence to be decoded
 * \param system which particular set of models to use.
 */
ChartManager::ChartManager(InputType const& source)
  :m_source(source)
  ,m_hypoStackColl(source, *this)
  ,m_start(clock())
  ,m_hypothesisId(0)
  ,m_parser(source, m_hypoStackColl)
  ,m_translationOptionList(StaticData::Instance().GetRuleLimit(), source)
{
}

ChartManager::~ChartManager()
{
  clock_t end = clock();
  float et = (end - m_start);
  et /= (float)CLOCKS_PER_SEC;
  VERBOSE(1, "Translation took " << et << " seconds" << endl);

}

//! decode the sentence. This contains the main laps. Basically, the CKY++ algorithm
void ChartManager::ProcessSentence()
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
      WordsRange range(startPos, endPos);

      // create trans opt
      m_translationOptionList.Clear();
      m_parser.Create(range, m_translationOptionList);
      m_translationOptionList.ApplyThreshold();

      const InputPath &inputPath = m_parser.GetInputPath(range);
      m_translationOptionList.EvaluateWithSourceContext(m_source, inputPath);

      // decode
      ChartCell &cell = m_hypoStackColl.Get(range);
      cell.ProcessSentence(m_translationOptionList, m_hypoStackColl);

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
        WordsRange range(startPos, startPos+width-1);
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
  // const StaticData &staticData = StaticData::Instance();

  const std::vector <ChartTranslationOptions*> xmlChartOptionsList = m_source.GetXmlChartTranslationOptions();
  IFVERBOSE(2) {
    cerr << "AddXmlChartOptions " << xmlChartOptionsList.size() << endl;
  }
  if (xmlChartOptionsList.size() == 0) return;

  for(std::vector<ChartTranslationOptions*>::const_iterator i = xmlChartOptionsList.begin();
      i != xmlChartOptionsList.end(); ++i) {
    ChartTranslationOptions* opt = *i;

    const WordsRange &range = opt->GetSourceWordsRange();

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
    WordsRange range(0, size-1);
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
  WordsRange range(0, m_source.GetSize()-1);
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
  const StaticData &staticData = StaticData::Instance();
  const std::size_t nBestFactor = staticData.GetNBestFactor();
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
  WordsRange fullRange(0, size-1);
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
      WordsRange range(startPos, endPos);
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

void ChartManager::OutputSearchGraphAsHypergraph(std::ostream &outputSearchGraphStream) const {
  ChartSearchGraphWriterHypergraph writer(&outputSearchGraphStream);
  WriteSearchGraph(writer);
}

void ChartManager::OutputSearchGraphMoses(std::ostream &outputSearchGraphStream) const {
  ChartSearchGraphWriterMoses writer(&outputSearchGraphStream, m_source.GetTranslationId());
  WriteSearchGraph(writer);
}

} // namespace Moses
