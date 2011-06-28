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

#include <map>

#include <boost/shared_ptr.hpp>

#include "FeatureFunction.h"
#include "Gibbler.h"
#include "LexicalReordering.h"

namespace Josiah {


typedef boost::shared_ptr<const Moses::LexicalReorderingState> LRStateHandle;

/** Wraps Moses lexical reordering */
class LexicalReorderingFeature : public Feature {
  public:
    LexicalReorderingFeature(Moses::LexicalReordering* lexReorder,size_t index);
    virtual FeatureFunctionHandle getFunction(const Sample& sample) const;

  private:
    Moses::LexicalReordering* m_mosesLexReorder;
    size_t m_index;
    std::vector<FName> m_featureNames;
    size_t m_beginIndex;
};

class LexicalReorderingFeatureFunction : public FeatureFunction {
  public:
    LexicalReorderingFeatureFunction
    (const Sample&, std::vector<FName> featureNames,
      Moses::LexicalReordering* lexReorder);

    /** Assign the total score of this feature on the current hypo */
    virtual void assignScore(FVector& scores);

    virtual void updateTarget();
    
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
    void addScore(std::vector<float>& accumulator, FVector& scores);
    std::vector<FName> m_featureNames;
    Moses::LexicalReordering* m_mosesLexReorder;

    //typedef std::map<Moses::WordsRange, const Moses::Hypothesis*> CurrentHypos_t;
    //typedef std::map<Moses::WordsRange, const Moses::FFState*> PreviousStates_t;

    //CurrentHypos_t m_currentHypos;
    //PreviousStates_t m_previousStates;
    //maps the word index to the previous state involved in score calculation
    std::vector<LRStateHandle> m_prevStates;
    ScoreComponentCollection m_accumulator;

};

}
