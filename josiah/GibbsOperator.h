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
#include <iomanip>

#include "DummyScoreProducers.h"
#include "Gibbler.h"
#include "Hypothesis.h"
#include "TranslationDelta.h"
#include "TranslationOptionCollection.h"
#include "WordsRange.h"

namespace Moses {

  class Sample;
  class SampleCollector;

  /** Abstract base class for gibbs operators **/
  class GibbsOperator {
    public:
      GibbsOperator(const string& name) : m_name(name) {}
        /**
          * Run an iteration of the Gibbs sampler, updating the hypothesis.
          **/
        virtual void doIteration(Sample& sample, const TranslationOptionCollection& toc) = 0;
        const string& name() const {return m_name;}
        virtual ~GibbsOperator() {}
        
     protected:
        /**
          * Pick random sample from given (un-normalised) log probabilities.
          **/
        size_t getSample(const std::vector<double>& scores);
        string m_name;
  };
  
  /**
    * Operator that keeps ordering constant, but visits each (internal) source word boundary, and 
    * merge or split the segment(s) at that boundary, and update the translation.
    **/
  class MergeSplitOperator : public virtual GibbsOperator {
    public:
        MergeSplitOperator() : GibbsOperator("merge-split") {}
        virtual void doIteration(Sample& sample, const TranslationOptionCollection& toc);
        virtual ~MergeSplitOperator() {}
    
    private:
        
  
  };
  
  /**
    * Operator which may update any translation option, but may not change segmentation or ordering.
    **/
  class TranslationSwapOperator : public virtual GibbsOperator {
    public:
      TranslationSwapOperator() : GibbsOperator("translation-swap") {}
      virtual void doIteration(Sample& sample, const TranslationOptionCollection& toc);
      virtual ~TranslationSwapOperator() {}
  };
  
  /**
   * Operator which performs local reordering provided both source segments and target segments are contiguous, and that the swaps
   * will not violate the reordering constraints of the model
   **/
  class FlipOperator : public virtual GibbsOperator {
  public:
    FlipOperator() : GibbsOperator("flip") {}
    virtual void doIteration(Sample& sample, const TranslationOptionCollection& toc);
    virtual const string& name() const {return m_name;}
    virtual ~FlipOperator() {}
    
  private:
    string m_name;
    //bool CheckValidReordering(const Hypothesis* leftTgtHypo, const Hypothesis *rightTgtHypo, const Hypothesis* leftPrevHypo, const Hypothesis* rightNextHypo, float & totalDistortion);
    bool CheckValidReordering(const Hypothesis* leftTgtHypo, const Hypothesis *rightTgtHypo, const Hypothesis* leftTgtPrevHypo, const Hypothesis* leftTgtNextHypo, const Hypothesis* rightTgtPrevHypo, const Hypothesis* rightTgtNextHypo, float & totalDistortion);
  };
 
}

