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

#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>
#include "Hypothesis.h"
#include "ScoreProducer.h"
#include "TranslationOptionCollection.h"
//#include "Gibbler.h"


using namespace Moses;
namespace po = boost::program_options;

namespace Moses {
  class ScoreProducer;
  class ScoreIndexManager;
}


namespace Josiah {

  class Sample;
/**
  * Base class for any state information required by a FeatureFunction.
  * For sampling, state is any information that the FeatureFunction deems
  * useful for computing updates efficiently. 
  * Since the Sample never actually needs to know anything about the 
  * FeatureState, it contains no useful members.
 **/
class FeatureState {};


/**
* Represents a gap in the target sentence. During score computation, this is used to specify where the proposed
* TranslationOptions are to be placed. The left and right hypothesis are to the left and to the right in target order.
* Note that the leftHypo could be the null hypo (if at start) and the rightHypo could be a null pointer (if at end).
**/
struct TargetGap {
  TargetGap(const Hypothesis* lh, const Hypothesis* rh, const WordsRange& s) :
      leftHypo(lh), rightHypo(rh), segment(s) {
      //check that they're in target order.
      assert(!lh->GetPrevHypo() || lh->GetCurrTargetWordsRange() < s);
      assert(!rh || s < rh->GetCurrTargetWordsRange());
  }
  
  const Hypothesis* leftHypo;
  const Hypothesis* rightHypo;
  WordsRange segment;
};

/**
 * Used to  get moses to manage our feature functions.
 **/
class FeatureFunctionScoreProducer : public ScoreProducer {
  
  public:
  
    FeatureFunctionScoreProducer(const std::string& name);
    
  
    size_t GetNumScoreComponents() const;
    std::string GetScoreProducerDescription() const;
  
  private:
    std::string m_name;
  
};

/**
  * Base class for Gibbler feature functions.
 **/
class FeatureFunction {
  public:
    FeatureFunction(const string& name) : m_scoreProducer(name) {}
    /** Initialise with new sample */
    virtual void init(const Sample& sample) {/* do nothing */};
    /** Update the target words.*/
    virtual void updateTarget(){/*do nothing*/}
    /** Insert the log of the importance weight. This is log(true score) - log (approximate score). The assignScore()
     *  method inserts the approximate, as do all the doXXXUpdate() methods. **/
    virtual void assignImportanceScore(ScoreComponentCollection& scores) = 0;
    
    /** Assign the total score of this feature on the current hypo */
    virtual void assignScore(ScoreComponentCollection& scores) = 0;
    
    /** Score due to one segment */
    virtual void doSingleUpdate(const TranslationOption* option, const TargetGap& gap, ScoreComponentCollection& scores) = 0;
    /** Score due to two segments. The left and right refer to the target positions.**/
    virtual void doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& gap, ScoreComponentCollection& scores) = 0;
    virtual void doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap, ScoreComponentCollection& scores) = 0;
    
    /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
    virtual void doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                     const TargetGap& leftGap, const TargetGap& rightGap, ScoreComponentCollection& scores) = 0;
    
    virtual ~FeatureFunction() = 0;
    
  protected:
    const Moses::ScoreProducer& getScoreProducer() const {return m_scoreProducer;}
    
  private:
    FeatureFunctionScoreProducer m_scoreProducer;
    
};

/** 
  * A feature function with a single value
  **/
class SingleValuedFeatureFunction: public FeatureFunction {
  public:
    SingleValuedFeatureFunction(const std::string& name) :
      FeatureFunction(name) {}
    virtual void assignImportanceScore(ScoreComponentCollection& scores)
      {scores.Assign(&getScoreProducer(),getImportanceWeight());}
    virtual void assignScore(ScoreComponentCollection& scores)
      {scores.Assign(&getScoreProducer(), computeScore());}
    virtual void doSingleUpdate(const TranslationOption* option, const TargetGap& gap, ScoreComponentCollection& scores)
      {scores.Assign(&getScoreProducer(), getSingleUpdateScore(option,gap));}
    virtual void doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                          const TargetGap& gap, ScoreComponentCollection& scores)
    {scores.Assign(&getScoreProducer(), getContiguousPairedUpdateScore(leftOption,rightOption,gap));}
    virtual void doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                             const TargetGap& leftGap, const TargetGap& rightGap, ScoreComponentCollection& scores)
    {scores.Assign(&getScoreProducer(), getDiscontiguousPairedUpdateScore(leftOption,rightOption,leftGap,rightGap));}
    /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
    virtual void doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                              const TargetGap& leftGap, const TargetGap& rightGap, ScoreComponentCollection& scores)
    {scores.Assign(&getScoreProducer(), getFlipUpdateScore(leftOption,rightOption,leftGap,rightGap));}
    
  protected:
    virtual float getImportanceWeight() {return 0;}
    virtual float computeScore() = 0;
    /** Score due to one segment */
    virtual float getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap) = 0;
    /** Score due to two segments. The left and right refer to the target positions.**/
    virtual float getContiguousPairedUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
       const TargetGap& gap) = 0;
    virtual float getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap) = 0;
    
    /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
    virtual float getFlipUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                     const TargetGap& leftGap, const TargetGap& rightGap) = 0;
    
    virtual ~SingleValuedFeatureFunction() {}
  
    
};





typedef boost::shared_ptr<FeatureFunction> feature_handle;
typedef std::vector<feature_handle> feature_vector;




} //namespace
