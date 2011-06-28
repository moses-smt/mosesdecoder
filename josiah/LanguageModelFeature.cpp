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

#include "LanguageModelFeature.h"

#include <vector>

#include "Gibbler.h"

using namespace Moses;
using namespace std;

namespace Josiah {
  
  LanguageModelFeature::LanguageModelFeature(const Moses::LanguageModel* lmodel) :
      m_lmodel(lmodel) {}
  
  FeatureFunctionHandle LanguageModelFeature::getFunction(const Sample& sample) const {
    return FeatureFunctionHandle(new LanguageModelFeatureFunction(sample, m_lmodel));
  }
  
  LanguageModelFeatureFunction::LanguageModelFeatureFunction
      (const Sample& sample, const LanguageModel* lmodel):
      SingleValuedFeatureFunction(sample,lmodel->GetScoreProducerDescription()),
  m_lmodel(lmodel) {}
  
  
  
  /** Compute total score for sentence */
  FValue LanguageModelFeatureFunction::computeScore() {
    FValue score = 0;
    size_t order = m_lmodel->GetNGramOrder();
    vector<const Word*> lmcontext;
    const vector<Word>& target = getSample().GetTargetWords();
    lmcontext.reserve(target.size() + 2*(order-1));
    for (size_t i = 0; i < order-1; ++i) {
      lmcontext.push_back(&(m_lmodel->GetImplementation()->GetSentenceStartArray()));
    }
    for (size_t i = 0; i < target.size(); ++i) {
      lmcontext.push_back(&(target[i]));
    }
    lmcontext.push_back(&(m_lmodel->GetImplementation()->GetSentenceEndArray()));
    
    vector<const Word*> ngram(order);
    for (size_t ngramstart = 0; ngramstart < lmcontext.size() - (order -1); ++ngramstart) {
      size_t ngramCtr = 0;
      for (size_t j = ngramstart; j < ngramstart+order; ++j) {
        ngram[ngramCtr++] = lmcontext[j];
      }
      score += GetValue(ngram);
    }
    
    return score;
  }
  
  /** Score due to one segment */
  FValue LanguageModelFeatureFunction::getSingleUpdateScore(const Moses::TranslationOption* option, const TargetGap& gap) {
    size_t order = m_lmodel->GetNGramOrder();
    const TargetPhrase& targetPhrase = option->GetTargetPhrase();
    vector<const Word*> lmcontext;
    lmcontext.reserve(targetPhrase.GetSize() + 2*(order-1));
      
    int start = gap.segment.GetStartPos() - (order-1);
      
    //fill in the pre-context
    for (size_t i = 0; i < order-1; ++i) {
      if (start+(int)i < 0) {
        lmcontext.push_back(&(m_lmodel->GetImplementation()->GetSentenceStartArray()));
      } else {
        lmcontext.push_back(&(getSample().GetTargetWords()[i+start]));
      }
    }
      
    size_t startOption = lmcontext.size();
    //fill in the target phrase
    for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
      lmcontext.push_back(&(targetPhrase.GetWord(i)));
    }
    size_t endOption = lmcontext.size();
      
    //fill in the postcontext
    for (size_t i = 0; i < order-1; ++i) {
      size_t targetPos = i + gap.segment.GetEndPos() + 1;
      if (targetPos >= getSample().GetTargetWords().size()) {
        if (targetPos == getSample().GetTargetWords().size()) {
          lmcontext.push_back(&(m_lmodel->GetImplementation()->GetSentenceEndArray()));
        }
      } else {
        lmcontext.push_back(&(getSample().GetTargetWords()[targetPos]));
      }
    }
      
    //debug
    IFVERBOSE(3) {
      VERBOSE(3,"Segment: " << gap.segment << " phrase: " << option->GetTargetPhrase() << endl);
      VERBOSE(3,"LM context ");
      for (size_t j = 0;  j < lmcontext.size(); ++j) {
        VERBOSE(3,*(lmcontext[j]) << " ");
      }
      VERBOSE(3,endl);
    }
      
    //score lm
    FValue lmscore = 0;
    vector<const Word*> ngram(order);
    bool useOptionCachedLMScore = false;
    size_t ngramCtr;
    
    for (size_t ngramstart = 0; ngramstart < lmcontext.size() - (order -1); ++ngramstart) {
      if (ngramstart >= startOption && ngramstart + order - 1 < endOption) {
        useOptionCachedLMScore = true;
      }  
      else {
        ngramCtr = 0;
        for (size_t j = ngramstart; j < ngramstart+order; ++j) {
          ngram[ngramCtr++] = lmcontext[j];
        }
        lmscore += GetValue(ngram);
      }  
    }
    if (useOptionCachedLMScore) {
      const ScoreComponentCollection& sc = option->GetScoreBreakdown();
      lmscore += sc.GetScoreForProducer(m_lmodel);  
    }
    VERBOSE(2,"Language model score: " << lmscore << endl); 
    return lmscore;
  }
  
  /** Score due to two segments. The left and right refer to the target positions.**/
  FValue LanguageModelFeatureFunction::getContiguousPairedUpdateScore(const TranslationOption* leftOption,
      const TranslationOption* rightOption, const TargetGap& gap) {
        
        //Create the whole segment
        const WordsRange& targetSegment = gap.segment;
    
        //create the phrase
        size_t lsize = leftOption->GetTargetPhrase().GetSize();
        size_t rsize = rightOption->GetTargetPhrase().GetSize();
        vector<const Word*> targetPhrase(lsize+rsize);
        size_t i = 0; 
        for (size_t j = 0; j < lsize; ++j, ++i) {
          targetPhrase[i] = &(leftOption->GetTargetPhrase().GetWord(j));
        }
        for (size_t j = 0; j < rsize; ++j, ++i) {
          targetPhrase[i] = &(rightOption->GetTargetPhrase().GetWord(j));
        } 
        
    
        //set the indices for start and end positions
        size_t leftStartPos(0);
        size_t leftEndPos(leftOption->GetTargetPhrase().GetSize()); 
        size_t rightStartPos(leftEndPos);
        size_t rightEndPos(targetPhrase.size());
    
       
        size_t order = m_lmodel->GetNGramOrder();
        vector<const Word*> lmcontext;
        lmcontext.reserve(targetPhrase.size() + 2*(order-1));
    
        int start = targetSegment.GetStartPos() - (order-1);
    
        //fill in the pre-context
        for (size_t i = 0; i < order-1; ++i) {
          if (start+(int)i < 0) {
            lmcontext.push_back(&(m_lmodel->GetImplementation()->GetSentenceStartArray()));
          } else {
            lmcontext.push_back(&(getSample().GetTargetWords()[i+start]));
          }
        }
    
        //Offset the indices by pre-context size
        leftStartPos += lmcontext.size();
        leftEndPos += lmcontext.size();
        rightStartPos += lmcontext.size();
        rightEndPos += lmcontext.size();
    
        //fill in the target phrase
        for (size_t i = 0; i < targetPhrase.size(); ++i) {
          lmcontext.push_back(targetPhrase[i]);
        }
    
        //fill in the postcontext
        for (size_t i = 0; i < order-1; ++i) {
          size_t targetPos = i + targetSegment.GetEndPos() + 1;
          if (targetPos >= getSample().GetTargetWords().size()) {
            if (targetPos == getSample().GetTargetWords().size()) {
              lmcontext.push_back(&(m_lmodel->GetImplementation()->GetSentenceEndArray()));
            }
          } else {
            lmcontext.push_back(&(getSample().GetTargetWords()[targetPos]));
          }
        }
    
        //debug
        IFVERBOSE(3) {
          VERBOSE(3,"Segment: " << targetSegment << /*" phrase: " << targetPhrase << */endl);
          VERBOSE(3,"LM context ");
          for (size_t j = 0;  j < lmcontext.size(); ++j) {
            VERBOSE(3,*(lmcontext[j]) << " ");
          }
          VERBOSE(3,endl);
        }
    
        //score lm
        FValue lmscore = 0;
        vector<const Word*> ngram(order);
        bool useLeftOptionCacheLM(false), useRightOptionCacheLM(false) ;
        size_t ngramCtr;
        for (size_t ngramstart = 0; ngramstart < lmcontext.size() - (order -1); ++ngramstart) {
          if (ngramstart >= leftStartPos && ngramstart + order - 1 < leftEndPos) {
            useLeftOptionCacheLM = true;
          }
          else if (ngramstart >= rightStartPos && ngramstart + order - 1 < rightEndPos) {
            useRightOptionCacheLM = true;
          }
          else {
            ngramCtr = 0;
            for (size_t j = ngramstart; j < ngramstart+order; ++j) {
              ngram[ngramCtr++] = lmcontext[j];
            }
            lmscore += GetValue(ngram);
          }
        }
        if (useLeftOptionCacheLM) {
          const ScoreComponentCollection & sc = leftOption->GetScoreBreakdown();
          lmscore += sc.GetScoreForProducer(m_lmodel);
        }
        if (useRightOptionCacheLM) {
          const ScoreComponentCollection & sc = rightOption->GetScoreBreakdown();
          lmscore += sc.GetScoreForProducer(m_lmodel);
        }
    
        VERBOSE(2,"Language model score: " << lmscore << endl);
             
        return lmscore;
        
   }
   
   FValue LanguageModelFeatureFunction::getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
       const TargetGap& leftGap, const TargetGap& rightGap) 
   {
     const Phrase& leftTgtPhrase = leftOption->GetTargetPhrase();
     const Phrase& rightTgtPhrase = rightOption->GetTargetPhrase();
     size_t order = m_lmodel->GetNGramOrder();
     vector<const Word*> lmcontext;
     lmcontext.reserve(max(leftTgtPhrase.GetSize(), rightTgtPhrase.GetSize()) + 2*(order-1));
      
     int start = leftGap.segment.GetStartPos() - (order-1);
      
      //fill in the pre-context
     for (size_t i = 0; i < order-1; ++i) {
       if (start+(int)i < 0) {
         lmcontext.push_back(&(m_lmodel->GetImplementation()->GetSentenceStartArray()));
       } else {
         lmcontext.push_back(&(getSample().GetTargetWords()[i+start]));
       }
     }
      
     size_t leftStartPos(lmcontext.size()); // to track option's cached LM Score
      
      //fill in the target phrase
     for (size_t i = 0; i < leftTgtPhrase.GetSize(); ++i) {
       lmcontext.push_back(&(leftTgtPhrase.GetWord(i)));
     }
      
      // to track option's cached LM Score
     size_t leftEndPos(lmcontext.size());      
     size_t rightStartPos(0), rightEndPos(0);
      
      
      //fill in the postcontext needed for leftmost phrase
      //First get words from phrases in between, then from right phrase, then words past right phrase, then end of sentence
     size_t gapSize = rightGap.segment.GetStartPos() - leftGap.segment.GetEndPos() - 1;
     size_t leftSegmentEndPos = leftGap.segment.GetEndPos();
      
     for (size_t i = 0; i < order - 1; i++) {
       int rightOffset = i - gapSize;
       if (rightOffset < 0) {
         lmcontext.push_back(&(getSample().GetTargetWords()[leftSegmentEndPos + i + 1]));    
       }
       else if (rightOffset < (int)rightTgtPhrase.GetSize() ) {
         if (rightOffset == 0) {
           rightStartPos = lmcontext.size();
         }
         lmcontext.push_back(&(rightTgtPhrase.GetWord(rightOffset)));
         rightEndPos = lmcontext.size();
       }
       else if (rightOffset - rightTgtPhrase.GetSize() + rightGap.segment.GetEndPos() + 1 < getSample().GetTargetWords().size() ) {
         lmcontext.push_back(&(getSample().GetTargetWords()[(rightOffset - rightTgtPhrase.GetSize() + rightGap.segment.GetEndPos()  + 1)]));  
       }
       else {
         lmcontext.push_back(&(m_lmodel->GetImplementation()->GetSentenceEndArray()));
         break;
       }
     }
      
      
     VERBOSE(3,"Left LM Context : "); 
     for (size_t i = 0; i < lmcontext.size(); i++) {
       VERBOSE(3,*lmcontext[i] << " ");
     }
     VERBOSE(3, endl);
      
      //score lm
     FValue lmscore = 0;
     vector<const Word*> ngram(order);
     size_t ngramCtr;
     bool useLeftOptionCacheLM(false), useRightOptionCacheLM(false) ;
      
     for (size_t ngramstart = 0; ngramstart < lmcontext.size() - (order -1); ++ngramstart) {
       if (ngramstart >= leftStartPos && ngramstart + order - 1 < leftEndPos) {
         useLeftOptionCacheLM = true;
         VERBOSE(3, "In flip, Left LM Context, Using cached option LM score for left Option: " << leftOption->GetTargetPhrase() << endl;)
       }  
       else if (ngramstart >= rightStartPos && ngramstart + order - 1 < rightEndPos) {
         useRightOptionCacheLM = true;
         VERBOSE(3, "In flip, Left LM Context, Using cached option LM score for right Option: " << rightOption->GetTargetPhrase() << endl;)
       }
       else {
         ngramCtr =0;
         for (size_t j = ngramstart; j < ngramstart+order; ++j) {
           ngram[ngramCtr++] =lmcontext[j];
         }
         lmscore += GetValue(ngram);
       }  
     }
      
     if (useLeftOptionCacheLM) {
       const ScoreComponentCollection & sc = leftOption->GetScoreBreakdown();
       lmscore += sc.GetScoreForProducer(m_lmodel);
     }
      
     VERBOSE(2,"Left option Language model score: " << lmscore << endl); 
      
      //Now for the right target phrase
     lmcontext.clear();
      //Reset the indices
     leftStartPos = 0;
     leftEndPos = 0;
     rightStartPos = 0;
     rightEndPos = 0;
      
      //Fill in the pre-context
     size_t i = 0;
     if (order <= gapSize) { //no risk of ngram overlaps with left phrase post context
       i = order -1;
     }
     else {//how far back can we go
       i = gapSize;
     }
      
     size_t leftOffset = gapSize + leftTgtPhrase.GetSize();
      
     for ( ; i > 0 ; --i) {
       if (i > leftOffset + leftGap.segment.GetStartPos()) {                      
         lmcontext.push_back(&(m_lmodel->GetImplementation()->GetSentenceStartArray()));
       }
       else if (i > leftOffset) {
         lmcontext.push_back(&(getSample().GetTargetWords()[leftOffset - i + leftGap.segment.GetStartPos() ]));
       }                      
       else if ( i > gapSize) {
         if (i - gapSize == 1){
           leftStartPos = lmcontext.size();
         }
         lmcontext.push_back(&(leftTgtPhrase.GetWord(leftOffset - i)));
         leftEndPos = lmcontext.size();
       }
       else {
         lmcontext.push_back(&(getSample().GetTargetWords()[leftGap.segment.GetEndPos() + gapSize - i + 1 ]));
       }
     }  
      
      //Fill in right target phrase
     rightStartPos = lmcontext.size();
      
      //fill in the target phrase
     for (size_t i = 0; i < rightTgtPhrase.GetSize(); ++i) {
       lmcontext.push_back(&(rightTgtPhrase.GetWord(i)));
     }
      
     rightEndPos = lmcontext.size();      
      
      //Fill in post context
     for (size_t i = 0; i < order-1; ++i) {
       if ( i + rightGap.segment.GetEndPos() + 1 < getSample().GetTargetWords().size() ) {
         lmcontext.push_back(&(getSample().GetTargetWords()[i + rightGap.segment.GetEndPos() + 1]));         
       }
       else { 
         lmcontext.push_back(&(m_lmodel->GetImplementation()->GetSentenceEndArray()));
         break;
       }
     }  
      
     VERBOSE(3,"Right LM Context : "); 
     for (size_t i = 0; i < lmcontext.size(); i++) {
       VERBOSE(3,*lmcontext[i] << " ");
     }
     VERBOSE(3, endl);
      
     useRightOptionCacheLM = false;
      
     if ((int) lmcontext.size() - (int) (order -1) < 0 ) {//The left LM context completely subsumes the right LM Context, we're done
       VERBOSE(2,"Language model score: " << lmscore << endl); 
       return lmscore;
     }
      
     size_t maxNgram = lmcontext.size() - (order -1);

     for (size_t ngramstart = 0; ngramstart < maxNgram; ++ngramstart) {
       if (ngramstart >= leftStartPos && ngramstart + order - 1 < leftEndPos) {
         useLeftOptionCacheLM = true;
         VERBOSE(3, "In flip, Right LM Context, Using cached option LM score for left Option: " << leftOption->GetTargetPhrase() << endl;)
       }  
       if (ngramstart >= rightStartPos && ngramstart + order - 1 < rightEndPos) {
         useRightOptionCacheLM = true;
         VERBOSE(3, "In flip, Right LM Context, Using cached option LM score for right Option: " << rightOption->GetTargetPhrase() << endl;)
       }
       else {
         ngramCtr = 0;
         for (size_t j = ngramstart; j < ngramstart+order; ++j) {
           ngram[ngramCtr++] = lmcontext[j]; 
         }
         lmscore += GetValue(ngram);
       }  
     }
      
     if (useRightOptionCacheLM) {
       const ScoreComponentCollection & sc = rightOption->GetScoreBreakdown();
       lmscore += sc.GetScoreForProducer(m_lmodel);
     }
      
      
     VERBOSE(2,"Language model score: " << lmscore << endl); 
     return lmscore;  
   }
   
   /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
   FValue LanguageModelFeatureFunction::getFlipUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                     const TargetGap& leftGap, const TargetGap& rightGap)
   {
     
     bool contiguous =  (leftGap.segment.GetEndPos() + 1 ==  rightGap.segment.GetStartPos()) ;
    
     if (contiguous) {
       WordsRange segment(leftGap.segment.GetStartPos(), rightGap.segment.GetEndPos());
       TargetGap gap(leftGap.leftHypo, rightGap.rightHypo, segment);
       return getContiguousPairedUpdateScore(leftOption, rightOption, gap);
     } else {
       return getDiscontiguousPairedUpdateScore(leftOption, rightOption, leftGap, rightGap);
     }
     
   }
  
  float LanguageModelFeatureFunction::GetValue(const std::vector<const Word*>& context) {
    auto_ptr<FFState> state(m_lmodel->GetImplementation()->NewState());
    return m_lmodel->GetImplementation()->GetValueForgotState(context,*state.get());
  }
  
}

