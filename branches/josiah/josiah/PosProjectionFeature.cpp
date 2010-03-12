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

#include "PosProjectionFeature.h"

using namespace std;
using namespace Moses;

namespace Josiah {
    void PosProjectionFeature::updateTarget() {
        m_tagProjection.clear();
        //step through the hypotheses in target order,
        //extracting the pos tags from the source
        //cerr << "PPF update:";
        for (const Hypothesis* currHypo = m_sample->GetTargetTail()->GetNextHypo();
                currHypo != NULL; currHypo = currHypo->GetNextHypo()) {
            const Phrase* sourcePhrase = currHypo->GetSourcePhrase();
            for (Phrase::const_iterator i = sourcePhrase->begin();
                i != sourcePhrase->end(); ++i) {
                m_tagProjection.push_back(i->GetFactor(m_sourceFactorType));
                //cerr << " " << i->GetFactor(0)->GetString();
            }
        }
        //cerr << endl;
        //error checking
        /*
        ScoreComponentCollection expectedScores;
        assignScore(expectedScores);
        ScoreComponentCollection actualScores = 
            m_sample->GetFeatureValues();
        bool error = false;
        for (size_t i = 0; i < m_numValues; ++i) {
            if (actualScores[i] != expectedScores[i]) {
                error = true;
                break;
            }
        }
        if (error) {
            cerr << "ERROR" << endl;
            cerr << "EXPECTED: " << expectedScores << endl;
            cerr << "ACTUAL: " << actualScores << endl;
        }*/
    }

    void PosProjectionBigramFeature::countBigrams
        (const TagSequence& tagSequence, std::vector<float>& counts) {
        //cerr << "Tag bigrams ";
        for (TagSequence::const_iterator tagIter = tagSequence.begin();
            tagIter+1 != tagSequence.end(); ++tagIter) {
            const string& currTag = (*tagIter)->GetString();
            const string& nextTag = (*(tagIter+1))->GetString();
            map<string,size_t>::const_iterator currTagId = 
                m_tags.find(currTag);
            if (currTagId == m_tags.end()) continue;
            map<string,size_t>::const_iterator nextTagId = 
                m_tags.find(nextTag);
            if (nextTagId == m_tags.end()) continue;
            //cerr << currTag << "-" << nextTag << " ";
            size_t index = m_tags.size() * currTagId->second + nextTagId->second;
            ++counts[index];
        }
        //cerr << endl;
    }

    /** Assign the total score of this feature on the current hypo */
    void PosProjectionBigramFeature::assignScore(ScoreComponentCollection& scores) {
        m_scores.assign(numValues(),0);
        countBigrams(getCurrentTagProjection(),m_scores);
        scores.Assign(&getScoreProducer(), m_scores);
    }

    /** Score due to one segment */
    void PosProjectionBigramFeature::doSingleUpdate(const TranslationOption* option, const TargetGap& gap, ScoreComponentCollection& scores) 
    {
        //no change
    }
    /** Score due to two segments. The left and right refer to the target positions.**/
    void PosProjectionBigramFeature::doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                          const TargetGap& gap, ScoreComponentCollection& scores) 
    {
        //no change
    }

    void PosProjectionBigramFeature::doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
            const TargetGap& leftGap, const TargetGap& rightGap, ScoreComponentCollection& scores) 
    {
        //no change
    }

    /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
    void PosProjectionBigramFeature::doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                              const TargetGap& leftGap, const TargetGap& rightGap, ScoreComponentCollection& scores) 
    { 
        bool contiguous = (leftGap.segment.GetEndPos()+1 == rightGap.segment.GetStartPos());
        m_scores.assign(numValues(),0);
        //changes the projection, so recalculate
        TagSequence tagProjection;
        if (leftGap.leftHypo->GetPrevHypo()) {
            //include word to left of gap
            const Phrase* leftPhrase = leftGap.leftHypo->GetSourcePhrase();
            tagProjection.push_back(leftPhrase->GetWord(leftPhrase->GetSize()-1).GetFactor(sourceFactorType()));
        }
        //include words to go in left gap
        for (Phrase::const_iterator i = leftOption->GetSourcePhrase()->begin();
            i != leftOption->GetSourcePhrase()->end(); ++i) {
            tagProjection.push_back(i->GetFactor(sourceFactorType()));
        }
        if (contiguous) {
            //include words to go in right gap
            for (Phrase::const_iterator i = rightOption->GetSourcePhrase()->begin();
                i != rightOption->GetSourcePhrase()->end(); ++i) {
                tagProjection.push_back(i->GetFactor(sourceFactorType()));
            }
            //and word to right of gap
            if (rightGap.rightHypo) {
                const Phrase* rightPhrase = rightGap.rightHypo->GetSourcePhrase();
                tagProjection.push_back(rightPhrase->GetWord(0).GetFactor(sourceFactorType()));
            }
        } else {
            //word to right of left gap
            const Phrase* rightPhrase = leftGap.rightHypo->GetSourcePhrase();
            tagProjection.push_back(rightPhrase->GetWord(0).GetFactor(sourceFactorType()));
        }
        countBigrams(tagProjection,m_scores);
        if (!contiguous) {
            //right gap
            tagProjection.clear();
            //word to the left
            const Phrase* leftPhrase = rightGap.leftHypo->GetSourcePhrase();
            tagProjection.push_back(leftPhrase->GetWord(leftPhrase->GetSize()-1).GetFactor(sourceFactorType()));
            //words to go in right gap
            for (Phrase::const_iterator i = rightOption->GetSourcePhrase()->begin();
                i != rightOption->GetSourcePhrase()->end(); ++i) {
                tagProjection.push_back(i->GetFactor(sourceFactorType()));
            }
            if (rightGap.rightHypo) {
                const Phrase* rightPhrase = rightGap.rightHypo->GetSourcePhrase();
                tagProjection.push_back(rightPhrase->GetWord(0).GetFactor(sourceFactorType()));
            }
            countBigrams(tagProjection,m_scores);
        }
        scores.Assign(&getScoreProducer(),m_scores);
    }



}

