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
#include <utility>


#include <boost/shared_ptr.hpp>

#include "DummyScoreProducers.h"
#include "FeatureFunction.h"
#include "FeatureVector.h"
#include "WeightManager.h"



namespace Moses {
  class TranslationOption;
  class TranslationOptionCollection;
  class Hypothesis;
  class Factor;
  class WordsRange;
  class Word;
}

using namespace Moses;

namespace Josiah {

class Sample;
class GibbsOperator;  

/**
  * This class hierarchy represents the possible changes in the translation effected
  * by the gibbs operators.
  **/
class TranslationDelta {
  public:
    TranslationDelta(Sample& sample): m_score(-1e6), m_sample(sample) {}
    /**
      Get the absolute score of this delta
    **/
    double getScore() const { return m_score;}
    /** 
    * Apply to the sample
    **/
    virtual void apply(const TranslationDelta& noChangeDelta) = 0;
    /**
    For gain calculation
     **/
    virtual void getNewSentence(std::vector<const Factor*>& newSentence) const = 0;
   
    Sample& getSample() const {return m_sample;}
    virtual ~TranslationDelta() {}
    void updateWeightedScore();
    const FVector& getScores() const { return m_scores;}
    void setScores(const FVector& scores)  { m_scores = scores;}
  protected:
    
    FVector m_scores;
    FValue m_score;
    Sample& m_sample;
  
    
    
    void  getNewSentenceSingle(const TranslationOption* option, const WordsRange& targetSegment, std::vector<const Factor*>& newSentence)  const;
  
    void  getNewSentencePaired(const TranslationOption* leftOption, const TranslationOption* rightOption, const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment, std::vector<const Factor*>& newSentence)  const;
  
    void  getNewSentenceContiguousPaired(const TranslationOption* leftOption, const TranslationOption* rightOption, const WordsRange* leftSegment, const WordsRange* rightTargetSegment, std::vector<const Factor*>& newSentence)  const;
  
    void  getNewSentenceDiscontiguousPaired(const TranslationOption* leftOption, const TranslationOption* rightOption, const WordsRange* leftSegment, const WordsRange* rightTargetSegment, std::vector<const Factor*>& newSentence)  const;
    /**
      * Initialise the scores for the case where only one source-target pair needs to be considered.
     **/
    void initScoresSingleUpdate(const Sample&, const TranslationOption* option, const TargetGap& gap);
    /**
     * Initialise the scores for the case where two (target contiguous) source-target pairs need to be considered.
     * Note that left and right refers to the target order.
     **/
    void initScoresContiguousPairedUpdate(const Sample&, const TranslationOption* leftOption,
                              const TranslationOption* rightOption, const TargetGap& gap);
    
    /** Discontiguous version of above. */
    void initScoresDiscontiguousPairedUpdate(const Sample&, const TranslationOption* leftOption,
                              const TranslationOption* rightOption, const TargetGap& leftGap, 
                              const TargetGap& rightGap);
                              
 
};
 
/**
  * An update that only changes a single source/target phrase pair. May change length of target
  **/
class TranslationUpdateDelta : public virtual TranslationDelta {
  public:
     TranslationUpdateDelta(Sample& sample, const TranslationOption* option , const TargetGap& gap);
     virtual void apply(const TranslationDelta& noChangeDelta);
     const TranslationOption* getOption() const {return m_option;} 
     const TargetGap& getGap() const { return m_gap;}
    virtual void getNewSentence(std::vector<const Factor*>& newSentence)  const;
  private:
    const TranslationOption* m_option;
    TargetGap m_gap;
};

/**
  * An update that merges two source phrases and their corresponding target phrases.
 **/
class MergeDelta : public virtual TranslationDelta {
  public: 
    /**
     * targetWords - the words in the current target sentence
     * option - the source/target phrase to go into the merged segment
     * targetSegment - the location of the target segment
     **/
    MergeDelta(Sample& sample, const TranslationOption* option, const TargetGap& gap);
    virtual void apply(const TranslationDelta& noChangeDelta);
  const TranslationOption* getOption() const  {return m_option;} 
  const TargetGap& getGap()  const { return m_gap;}
    virtual void getNewSentence(std::vector<const Factor*>& newSentence)  const;
  private:
    const TranslationOption* m_option;
    TargetGap m_gap;
  
};

/**
 * Like TranslationUpdateDelta, except that it updates a pair of source/target phrase pairs.
**/
class PairedTranslationUpdateDelta : public virtual TranslationDelta {
  public: 
    /** Options and gaps in target order */
    PairedTranslationUpdateDelta(Sample& sample,
        const TranslationOption* leftOption, const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap);
    
    virtual void apply(const TranslationDelta& noChangeDelta);
    const TranslationOption* getLeftOption()  const {return m_leftOption;} 
    const TranslationOption* getRightOption()  const {return m_rightOption;}  
    const TargetGap& getLeftGap()  const { return m_leftGap;} 
    const TargetGap& getRightGap()  const { return m_rightGap;}
    virtual void getNewSentence(std::vector<const Factor*>& newSentence)  const;
  
  private:
    const TranslationOption* m_leftOption;
    const TranslationOption* m_rightOption;
    TargetGap m_leftGap;
    TargetGap m_rightGap;
};

/**
 * Updates the sample by splitting a source phrase and its corresponding target phrase, choosing new options.
 **/
class SplitDelta : public virtual TranslationDelta {
  public:
    /** Options and gaps in target order */
    SplitDelta(Sample& sample, const TranslationOption* leftOption, const TranslationOption* rightOption, 
    const TargetGap& gap);
    virtual void apply(const TranslationDelta& noChangeDelta);
  const TranslationOption* getLeftOption()  const {return m_leftOption;} 
  const TranslationOption* getRightOption() const  {return m_rightOption;}  
  const TargetGap& getGap()  const { return m_gap;} 
    virtual void getNewSentence(std::vector<const Factor*>& newSentence)  const;
  
  private:
    const TranslationOption* m_leftOption;
    const TranslationOption* m_rightOption;
    TargetGap m_gap;
};

/**
  * Switch the translations on the target side.
 **/
class FlipDelta : public virtual TranslationDelta {
  public: 
    /**  Options and gaps in target order */
    FlipDelta(Sample& sample, const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
              const TargetGap& leftGap, const TargetGap& rightGap);
    
    virtual void apply(const TranslationDelta& noChangeDelta);
  const TranslationOption* getLeftOption()  const {return m_leftTgtOption;} 
  const TranslationOption* getRightOption()  const {return m_rightTgtOption;}  
  const TargetGap& getLeftGap() const  { return m_leftGap;} 
  const TargetGap& getRightGap()  const { return m_rightGap;}
    virtual void getNewSentence(std::vector<const Factor*>& newSentence)  const;
  private:
    const TranslationOption* m_leftTgtOption;
    const TranslationOption* m_rightTgtOption;
    TargetGap m_leftGap;
    TargetGap m_rightGap;
    Hypothesis* m_prevTgtHypo;
    Hypothesis* m_nextTgtHypo;
};

typedef boost::shared_ptr<TranslationDelta> TDeltaHandle;
typedef std::vector<TDeltaHandle> TDeltaVector;


} //namespace

