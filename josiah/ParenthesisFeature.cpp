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

#include <algorithm>
#include <iterator>

#include "ParenthesisFeature.h"

using namespace std;

namespace Josiah {
  
  ParenthesisFeature::ParenthesisFeature(const std::string lefts, const std::string rights) :
      m_lefts(lefts), m_rights(rights) {}
  
  FeatureFunctionHandle ParenthesisFeature::getFunction(const Sample& sample) const {
    return FeatureFunctionHandle(new ParenthesisFeatureFunction(sample, m_lefts, m_rights));
  }
    
    
void ParenthesisCounts::count(vector<Word>::const_iterator begin, vector<Word>::const_iterator end,
           const string& lefts, const string& rights) 
{
    size_t segmentSize = end - begin;
    for (size_t i = 0; i < m_ll.size(); ++i) {
        m_ll[i].resize(segmentSize); 
        m_lr[i].resize(segmentSize); 
        m_rl[i].resize(segmentSize); 
        m_rr[i].resize(segmentSize);
        m_leftPositions[i].clear();
        m_rightPositions[i].clear();
    }
    
    //iterate right to left
    reverse_iterator<vector<Word>::const_iterator> ri(end);
    reverse_iterator<vector<Word>::const_iterator> rend(begin);
    size_t wpos = segmentSize;
    for ( ;ri != rend; ++ri) {
        --wpos;
        const string& text = ri->operator[](0)->GetString();
        size_t lid = string::npos;
        size_t rid = string::npos;
        if (text.size() == 1) {
            lid = lefts.find(text);
            rid = rights.find(text);
        }
        for (size_t pid = 0; pid < m_ll.size(); ++pid) {
            size_t curr_lr = m_lr[pid].size() > wpos+1 ? m_lr[pid][wpos+1] : 0;
            size_t curr_rr = m_rr[pid].size() > wpos+1 ? m_rr[pid][wpos+1] : 0;
            if (pid == lid) {
                //found a left parenthesis
                ++curr_lr;
                m_leftPositions[pid].push_back(wpos);
            }
            if (pid == rid) {
                //found a right parenthesis
                ++curr_rr;
                m_rightPositions[pid].push_back(wpos);
            }
            m_lr[pid][wpos] = curr_lr;
            m_rr[pid][wpos] = curr_rr;
        }
    }
    
    assert(wpos == 0);
    
    //iterate left to right
    vector<Word>::const_iterator i = begin;
    for (; i != end; ++i) {
        const string& text = i->operator[](0)->GetString();
        size_t lid = string::npos;
        size_t rid = string::npos;
        if (text.size() == 1) {
            lid = lefts.find(text);
            rid = rights.find(text);
        }
        for (size_t pid = 0; pid < m_ll.size(); ++pid) {
            size_t curr_ll = wpos > 0 ? m_ll[pid][wpos-1] : 0;
            size_t curr_rl = wpos > 0 ? m_rl[pid][wpos-1] : 0;
            if (pid == lid) ++curr_ll;
            if (pid == rid) ++curr_rl;
            m_ll[pid][wpos] = curr_ll;
            m_rl[pid][wpos] = curr_rl; 
        }
        
        ++wpos;
    }
}




void ParenthesisFeatureFunction::updateTarget() {
    const vector<Word>& words = getSample().GetTargetWords();
    m_counts.count(words.begin(),words.end(),m_lefts,m_rights);
}


void ParenthesisFeatureFunction::assignScore(FVector& scores) {
    //count number of mismatches of each type
    getViolations(m_counts,scores);
}

struct WordsRangeCovers {
    WordsRangeCovers(const WordsRange& range) : m_range(range) {}
    bool operator() (size_t pos) {return m_range.covers(pos);}
    const WordsRange& m_range;
};

void ParenthesisFeatureFunction::getViolations(const ParenthesisCounts& counts, FVector& violations,
                                       const ParenthesisCounts* outsideCounts, const WordsRange* segment) 
{
                                          
    for (size_t pid = 0; pid < m_numValues; ++pid) {
        //left violations to left of segment
        if (outsideCounts) {
            size_t leftsInSegment = count_if(outsideCounts->leftPositions()[pid].begin(), 
                                                 outsideCounts->leftPositions()[pid].end(),
                                                 WordsRangeCovers(*segment));
            size_t rightsInSegment = count_if(outsideCounts->rightPositions()[pid].begin(), 
                                             outsideCounts->rightPositions()[pid].end(),
                                             WordsRangeCovers(*segment));
            for (size_t i = 0; i < outsideCounts->leftPositions()[pid].size(); ++i) {
                size_t ppos = outsideCounts->leftPositions()[pid][i];
                if (ppos >= segment->GetStartPos()) continue;
                size_t lr = outsideCounts->lr(pid,ppos);
                size_t rr = outsideCounts->rr(pid,ppos);
                //cerr << "lr: " <<  lr << " rr: " << rr << " ";
                //account for contents of segment
                lr += counts.leftPositions()[pid].size() - leftsInSegment;
                rr += counts.rightPositions()[pid].size() - rightsInSegment;
                //cerr << "lr: " <<  lr << " rr: " << rr << " rpos " << counts.rightPositions().size() << " ris: " << rightsInSegment << endl;
                if (lr > rr) {
                    violations[m_names[pid]] = violations[m_names[pid]] + 1;
                }
            }
        }
        
        //right violations to left of segment
        //Ignore since the new segment cannot change these
        
        //left violations inside the segment 
        for (size_t i = 0; i < counts.leftPositions()[pid].size(); ++i) {
            size_t ppos = counts.leftPositions()[pid][i];
            size_t lr = counts.lr(pid,ppos);
            size_t rr = counts.rr(pid,ppos);
            if (outsideCounts) {
                if (segment->GetEndPos()+1 < outsideCounts->segmentLength()) {
                    lr += outsideCounts->lr(pid,segment->GetEndPos()+1);
                    rr += outsideCounts->rr(pid,segment->GetEndPos()+1);
                }
            }
            if (lr > rr) {
              violations[m_names[pid]] = violations[m_names[pid]] + 1;
            }
        }
        //right violations in the segment
        for (size_t i = 0; i < counts.rightPositions()[pid].size(); ++i) {
            size_t ppos = counts.rightPositions()[pid][i];
            size_t rl = counts.rl(pid,ppos);
            size_t ll = counts.ll(pid,ppos);
            if (outsideCounts) {
                if (segment->GetStartPos() > 0) {
                    rl += outsideCounts->rl(pid,segment->GetStartPos()-1);
                    ll += outsideCounts->ll(pid,segment->GetStartPos()-1);
                }
            }
            if (rl > ll) {
              violations[m_names[pid]] = violations[m_names[pid]] + 1;
            }
        }
        
        //left violations to right of segment
        //Ignore since the new segment cannot change these
        
        //right violations to right of segment
        if (outsideCounts) {
            size_t leftsInSegment = count_if(outsideCounts->leftPositions()[pid].begin(), 
                                             outsideCounts->leftPositions()[pid].end(),
                                             WordsRangeCovers(*segment));
            size_t rightsInSegment = count_if(outsideCounts->rightPositions()[pid].begin(), 
                                              outsideCounts->rightPositions()[pid].end(),
                                              WordsRangeCovers(*segment));
            for (size_t i = 0; i < outsideCounts->rightPositions()[pid].size(); ++i) {
                size_t ppos = outsideCounts->rightPositions()[pid][i];
                if (ppos <= segment->GetEndPos()) continue;
                size_t rl = outsideCounts->rl(pid,ppos);
                size_t ll = outsideCounts->ll(pid,ppos);
                //account for segment
                rl += counts.rightPositions()[pid].size() - rightsInSegment;
                ll += counts.leftPositions()[pid].size() - leftsInSegment;
                if (rl > ll) {
                  violations[m_names[pid]] = violations[m_names[pid]] + 1;
                }
            }
        }
        
    }
}



void ParenthesisFeatureFunction::getViolations(const ParenthesisCounts& leftSegmentCounts, const ParenthesisCounts& rightSegmentCounts,
                   const WordsRange& leftSegment, const WordsRange& rightSegment,
                   const ParenthesisCounts& outsideCounts, FVector& violations) 
{
    for (size_t pid = 0; pid < m_numValues; ++pid) {
        //count the existing parentheses in the left and right segments
        size_t leftsInLeftSegment = count_if(outsideCounts.leftPositions()[pid].begin(), 
                                             outsideCounts.leftPositions()[pid].end(),
                                             WordsRangeCovers(leftSegment));
        size_t leftsInRightSegment = count_if(outsideCounts.leftPositions()[pid].begin(),
                                              outsideCounts.leftPositions()[pid].end(),
                                              WordsRangeCovers(rightSegment));
        size_t rightsInLeftSegment = count_if(outsideCounts.rightPositions()[pid].begin(),
                                              outsideCounts.rightPositions()[pid].end(),
                                              WordsRangeCovers(leftSegment));
        size_t rightsInRightSegment = count_if(outsideCounts.rightPositions()[pid].begin(),
                                              outsideCounts.rightPositions()[pid].end(),
                                              WordsRangeCovers(rightSegment));
        
        //check left parentheses in  left segment
        for (size_t i = 0; i < leftSegmentCounts.leftPositions()[pid].size(); ++i) {
            size_t ppos = leftSegmentCounts.leftPositions()[pid][i];
            size_t lr = leftSegmentCounts.lr(pid,ppos);
            size_t rr = leftSegmentCounts.rr(pid,ppos);
            if (leftSegment.GetEndPos()+1 < outsideCounts.segmentLength()) {
                lr += outsideCounts.lr(pid,leftSegment.GetEndPos()+1);
                rr += outsideCounts.rr(pid,leftSegment.GetEndPos()+1);
            }
            //account for right segment
            lr += (rightSegmentCounts.leftPositions()[pid].size() - leftsInRightSegment);
            rr += (rightSegmentCounts.rightPositions()[pid].size() - rightsInRightSegment);
            if (lr > rr) {
              violations[m_names[pid]] = violations[m_names[pid]] + 1;
            }
        }
        
        //check right parentheses in left segment
        for (size_t i = 0; i < leftSegmentCounts.rightPositions()[pid].size(); ++i) {
            size_t ppos = leftSegmentCounts.rightPositions()[pid][i];
            size_t rl = leftSegmentCounts.rl(pid,ppos);
            size_t ll = leftSegmentCounts.ll(pid,ppos);
            if (leftSegment.GetStartPos() > 0) {
                rl += outsideCounts.rl(pid, leftSegment.GetStartPos()-1);
                ll += outsideCounts.ll(pid, leftSegment.GetStartPos()-1);
            }
            if (rl > ll) {
              violations[m_names[pid]] = violations[m_names[pid]] + 1;
            }
        }
        
        //check left parentheses in right segment
        for (size_t i = 0; i < rightSegmentCounts.leftPositions()[pid].size(); ++i) {
            size_t ppos = rightSegmentCounts.leftPositions()[pid][i];
            size_t lr = rightSegmentCounts.lr(pid,ppos);
            size_t rr = rightSegmentCounts.rr(pid,ppos);
            if (rightSegment.GetEndPos()+1 < outsideCounts.segmentLength()) {
                lr += outsideCounts.lr(pid, rightSegment.GetEndPos()+1);
                rr += outsideCounts.rr(pid, rightSegment.GetEndPos()+1);
            }
            if (lr > rr) {
              violations[m_names[pid]] = violations[m_names[pid]] + 1;
            }
        }
        
        //check for right parentheses in right segment
        for (size_t i =  0; i < rightSegmentCounts.rightPositions()[pid].size(); ++i) {
            size_t ppos = rightSegmentCounts.rightPositions()[pid][i];
            size_t rl = rightSegmentCounts.rl(pid,ppos);
            size_t ll = rightSegmentCounts.ll(pid,ppos);
            if (rightSegment.GetStartPos() > 0) {
                rl += outsideCounts.rl(pid,rightSegment.GetStartPos()-1);
                ll += outsideCounts.ll(pid, rightSegment.GetStartPos()-1);
            }
            //account for left segment
            rl += (leftSegmentCounts.rightPositions()[pid].size() - rightsInLeftSegment);
            ll += (leftSegmentCounts.leftPositions()[pid].size() - leftsInLeftSegment);
            if (rl > ll) {
              violations[m_names[pid]] = violations[m_names[pid]] + 1;
            }
        }
        
        //check for left parentheses outside of both segments
        for (size_t i = 0; i < outsideCounts.leftPositions()[pid].size(); ++i) {
            size_t ppos = outsideCounts.leftPositions()[pid][i];
            //ignore if parenthesis is in the left segment, right segment, or to the right of the right segment
            if (ppos >= rightSegment.GetStartPos() || leftSegment.covers(ppos)) continue;
            size_t lr = outsideCounts.lr(pid,ppos);
            size_t rr = outsideCounts.rr(pid,ppos);
            if (ppos < rightSegment.GetStartPos()) {
                //account for right segment
                lr += (rightSegmentCounts.leftPositions()[pid].size() - leftsInRightSegment);
                rr += (rightSegmentCounts.rightPositions()[pid].size() - rightsInRightSegment);
            }
            if (ppos < leftSegment.GetStartPos()) {
                //account for left segment
                lr += (leftSegmentCounts.leftPositions()[pid].size() - leftsInLeftSegment);
                rr += (leftSegmentCounts.rightPositions()[pid].size() - rightsInLeftSegment);
            }
            if (lr > rr) {
              violations[m_names[pid]] = violations[m_names[pid]] + 1;
            }
        }
        
        //check for right parentheses outside of both segments
        for (size_t i = 0; i < outsideCounts.rightPositions()[pid].size(); ++i) {
            size_t ppos = outsideCounts.rightPositions()[pid][i];
            //ignore if parenthesis is in the right segment, left segment, or to the left of left segment
            if (ppos <= leftSegment.GetEndPos() || rightSegment.covers(ppos)) continue;
            size_t rl = outsideCounts.rl(pid,ppos);
            size_t ll = outsideCounts.ll(pid,ppos);
            if (ppos > leftSegment.GetEndPos()) {
                //account for the left segment
                rl += (leftSegmentCounts.rightPositions()[pid].size() - rightsInLeftSegment);
                ll += (leftSegmentCounts.leftPositions()[pid].size() - leftsInLeftSegment);
            }
            if (ppos > rightSegment.GetEndPos()) {
                //account for right segment
                rl += (rightSegmentCounts.rightPositions()[pid].size() - rightsInRightSegment);
                ll += (rightSegmentCounts.leftPositions()[pid].size() - leftsInRightSegment);
            }
            if (rl > ll) {
              violations[m_names[pid]] = violations[m_names[pid]] + 1;
            }
        }
        
        
    }
}

void ParenthesisFeatureFunction::scoreUpdate(const Phrase& phrase, const WordsRange& segment, FVector& scores) {
    m_leftSegmentCounts.count(phrase.begin(), phrase.end(), m_lefts, m_rights);
    getViolations(m_leftSegmentCounts,scores,&m_counts,&segment);
}

void ParenthesisFeatureFunction::doSingleUpdate(const TranslationOption* option, const TargetGap& gap, FVector& scores) {
    scoreUpdate(option->GetTargetPhrase(),gap.segment,scores);
}


void ParenthesisFeatureFunction::doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& gap, FVector& scores) 
{
    Phrase phrase(leftOption->GetTargetPhrase());
    phrase.Append(rightOption->GetTargetPhrase());
    scoreUpdate(phrase,gap.segment,scores);
}

void ParenthesisFeatureFunction::doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores)
{
    const Phrase& leftPhrase = leftOption->GetTargetPhrase();
    m_leftSegmentCounts.count(leftPhrase.begin(),leftPhrase.end(),m_lefts,m_rights);
    const Phrase& rightPhrase = rightOption->GetTargetPhrase();
    m_rightSegmentCounts.count(rightPhrase.begin(),rightPhrase.end(),m_lefts,m_rights);
    getViolations(m_leftSegmentCounts,m_rightSegmentCounts,leftGap.segment,rightGap.segment,m_counts,scores);
}

/** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
void ParenthesisFeatureFunction::doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                            const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores)
{
    if (leftGap.segment.GetEndPos()+1 == rightGap.segment.GetStartPos()) {
        TargetGap gap(leftGap.leftHypo, rightGap.rightHypo,WordsRange(leftGap.segment.GetStartPos(), rightGap.segment.GetEndPos()));
        doContiguousPairedUpdate(leftOption,rightOption,gap,scores);
    } else {
        doDiscontiguousPairedUpdate(leftOption,rightOption,leftGap,rightGap,scores);
    }
}



}
