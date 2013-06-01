#pragma once

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "ChartHypothesis.h"
#include "ChartTranslationOptions.h"
#include "TargetPhraseMBOT.h"
#include "ProcessedNonTerminals.h"

namespace Moses
{

class ChartManager;
class RuleCubeItemMBOT;
class ChartTranslationOptionMBOT;
class ChartHypothesisMBOT;
class ProcessedNonTerminals;

typedef std::vector<ChartHypothesisMBOT*> ChartArcListMBOT;

class ChartHypothesisMBOT : public ChartHypothesis
{

  public:

  friend std::ostream& operator<<(std::ostream&, const ChartHypothesisMBOT&);

protected:
#ifdef USE_HYPO_POOL
  static ObjectPool<ChartHypothesisMBOT> s_objectPool;
#endif

  //const ChartHypothesisMBOT *m_mbotWinningHypo;
  //std::vector<const ChartHypothesisMBOT*> m_mbotPrevHypos;
  //ChartArcListMBOT *m_mbotArcList; /*! all arcs that end at the same trellis point as this hypothesis */

public:

//Fabienne Braune : Needed a mutable l-MBOT target phrase here. Maybe it would be good to put a mutable target phrase into base class.
TargetPhraseMBOT *m_mbotTargetPhrase;

#ifdef USE_HYPO_POOL
  void *operator new(size_t /* num_bytes */) {
    void *ptr = s_objectPool.getPtr();
    return ptr;
  }

  static void Delete(ChartHypothesisMBOT *hypo) {
    s_objectPool.freeObject(hypo);
  }
#else
  static void Delete(ChartHypothesisMBOT *hypo) {
    delete hypo;
  }
#endif

  ChartHypothesisMBOT(const ChartTranslationOptions &, const RuleCubeItemMBOT &item,
                  ChartManager &manager);

  ~ChartHypothesisMBOT();

  const TargetPhraseMBOT * GetCurrTargetPhraseMBOT()const {
    return m_mbotTargetPhrase;
  }

  const PhraseSequence * GetCurrentTargetSequence() const {
	return  m_mbotTargetPhrase->GetMBOTPhrases();
  }

  size_t GetSizeOfTargetSequence() const {
	  return  m_mbotTargetPhrase->GetMBOTPhrases()->GetSize();
  }

  //just in case we try to call target phrase
  const TargetPhrase &GetCurrTargetPhrase()const {
    std::cout << "Get current Target Phrase NOT implemented in chart hypothesis MBOT" << std::endl;
  }

  //virtual inline const ChartArcListMBOT* GetArcListMBOT() const {
    //return m_mbotArcList;
  //}

   //just in case we try to call target phrase
  //virtual inline const ChartArcList* GetArcList() const {
    //std::cout << "Get current Arc List NOT implemented in chart hypothesis MBOT" << std::endl;
  //}

  //just in case we try to call target phrase
  const Word &GetTargetLHS() const {
    std::cout << "Get non mbot target LHS NOT IMPLRMENTED in chart hypothesis MBOT" << std::endl;
  }

  const WordSequence GetTargetLHSMBOT() const {
    return GetCurrTargetPhraseMBOT()->GetTargetLHSMBOT();
  }

  void CreateOutputPhrase(Phrase &outPhrase, ProcessedNonTerminals &processedNonTerms) const;

  Phrase GetOutputPhrase() const;

  int RecombineCompare(const ChartHypothesisMBOT &compare) const;

  //Fabienne Braune : Overriding CalcScore causes a memory leak. Don't know why.
  void CalcScoreMBOT();

  void AddArc(ChartHypothesisMBOT *loserHypo);

  void SetWinningHypo(const ChartHypothesisMBOT *hypo);

  const ScoreComponentCollection &GetScoreBreakdown() const {
    return m_scoreBreakdown;
  }

  //const std::vector<const ChartHypothesisMBOT*> &GetPrevHyposMBOT() const {
  //  return m_mbotPrevHypos;
  //}

  //! forbid access to non mbot previous hypos
  // const std::vector<const ChartHypothesis*> &GetPrevHypos() const {
  //  std::cout << "Get previous hypo NOT implemented in chart hypothesis MBOT" << std::endl;
  // }

  //	const ChartHypothesisMBOT* GetPrevHypoMBOT(size_t pos) const {
 //		return m_mbotPrevHypos[pos];
//	}

     //! forbid access to non mbot previous hypos
	//const ChartHypothesis* GetPrevHypo(size_t pos) const {
     // std::cout << "Get previous hypo (with pos) NOT implemented in chart hypothesis MBOT"<< std::endl;
	// }

	//const ChartHypothesisMBOT* GetWinningHypothesisMBOT() const {
	//	return m_mbotWinningHypo;
	//}

    //! forbid access to non mbot winning hypo
    //const ChartHypothesis* GetWinningHypothesis() const {
	//	std::cout << "Get winning hypo NOT implemented in chart hypothesis MBOT"<< std::endl;
	//}
  TO_STRING();

}; // class ChartHypothesis

}

