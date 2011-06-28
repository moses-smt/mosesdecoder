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
    void PosProjectionFeatureFunction::updateTarget() {
        m_tagProjection.clear();
        for (const Hypothesis* currHypo = getSample().GetTargetTail()->GetNextHypo();
                currHypo != NULL; currHypo = currHypo->GetNextHypo()) {
            const Phrase* sourcePhrase = currHypo->GetSourcePhrase();
            for (Phrase::const_iterator i = sourcePhrase->begin();
                i != sourcePhrase->end(); ++i) {
                m_tagProjection.push_back(i->GetFactor(m_sourceFactorType));
                //cerr << " " << i->GetFactor(0)->GetString();
            }
        }
        
    }
    
    PosProjectionBigramFeature::PosProjectionBigramFeature(Moses::FactorType sourceFactorType,const std::string& tags):
        m_sourceFactorType(sourceFactorType)
    {
      if (tags != "*") {
        vector<string> tagList = Tokenize(tags, ",");
        copy(tagList.begin(), tagList.end(), inserter(m_tags, m_tags.end()));
        VERBOSE(1, "PosProjectionBigramFeature configured with " << m_tags.size() << " tags" << endl);
      } else {
        VERBOSE(1, "PosProjectionBigramFeature will consider all tags" << endl);
      }
    }
        
    FeatureFunctionHandle PosProjectionBigramFeature::getFunction( const Sample& sample ) const {
          return FeatureFunctionHandle(new PosProjectionBigramFeatureFunction(sample, m_sourceFactorType, m_tags));
    }
    
    const string PosProjectionBigramFeatureFunction::ROOT = "ppf";
    
     PosProjectionBigramFeatureFunction::PosProjectionBigramFeatureFunction
          (const Sample& sample, Moses::FactorType sourceFactorType,const set<string>& tags) :
         PosProjectionFeatureFunction(sample, sourceFactorType), m_tags(tags){}

    void PosProjectionBigramFeatureFunction::countBigrams
        (const TagSequence& tagSequence, FVector& counts) {
        //cerr << "Tag bigrams ";
        for (TagSequence::const_iterator tagIter = tagSequence.begin();
            tagIter+1 != tagSequence.end(); ++tagIter) {
            const string& currTag = (*tagIter)->GetString();
            if (m_tags.size() && m_tags.find(currTag) == m_tags.end()) continue;
            const string& nextTag = (*(tagIter+1))->GetString();
            if (m_tags.size() && m_tags.find(nextTag) == m_tags.end()) continue;
            FName name(ROOT, currTag + ":" + nextTag);
            ++counts[name];
        }
        //cerr << endl;
    }

    /** Assign the total score of this feature on the current hypo */
    void PosProjectionBigramFeatureFunction::assignScore(FVector& scores) {
        countBigrams(getCurrentTagProjection(),scores);
    }

    /** Score due to one segment */
    void PosProjectionBigramFeatureFunction::doSingleUpdate(const TranslationOption* option, const TargetGap& gap, FVector& scores) 
    {
        //no change
    }
    /** Score due to two segments. The left and right refer to the target positions.**/
    void PosProjectionBigramFeatureFunction::doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                          const TargetGap& gap, FVector& scores) 
    {
        //no change
    }

    void PosProjectionBigramFeatureFunction::doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
            const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) 
    {
        //no change
    }

    /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
    void PosProjectionBigramFeatureFunction::doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                              const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) 
    { 
        bool contiguous = (leftGap.segment.GetEndPos()+1 == rightGap.segment.GetStartPos());
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
        countBigrams(tagProjection,scores);
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
            countBigrams(tagProjection,scores);
        }
    }



}

