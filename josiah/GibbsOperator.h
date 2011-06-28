// vim:tabstop=2
/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

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
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iomanip>



#include "FeatureVector.h"
#include "Gibbler.h"
#include "TranslationDelta.h"
#include "TypeDef.h"

namespace Moses {
  class Hypothesis;
  class TranslationOptionCollection;
  class WordsRange;
}

using namespace Moses;

namespace Josiah {

  /**
   * Used to extract the top-n translation options.
   **/
  class PrunedTranslationOptionList {
    public:
      PrunedTranslationOptionList(
        const Moses::TranslationOptionCollection& toc, 
        const Moses::WordsRange& segment,
        size_t count);

      TranslationOptionList::const_iterator begin() const;
      TranslationOptionList::const_iterator end() const;

    private:
      const TranslationOptionList& m_options;
      size_t m_count;
  };

  /** Abstract base class for gibbs operators **/
  class GibbsOperator {
    public:
        GibbsOperator(const std::string& name, float prob) : m_name(name),  m_prob(prob) {}
        /** Proposes a set of possible changes to the current sample, and a delta signifying 'noChange'. */
        virtual void propose(Sample& sample, const TranslationOptionCollection& toc,
                             TDeltaVector& deltas, TDeltaHandle& noChangeDelta) = 0;
        /** The name of this operator */
        const std::string& name() const {return m_name;}
        /** The weight given to this operator in random scanning */
        float GetScanProb() const {return m_prob;}
        virtual ~GibbsOperator();
     protected:
        std::string m_name;
        float m_prob; // the probability of sampling this operator
  };
  
  /**
    * Operator that keeps ordering constant, but visits each (internal) source word boundary, and 
    * merge or split the segment(s) at that boundary, and update the translation.
    **/
  class MergeSplitOperator : public virtual GibbsOperator {
    public:
      MergeSplitOperator(float scanProb = 0.333,size_t toptionLimit=20) :
         GibbsOperator("merge-split", scanProb),
         m_toptionLimit(toptionLimit) {}
      virtual ~MergeSplitOperator() {}
      virtual void propose(Sample& sample, const TranslationOptionCollection& toc,
                             TDeltaVector& deltas, TDeltaHandle& noChangeDelta);
    private:
      size_t m_toptionLimit;
  };
  
  /**
    * Operator which may update any translation option, but may not change segmentation or ordering.
    **/
  class TranslationSwapOperator : public virtual GibbsOperator {
    public:
      TranslationSwapOperator(float scanProb = 0.333, size_t toptionLimit = 0) :
          GibbsOperator("translation-swap", scanProb),
          m_toptionLimit(toptionLimit) {}
      virtual ~TranslationSwapOperator() {}
      virtual void propose(Sample& sample, const TranslationOptionCollection& toc,
                             TDeltaVector& deltas, TDeltaHandle& noChangeDelta);
    private:
      size_t m_toptionLimit;
  };
  
  /**
   * Operator which performs local reordering provided both source segments and target segments are contiguous, and that the swaps
   * will not violate the reordering constraints of the model
   **/
  class FlipOperator : public virtual GibbsOperator {
  public:
    FlipOperator(float scanProb = 0.333) : GibbsOperator("flip", scanProb) {}
    virtual ~FlipOperator() {}
    virtual void propose(Sample& sample, const TranslationOptionCollection& toc,
                             TDeltaVector& deltas, TDeltaHandle& noChangeDelta);
    
    const std::vector<size_t> & GetSplitPoints() {
      return m_splitPoints;
    }
  private:
    void CollectAllSplitPoints(Sample& sample);
    std::vector<size_t> m_splitPoints;
  };
  
  bool CheckValidReordering(const WordsRange& leftSourceSegment, const WordsRange& rightSourceSegment, const Hypothesis* leftTgtPrevHypo, const Hypothesis* leftTgtNextHypo, const Hypothesis* rightTgtPrevHypo, const Hypothesis* rightTgtNextHypo, float & totalDistortion);
  
  
}

