
#pragma once

#include <cstdlib>
#include <map>
#include <vector>

#include "Search.h"
#include "HypothesisStackNormal.h"
#include "TranslationOptionCollection.h"
#include "Timer.h"

#include "GibbsOperator.h"

namespace Josiah
{

  class Moses::InputType;
  class Moses::TranslationOptionCollection;
  
  /** Stack for instances of Hypothesis, includes functions for pruning. 
      Modified for random searching. Ignores hypothesis scores and uses random values.*/ 
  class HypothesisStackRandom: public Moses::HypothesisStack
  {
    public:
      friend std::ostream& operator<<(std::ostream&, const HypothesisStackRandom&);

    protected:
      float m_bestScore; /**< score of the best hypothesis in collection */
      float m_worstScore; /**< score of the worse hypothesis in collection */
      map<Moses::WordsBitmapID, float > m_diversityWorstScore; /**< score of worst hypothesis for particular source word coverage */
      float m_beamWidth; /**< minimum score due to threashold pruning */
      size_t m_maxHypoStackSize; /**< maximum number of hypothesis allowed in this stack */
      size_t m_minHypoStackDiversity; /**< minimum number of hypothesis with different source word coverage */
      bool m_nBestIsEnabled; /**< flag to determine whether to keep track of old arcs */
      map<const Hypothesis*, float> m_scores;

    /** add hypothesis to stack. Prune if necessary. 
      * Returns false if equiv hypo exists in collection, otherwise returns true
    */
      std::pair<HypothesisStackRandom::iterator, bool> Add(Moses::Hypothesis *hypothesis);

      /** destroy all instances of Hypothesis in this collection */
      void RemoveAll();

      void SetWorstScoreForBitmap( Moses::WordsBitmapID id, float worstScore ) {
        m_diversityWorstScore[ id ] = worstScore;
      }
      
      float GetTotalScore(const Hypothesis* hypo);
      float GetTotalScore(const Hypothesis* hypo) const;
      // sorting helper
      struct CompareHypothesisTotalScore
      {
        HypothesisStackRandom* m_parent;
        CompareHypothesisTotalScore(HypothesisStackRandom* parent) {m_parent = parent;}
        bool operator()(const Hypothesis* hypo1, const Hypothesis* hypo2) const
        {
          return m_parent->GetTotalScore(hypo1) > m_parent->GetTotalScore(hypo2);
        }
      };

    public:
      float GetWorstScoreForBitmap( WordsBitmapID id ) {
        if (m_diversityWorstScore.find( id ) == m_diversityWorstScore.end())
          return -numeric_limits<float>::infinity();
        return m_diversityWorstScore[ id ];
      }
      float GetWorstScoreForBitmap( const Moses::WordsBitmap &coverage ) {
        return GetWorstScoreForBitmap( coverage.GetID() );
      }

      HypothesisStackRandom();

      /** adds the hypo, but only if within thresholds (beamThr, stackSize).
      * This function will recombine hypotheses silently!  There is no record
      * (could affect n-best list generation...TODO)
      * Call stack for adding hypothesis is
      AddPrune()
      Add()
      AddNoPrune()
      */
      bool AddPrune(Moses::Hypothesis *hypothesis);

      /** set maximum number of hypotheses in the collection
      * \param maxHypoStackSize maximum number (typical number: 100)
      * \param maxHypoStackSize maximum number (defauly: 0)
      */
      inline void SetMaxHypoStackSize(size_t maxHypoStackSize, size_t minHypoStackDiversity)
      {
        m_maxHypoStackSize = maxHypoStackSize;
        m_minHypoStackDiversity = minHypoStackDiversity;
      }

      /** set beam threshold, hypotheses in the stack must not be worse than 
      * this factor times the best score to be allowed in the stack
      * \param beamThreshold minimum factor (typical number: 0.03)
      */
      inline void SetBeamWidth(float beamWidth)
      {
        m_beamWidth = beamWidth;
      }
      /** return score of the best hypothesis in the stack */
      inline float GetBestScore() const
      {
        return m_bestScore;
      }
      /** return worst allowable score */
      inline float GetWorstScore() const
      {
        return m_worstScore;
      }

    /** pruning, if too large.
      * Pruning algorithm: find a threshold and delete all hypothesis below it.
      * The threshold is chosen so that exactly newSize top items remain on the 
      * stack in fact, in situations where some of the hypothesis fell below 
      * m_beamWidth, the stack will contain less items.
      * \param newSize maximum size */
      void PruneToSize(size_t newSize);

      //! return the hypothesis with best score. Used to get the translated at end of decoding
      const Moses::Hypothesis *GetBestHypothesis() const;
    //! return all hypothesis, sorted by descending score. Used in creation of N best list
      std::vector<const Moses::Hypothesis*> GetSortedList() const;
      std::vector<Moses::Hypothesis*> GetSortedListNOTCONST();

      /** make all arcs in point to the equiv hypothesis that contains them. 
      * Ie update doubly linked list be hypo & arcs
      */
      void CleanupArcList();

      TO_STRING();
  };

  /**
  * Generate random hypotheses.
  **/
  class SearchRandom: public Moses::Search
{
  protected:
    const Moses::InputType &m_source;
    std::vector <Moses::HypothesisStack* > m_hypoStackColl; /**< stacks to store hypotheses (partial translations) */ 
  // no of elements = no of words in source + 1
    Moses::TargetPhrase m_initialTargetPhrase; /**< used to seed 1st hypo */
    HypothesisStackRandom* actual_hypoStack; /**actual (full expanded) stack of hypotheses*/ 
    const Moses::TranslationOptionCollection &m_transOptColl; /**< pre-computed list of translation options for the phrases in this sentence */

  // functions for creating hypotheses
    void ProcessOneHypothesis(const Moses::Hypothesis &hypothesis);
    void ExpandAllHypotheses(const Moses::Hypothesis &hypothesis, size_t startPos, size_t endPos);
    void ExpandHypothesis(const Moses::Hypothesis &hypothesis,const Moses::TranslationOption &transOpt, float expectedScore);

  public:
    SearchRandom(const Moses::InputType &source, const Moses::TranslationOptionCollection &transOptColl);
    ~SearchRandom();

    void ProcessSentence();

    void OutputHypoStackSize();
    void OutputHypoStack(int stack);

    virtual const std::vector <Moses::HypothesisStack* >& GetHypothesisStacks() const;
    virtual const Moses::Hypothesis *GetBestHypothesis() const;
};


}

