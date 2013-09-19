// $Id: ChartManager.cpp,v 1.1.1.1 2013/01/06 16:54:16 braunefe Exp $
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
#include "ChartTrellisDetourQueue.h"
#include "ChartTrellisNode.h"
#include "ChartTrellisPath.h"
#include "ChartTrellisPathList.h"
#include "StaticData.h"
#include "DecodeStep.h"
#include <boost/shared_ptr.hpp>

//MBOT objects for computation of nbest list
#include "ChartTreillisDetourQueueMBOT.h"
#include "ChartTreillisNodeMBOT.h"
#include "ChartTreillisPathMBOT.h"
#include "ChartTreillisPathListMBOT.h"

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
  ,m_hypothesisId(0)
{
  m_system->InitializeBeforeSentenceProcessing(source);
  const std::vector<PhraseDictionaryFeature*> &dictionaries = m_system->GetPhraseDictionaries();
  m_ruleLookupManagers.reserve(dictionaries.size());
  for (std::vector<PhraseDictionaryFeature*>::const_iterator p = dictionaries.begin();
       p != dictionaries.end(); ++p) {
    PhraseDictionaryFeature *pdf = *p;
    const PhraseDictionary *dict = pdf->GetDictionary();
    PhraseDictionary *nonConstDict = const_cast<PhraseDictionary*>(dict);

    //std::cout << "CREATING RULE LOOKUP MANAGER" << std::endl;
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

  //std::cout << "MANAGER : PROCESSING SENTENCE" << std::endl;
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
      //new : inserted for testing
      //std::cerr << "MANAGER : CREATING CHART TRANSLATION OPTIONS : " << "S" << startPos << "E" << endPos << endl;
      //std::cout << "Translation Options before Creation: "<< m_transOptColl << endl;
      m_transOptColl.CreateTranslationOptionsForRange(startPos, endPos);
      //if (g_debug)
      //std::cout << m_transOptColl.GetTranslationOptionList(WordsRange(startPos, endPos)) << std::endl;
      //std::cout << "S" << startPos << "E" << endPos << "TRANSLATION OPTIONS : "<< m_transOptColl << endl;
      // decode
      //std::cout << "Creating Chart Cell"<< std::endl;

      ChartCell &cell = m_hypoStackColl.Get(range);

      //std::cout << "Processing Sentence"<< std::endl;
      cell.ProcessSentenceWithSourceLabels(m_transOptColl.GetTranslationOptionList(range)
                           ,m_hypoStackColl,m_source,startPos,endPos);

      //cell.ProcessSentence(m_transOptColl.GetTranslationOptionList(range)
      //                          ,m_hypoStackColl);

      //std::cout << "Pruning"<< std::endl;
      cell.PruneToSize();

      //std::cout << "Cleaning up arc list"<< std::endl;
      cell.CleanupArcList();

       //std::cout << "Sorting Hypotheses"<< std::endl;
       cell.SortHypotheses();

       //std::cout << "End" << std::endl;

      //cerr << cell.GetSize();
      //cerr << cell << endl;
      //cell.OutputSizes(cerr);
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
        //BEWARE : TAKES MBOT SIZE INSTEAD OF SCFG
        cerr << m_hypoStackColl.GetMBOT(range).GetSize() << " ";
      }
      cerr << endl;
    }
  }
}

const ChartHypothesis *ChartManager::GetBestHypothesis() const
{
  //std::cout << "CHART MANAGER : GETTING BEST HYPOTHESIS" << std::endl;
  size_t size = m_source.GetSize();

  if (size == 0) // empty source
    return NULL;
  else {
    WordsRange range(0, size-1);
    const ChartCell &lastCell = m_hypoStackColl.Get(range);
    return lastCell.GetBestHypothesis();
  }
}

//BEWARE : Added for getting best MBOT hypothesis
const ChartHypothesisMBOT *ChartManager::GetBestHypothesisMBOT() const
{
  //std::cout << "CHART MANAGER : GETTING BEST HYPOTHESIS" << std::endl;
  size_t size = m_source.GetSize();

  if (size == 0) // empty source
    return NULL;
  else {
    WordsRange range(0, size-1);
    //BEWARE : TAKES MBOT CELL INSTEAD OF SCFG
    const ChartCellMBOT &lastCell = m_hypoStackColl.GetMBOT(range);
    return lastCell.GetBestHypothesis();
  }
}


void ChartManager::CalcNBest(size_t count, ChartTrellisPathList &ret,bool onlyDistinct) const
{

  //std::cout << "CHART MANAGER : COMPUTING N-BEST" << std::endl;

  size_t size = m_source.GetSize();
  if (count == 0 || size == 0)
    return;

  // Build a ChartTrellisPath for the 1-best path, if any.
  WordsRange range(0, size-1);
  const ChartCell &lastCell = m_hypoStackColl.Get(range);
  const ChartHypothesis *hypo = lastCell.GetBestHypothesis();
  if (hypo == NULL) {
    // no hypothesis
    return;
  }
  boost::shared_ptr<ChartTrellisPath> basePath(new ChartTrellisPath(*hypo));

  // Add it to the n-best list.
  ret.Add(basePath);
  if (count == 1) {
    return;
  }

  // Record the output phrase if distinct translations are required.
  set<Phrase> distinctHyps;
  if (onlyDistinct) {
    distinctHyps.insert(basePath->GetOutputPhrase());
  }

  // Set a limit on the number of detours to pop.  If the n-best list is
  // restricted to distinct translations then this limit should be bigger
  // than n.  The n-best factor determines how much bigger the limit should be.
  const size_t nBestFactor = StaticData::Instance().GetNBestFactor();
  size_t popLimit;
  if (!onlyDistinct) {
    popLimit = count-1;
  } else if (nBestFactor == 0) {
    // 0 = 'unlimited.'  This actually sets a large-ish limit in case too many
    // translations are identical.
    popLimit = count * 1000;
  } else {
    popLimit = count * nBestFactor;
  }

  // Create an empty priority queue of detour objects.  It is bounded to
  // contain no more than popLimit items.
  ChartTrellisDetourQueue contenders(popLimit);

  // Create a ChartTrellisDetour for each single-point deviation from basePath
  // and add them to the queue.
  CreateDeviantPaths(basePath, contenders);

  // MAIN loop
  for (size_t i = 0;
       ret.GetSize() < count && !contenders.Empty() && i < popLimit;
       ++i) {
    // Get the best detour from the queue.
    std::auto_ptr<const ChartTrellisDetour> detour(contenders.Pop());
    CHECK(detour.get());

    // Create a full base path from the chosen detour.
    basePath.reset(new ChartTrellisPath(*detour));

    // Generate new detours from this base path and add them to the queue of
    // contenders.  The new detours deviate from the base path by a single
    // replacement along the previous detour sub-path.
    CHECK(basePath->GetDeviationPoint());
    CreateDeviantPaths(basePath, *(basePath->GetDeviationPoint()), contenders);

    // If the n-best list is allowed to contain duplicate translations (at the
    // surface level) then add the new path unconditionally, otherwise check
    // whether the translation has seen before.
    if (!onlyDistinct) {
      ret.Add(basePath);
    } else {
      Phrase tgtPhrase = basePath->GetOutputPhrase();
      if (distinctHyps.insert(tgtPhrase).second) {
        ret.Add(basePath);
      }
    }
  }
}

//BEWARE : Added for getting best MBOT hypothesis
void ChartManager::CalcNBestMBOT(size_t count, ChartTreillisPathListMBOT &ret, ProcessedNonTerminals * nt, bool onlyDistinct) const
{

  //std::cout << "CHART MANAGER : COMPUTING N-BEST for MBOT" << std::endl;

  size_t size = m_source.GetSize();
  if (count == 0 || size == 0)
    return;

  // Build a ChartTrellisPath for the 1-best path, if any.
  WordsRange range(0, size-1);
  const ChartCellMBOT &lastCell = m_hypoStackColl.GetMBOT(range);
  const ChartHypothesisMBOT *hypo = lastCell.GetBestHypothesis();
  if (hypo == NULL) {
    // no hypothesis
    return;
  }
  boost::shared_ptr<ChartTreillisPathMBOT> basePath(new ChartTreillisPathMBOT(*hypo));

  // Add it to the n-best list.
  ret.Add(basePath);
  if (count == 1) {
    return;
  }

  set<Phrase> distinctHyps;
  if (onlyDistinct) {
    // Record the output phrase if distinct translations are required.
    distinctHyps.insert(basePath->GetOutputPhraseMBOT(nt));
  }

  // Set a limit on the number of detours to pop.  If the n-best list is
  // restricted to distinct translations then this limit should be bigger
  // than n.  The n-best factor determines how much bigger the limit should be.
  const size_t nBestFactor = StaticData::Instance().GetNBestFactor();
  size_t popLimit;
  if (!onlyDistinct) {
    popLimit = count-1;
  } else if (nBestFactor == 0) {
    // 0 = 'unlimited.'  This actually sets a large-ish limit in case too many
    // translations are identical.
    popLimit = count * 1000;
  } else {
    popLimit = count * nBestFactor;
  }

  // Create an empty priority queue of detour objects.  It is bounded to
  // contain no more than popLimit items.
  ChartTreillisDetourQueueMBOT contenders(popLimit);

  // Create a ChartTrellisDetour for each single-point deviation from basePath
  // and add them to the queue.
  CreateDeviantPathsMBOT(basePath, contenders);

  // MAIN loop
  for (size_t i = 0;
       ret.GetSize() < count && !contenders.Empty() && i < popLimit;
       ++i) {
    // Get the best detour from the queue.
    std::auto_ptr<const ChartTreillisDetourMBOT> detour(contenders.Pop());
    CHECK(detour.get());

    // Create a full base path from the chosen detour.
    basePath.reset(new ChartTreillisPathMBOT(*detour));

    // Generate new detours from this base path and add them to the queue of
    // contenders.  The new detours deviate from the base path by a single
    // replacement along the previous detour sub-path.
    CHECK(basePath->GetDeviationPointMBOT());
    CreateDeviantPathsMBOT(basePath, *(basePath->GetDeviationPointMBOT()), contenders);

    // If the n-best list is allowed to contain duplicate translations (at the
    // surface level) then add the new path unconditionally, otherwise check
    // whether the translation has seen before.
    if (!onlyDistinct) {
      ret.Add(basePath);
    } else {
      //nt->Reset();
      Phrase tgtPhrase = basePath->GetOutputPhraseMBOT(nt);
      nt->Reset();
      if (distinctHyps.insert(tgtPhrase).second) {
        ret.Add(basePath);
      }
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
	std::map<unsigned,bool> reachable;
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

void ChartManager::GetSearchGraphMBOT(long translationId, std::ostream &outputSearchGraphStream) const
{
  size_t size = m_source.GetSize();

	// which hypotheses are reachable?
	std::map<unsigned,bool> reachable;
	WordsRange fullRange(0, size-1);
	const ChartCellMBOT &lastCell = m_hypoStackColl.GetMBOT(fullRange);
  const ChartHypothesisMBOT *hypo = lastCell.GetBestHypothesis();

  if (hypo == NULL) {
    // no hypothesis
    return;
  }
	FindReachableHypothesesMBOT( hypo, reachable);

  for (size_t width = 1; width <= size; ++width) {
    for (size_t startPos = 0; startPos <= size-width; ++startPos) {
      size_t endPos = startPos + width - 1;
      WordsRange range(startPos, endPos);
      TRACE_ERR(" " << range << "=");

      const ChartCellMBOT &cell = m_hypoStackColl.GetMBOT(range);
      //std::cout << "Beware could be wrong here..." << std::endl;
      cell.GetSearchGraph(translationId, outputSearchGraphStream, reachable);
    }
  }
}

void ChartManager::FindReachableHypotheses( const ChartHypothesis *hypo, std::map<unsigned,bool> &reachable ) const
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

//BEWARE : Added for getting reachable MBOT hypothesis
void ChartManager::FindReachableHypothesesMBOT( const ChartHypothesisMBOT *hypo, std::map<unsigned,bool> &reachable ) const
{
	// do not recurse, if already visited
	if (reachable.find(hypo->GetId()) != reachable.end())
	{
		return;
	}

	// recurse
	reachable[ hypo->GetId() ] = true;
	const std::vector<const ChartHypothesisMBOT*> &previous = hypo->GetPrevHyposMBOT();
	for(std::vector<const ChartHypothesisMBOT*>::const_iterator i = previous.begin(); i != previous.end(); ++i)
	{
		FindReachableHypothesesMBOT( *i, reachable );
	}

	// also loop over recombined hypotheses (arcs)
	const ChartArcListMBOT *arcList = hypo->GetArcListMBOT();
	if (arcList) {
		ChartArcListMBOT::const_iterator iterArc;
		for (iterArc = arcList->begin(); iterArc != arcList->end(); ++iterArc) {
			const ChartHypothesisMBOT &arc = **iterArc;
			FindReachableHypothesesMBOT( &arc, reachable );
		}
	}
}

void ChartManager::CreateDeviantPaths(
    boost::shared_ptr<const ChartTrellisPath> basePath,
    ChartTrellisDetourQueue &q)
{
  CreateDeviantPaths(basePath, basePath->GetFinalNode(), q);
}

void ChartManager::CreateDeviantPathsMBOT(
    boost::shared_ptr<const ChartTreillisPathMBOT> basePath,
    ChartTreillisDetourQueueMBOT &q)
{
  CreateDeviantPathsMBOT(basePath, basePath->GetFinalNodeMBOT(), q);
}

void ChartManager::CreateDeviantPaths(
    boost::shared_ptr<const ChartTrellisPath> basePath,
    const ChartTrellisNode &substitutedNode,
    ChartTrellisDetourQueue &queue)
{
  const ChartArcList *arcList = substitutedNode.GetHypothesis().GetArcList();
  if (arcList) {
    for (ChartArcList::const_iterator iter = arcList->begin();
         iter != arcList->end(); ++iter) {
      const ChartHypothesis &replacement = **iter;
      queue.Push(new ChartTrellisDetour(basePath, substitutedNode,
                                        replacement));
    }
  }
  // recusively create deviant paths for child nodes
  const ChartTrellisNode::NodeChildren &children = substitutedNode.GetChildren();
  ChartTrellisNode::NodeChildren::const_iterator iter;
  for (iter = children.begin(); iter != children.end(); ++iter) {
    const ChartTrellisNode &child = **iter;
    CreateDeviantPaths(basePath, child, queue);
  }
}

void ChartManager::CreateDeviantPathsMBOT(
    boost::shared_ptr<const ChartTreillisPathMBOT> basePath,
    const ChartTreillisNodeMBOT &substitutedNode,
    ChartTreillisDetourQueueMBOT &queue)
{
  const ChartArcListMBOT *arcList = substitutedNode.GetHypothesisMBOT().GetArcListMBOT();
  if (arcList) {
    for (ChartArcListMBOT::const_iterator iter = arcList->begin();
         iter != arcList->end(); ++iter) {
      const ChartHypothesisMBOT &replacement = **iter;
      queue.Push(new ChartTreillisDetourMBOT(basePath, substitutedNode,
                                        replacement));
    }
  }
  // recusively create deviant paths for child nodes
  const ChartTreillisNodeMBOT::NodeChildrenMBOT &children = substitutedNode.GetChildrenMBOT();
  ChartTreillisNodeMBOT::NodeChildrenMBOT::const_iterator iter;
  for (iter = children.begin(); iter != children.end(); ++iter) {
    const ChartTreillisNodeMBOT &child = **iter;
    CreateDeviantPathsMBOT(basePath, child, queue);
  }
}


} // namespace Moses
