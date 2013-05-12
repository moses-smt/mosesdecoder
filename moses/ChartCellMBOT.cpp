//Fabienne Braune : Chart Cells for l-MBOT rules

#include <algorithm>
#include "ChartCellMBOT.h"
#include "ChartCellCollection.h"
#include "RuleCubeMBOT.h"
#include "WordsRange.h"
#include "Util.h"
#include "StaticData.h"
#include "ChartTranslationOptionList.h"
#include "ChartManager.h"

using namespace std;

namespace Moses
{
extern bool g_debug;

ChartCellBaseMBOT::ChartCellBaseMBOT(size_t startPos, size_t endPos)
:
  m_mbotCoverage(startPos, endPos),
  m_mbotTargetLabelSet(m_mbotCoverage.front()),
  m_mbotSourceWordLabel(NULL){}

ChartCellBaseMBOT::~ChartCellBaseMBOT()
{
	 delete m_mbotSourceWordLabel;
}

ChartCellMBOT::ChartCellMBOT(size_t startPos, size_t endPos, ChartManager &manager):
ChartCell(startPos,endPos,manager)
,ChartCellBaseMBOT(startPos, endPos)
, m_manager(manager)
{
	const StaticData &staticData = StaticData::Instance();
	m_nBestIsEnabled = staticData.IsNBestEnabled();
}

/** Add the given hypothesis to the cell */
bool ChartCellMBOT::AddHypothesis(ChartHypothesisMBOT *hypo)
{
  const std::vector<Word> &targetLHS = hypo->GetTargetLHSMBOT();
  CHECK(targetLHS.size() != 0);

  bool ret = m_mbotHypoColl[targetLHS].AddHypothesis(hypo, m_manager);
  return ret;
}

/** Pruning */
void ChartCellMBOT::PruneToSize()
{
  std::map<std::vector<Word>, ChartHypothesisCollectionMBOT>::iterator iter;
  for (iter = m_mbotHypoColl.begin(); iter != m_mbotHypoColl.end(); ++iter) {
    ChartHypothesisCollectionMBOT &coll = iter->second;
    coll.PruneToSize(m_manager);
  }
}


/** Decoding at span level: fill chart cell with mbot hypotheses
 *  (implementation of cube pruning)
 * \param transOptList list of applicable rules to create hypotheses for the cell
 * \param allChartCells entire chart - needed to look up underlying hypotheses
 */
void ChartCellMBOT::ProcessSentence(const ChartTranslationOptionList &transOptList
                                , const ChartCellCollectionMBOT &allChartCells)
{

  const StaticData &staticData = StaticData::Instance();

  RuleCubeQueue queue(m_manager);

  ChartTranslationOptionList::const_iterator iterList;
  for (iterList = transOptList.begin(); iterList != transOptList.end(); ++iterList)
  {
    RuleCubeMBOT *ruleCube = new RuleCubeMBOT(*iterList, allChartCells, m_manager);
    queue.Add(ruleCube);
  }
  const size_t popLimit = staticData.GetCubePruningPopLimit();
  for (size_t numPops = 0; numPops < popLimit && !queue.IsEmpty(); ++numPops)
  {
    ChartHypothesisMBOT *hypo = queue.Pop();
    AddHypothesis(hypo);
  }
}


void ChartCellMBOT::ProcessSentenceWithSourceLabels(const ChartTranslationOptionList &transOptList
                        ,const ChartCellCollection &allChartCells, const InputType &source, size_t startPos, size_t endPos)
{

  const StaticData &staticData = StaticData::Instance();

  RuleCubeQueue queue(m_manager);

  // add all trans opt into queue. using only 1st child node.
  ChartTranslationOptionList::const_iterator iterList;
  for (iterList = transOptList.begin(); iterList != transOptList.end(); ++iterList)
   {

       std::vector<Word> cellSourceLabels;

       NonTerminalSet::const_iterator iter;
       for(iter = source.GetLabelSet(startPos,endPos).begin();iter != source.GetLabelSet(startPos,endPos).end();iter++)
       {
           const Word &word = *iter;
           cellSourceLabels.push_back(*iter);

        }

       bool isMatch = false;
       std::vector<Word>::iterator itrLab;

       vector<TargetPhrase*> :: const_iterator itr_coll;
       for(itrLab = cellSourceLabels.begin(); itrLab != cellSourceLabels.end(); itrLab++)
       {
    	   for(itr_coll = iterList->GetTargetPhraseCollection().GetCollection().begin(); itr_coll != iterList->GetTargetPhraseCollection().GetCollection().end(); itr_coll++)
    	    {
    	           TargetPhraseMBOT * mbotTp = static_cast<TargetPhraseMBOT*>(*itr_coll);
    	           //std::cerr << "CHECKED TARGET PHRASE ... : " << *mbotTp << std::endl;

    	           //indicate that the rule <s> X <\s> matches each source label
    	           if(mbotTp->GetMBOTPhrase(0)->GetWord(0).GetFactor(0)->GetString().compare("<s>") == 0)
    	           {
    	        	  //std::cerr << "WE ARE LOOKING FOR : " << mbotTp->GetMBOTPhrase(0)->GetWord(0).GetFactor(0)->GetString() << std::endl;
    	        	  //std::cerr << "WE COMPARE IT TO : " << "<s>" << std::endl;
    	              mbotTp->setMatchesSource(true);
    	           }

    	           std::vector<Word>::iterator itrLab;
    	           for(itrLab = cellSourceLabels.begin(); itrLab != cellSourceLabels.end(); itrLab++)
    	           {
    	        	   //std::cerr << "COMPARING : " << mbotTp->GetSourceLHS() << " with " << *itrLab << std::endl;
    	        	   //std::cerr << "WHAT IS THE VALUE OF THE OPTION : "<< StaticData::Instance().IsMatchingSourceAtRuleApplication() << std::endl;

    	        	   //check we want to match source labels for each chart cell
    	        	   if(StaticData::Instance().IsMatchingSourceAtRuleApplication() == 1)
    	        	   {

    	        		   if(mbotTp->GetSourceLHS() == *itrLab)
    	        		   {
    	        			   isMatch = true;
    	        			   //mark target phrases that match the source label
    	        			   //=> at hypothesis creation, we will only consider target phrase that match the source label
    	        			   mbotTp->setMatchesSource(true);
    	        			   //std::cerr << "LABEL TO MATCH : " << *itrLab << std::endl;
    	        			   //std::cerr << "TARGETPHRASE MARKED..."<< *mbotTp << std::endl;
    	        		   }
    	        	   }
    	        	   else
    	        	   {
    	        		   	isMatch = true;
    	        		   	mbotTp->setMatchesSource(true);
    	        	   }
    	           }
    	    }
       }
       if(isMatch == true)
       {
    	   /*std::cerr << "------------------------------------------"<< std::endl;
    	   std::cerr << "CREATING CUBE FOR TRANSLATION OPTION : " << std::endl;
    	   std::cerr << *transOpt  << std::endl;
    	   std::cerr << "------------------------------------------"<< std::endl;*/
    	   RuleCubeMBOT *ruleCube = new RuleCubeMBOT(*iterList, allChartCells, m_manager);
    	   //std::cerr << "CUBE CREATED : " << std::endl;
    	   queue.Add(ruleCube);
       }
     }

  // pluck things out of queue and add to hypo collection
  const size_t popLimit = staticData.GetCubePruningPopLimit();
  //std::cout << "HYPOTHESES FOR TRANSLATION OPTION : " << std::endl;
  size_t numPops = 0;
  while(numPops < popLimit && !queue.IsEmptyMBOT())
  {
    ChartHypothesisMBOT *hypo = queue.PopMBOT();
    //if we find, on the translation dimension, a target phrase that does not
    //match the source label, then we return a NULL hypothesis
    if(hypo == NULL)
    {
    	//Do not count null hypos : increase pop limit for each such hypos
    	//std::cerr << "NULL HYPO : NUMBER OF POPS NOT INCREASED  " << numPops << std::endl;
    }
    else
    {
    	//otherwise, we add it to the chart cell
    	//std::cerr << "------------------------------------------"<< std::endl;
    	//std::cerr << "POPPING HYPOTHESIS FROM CUBE : " << std::endl;

    	//std::cerr << hypo->GetTranslationOptionMBOT() << std::endl;
    	//std::cerr << (*hypo) << std::endl;
    	//std::cerr << hypo->GetCurrTargetPhraseMBOT() << std::endl;
    	AddHypothesis(hypo);
    	numPops++;
       //std::cout << "Added Hypothesis..." << std::endl;
    }
  }
  //std::cout << "EXIT PROCESS SENTENCE"<< std::endl;
}

void ChartCellMBOT::SortHypotheses()
{
   //std::cout << "IN CHART CELL MBOT : SORTING HYPOS : " << std::endl;
  // sort each mini cells & fill up target lhs list
  CHECK(m_mbotTargetLabelSet.Empty());
  std::map< std::vector<Word>, ChartHypothesisCollectionMBOT>::iterator iter;
  for (iter = m_mbotHypoColl.begin(); iter != m_mbotHypoColl.end(); ++iter) {
    ChartHypothesisCollectionMBOT &coll = iter->second;
    //std::cout << "CONSIDERED COLLECTION : " << coll << std::endl;
    //std::cout << "ADDING CONSTITUENT TO TARGET LABEL SET : " << std::endl;
    //std::cout << "SIZE OF VECTOR " << (iter->first).size() << std::endl;
    //std::vector<Word>::iterator itr_words;
    //std::vector<Word> target = iter->first;
    //for(itr_words = target.begin(); itr_words != target.end(); itr_words++)
    //{
      //  std::cout << "CONSTITUENT TO ADD : " << *itr_words << std::endl;
    //}
    //std::cout << "SIZE OF VECTOR " << (iter->first).size() << std::endl;
    m_mbotTargetLabelSet.AddConstituent(iter->first, coll);
    //std::cout << "SORTING HYPOTHESES : " << coll << std::endl;
    coll.SortHypotheses();
  }
}

/** Return the highest scoring hypothesis in the cell */
const ChartHypothesisMBOT *ChartCellMBOT::GetBestHypothesis() const
{

  const ChartHypothesisMBOT *ret = NULL;
  float bestScore = -std::numeric_limits<float>::infinity();

  std::map<std::vector<Word>, ChartHypothesisCollection>::const_iterator iter;
  for (iter = m_mbotHypoColl.begin(); iter != m_mbotHypoColl.end(); ++iter) {
    const HypoList &sortedList = iter->second.GetSortedHypotheses();
    CHECK(sortedList.size() > 0);

    const ChartHypothesisMBOT *hypo = sortedList[0];
    if (hypo->GetTotalScore() > bestScore) {
      bestScore = hypo->GetTotalScore();
      ret = hypo;
    };
  }
  return ret;
}

void ChartCellMBOT::CleanupArcList()
{
  //std::cout << "Cleaning Up Arc List MBOT" << std::endl;
  // only necessary if n-best calculations are enabled
  if (!m_nBestIsEnabled) return;

  std::map<std::vector<Word>, ChartHypothesisCollection>::iterator iter;
  for (iter = m_mbotHypoColl.begin(); iter != m_mbotHypoColl.end(); ++iter) {
        ChartHypothesisCollection &coll = iter->second;
        coll.CleanupArcList();
  }
}

void ChartCellMBOT::OutputSizes(std::ostream &out) const
{
  std::map<std::vector<Word>, ChartHypothesisCollection>::const_iterator iter;
  for (iter = m_mbotHypoColl.begin(); iter != m_mbotHypoColl.end(); ++iter) {
    const std::vector<Word> &targetLHS = iter->first;
    const ChartHypothesisCollection &coll = iter->second;

    //iterate over words of rhs and given size of hypothesis collection
    std::vector<Word> :: const_iterator itr_target_lhs;
    int counter = 1;
    for(itr_target_lhs = targetLHS.begin(); itr_target_lhs != targetLHS.end(); itr_target_lhs++)
    {
            Word oneTargetLHS = *itr_target_lhs;
            out << oneTargetLHS << "(" << counter << ")" << " ";
            counter++;
    }
    out << "=" << coll.GetSize() << " ";
  }
}

size_t ChartCellMBOT::GetSize() const
{
  size_t ret = 0;
  std::map<std::vector<Word>, ChartHypothesisCollectionMBOT>::const_iterator iter;
  for (iter = m_mbotHypoColl.begin(); iter != m_mbotHypoColl.end(); ++iter) {
    const ChartHypothesisCollectionMBOT &coll = iter->second;
    ret += coll.GetSizeMBOT();
  }

  return ret;
}

void ChartCellMBOT::GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream, const std::map<unsigned, bool> &reachable) const
{
  std::map<std::vector<Word>, ChartHypothesisCollectionMBOT>:: const_iterator iterOutside;
  for (iterOutside = m_mbotHypoColl.begin(); iterOutside != m_mbotHypoColl.end(); ++iterOutside) {
    const ChartHypothesisCollectionMBOT &coll = iterOutside->second;
    coll.GetSearchGraph(translationId, outputSearchGraphStream, reachable);
  }
}

std::ostream& operator<<(std::ostream &out, const ChartCellMBOT &cell)
{

  std::cout << "------------------------------------"<< std::endl;
  std::cout << "THIS IS AN MBOT CHART CELL" << std::endl;
  std::cout << "------------------------------------"<< std::endl;

  std::map<std::vector<Word>, ChartHypothesisCollectionMBOT>::const_iterator iterOutside;
  for (iterOutside = cell.m_mbotHypoColl.begin(); iterOutside != cell.m_mbotHypoColl.end(); ++iterOutside) {
    const std::vector<Word> &targetLHS = iterOutside->first;
    std::vector<Word> :: const_iterator itr_words;
    int counter = 1;

    for(itr_words = targetLHS.begin(); itr_words != targetLHS.end(); itr_words++)
    {
        Word oneTargetLHS = *itr_words;
        cerr << oneTargetLHS << "(" << counter << ")" << " :" ;
    }

    out << endl;

    const ChartHypothesisCollectionMBOT &coll = iterOutside->second;
    //new : inserted for testing
    std::cout << "Displaying chart hypothesis collection" << std::endl;
    cerr << coll;
  }

  return out;
}

} // namespace
