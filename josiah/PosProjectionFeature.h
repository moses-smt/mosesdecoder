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


#include "FeatureFunction.h"
#include "Gibbler.h"
#include "Pos.h"

namespace Josiah {


/**
 * Features derived from projection of source pos tags onto target.
**/
class PosProjectionFeature: public FeatureFunction {
    public:
        PosProjectionFeature(const string& name,
             Moses::FactorType sourceFactorType, size_t numValues)
            : FeatureFunction(name,numValues),
              m_defaultImportanceWeights(numValues),
              m_sample(NULL),
              m_sourceFactorType(sourceFactorType),
              m_numValues(numValues) {}

        PosProjectionFeature(Moses::FactorType targetFactorType);
        /** Initialise with a new sample */
        virtual void init(const Sample& sample) {m_sample = &sample;}
        /** Update the target words.*/
        virtual void updateTarget();
        virtual void assignImportanceScore(ScoreComponentCollection& scores) {
            scores.Assign(&getScoreProducer(), m_defaultImportanceWeights);
        }
        virtual ~PosProjectionFeature() {}
    
    protected:
        /** All projected tags */
        const TagSequence& getCurrentTagProjection() const 
            { return m_tagProjection;}
        
        size_t numValues() const {return m_numValues;}
        Moses::FactorType sourceFactorType() const {return m_sourceFactorType;}

    private:
       std::vector<float> m_defaultImportanceWeights;
       const Sample* m_sample; 
       Moses::FactorType m_sourceFactorType;
       size_t m_numValues;
       TagSequence m_tagProjection;
 
};

class PosProjectionBigramFeature : public PosProjectionFeature {
    public:

        PosProjectionBigramFeature(Moses::FactorType sourceFactorType,
            const std::vector<std::string>& tags) :
            PosProjectionFeature("PosProjectionBigram", sourceFactorType,
            tags.size()*tags.size()),
            m_scores(tags.size()*tags.size()) {
                for (std::vector<std::string>::const_iterator ti = tags.begin();
                    ti != tags.end(); ++ti) {
                    m_tags[*ti] = ti-tags.begin();
                }
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
        //Count the bigrams in the given tag sequence
        void countBigrams(const TagSequence& tagSequence, std::vector<float>& counts);
        std::map<string,size_t> m_tags; //map tag to id
        std::vector<float> m_scores;
};

}
