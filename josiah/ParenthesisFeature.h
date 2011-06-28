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

namespace Josiah {

/**
 * Store the various counts at each position.
**/
class ParenthesisCounts {
    public:
        ParenthesisCounts(size_t numValues):
            m_ll(numValues), m_rl(numValues), m_lr(numValues), m_rr(numValues)
                ,m_leftPositions(numValues),m_rightPositions(numValues) {}
        
        //getters
        size_t numValues() const {return m_ll.size();}
        size_t segmentLength() const {return m_ll[0].size();}
        size_t ll(size_t pid, size_t position) const {return m_ll[pid][position];}
        size_t rl(size_t pid, size_t position) const {return m_rl[pid][position];}
        size_t lr(size_t pid, size_t position) const {return m_lr[pid][position];}
        size_t rr(size_t pid, size_t position) const {return m_rr[pid][position];}
        
        const std::vector<std::vector<size_t> >& leftPositions() const {return m_leftPositions;}
        const std::vector<std::vector<size_t> >& rightPositions() const {return m_rightPositions;}
        
        //Initialise counts
        void count(std::vector<Word>::const_iterator begin, std::vector<Word>::const_iterator end,
                  const std::string& lefts, const std::string& rights);

    private:
        std::vector<std::vector<size_t> > m_ll; //left brackets to left
        std::vector<std::vector<size_t> > m_rl; //right brackets to left
        std::vector<std::vector<size_t> > m_lr; //left brackets to right
        std::vector<std::vector<size_t> > m_rr; //right brackets to right
        
        std::vector<std::vector<size_t> > m_leftPositions; //positions of left parentheses
        std::vector<std::vector<size_t> > m_rightPositions; //positions of right parentheses
};


/**
 * Feature that checks for matching between brackets and similar construcions.
**/
class ParenthesisFeature : public Feature {
  public:
    ParenthesisFeature(const std::string lefts, const std::string rights);
    virtual FeatureFunctionHandle getFunction(const Sample& sample) const;
    
  private:
    std::string m_lefts,m_rights;
};

class ParenthesisFeatureFunction: public FeatureFunction {
    public:
        ParenthesisFeatureFunction(const Sample& sample,const std::string lefts, const std::string rights) : 
          FeatureFunction(sample),
        m_numValues(lefts.size()), m_lefts(lefts), m_rights(rights),
        m_counts(m_numValues), m_leftSegmentCounts(m_numValues), m_rightSegmentCounts(m_numValues) {
          for (size_t i = 0; i < lefts.size(); ++i) {
            m_names.push_back(FName("par",lefts.substr(i,1)));
          }
        }
        
        /** Update the target words.*/
        virtual void updateTarget();
    
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
                                  const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) ;
    
        virtual ~ParenthesisFeatureFunction() {}
    
    private:
        /** Violations from a segment, with optional outside counts. If outside counts and segment are missing, then 
        it is assumed that we are */
        void getViolations(const ParenthesisCounts& counts, FVector& violations, 
                           const ParenthesisCounts* outsideCounts=NULL, const WordsRange* segment=NULL);
        
        /** Violations from a pair of segments, with outside counts */
        void getViolations(const ParenthesisCounts& leftSegmentCounts, const ParenthesisCounts& rightSegmentCounts,
                           const WordsRange& leftSegment, const WordsRange& rightSegment,
                           const ParenthesisCounts& outsideCounts, FVector& scores);
        
        void scoreUpdate(const Moses::Phrase& phrase, const Moses::WordsRange& segment, FVector& scores);
                           
        
        size_t m_numValues;
        
        //left and right parenthesis characters
        std::string m_lefts;
        std::string m_rights;
        
        //Counts for current target
        ParenthesisCounts m_counts;
        
        //counts for current segments
        ParenthesisCounts m_leftSegmentCounts;
        ParenthesisCounts m_rightSegmentCounts;
        
        std::vector<FName> m_names;
        
        
        
};

}
