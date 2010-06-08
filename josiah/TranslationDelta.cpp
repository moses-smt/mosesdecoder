/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

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

#include <boost/lambda/lambda.hpp>
#include "TranslationDelta.h"
#include "Derivation.h"
#include "FeatureFunction.h"
#include "Gibbler.h"
#include "SentenceBleu.h"
#include "ScoreComponentCollection.h"
#include "DummyScoreProducers.h"
#include "GibbsOperator.h"

using namespace std;

namespace Josiah {
  
  
void TranslationDelta::calcSufficientStatsAndGain(const vector<const Factor*> & sentence){
  BleuSufficientStats stats(4);
  m_gf->GetSufficientStats(sentence,&stats);
  m_gain = static_cast<SentenceBLEU*>(const_cast<GainFunction*>(m_gf))->CalcBleu(stats, *(static_cast<BleuSufficientStats*>(static_cast<SentenceBLEU*>(const_cast<GainFunction*>(m_gf))->GetCurrentSmoothing())));
  m_sufficientStats = stats;
  
   IFVERBOSE(2) {
    //cerr << "Sentence: "<< sentence << endl;
    cerr << "Gain " << m_gain << endl;
  }  
}

  /**
   Compute the change in language model score by adding this target phrase
   into the hypothesis at the given target position.
 **/
void  TranslationDelta::addSingleOptionLanguageModelScore(const TranslationOption* option, const WordsRange& targetSegment) {
  const Phrase& targetPhrase = option->GetTargetPhrase();
  
  if (m_gf) {
    vector<const Factor*> newSentence;
    size_t start = targetSegment.GetStartPos();
    for (size_t i = 0; i < start; ++i) {
      const Factor* factor =getSample().GetTargetWords()[i][0]; 
      newSentence.push_back(factor);
    }
    //fill in the target phrase
    for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
      newSentence.push_back(targetPhrase.GetWord(i)[0]);
    }
    //fill in the end of the sentence
    size_t end = targetSegment.GetEndPos() + 1;
    for (size_t i = end; i < getSample().GetTargetWords().size(); ++i) {
      newSentence.push_back(getSample().GetTargetWords()[i][0]);
    }
    
    calcSufficientStatsAndGain(newSentence);
  }
}
  
void TranslationDelta::initScoresSingleUpdate(const Sample& s, const TranslationOption* option, const TargetGap& gap) {
  //don't worry about reordering because they don't change
        
        
  addSingleOptionLanguageModelScore(option, gap.segment); //let's do this here itself for now
  
  // extra features
  typedef Josiah::feature_vector fv;
  for (fv::const_iterator i=s.extra_features().begin(); i<s.extra_features().end(); ++i) {
    (*i)->doSingleUpdate(option,gap,m_scores);
  }

  updateWeightedScore();
  
  VERBOSE(2, "Single Update: Scores " << m_scores << endl);
  VERBOSE(2,"Single Update: Total score is  " << m_score << endl);  
}

  
//Note that left and right refer to the target order.
void TranslationDelta::initScoresContiguousPairedUpdate(const Sample& s, const TranslationOption* leftOption,
                                              const TranslationOption* rightOption, const TargetGap& gap) {
  
    
  //don't worry about reordering because they don't change
    
    
    
  // extra features
  typedef Josiah::feature_vector fv;
  for (fv::const_iterator i=s.extra_features().begin(); i<s.extra_features().end(); ++i) {
    (*i)->doContiguousPairedUpdate(leftOption,rightOption,gap,m_scores);
  }
}

void TranslationDelta::initScoresDiscontiguousPairedUpdate(const Sample& s, const TranslationOption* leftOption,
                                              const TranslationOption* rightOption, const TargetGap& leftGap,
                                              const TargetGap& rightGap) {
 
  
  
  //don't worry about reordering because they don't change

  
   // extra features
  typedef Josiah::feature_vector fv;
  for (fv::const_iterator i=s.extra_features().begin(); i<s.extra_features().end(); ++i) {
    (*i)->doDiscontiguousPairedUpdate(leftOption, rightOption, leftGap, rightGap,m_scores);
   }

}
  
  void  TranslationDelta::addContiguousPairedOptionLMScore(const TranslationOption* leftOption, const TranslationOption* rightOption, const WordsRange* leftSegment, const WordsRange* rightTargetSegment) {
    //Create the segment
    WordsRange targetSegment = *leftSegment;
    targetSegment.SetEndPos(rightTargetSegment->GetEndPos());
    
    //create the phrase
    Phrase targetPhrase(leftOption->GetTargetPhrase());
    targetPhrase.Append(rightOption->GetTargetPhrase());
    
    //set the indices for start and end positions
    size_t leftStartPos(0);
    size_t leftEndPos(leftOption->GetTargetPhrase().GetSize()); 
    size_t rightStartPos(leftEndPos);
    size_t rightEndPos(targetPhrase.GetSize());
    
    if (m_gf) {
      vector<const Factor*> newSentence;
      size_t start = targetSegment.GetStartPos();
      for (size_t i = 0; i < start; ++i) {
        newSentence.push_back(getSample().GetTargetWords()[i][0]);
      }
      //fill in the target phrase
      for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
        newSentence.push_back(targetPhrase.GetWord(i)[0]);
      }
      //fill in the end of the sentence
      size_t end = targetSegment.GetEndPos() + 1;
      for (size_t i = end; i < getSample().GetTargetWords().size(); ++i) {
        newSentence.push_back(getSample().GetTargetWords()[i][0]);
      }
      
      calcSufficientStatsAndGain(newSentence);
    }
  }
  
  
  void  TranslationDelta::addDiscontiguousPairedOptionLMScore(const TranslationOption* leftOption, const TranslationOption* rightOption, const WordsRange* leftSegment, const WordsRange* rightSegment) {
    const Phrase& leftTgtPhrase = leftOption->GetTargetPhrase();
    const Phrase& rightTgtPhrase = rightOption->GetTargetPhrase();
    
    VERBOSE(2, "Sample : " << Josiah::Derivation(m_sample) << endl);
    VERBOSE(2, *leftSegment << " " << *rightSegment << endl); 
    
    if (m_gf) {
      vector<const Factor*> newSentence;
      size_t start = leftSegment->GetStartPos();
      for (size_t i = 0; i < start; ++i) {
        newSentence.push_back(getSample().GetTargetWords()[i][0]);
      }
      //fill in the left target phrase
      for (size_t i = 0; i < leftTgtPhrase.GetSize(); ++i) {
        newSentence.push_back(leftTgtPhrase.GetWord(i)[0]);
      }
      for (size_t i = leftSegment->GetEndPos()+ 1 ; i < rightSegment->GetStartPos(); ++i) {
        newSentence.push_back(getSample().GetTargetWords()[i][0]);
      }
      //fill in the right target phrase
      for (size_t i = 0; i < rightTgtPhrase.GetSize(); ++i) {
        newSentence.push_back(rightTgtPhrase.GetWord(i)[0]);
      }
      //fill in the end of the sentence
      size_t end = rightSegment->GetEndPos() + 1;
      for (size_t i = end; i < getSample().GetTargetWords().size(); ++i) {
        newSentence.push_back(getSample().GetTargetWords()[i][0]);
      }
      
      calcSufficientStatsAndGain(newSentence);
      
    }
  }
  
  void  TranslationDelta::addPairedOptionLanguageModelScore(const TranslationOption* leftOption, const TranslationOption* rightOption, const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment) {
    
    WordsRange* leftSegment = const_cast<WordsRange*> (&leftTargetSegment);
    WordsRange* rightSegment = const_cast<WordsRange*> (&rightTargetSegment);
    
    if (rightTargetSegment < leftTargetSegment) {
      leftSegment = const_cast<WordsRange*> (&rightTargetSegment);
      rightSegment = const_cast<WordsRange*>(&leftTargetSegment);
    }
    
    bool contiguous =  (leftSegment->GetEndPos() + 1 ==  rightSegment->GetStartPos()) ;
    
    if (contiguous)
      addContiguousPairedOptionLMScore(leftOption, rightOption, leftSegment, rightSegment);
    else
      addDiscontiguousPairedOptionLMScore(leftOption, rightOption, leftSegment, rightSegment);
    
    VERBOSE(2, "In POLMS, Left Target phrase: " << leftOption->GetTargetPhrase() << endl);
    VERBOSE(2, "In POLMS, Right Target phrase: " << rightOption->GetTargetPhrase() << endl);
    VERBOSE(2, "In POLMS, Left Target segment: " << leftTargetSegment << endl);    
    VERBOSE(2, "In POLMS, Right Target segment: " << rightTargetSegment << endl);
  }
  
  
void TranslationDelta::updateWeightedScore() {
  //weight the scores
  m_score = inner_product(m_scores, WeightManager::instance().get());
    
  VERBOSE(2, "Scores " << m_scores << endl);
  VERBOSE(2,"Total score is  " << m_score << endl);      
}
  
  
TranslationUpdateDelta::TranslationUpdateDelta(GibbsOperator* g_operator, Sample& sample, const TranslationOption* option ,const TargetGap& gap, const GainFunction* gf) :
    TranslationDelta(g_operator, sample, gf),  m_option(option), m_gap(gap) {
  initScoresSingleUpdate(m_sample, m_option,m_gap);
}

void TranslationUpdateDelta::apply(const TranslationDelta& noChangeDelta) {
  VERBOSE(3, "Applying Translation Update Delta" << endl);
  m_scores -= noChangeDelta.getScores();
  getSample().ChangeTarget(*m_option,m_scores);
}

TranslationUpdateDelta* TranslationUpdateDelta::Create() const {
  return new TranslationUpdateDelta(getOperator(), getSample(), getOption(), getGap(), getGainFunction());
}  

MergeDelta::MergeDelta(GibbsOperator* g_operator, Sample& sample, const TranslationOption* option, const TargetGap& gap, const GainFunction* gf) :
    TranslationDelta(g_operator, sample, gf),  m_option(option), m_gap(gap) {
  initScoresSingleUpdate(m_sample, m_option,m_gap);
}

void MergeDelta::apply(const TranslationDelta& noChangeDelta) {
  VERBOSE(3, "Applying MergeDelta" << endl);
  m_scores -= noChangeDelta.getScores();
  getSample().MergeTarget(*m_option,m_scores);
}

MergeDelta* MergeDelta::Create() const {
  return new MergeDelta(getOperator(), getSample(), getOption(), getGap(), getGainFunction());
}
  
PairedTranslationUpdateDelta::PairedTranslationUpdateDelta(GibbsOperator* g_operator, Sample& sample,
   const TranslationOption* leftOption, const TranslationOption* rightOption, 
   const TargetGap& leftGap, const TargetGap& rightGap, const GainFunction* gf) : TranslationDelta(g_operator, sample, gf), m_leftOption(leftOption),
    m_rightOption(rightOption), m_leftGap(leftGap), m_rightGap(rightGap) {
   
  VERBOSE(2, "Left Target phrase: " << m_leftOption->GetTargetPhrase() << endl);
  VERBOSE(2, "Right Target phrase: " << m_rightOption->GetTargetPhrase() << endl);
  VERBOSE(2, "Left Target segment: " << m_leftGap.segment << endl);    
  VERBOSE(2, "Right Target segment: " << m_rightGap.segment << endl);
     
  assert(m_leftGap.segment < m_rightGap.segment);
  if (m_leftGap.segment.GetEndPos() + 1 == m_rightGap.segment.GetStartPos()) {
    TargetGap gap(m_leftGap.leftHypo, m_rightGap.rightHypo, 
      WordsRange(m_leftGap.segment.GetStartPos(), m_rightGap.segment.GetEndPos()));
    initScoresContiguousPairedUpdate(m_sample, m_leftOption,m_rightOption, gap);
  } else {
    initScoresDiscontiguousPairedUpdate(m_sample,m_leftOption,m_rightOption,m_leftGap,m_rightGap);
  }
  addPairedOptionLanguageModelScore(m_leftOption, m_rightOption, m_leftGap.segment, m_rightGap.segment);
  updateWeightedScore();
  VERBOSE(2, "Left Target segment: " << m_leftGap.segment << endl);    
  VERBOSE(2, "Right Target segment: " << m_rightGap.segment << endl);
}

PairedTranslationUpdateDelta* PairedTranslationUpdateDelta::Create() const {
  VERBOSE(2, "Left Target phrase: " << m_leftOption->GetTargetPhrase() << endl);
  VERBOSE(2, "Right Target phrase: " << m_rightOption->GetTargetPhrase() << endl);
  VERBOSE(2, "Left Target segment: " << getLeftGap().segment << endl);    
  VERBOSE(2, "Right Target segment: " << getRightGap().segment << endl); 
  PairedTranslationUpdateDelta *ret = new PairedTranslationUpdateDelta(getOperator(), getSample(), getLeftOption(), getRightOption(), getLeftGap(), getRightGap(), getGainFunction());    

  return ret;
}  

void PairedTranslationUpdateDelta::apply(const TranslationDelta& noChangeDelta) {
  VERBOSE(3, "Applying Paired  Translation Update Delta" << endl);
  m_scores -= noChangeDelta.getScores();
  getSample().ChangeTarget(*m_leftOption,m_scores);
  FVector emptyScores;
  getSample().ChangeTarget(*m_rightOption,emptyScores);
}

SplitDelta::SplitDelta(GibbsOperator* g_operator, Sample& sample, const TranslationOption* leftOption, 
                       const TranslationOption* rightOption, const TargetGap& gap, const GainFunction* gf) : TranslationDelta(g_operator, sample, gf),
    m_leftOption(leftOption), m_rightOption(rightOption), m_gap(gap){
  
  
  VERBOSE(2, "Target phrase: " << m_leftOption->GetTargetPhrase() << " " << m_rightOption->GetTargetPhrase() << endl);
  VERBOSE(2, "Target segment: " << m_gap.segment << endl);    
  
  initScoresContiguousPairedUpdate(m_sample, m_leftOption, m_rightOption, m_gap);        
  addContiguousPairedOptionLMScore(m_leftOption, m_rightOption, &(m_gap.segment), &(m_gap.segment));    
  updateWeightedScore();
}

SplitDelta* SplitDelta::Create() const {
  return new SplitDelta(getOperator(), getSample(), getLeftOption(), getRightOption(), getGap(), getGainFunction());    
}
  
void SplitDelta::apply(const TranslationDelta& noChangeDelta) {
  m_scores -= noChangeDelta.getScores();
  getSample().SplitTarget(*m_leftOption,*m_rightOption,m_scores);
}
  
void FlipDelta::apply(const TranslationDelta& noChangeDelta) {
  VERBOSE(3, "Applying Flip Delta" << endl);
  m_scores  -= noChangeDelta.getScores();
  getSample().FlipNodes(*m_leftTgtOption, *m_rightTgtOption, m_prevTgtHypo, m_nextTgtHypo, m_scores);
}

FlipDelta::FlipDelta(GibbsOperator* g_operator, Sample& sample, 
      const TranslationOption* leftTgtOption ,const TranslationOption* rightTgtOption,
      const TargetGap& leftGap, const TargetGap& rightGap,  const GainFunction* gf) :
  TranslationDelta(g_operator, sample, gf),
  m_leftTgtOption(leftTgtOption), m_rightTgtOption(rightTgtOption), m_leftGap(leftGap), m_rightGap(rightGap), 
      m_prevTgtHypo(const_cast<Hypothesis*> (leftGap.leftHypo)), m_nextTgtHypo(const_cast<Hypothesis*> (rightGap.rightHypo))
        {
    
    
    
  addPairedOptionLanguageModelScore(m_leftTgtOption, m_rightTgtOption, leftGap.segment, rightGap.segment);
    
    
  // extra features
  typedef Josiah::feature_vector fv;
  for (fv::const_iterator i=sample.extra_features().begin(); i<sample.extra_features().end(); ++i) {
    (*i)->doFlipUpdate(leftTgtOption, rightTgtOption, leftGap, rightGap,m_scores);
  }
    
  updateWeightedScore();
    
  VERBOSE(2, "Flip delta: Scores " << m_scores << endl);
  VERBOSE(2,"Flip delta: Total score is  " << m_score << endl);  
}  
  
FlipDelta* FlipDelta::Create() const {
  return new FlipDelta(getOperator(), getSample(), getLeftOption(), getRightOption(), getLeftGap(), getRightGap(), getGainFunction());    
}  

}//namespace
