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
#include "ChartTrellisPath.h"
#include "ChartTrellisPathList.h"
#include "ChartTrellisPathCollection.h"
#include "StaticData.h"
#include "DecodeStep.h"

using namespace std;
using namespace Moses;

namespace Moses
{
extern bool g_debug;

ChartManager::ChartManager(InputType const& source, const TranslationSystem* system)
  :m_source(source)
  ,m_hypoStackColl(source, *this)
  ,m_transOptColl(source, system, m_hypoStackColl, m_ruleLookupManagers)
  ,m_system(system)
  ,m_start(clock())
{
  m_system->InitializeBeforeSentenceProcessing(source);
  const std::vector<PhraseDictionaryFeature*> &dictionaries = m_system->GetPhraseDictionaries();
  m_ruleLookupManagers.reserve(dictionaries.size());
  for (std::vector<PhraseDictionaryFeature*>::const_iterator p = dictionaries.begin();
       p != dictionaries.end(); ++p) {
    PhraseDictionaryFeature *pdf = *p;
    const PhraseDictionary *dict = pdf->GetDictionary();
    PhraseDictionary *nonConstDict = const_cast<PhraseDictionary*>(dict);
    m_ruleLookupManagers.push_back(nonConstDict->CreateRuleLookupManager(source, m_hypoStackColl));
  }
}

ChartManager::~ChartManager()
{
  m_system->CleanUpAfterSentenceProcessing();

  RemoveAllInColl(m_ruleLookupManagers);

  clock_t end = clock();
  float et = (end - m_start);
  et /= (float)CLOCKS_PER_SEC;
  VERBOSE(1, "Translation took " << et << " seconds" << endl);

}

void ChartManager::ProcessSentence()
{
  VERBOSE(1,"Translating: " << m_source << endl);

  ResetSentenceStats(m_source);

  VERBOSE(2,"Decoding: " << endl);
  //ChartHypothesis::ResetHypoCount();

  // MAIN LOOP
  size_t size = m_source.GetSize();
  for (size_t width = 1; width <= size; ++width) {
    for (size_t startPos = 0; startPos <= size-width; ++startPos) {
      size_t endPos = startPos + width - 1;
      WordsRange range(startPos, endPos);
      //TRACE_ERR(" " << range << "=");

      // create trans opt
      m_transOptColl.CreateTranslationOptionsForRange(startPos, endPos);
      //if (g_debug)
      //	cerr << m_transOptColl.GetTranslationOptionList(WordsRange(startPos, endPos));

      // decode
      ChartCell &cell = m_hypoStackColl.Get(range);

      cell.ProcessSentence(m_transOptColl.GetTranslationOptionList(range)
                           ,m_hypoStackColl);
      cell.PruneToSize();
      cell.CleanupArcList();
      cell.SortHypotheses();

      //cerr << cell.GetSize();
      //cerr << cell << endl;
      //cell.OutputSizes(cerr);
    }
  }

  IFVERBOSE(1) {
    cerr << "Num of hypo = " << ChartHypothesis::GetHypoCount() << " --- cells:" << endl;

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

void ChartManager::CalcNBest(size_t count, ChartTrellisPathList &ret,bool onlyDistinct) const
{
  size_t size = m_source.GetSize();
  if (count == 0 || size == 0)
    return;

  ChartTrellisPathCollection contenders;
  set<Phrase> distinctHyps;

  // add all pure paths
  WordsRange range(0, size-1);
  const ChartCell &lastCell = m_hypoStackColl.Get(range);
  const ChartHypothesis *hypo = lastCell.GetBestHypothesis();

  if (hypo == NULL) {
    // no hypothesis
    return;
  }

  ChartTrellisPath *purePath = new ChartTrellisPath(hypo);
  contenders.Add(purePath);

  // factor defines stopping point for distinct n-best list if too many candidates identical
  size_t nBestFactor = StaticData::Instance().GetNBestFactor();
  if (nBestFactor < 1) nBestFactor = 1000; // 0 = unlimited

  // MAIN loop
  for (size_t iteration = 0 ; (onlyDistinct ? distinctHyps.size() : ret.GetSize()) < count && contenders.GetSize() > 0 && (iteration < count * nBestFactor) ; iteration++) {
    // get next best from list of contenders
    ChartTrellisPath *path = contenders.pop();
    assert(path);

    // create deviations from current best
    path->CreateDeviantPaths(contenders);

    if(onlyDistinct) {
      Phrase tgtPhrase = path->GetOutputPhrase();
      if (distinctHyps.insert(tgtPhrase).second)
        ret.Add(path);
      else
        delete path;

      const size_t nBestFactor = StaticData::Instance().GetNBestFactor();
      if (nBestFactor > 0)
        contenders.Prune(count * nBestFactor);
    } else {
      ret.Add(path);
      contenders.Prune(count);
    }
  }
}

void ChartManager::CalcDecoderStatistics() const
{
}

void ChartManager::GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream) const
{
  size_t size = m_source.GetSize();

	// which hypotheses are reachable?
	std::map<int,bool> reachable;
	WordsRange fullRange(0, size-1);
	const ChartCell &lastCell = m_hypoStackColl.Get(fullRange);
  const ChartHypothesis *hypo = lastCell.GetBestHypothesis();

  if (hypo == NULL) {
    // no hypothesis
    return;
  }
	FindReachableHypotheses( hypo, reachable);

  for (size_t width = 1; width <= size; ++width) {
    for (size_t startPos = 0; startPos <= size-width; ++startPos) {
      size_t endPos = startPos + width - 1;
      WordsRange range(startPos, endPos);
      TRACE_ERR(" " << range << "=");

      const ChartCell &cell = m_hypoStackColl.Get(range);
      cell.GetSearchGraph(translationId, outputSearchGraphStream, reachable);
    }
  }
}

void ChartManager::FindReachableHypotheses( const ChartHypothesis *hypo, std::map<int,bool> &reachable ) const
{
	// do not recurse, if already visited
	if (reachable.find(hypo->GetId()) != reachable.end())
	{
		return;
	}

	// recurse
	reachable[ hypo->GetId() ] = true;
	const std::vector<const ChartHypothesis*> &previous = hypo->GetPrevHypos();
	for(std::vector<const ChartHypothesis*>::const_iterator i = previous.begin(); i != previous.end(); ++i)
	{
		FindReachableHypotheses( *i, reachable );
	}	

	// also loop over recombined hypotheses (arcs)
	const ChartArcList *arcList = hypo->GetArcList();
	if (arcList) {
		ChartArcList::const_iterator iterArc;
		for (iterArc = arcList->begin(); iterArc != arcList->end(); ++iterArc) {
			const ChartHypothesis &arc = **iterArc;
			FindReachableHypotheses( &arc, reachable );
		}
	}
}

} // namespace

