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
 * FIXME: Temporary hack to get moses to manager our feature functions.
 **/
class FeatureFunctionScoreProducer : public ScoreProducer {
  
  public:
  
    FeatureFunctionScoreProducer(Moses::ScoreIndexManager& scoreIndexManager, const std::string& name);
  
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
    /** Compute full score of a sample from scratch **/
    virtual float computeScore(const Sample& sample) = 0;
    /** Change in score when updating one segment */
    virtual float getSingleUpdateScore(const Sample& sample, const TranslationOption* option, const WordsRange& targetSegment) = 0;
    /** Change in score when updating two segments **/
    virtual float getPairedUpdateScore(const Sample& s, const TranslationOption* leftOption,
                               const TranslationOption* rightOption, const WordsRange& targetSegment, const Phrase& targetPhrase) = 0;
    virtual float getPairedUpdateScore(const Sample& s, const TranslationOption* leftOption,
                                     const TranslationOption* rightOption, const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment) = 0;
    /** Change in score when flipping */
    virtual float getFlipUpdateScore(const Sample& s, const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
                             const Hypothesis* leftTgtHyp, const Hypothesis* rightTgtHyp, 
                             const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment) = 0;
    virtual std::string getName() = 0;
    virtual const Moses::ScoreProducer* getScoreProducer() const = 0;
    virtual ~FeatureFunction() = 0;
};

/**
 * For testing.
 **/
class DummyFeatureFunction : public FeatureFunction {
  public:
    virtual bool isStateful() { return false; }
    virtual float computeScore(const Sample& sample) { return 1; }
    virtual float getSingleUpdateScore(const Sample&, const TranslationOption*, const WordsRange&)
      {return 2;}
    virtual float getPairedUpdateScore(const Sample&, const TranslationOption*,
                                      const TranslationOption*, const WordsRange&, const Phrase&)
    {return 3;}
    virtual float getPairedUpdateScore(const Sample&, const TranslationOption*,
                                     const TranslationOption*, const WordsRange&, const WordsRange&)
    {return 5;}
    virtual float getFlipUpdateScore(const Sample&, const TranslationOption*, const TranslationOption*, 
                                    const Hypothesis*, const Hypothesis*, 
                                    const WordsRange&, const WordsRange&)
    {return 4;}
    virtual std::string getName() {return "dummy";}
    virtual const Moses::ScoreProducer* getScoreProducer() const {return NULL;} //FIXME
    virtual ~DummyFeatureFunction() {}
};



typedef boost::shared_ptr<FeatureFunction> feature_handle;
typedef std::vector<feature_handle> feature_vector;
void configure_features_from_file(const std::string& filename, feature_vector& fv); 

/**
 * Singleton which stores and creates the Gibbler feature functions.
 * TODO: Needs some way of configuring.
 **/
class FeatureRegistry {
  public:
    /** The singleton */
    static const FeatureRegistry* instance();
    /** The names of the features currently in use*/
    const std::vector<string>& getFeatureNames() const;
    /** Create a set of features for this sample. These are heap-allocated. */
    void createFeatures(Sample& sample, std::vector<FeatureFunction*> features);
  
  private:
    std::vector<std::string> m_names;
    static auto_ptr<FeatureRegistry> s_instance;
    
    FeatureRegistry();
    FeatureRegistry(const FeatureRegistry&);
    FeatureRegistry operator=(const FeatureRegistry&);
};

} //namespace
