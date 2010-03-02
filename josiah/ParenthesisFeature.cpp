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


void ParenthesisFeature::init(const Sample& sample) {
    m_sample = &sample;
}



void ParenthesisFeature::updateTarget() {
    const vector<Word>& words = m_sample->GetTargetWords();
    m_counts.count(words.begin(),words.end(),m_lefts,m_rights);
    
    /*cerr << "Target: ";
    for (size_t i = 0; i < words.size(); ++i) {
        cerr << words[i][0]->GetString() << " ";
    }
    cerr << endl;
    cerr << m_sample->GetFeatureValues() << endl;
    
    ScoreComponentCollection scores;
    assignScore(scores);
    if (scores[0] != m_sample->GetFeatureValues()[0]) {
        cerr << "ERROR: PF mismatch expected=" << scores[0] << " actual=" << m_sample->GetFeatureValues()[0] << endl;
}*/
}

void ParenthesisFeature::assignImportanceScore(ScoreComponentCollection& scores) {
    scores.Assign(&getScoreProducer(), m_defaultImportanceWeights);
}

void ParenthesisFeature::assignScore(ScoreComponentCollection& scores) {
    //count number of mismatches of each type
    vector<float> violations(m_numValues);
    getViolations(m_counts,violations);
    scores.Assign(&getScoreProducer(), violations);
    
}

void ParenthesisFeature::getViolations(const ParenthesisCounts& counts, vector<float>& violations,
                                       const ParenthesisCounts* outsideCounts, const WordsRange* segment) {
    
    for (size_t pid = 0; pid < m_numValues; ++pid) {
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
                ++violations[pid];
            }
        }
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
                ++violations[pid];
            }
        }
        
    }
}

void ParenthesisFeature::scoreUpdate(const Phrase& phrase, const WordsRange& segment, ScoreComponentCollection& scores) {
    vector<float> violations(m_numValues);
    ParenthesisCounts counts(m_numValues);
    counts.count(phrase.begin(), phrase.end(), m_lefts, m_rights);
    getViolations(counts,violations,&m_counts,&segment);
    scores.Assign(&getScoreProducer(),violations);
}

void ParenthesisFeature::doSingleUpdate(const TranslationOption* option, const TargetGap& gap, ScoreComponentCollection& scores) {
    scoreUpdate(option->GetTargetPhrase(),gap.segment,scores);
}


void ParenthesisFeature::doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& gap, ScoreComponentCollection& scores) 
{
    Phrase phrase(leftOption->GetTargetPhrase());
    phrase.Append(rightOption->GetTargetPhrase());
    scoreUpdate(phrase,gap.segment,scores);
}

void ParenthesisFeature::doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap, ScoreComponentCollection& scores)
{
    WordsRange segment(leftGap.segment.GetStartPos(), rightGap.segment.GetEndPos());
    Phrase phrase(leftOption->GetTargetPhrase());
    for (const Hypothesis* h = leftGap.rightHypo; h != rightGap.leftHypo; h = h->GetNextHypo()) {
        phrase.Append(h->GetCurrTargetPhrase());
    }
    phrase.Append(rightGap.leftHypo->GetTargetPhrase());
    phrase.Append(rightOption->GetTargetPhrase());
    scoreUpdate(phrase,segment,scores);
}

/** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
void ParenthesisFeature::doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                            const TargetGap& leftGap, const TargetGap& rightGap, ScoreComponentCollection& scores)
{
    if (leftGap.segment.GetEndPos()+1 == rightGap.segment.GetStartPos()) {
        TargetGap gap(leftGap.leftHypo, rightGap.rightHypo,WordsRange(leftGap.segment.GetStartPos(), rightGap.segment.GetEndPos()));
        doContiguousPairedUpdate(leftOption,rightOption,gap,scores);
    } else {
        doDiscontiguousPairedUpdate(leftOption,rightOption,leftGap,rightGap,scores);
    }
}



}
