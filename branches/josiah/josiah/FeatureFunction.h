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
    /** Compute full score of a sample from scratch **/
    virtual float computeScore(const Sample& sample) = 0;
    /** Change in score when updating one segment */
    virtual float getSingleUpdateScore(const Sample& sample, const TranslationOption* option, const WordsRange& targetSegment) = 0;
    /** Change in score when updating two segments **/
    virtual float getPairedUpdateScore(const Sample& s, const TranslationOption* leftOption,
                               const TranslationOption* rightOption, const WordsRange& targetSegment, const Phrase& targetPhrase) = 0;
    
    /** Change in score when flipping */
    virtual float getFlipUpdateScore(const Sample& s, const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
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
