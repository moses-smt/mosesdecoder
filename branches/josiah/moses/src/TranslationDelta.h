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

#include "DummyScoreProducers.h"
#include "Gibbler.h"
#include "Hypothesis.h"
#include "TranslationOptionCollection.h"

namespace Moses {

/**
  * This class hierarchy represents the possible changes in the translation effected
  * by the gibbs operators.
  **/
class TranslationDelta {
  public:
    TranslationDelta(): m_score(-1e6) {}
  
    /**
      Get the absolute score of this delta
      **/
    double getScore() { return m_score;}
    /** 
      * Apply to the sample
      **/
    virtual void apply(Sample& sample) = 0;
    /**
      Compute the change in language model score by adding this target phrase
      into the hypothesis at the given target position.
      **/
      void  addLanguageModelScore(const Hypothesis* hypothesis, const Phrase& targetPhrase);
    const ScoreComponentCollection& getScores() {return m_scores;}
    virtual ~TranslationDelta() {}
    
  protected:
    ScoreComponentCollection m_scores;
    double m_score;
    
};
 
/**
  * An update that only changes a single source/target phrase pair. May change length of target
  **/
class TranslationUpdateDelta : public virtual TranslationDelta {
  public:
     TranslationUpdateDelta(const Hypothesis* hypothesis,  const TranslationOption* option , const WordsRange& targetSegment);
     virtual void apply(Sample& sample);
     
  private:
    const TranslationOption* m_option;
    WordsRange m_targetSegment;
};

//TODO:

// MergeDelta - merges two contiguous source phrases, and corresponding contiguous target phrases

// PairedTranslationUpdatedDelta - like TranslationUpdateDelta, except updates a pair

// SplitDelta - Splits a source segment, and also target segment

// FlipDelta - Pair of source segments translates to pair of target segments. This delta just switches
//  the order on the target side

} //namespace

