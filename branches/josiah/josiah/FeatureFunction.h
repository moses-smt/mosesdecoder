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


using namespace Moses;
namespace po = boost::program_options;

namespace Moses {
  class Sample;
  class ScoreProducer;
  class ScoreIndexManager;
}

namespace Josiah {
  
/**
  * Base class for any state information required by a FeatureFunction.
  * For sampling, state is any information that the FeatureFunction deems
  * useful for computing updates efficiently. 
  * Since the Sample never actually needs to know anything about the 
  * FeatureState, it contains no useful members.
 **/
class FeatureState {};

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
    FeatureFunction(const std::string& name) :
      m_scoreProducer(name) {}
    /** Initialise with new sample */
    virtual void init(const Sample& sample) = 0;
    /** Compute full score of a sample from scratch **/
    virtual float computeScore() = 0;
    /** Compute the log of the importance weight. This is log(true score) - log (importance score). The computeScore()
     *  method returns the importance score, as do all the getXXScore() methods. **/
    virtual float getImportanceWeight() {return 0;}
    /** Score due to one segment */
    virtual float getSingleUpdateScore(const TranslationOption* option, const WordsRange& targetSegment) = 0;
    /** Score due to two segments **/
    virtual float getPairedUpdateScore(const TranslationOption* leftOption,
                               const TranslationOption* rightOption, const WordsRange& targetSegment, const Phrase& targetPhrase) = 0;
    
    /** Score due to flip */
    virtual float getFlipUpdateScore(const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
                             const Hypothesis* leftTgtHyp, const Hypothesis* rightTgtHyp, 
                             const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment) = 0;
    const Moses::ScoreProducer& getScoreProducer() const {return m_scoreProducer;}
    virtual ~FeatureFunction() = 0;
  
  private:
    FeatureFunctionScoreProducer m_scoreProducer;
    
};





typedef boost::shared_ptr<FeatureFunction> feature_handle;
typedef std::vector<feature_handle> feature_vector;
void configure_features_from_file(const std::string& filename, feature_vector& fv); 



} //namespace
