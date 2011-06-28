/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

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

#include <boost/unordered_set.hpp>

#include "FeatureFunction.h"

namespace Josiah {

    typedef boost::unordered_set<std::string> vocab_t;


/** 
  * Used to define different types of reordering features.
 **/
class ReorderingFeatureTemplate {
  public:
    ReorderingFeatureTemplate(): m_vocab(NULL) {}
    static std::string BOS;
    virtual void assign(const Moses::TranslationOption* prevOption, const Moses::TranslationOption* currOption, 
                        const std::string& prefix, FVector& scores) = 0;
    
    void setVocab(vocab_t* vocab) {m_vocab = vocab;}
    bool checkVocab(const std::string& word) const ;
    virtual ~ReorderingFeatureTemplate() {}

  private:
    vocab_t* m_vocab;
};

class EdgeReorderingFeatureTemplate : public ReorderingFeatureTemplate {
  public:
    EdgeReorderingFeatureTemplate(size_t factor, bool source, bool curr) : m_factor(factor), m_source(source), m_curr(curr) {}
    virtual void assign(const Moses::TranslationOption* prevOption, const Moses::TranslationOption* currOption, 
                        const std::string& prefix, FVector& scores);
  
  private:
    size_t m_factor;
    bool m_source; //source or target?
    bool m_curr; //curr of prev?
};

/**
 * Features related to the ordering between segments.
**/
class ReorderingFeature : public Feature {
  public:
    
  /**
   * The msd vector will indicate which types of msd features are to be included. Each element is made
      * up of four parts, separated by colons. The fields are:
   * type: The type of feature (currently only edge is supported)
   * factor_id: An integer representing the factor
   * source/target: One of two possible values indicating whether the 
   *         source or target words are used.
   * prev/curr:  Indicates whether the feature uses the previous or 
  current segment
   *
   * The msdVocab configuration items specify a vocabulary file for
   * the source or target of a given factor. The format of these config
   * strings is factor_id:source/target:filename  
   *
   */
  ReorderingFeature(const std::vector<std::string>& msd,
                    const std::vector<std::string>& msdVocab);
  
  virtual FeatureFunctionHandle getFunction(const Sample& sample) const;
  
  const std::vector<ReorderingFeatureTemplate*>& getTemplates() const;
  
  private:
    std::vector<ReorderingFeatureTemplate*> m_templates;
    std::map<size_t,vocab_t> m_sourceVocabs;
    std::map<size_t,vocab_t > m_targetVocabs;
    
   
    void loadVocab(std::string filename, vocab_t* vocab);

};


class ReorderingFeatureFunction : public FeatureFunction {
    
  public:
    
    ReorderingFeatureFunction(const Sample& sample, const ReorderingFeature& parent);
    
   
    /** Assign the total score of this feature on the current hypo */
    virtual void assignScore(FVector& scores);
    
    /** Score due to one segment */
    virtual void doSingleUpdate(const TranslationOption* option, const TargetGap& gap, FVector& scores);
    /** Score due to two segments. The left and right refer to the target positions.**/
    virtual void doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                          const TargetGap& gap, FVector& scores);
    virtual void doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                             const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores);
    
    /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
    virtual void doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                              const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores);
    
    
  private:
    const ReorderingFeature& m_parent;
    
    
    /** Assign features for the following tow options, assuming they are contiguous on the target side */
    void assign(const Moses::TranslationOption* prevOption, const Moses::TranslationOption* currOption, FVector& scores);
    
    /** Monotone, swapped or discontinuous? The segments are assumed to have contiguous translations on the target side. */
    const std::string& getMsd(const Moses::TranslationOption* prevOption, const Moses::TranslationOption* currOption);
    
    
};

}

