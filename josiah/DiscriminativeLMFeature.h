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

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "FeatureFunction.h"
#include "Gibbler.h"

namespace Josiah {

/**
  * Feature based on target bigrams.
 **/
class DiscriminativeLMBigramFeature : public FeatureFunction {
    public:
        DiscriminativeLMBigramFeature(const std::vector<std::string>& bigrams);
        
        /** Initialise with a new sample */
        virtual void init(const Sample& sample); 
        /** Update the target words.*/
        virtual void updateTarget();
        virtual void assignImportanceScore(ScoreComponentCollection& scores) {
            scores.Assign(&getScoreProducer(), m_defaultImportanceWeights);
        }
        
        /** Assign the total score of this feature on the current hypo */
        virtual void assignScore(ScoreComponentCollection& scores);
    
        /** Score due to one segment */
        virtual void doSingleUpdate(const TranslationOption* option, const TargetGap& gap, ScoreComponentCollection& scores);
        /** Score due to two segments. The left and right refer to the target positions.**/
        virtual void doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                              const TargetGap& gap, ScoreComponentCollection& scores);
        virtual void doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                const TargetGap& leftGap, const TargetGap& rightGap, ScoreComponentCollection& scores);
    
        /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
        virtual void doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                  const TargetGap& leftGap, const TargetGap& rightGap, ScoreComponentCollection& scores) ;
        
        
    private:
        void scoreBigram(const Word& word1, const Word& word2, std::vector<float>& scores);
        /** Score change due to filling in the gapPhrase in the gap.*/
        void doUpdate(const Phrase& gapPhrase, const TargetGap& gap);
        
        std::vector<float> m_defaultImportanceWeights;
        std::map<std::string, std::map<std::string, size_t> > m_bigrams;
        const Sample* m_sample;
        std::vector<Word> m_targetWords;
        std::vector<float> m_scores;
        size_t m_numValues;
};


}
