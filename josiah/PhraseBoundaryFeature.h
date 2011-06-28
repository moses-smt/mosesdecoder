/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2011 University of Edinburgh

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

#include<set>
#include<vector>

#include "FeatureFunction.h"

namespace Josiah {

/**
  * Bigrams at phrase boundaries. 
 **/
class PhraseBoundaryFeature : public Feature {
  public:
    PhraseBoundaryFeature(
      const Moses::FactorList& sourceFactors,
      const Moses::FactorList& targetFactors,
      const std::vector<std::string>& sourceVocabFiles,
      const std::vector<std::string>& targetVocabFiles);

    virtual FeatureFunctionHandle getFunction(const Sample& sample) const;

    /** If either word is null, then eos or bos is assumed */
    void addSourceFeatures(
      const Moses::Word* leftWord, const Moses::Word* rightWord,
         FVector& scores) const;

    void addTargetFeatures(
      const Moses::Word* leftWord, const Moses::Word* rightWord,
         FVector& scores) const;

  private:
    void addFeatures(
      const Moses::Word* leftWord, const Moses::Word* rightWord,
         const Moses::FactorList& factors, const std::string& side,
         const std::vector<std::set<std::string> >& vocabs,
         FVector& scores) const;
    void loadVocab(const std::string& filename, std::set<std::string>& vocab);

    static const std::string SEP;
    static const std::string STEM;
    static const std::string SOURCE;
    static const std::string TARGET;
    static const std::string BOS;
    static const std::string EOS;

    Moses::FactorList m_sourceFactors;
    Moses::FactorList m_targetFactors;

    std::vector<std::set<std::string> > m_sourceVocabs;
    std::vector<std::set<std::string> > m_targetVocabs;
};

class PhraseBoundaryFeatureFunction : public FeatureFunction {
  public:
    PhraseBoundaryFeatureFunction(const Sample& sample, const PhraseBoundaryFeature& parent);
    
    /** Update the target words.*/
    virtual void updateTarget();
    
    /** Assign the total score of this feature on the current hypo */
    virtual void assignScore(FVector& scores);

    /** Score due to one segment */
    virtual void doSingleUpdate(
      const TranslationOption* option, const TargetGap& gap, FVector& scores);

    /** Score due to two segments. The left and right refer to the target positions.**/
    virtual void doContiguousPairedUpdate(
      const TranslationOption* leftOption,const TranslationOption* rightOption, 
      const TargetGap& gap, FVector& scores);

    virtual void doDiscontiguousPairedUpdate(
      const TranslationOption* leftOption,const TranslationOption* rightOption, 
      const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores);

    /** Score due to flip. Again, left and right refer to order on the
         <emph>target</emph> side. */
    virtual void doFlipUpdate(
      const TranslationOption* leftOption,const TranslationOption* rightOption, 
      const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) ;

  private:
    void scoreOptions(
      const TranslationOption* leftOption, const TranslationOption* rightOption,
        FVector& scores);
    const PhraseBoundaryFeature& m_parent;
};



}


