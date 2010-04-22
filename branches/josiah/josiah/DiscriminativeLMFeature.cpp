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

#include "DiscriminativeLMFeature.h"

using namespace Moses;
using namespace std;

namespace Josiah {

DiscriminativeLMBigramFeature::DiscriminativeLMBigramFeature(const vector<string>& bigrams):
        FeatureFunction("DiscriminativeLMBigram",bigrams.size()),
        m_defaultImportanceWeights(bigrams.size()),
        m_sample(NULL),
        m_scores(bigrams.size()),
        m_numValues(bigrams.size())
{
    for (size_t i = 0; i < bigrams.size(); ++i) {
        istringstream split(bigrams[i]);
        string first,second;
        split >> first >> second;
        if (first.empty() || second.empty() || !split.eof()) {
            cerr << "ERROR: invalid discrimlm bigram: '" << bigrams[i] << "'" << endl;
            throw runtime_error("invalid discrimlm bigram");
        }
        m_bigrams[first][second] = i;
    }
}

void DiscriminativeLMBigramFeature::init(const Sample& sample) {
    m_sample = &sample;
}

void DiscriminativeLMBigramFeature::updateTarget() {
    m_targetWords = m_sample->GetTargetWords();
    
    IFVERBOSE(2) {
        ScoreComponentCollection scores;
        assignScore(scores);
        for (size_t i = 0; i < m_targetWords.size(); ++i) {
            VERBOSE(2,m_targetWords[i][0]->GetString() << " ");
            
        }
        VERBOSE(2,endl);
        //NOTE: These may differ, as some operators update in two stages
        VERBOSE(2,"EXPECTED: " << scores << endl);
        VERBOSE(2,"ACTUAL: " << m_sample->GetFeatureValues() << endl);
    }
}

void DiscriminativeLMBigramFeature::scoreBigram(const Word& word1, const Word& word2, vector<float>& scores) {
    map<string,map<string,size_t> >::const_iterator firstIter = m_bigrams.find(word1[0]->GetString());
    if (firstIter != m_bigrams.end()) {
        map<string,size_t>::const_iterator secondIter = firstIter->second.find(word2[0]->GetString());
        if (secondIter != firstIter->second.end()) {
            size_t index = secondIter->second;
            ++scores[index];
        }
    }
}



/** Assign the total score of this feature on the current hypo */
void DiscriminativeLMBigramFeature::assignScore(ScoreComponentCollection& scores)
{
    m_scores.assign(m_numValues,0);
    for (size_t i = 0; i < m_targetWords.size()-1; ++i) {
        scoreBigram(m_targetWords[i],m_targetWords[i+1],m_scores);
    }
    scores.Assign(&getScoreProducer(),m_scores);
}

void DiscriminativeLMBigramFeature::doUpdate(const Phrase& gapPhrase, const TargetGap& gap)
{
    if (gap.leftHypo->GetPrevHypo()) {
        //left edge
        const TargetPhrase& leftPhrase = gap.leftHypo->GetTargetPhrase();
        scoreBigram(leftPhrase.GetWord(leftPhrase.GetSize()-1), gapPhrase.GetWord(0),m_scores);
    }
    //gap phrase
    size_t i = 0;
    for (; i < gapPhrase.GetSize()-1; ++i) {
        scoreBigram(gapPhrase.GetWord(i), gapPhrase.GetWord(i+1),m_scores);
    }
    
    //right edge
    if (gap.rightHypo) {
        scoreBigram(gapPhrase.GetWord(i),gap.rightHypo->GetTargetPhrase().GetWord(0), m_scores);
    }
    /*cerr << gapPhrase << " ";
    for (size_t i = 0; i < m_scores.size(); ++i) {
        cerr << m_scores[i] << " ";
    }
    cerr << endl;*/
    
}

/** Score due to one segment */
void DiscriminativeLMBigramFeature::doSingleUpdate
        (const TranslationOption* option, const TargetGap& gap, ScoreComponentCollection& scores) 
{
    m_scores.assign(m_numValues,0);
    doUpdate(option->GetTargetPhrase(),gap);
    scores.Assign(&getScoreProducer(),m_scores);
}

/** Score due to two segments. The left and right refer to the target positions.**/
void DiscriminativeLMBigramFeature::doContiguousPairedUpdate
        (const TranslationOption* leftOption,const TranslationOption* rightOption,
            const TargetGap& gap, ScoreComponentCollection& scores)
{
    m_scores.assign(m_numValues,0);
    Phrase gapPhrase(leftOption->GetTargetPhrase());
    gapPhrase.Append(rightOption->GetTargetPhrase());
    doUpdate(gapPhrase,gap);
    scores.Assign(&getScoreProducer(),m_scores);
}

void DiscriminativeLMBigramFeature::doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap, ScoreComponentCollection& scores)
{
    m_scores.assign(m_numValues,0);
    doUpdate(leftOption->GetTargetPhrase(), leftGap);
    doUpdate(rightOption->GetTargetPhrase(), rightGap);
    scores.Assign(&getScoreProducer(),m_scores);
}

/** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
void DiscriminativeLMBigramFeature::doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap, ScoreComponentCollection& scores) 
{
    m_scores.assign(m_numValues,0);
    if (leftGap.segment.GetEndPos()+1 == rightGap.segment.GetStartPos()) {
        //contiguous
        Phrase gapPhrase(leftOption->GetTargetPhrase());
        gapPhrase.Append(rightOption->GetTargetPhrase());
        TargetGap gap(leftGap.leftHypo, rightGap.rightHypo, 
                      WordsRange(leftGap.segment.GetStartPos(), rightGap.segment.GetEndPos()));
        doUpdate(gapPhrase,gap);
    } else {
        //discontiguous
        doUpdate(leftOption->GetTargetPhrase(), leftGap);
        doUpdate(rightOption->GetTargetPhrase(), rightGap);
    }
    scores.Assign(&getScoreProducer(),m_scores);
}

}
