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
#include "ChartTrellisDetourQueue.h"
#include "ChartTrellisNode.h"
#include "ChartTrellisPath.h"
#include "ChartTrellisPathList.h"
#include "StaticData.h"
#include "DecodeStep.h"
#include "TreeInput.h"
#include "DummyScoreProducers.h"

using namespace std;
using namespace Moses;

namespace Moses
{
extern bool g_debug;

/* constructor. Initialize everything prior to decoding a particular sentence.
 * \param source the sentence to be decoded
 * \param system which particular set of models to use.
 */
ChartManager::ChartManager(InputType const& source, const TranslationSystem* system)
  :m_source(source)
  ,m_hypoStackColl(source, *this)
  ,m_system(system)
  ,m_start(clock())
  ,m_hypothesisId(0)
  ,m_translationOptionList(StaticData::Instance().GetRuleLimit())
  ,m_decodeGraphList(system->GetDecodeGraphs())

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
  m_system->CleanUpAfterSentenceProcessing(m_source);

  RemoveAllInColl(m_ruleLookupManagers);

  RemoveAllInColl(m_unksrcs);
  RemoveAllInColl(m_cacheTargetPhraseCollection);

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
  for (size_t width = 1; width <= size; ++width) {
    for (size_t startPos = 0; startPos <= size-width; ++startPos) {
      size_t endPos = startPos + width - 1;
      WordsRange range(startPos, endPos);

      // create trans opt
      CreateTranslationOptionsForRange(range);

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
void ChartManager::AddXmlChartOptions() {
  const std::vector <ChartTranslationOptions*> xmlChartOptionsList = m_source.GetXmlChartTranslationOptions();
  IFVERBOSE(2) { cerr << "AddXmlChartOptions " << xmlChartOptionsList.size() << endl; }
  if (xmlChartOptionsList.size() == 0) return;

  for(std::vector<ChartTranslationOptions*>::const_iterator i = xmlChartOptionsList.begin();
      i != xmlChartOptionsList.end(); ++i) {
    ChartTranslationOptions* opt = *i;

    Moses::Scores wordPenaltyScore(1, -0.434294482); // TODO what is this number?
    opt->GetTargetPhraseCollection().GetCollection()[0]->SetScore((ScoreProducer*)m_system->GetWordPenaltyProducer(), wordPenaltyScore);

    const WordsRange &range = opt->GetSourceWordsRange();
    RuleCubeItem* item = new RuleCubeItem( *opt, m_hypoStackColl );
    ChartHypothesis* hypo = new ChartHypothesis(*opt, *item, *this);
    hypo->CalcScore();
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
   * \param count how may paths to return
   * \param ret return argument
   * \param onlyDistinct whether to check for distinct output sentence or not (default - don't check, just return top n-paths)
   */
void ChartManager::CalcNBest(size_t count, ChartTrellisPathList &ret,bool onlyDistinct) const
{
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

void ChartManager::CreateDeviantPaths(
    boost::shared_ptr<const ChartTrellisPath> basePath,
    ChartTrellisDetourQueue &q)
{
  CreateDeviantPaths(basePath, basePath->GetFinalNode(), q);
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

void ChartManager::CreateTranslationOptionsForRange(const WordsRange &wordsRange)
{
  assert(m_decodeGraphList.size() == m_ruleLookupManagers.size());
  
  m_translationOptionList.Clear();
  
  std::vector <DecodeGraph*>::const_iterator iterDecodeGraph;
  std::vector <ChartRuleLookupManager*>::const_iterator iterRuleLookupManagers = m_ruleLookupManagers.begin();
  for (iterDecodeGraph = m_decodeGraphList.begin(); iterDecodeGraph != m_decodeGraphList.end(); ++iterDecodeGraph, ++iterRuleLookupManagers) {
    const DecodeGraph &decodeGraph = **iterDecodeGraph;
    assert(decodeGraph.GetSize() == 1);
    ChartRuleLookupManager &ruleLookupManager = **iterRuleLookupManagers;
    size_t maxSpan = decodeGraph.GetMaxChartSpan();
    if (maxSpan == 0 || wordsRange.GetNumWordsCovered() <= maxSpan) {
      ruleLookupManager.GetChartRuleCollection(wordsRange, m_translationOptionList);
    }
  }
  
  if (wordsRange.GetNumWordsCovered() == 1 && wordsRange.GetStartPos() != 0 && wordsRange.GetStartPos() != m_source.GetSize()-1) {
    bool alwaysCreateDirectTranslationOption = StaticData::Instance().IsAlwaysCreateDirectTranslationOption();
    if (m_translationOptionList.GetSize() == 0 || alwaysCreateDirectTranslationOption) {
      // create unknown words for 1 word coverage where we don't have any trans options
      const Word &sourceWord = m_source.GetWord(wordsRange.GetStartPos());
      ProcessOneUnknownWord(sourceWord, wordsRange);
    }
  }
  
  m_translationOptionList.ApplyThreshold();
}
  
//! special handling of ONE unknown words.
void ChartManager::ProcessOneUnknownWord(const Word &sourceWord, const WordsRange &range)
{
  // unknown word, add as trans opt
  const StaticData &staticData = StaticData::Instance();
  const UnknownWordPenaltyProducer *unknownWordPenaltyProducer = m_system->GetUnknownWordPenaltyProducer();
  vector<float> wordPenaltyScore(1, -0.434294482); // TODO what is this number?
  
  const ChartCell &chartCell = m_hypoStackColl.Get(range);
  const ChartCellLabel &sourceWordLabel = chartCell.GetSourceWordLabel();
  
  size_t isDigit = 0;
  if (staticData.GetDropUnknown()) {
    const Factor *f = sourceWord[0]; // TODO hack. shouldn't know which factor is surface
    const string &s = f->GetString();
    isDigit = s.find_first_of("0123456789");
    if (isDigit == string::npos)
      isDigit = 0;
    else
      isDigit = 1;
    // modify the starting bitmap
  }
  
  Phrase* m_unksrc = new Phrase(1);
  m_unksrc->AddWord() = sourceWord;
  m_unksrcs.push_back(m_unksrc);
  
  //TranslationOption *transOpt;
  if (! staticData.GetDropUnknown() || isDigit) {
    // loop
    const UnknownLHSList &lhsList = staticData.GetUnknownLHS();
    UnknownLHSList::const_iterator iterLHS;
    for (iterLHS = lhsList.begin(); iterLHS != lhsList.end(); ++iterLHS) {
      const string &targetLHSStr = iterLHS->first;
      float prob = iterLHS->second;
      
      // lhs
      //const Word &sourceLHS = staticData.GetInputDefaultNonTerminal();
      Word targetLHS(true);
      
      targetLHS.CreateFromString(Output, staticData.GetOutputFactorOrder(), targetLHSStr, true);
      CHECK(targetLHS.GetFactor(0) != NULL);
      
      // add to dictionary
      TargetPhrase *targetPhrase = new TargetPhrase(Output);
      TargetPhraseCollection *tpc = new TargetPhraseCollection;
      tpc->Add(targetPhrase);
      
      m_cacheTargetPhraseCollection.push_back(tpc);
      Word &targetWord = targetPhrase->AddWord();
      targetWord.CreateUnknownWord(sourceWord);
      
      // scores
      vector<float> unknownScore(1, FloorScore(TransformScore(prob)));
      
      //targetPhrase->SetScore();
      targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);
      targetPhrase->SetScore(m_system->GetWordPenaltyProducer(), wordPenaltyScore);
      targetPhrase->SetSourcePhrase(m_unksrc);
      targetPhrase->SetTargetLHS(targetLHS);
      
      // chart rule
      m_translationOptionList.Add(*tpc, m_emptyStackVec, range);
    } // for (iterLHS
  } else {
    // drop source word. create blank trans opt
    vector<float> unknownScore(1, FloorScore(-numeric_limits<float>::infinity()));
    
    TargetPhrase *targetPhrase = new TargetPhrase(Output);
    TargetPhraseCollection *tpc = new TargetPhraseCollection;
    tpc->Add(targetPhrase);
    // loop
    const UnknownLHSList &lhsList = staticData.GetUnknownLHS();
    UnknownLHSList::const_iterator iterLHS;
    for (iterLHS = lhsList.begin(); iterLHS != lhsList.end(); ++iterLHS) {
      const string &targetLHSStr = iterLHS->first;
      //float prob = iterLHS->second;
      
      Word targetLHS(true);
      targetLHS.CreateFromString(Output, staticData.GetOutputFactorOrder(), targetLHSStr, true);
      CHECK(targetLHS.GetFactor(0) != NULL);
      
      m_cacheTargetPhraseCollection.push_back(tpc);
      targetPhrase->SetSourcePhrase(m_unksrc);
      targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);
      targetPhrase->SetTargetLHS(targetLHS);
      
      // chart rule
      m_translationOptionList.Add(*tpc, m_emptyStackVec, range);
    }
  }
}
  
} // namespace Moses
