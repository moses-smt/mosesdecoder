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
#include "FeatureFunction.h"
#include "Gibbler.h"

using namespace std;

namespace Moses {
  
#ifdef LM_CACHE  
  std::map<LanguageModel*,LanguageModelCache> TranslationDelta::m_cache;
  
  float LanguageModelCache::GetValue(const std::vector<const Word*>& ngram) {
    EntryListIterator* entryListIter = m_listPointers[ngram];
    float score;
    if (!entryListIter) {
      //cache miss
      if ((int)m_listPointers.size() >= m_maxSize) {
        //too many entries
        Entry lruEntry = m_entries.back();
        m_entries.pop_back();
        delete m_listPointers[lruEntry.first];
        m_listPointers.erase(lruEntry.first);
      }
      score = m_languageModel->GetValue(ngram);
      m_entries.push_front(Entry(ngram,score));
      entryListIter = new EntryListIterator();
      *entryListIter = m_entries.begin();
      m_listPointers[ngram] = entryListIter;
    } else {
      //cache hit
      Entry entry  = *(*entryListIter);
      m_entries.erase(*entryListIter);
      m_entries.push_front(entry);
      *entryListIter = m_entries.begin();
      score = entry.second;
    } 
    return score;
  }
#endif


  /**
   Compute the change in language model score by adding this target phrase
   into the hypothesis at the given target position.
 **/
void  TranslationDelta::addSingleOptionLanguageModelScore(const TranslationOption* option, const WordsRange& targetSegment) {
  const Phrase& targetPhrase = option->GetTargetPhrase();
  const LMList& languageModels = StaticData::Instance().GetAllLM();
  
  
  for (LMList::const_iterator i = languageModels.begin(); i != languageModels.end(); ++i) {
    LanguageModel* lm = *i;
    /*
     map<LanguageModel*,LanguageModelCache>::iterator lmci = m_cache.find(lm);
     if (lmci == m_cache.end()) {
     m_cache.insert(pair<LanguageModel*,LanguageModelCache>(lm,LanguageModelCache(lm)));
     }*/
    size_t order = lm->GetNGramOrder();
    vector<const Word*> lmcontext;
    lmcontext.reserve(targetPhrase.GetSize() + 2*(order-1));
      
    int start = targetSegment.GetStartPos() - (order-1);
      
    //fill in the pre-context
      
    for (size_t i = 0; i < order-1; ++i) {
      if (start+(int)i < 0) {
        //if (start + (int)i == -1) {
        lmcontext.push_back(&(lm->GetSentenceStartArray()));
        //}
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
      size_t targetPos = i + targetSegment.GetEndPos() + 1;
      if (targetPos >= getSample().GetTargetWords().size()) {
        if (targetPos == getSample().GetTargetWords().size()) {
          lmcontext.push_back(&(lm->GetSentenceEndArray()));
        }
      } else {
        lmcontext.push_back(&(getSample().GetTargetWords()[targetPos]));
      }
    }
      
    //debug
    IFVERBOSE(3) {
      VERBOSE(3,"Segment: " << targetSegment << " phrase: " << targetPhrase << endl);
      VERBOSE(3,"LM context ");
      for (size_t j = 0;  j < lmcontext.size(); ++j) {
        if (j == order-1) {
          //VERBOSE(3, "[ ");
        }
        if (j == (targetPhrase.GetSize() + (order-1))) {
          //VERBOSE(3,"] ");
        }
        VERBOSE(3,*(lmcontext[j]) << " ");
        
      }
      VERBOSE(3,endl);
    }
      
    //score lm
    double lmscore = 0;
    vector<const Word*> ngram(order);
    bool useOptionCachedLMScore = false;
    size_t ngramCtr;
    
    for (size_t ngramstart = 0; ngramstart < lmcontext.size() - (order -1); ++ngramstart) {
      if (ngramstart >= startOption && ngramstart + order - 1 < endOption) {
        useOptionCachedLMScore = true;
      }  
      else {
        //cerr << "ngram: ";
        ngramCtr = 0;
        for (size_t j = ngramstart; j < ngramstart+order; ++j) {
          ngram[ngramCtr++] = lmcontext[j];
          //cerr << *lmcontext[j] << " ";
        }
        lmscore += lm->GetValue(ngram);
        //cerr << lm->GetValue(ngram)/log(10) << endl;
        //cache disabled for now
        //lmscore += m_cache.find(lm)->second.GetValue(ngram);  
      }  
    }
    
    if (useOptionCachedLMScore) {
      const ScoreComponentCollection & sc = option->GetScoreBreakdown();
      lmscore += sc.GetScoreForProducer(lm);  
    }
    
    VERBOSE(2,"Language model score: " << lmscore << endl); 
    m_scores.Assign(lm,lmscore);   
  }
}
  
void TranslationDelta::initScoresSingleUpdate(const Sample& s, const TranslationOption* option, const WordsRange& targetSegment) {
  //translation scores
  m_scores.PlusEquals(option->GetScoreBreakdown());
        
  //don't worry about reordering because they don't change
        
  //word penalty
  float penalty = -((int)option->GetTargetPhrase().GetSize());
  m_scores.Assign(StaticData::Instance().GetWordPenaltyProducer(),penalty);
        
  addSingleOptionLanguageModelScore(option, targetSegment); //let's do this here itself for now
  
  // extra features
  _extra_feature_values.clear();
  typedef Josiah::feature_vector fv;
  for (fv::const_iterator i=s.extra_features().begin(); i<s.extra_features().end(); ++i) {
    float feature_score = (*i)->getSingleUpdateScore(s, option, targetSegment);
    m_scores.Assign((*i)->getScoreProducer(),feature_score);
  }

  //weight the scores
  const vector<float> & weights = StaticData::Instance().GetAllWeights();
  m_score = m_scores.InnerProduct(weights);
  
  VERBOSE(2, "Single Update: Scores " << m_scores << endl);
  IFVERBOSE(2){
    std::cerr << "Single Update: extra scores ";
    std::for_each(_extra_feature_values.begin(), _extra_feature_values.end(), std::cerr << boost::lambda::_1 << " ");
    std::cerr << std::endl; 
  }
  VERBOSE(2,"Single Update: Total score is  " << m_score << endl);  
}

void TranslationDelta::initScoresPairedUpdate(const Sample& s, const TranslationOption* leftOption,
      const TranslationOption* rightOption, const WordsRange& targetSegment, const Phrase& targetPhrase) {
  //translation scores
  m_scores.PlusEquals(leftOption->GetScoreBreakdown());
  m_scores.PlusEquals(rightOption->GetScoreBreakdown());
  
  //don't worry about reordering because they don't change
  
  //word penalty
  float penalty = -((int)leftOption->GetTargetPhrase().GetSize()) -((int)rightOption->GetTargetPhrase().GetSize());
  m_scores.Assign(StaticData::Instance().GetWordPenaltyProducer(),penalty);
  
  //addLanguageModelScore(targetPhrase, targetSegment);

  // extra features
  _extra_feature_values.clear();
  typedef Josiah::feature_vector fv;
  for (fv::const_iterator i=s.extra_features().begin(); i<s.extra_features().end(); ++i) {
    //_extra_feature_values.push_back((*i)->getPairedUpdateScore(s, leftOption, rightOption, targetSegment, targetPhrase));
    float feature_score = (*i)->getPairedUpdateScore(s, leftOption, rightOption, targetSegment, targetPhrase);
    m_scores.Assign((*i)->getScoreProducer(),feature_score);
  }

  //weight the scores
  const vector<float> & weights = StaticData::Instance().GetAllWeights();
  m_score = m_scores.InnerProduct(weights);

  VERBOSE(2, "Paired Update: Scores " << m_scores << endl);
  IFVERBOSE(2){
    std::cerr << "Single Update: extra scores ";
    std::for_each(_extra_feature_values.begin(), _extra_feature_values.end(), std::cerr << boost::lambda::_1 << " ");
    std::cerr << std::endl; 
  }
  VERBOSE(2,"Paired Update: Total score is  " << m_score << endl);  

}


TranslationUpdateDelta::TranslationUpdateDelta(Sample& sample, const TranslationOption* option ,const WordsRange& targetSegment) :
    TranslationDelta(sample),  m_option(option)  {
  initScoresSingleUpdate(sample, option,targetSegment);
}

void TranslationUpdateDelta::apply(const TranslationDelta& noChangeDelta) {
  VERBOSE(3, "Applying Translation Update Delta" << endl);
  m_scores.MinusEquals(noChangeDelta.getScores());
  getSample().ChangeTarget(*m_option,m_scores);
}


MergeDelta::MergeDelta(Sample& sample, const TranslationOption* option, const WordsRange& targetSegment) :
    TranslationDelta(sample),  m_option(option) {
  initScoresSingleUpdate(sample, option,targetSegment);
}

void MergeDelta::apply(const TranslationDelta& noChangeDelta) {
  VERBOSE(3, "Applying MergeDelta" << endl);
  m_scores.MinusEquals(noChangeDelta.getScores());
  getSample().MergeTarget(*m_option,m_scores);
}

PairedTranslationUpdateDelta::PairedTranslationUpdateDelta(Sample& sample,
   const TranslationOption* leftOption, const TranslationOption* rightOption, 
   const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment) : TranslationDelta(sample){
//    initScoresPairedUpdate(sample, leftOption,rightOption,targetSegment,*targetPhrase);
      
  if (leftTargetSegment < rightTargetSegment) {
    m_leftOption = leftOption;
    m_rightOption = rightOption;  
  }
  else {
    m_leftOption = rightOption;
    m_rightOption = leftOption;  
  }
  
  //translation scores
  m_scores.PlusEquals(m_leftOption->GetScoreBreakdown());
  m_scores.PlusEquals(m_rightOption->GetScoreBreakdown());
      
  //don't worry about reordering because they don't change
      
  //word penalty
  float penalty = -((int)m_leftOption->GetTargetPhrase().GetSize()) -((int)m_rightOption->GetTargetPhrase().GetSize());
  m_scores.Assign(StaticData::Instance().GetWordPenaltyProducer(),penalty);
      
  //addLanguageModelScore(targetPhrase, targetSegment);
  VERBOSE(2, "Left Target phrase: " << m_leftOption->GetTargetPhrase() << endl;)
  VERBOSE(2, "Right Target phrase: " << m_rightOption->GetTargetPhrase() << endl;)
  VERBOSE(2, "Left Target segment: " << leftTargetSegment << endl;)    
  VERBOSE(2, "Right Target segment: " << rightTargetSegment << endl;)    
  addPairedOptionLanguageModelScore(m_leftOption, m_rightOption, leftTargetSegment, rightTargetSegment);
      
  // extra features
  _extra_feature_values.clear();
  typedef Josiah::feature_vector fv;
  for (fv::const_iterator i=sample.extra_features().begin(); i<sample.extra_features().end(); ++i) {
    //_extra_feature_values.push_back((*i)->getPairedUpdateScore(s, leftOption, rightOption, targetSegment, targetPhrase));
    //float feature_score = (*i)->getPairedUpdateScore(sample, leftOption, rightOption, targetSegment, targetPhrase);
    float feature_score = (*i)->getPairedUpdateScore(sample, m_leftOption, m_rightOption, leftTargetSegment, rightTargetSegment);
    m_scores.Assign((*i)->getScoreProducer(),feature_score);
  }
      
   //weight the scores
  const vector<float> & weights = StaticData::Instance().GetAllWeights();
  m_score = m_scores.InnerProduct(weights);
      
  VERBOSE(2, "Paired Update: Scores " << m_scores << endl);
  IFVERBOSE(2){
     std::cerr << "Single Update: extra scores ";
     std::for_each(_extra_feature_values.begin(), _extra_feature_values.end(), std::cerr << boost::lambda::_1 << " ");
     std::cerr << std::endl; 
  }
  VERBOSE(2,"Paired Update: Total score is  " << m_score << endl);  
}

void PairedTranslationUpdateDelta::apply(const TranslationDelta& noChangeDelta) {
  VERBOSE(3, "Applying Paired  Translation Update Delta" << endl);
  m_scores.MinusEquals(noChangeDelta.getScores());
  getSample().ChangeTarget(*m_leftOption,m_scores);
  ScoreComponentCollection emptyScores;
  getSample().ChangeTarget(*m_rightOption,emptyScores);
}

SplitDelta::SplitDelta(Sample& sample, const TranslationOption* leftOption, 
                       const TranslationOption* rightOption, const WordsRange& targetSegment) : TranslationDelta(sample),
    m_leftOption(leftOption), m_rightOption(rightOption) {
  
  Phrase targetPhrase (leftOption->GetTargetPhrase());
  targetPhrase.Append(rightOption->GetTargetPhrase());    
  
  //initScoresPairedUpdate(sample, leftOption,rightOption,targetSegment,targetPhrase);
      
  //translation scores
  m_scores.PlusEquals(leftOption->GetScoreBreakdown());
  m_scores.PlusEquals(rightOption->GetScoreBreakdown());
      
  //don't worry about reordering because they don't change
      
  //word penalty
  float penalty = -((int)leftOption->GetTargetPhrase().GetSize()) -((int)rightOption->GetTargetPhrase().GetSize());
  m_scores.Assign(StaticData::Instance().GetWordPenaltyProducer(),penalty);
  
  VERBOSE(2, "Target phrase: " << targetPhrase << endl;)
  VERBOSE(2, "Target segment: " << targetSegment << endl;)    
  //addLanguageModelScore(targetPhrase, targetSegment);
  addContiguousPairedOptionLMScore(leftOption, rightOption, &targetSegment, &targetSegment);    
  // extra features
  _extra_feature_values.clear();
  typedef Josiah::feature_vector fv;
  for (fv::const_iterator i=sample.extra_features().begin(); i<sample.extra_features().end(); ++i) {
      //_extra_feature_values.push_back((*i)->getPairedUpdateScore(s, leftOption, rightOption, targetSegment, targetPhrase));
      float feature_score = (*i)->getPairedUpdateScore(sample, leftOption, rightOption, targetSegment, targetPhrase);
      m_scores.Assign((*i)->getScoreProducer(),feature_score);
  }
      
  //weight the scores
  const vector<float> & weights = StaticData::Instance().GetAllWeights();
  m_score = m_scores.InnerProduct(weights);
      
  VERBOSE(2, "Split Delta Update: Scores " << m_scores << endl);
  IFVERBOSE(2){
      std::cerr << "Split Delta Update: extra scores ";
      std::for_each(_extra_feature_values.begin(), _extra_feature_values.end(), std::cerr << boost::lambda::_1 << " ");
      std::cerr << std::endl; 
  }
  VERBOSE(2,"Split Delta Update: Total score is  " << m_score << endl);     
}

void SplitDelta::apply(const TranslationDelta& noChangeDelta) {
  m_scores.MinusEquals(noChangeDelta.getScores());
  getSample().SplitTarget(*m_leftOption,*m_rightOption,m_scores);
}
  
void FlipDelta::apply(const TranslationDelta& noChangeDelta) {
  VERBOSE(3, "Applying Flip Delta" << endl);
  m_scores.MinusEquals(noChangeDelta.getScores());
  getSample().FlipNodes(*m_leftTgtOption, *m_rightTgtOption, m_prevTgtHypo, m_nextTgtHypo, m_scores);
}

FlipDelta::FlipDelta(Sample& sample, const TranslationOption* leftTgtOption , 
                       const TranslationOption* rightTgtOption,  const Hypothesis* prevTgtHypo, const Hypothesis* nextTgtHypo, const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment, float totalDistortion ) :
  TranslationDelta(sample),
  m_leftTgtOption(leftTgtOption), m_rightTgtOption(rightTgtOption), m_prevTgtHypo(const_cast<Hypothesis*> (prevTgtHypo)), m_nextTgtHypo(const_cast<Hypothesis*> (nextTgtHypo))  {
    
  //translation scores
  m_scores.PlusEquals(m_leftTgtOption->GetScoreBreakdown());
  m_scores.PlusEquals(m_rightTgtOption->GetScoreBreakdown());
    
  //word penalty
  float penalty = -((int) m_leftTgtOption->GetTargetPhrase().GetSize() + (int) m_rightTgtOption->GetTargetPhrase().GetSize());
  m_scores.Assign(StaticData::Instance().GetWordPenaltyProducer(),penalty);
    
  addPairedOptionLanguageModelScore(m_leftTgtOption, m_rightTgtOption, leftTargetSegment, rightTargetSegment);
    
  //linear distortion
  const DistortionScoreProducer *dsp = StaticData::Instance().GetDistortionScoreProducer();
  m_scores.PlusEquals(dsp, totalDistortion);
    
  //TODO: extra feature scores
    
  //weight the scores
  const vector<float> & weights = StaticData::Instance().GetAllWeights();
  m_score = m_scores.InnerProduct(weights);
    
  VERBOSE(2, "Flip delta: Scores " << m_scores << endl);
  VERBOSE(2,"Flip delta: Total score is  " << m_score << endl);  
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
    
  //LM
  const LMList& languageModels = StaticData::Instance().GetAllLM();
    
  for (LMList::const_iterator i = languageModels.begin(); i != languageModels.end(); ++i) {
    LanguageModel* lm = *i;
    /*
     map<LanguageModel*,LanguageModelCache>::iterator lmci = m_cache.find(lm);
     if (lmci == m_cache.end()) {
     m_cache.insert(pair<LanguageModel*,LanguageModelCache>(lm,LanguageModelCache(lm)));
     }*/
    size_t order = lm->GetNGramOrder();
    vector<const Word*> lmcontext;
    lmcontext.reserve(targetPhrase.GetSize() + 2*(order-1));
      
    int start = targetSegment.GetStartPos() - (order-1);
      
    //fill in the pre-context
    for (size_t i = 0; i < order-1; ++i) {
      if (start+(int)i < 0) {
        lmcontext.push_back(&(lm->GetSentenceStartArray()));
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
    for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
      lmcontext.push_back(&(targetPhrase.GetWord(i)));
    }
      
    //fill in the postcontext
    for (size_t i = 0; i < order-1; ++i) {
      size_t targetPos = i + targetSegment.GetEndPos() + 1;
      if (targetPos >= getSample().GetTargetWords().size()) {
        if (targetPos == getSample().GetTargetWords().size()) {
          lmcontext.push_back(&(lm->GetSentenceEndArray()));
        }
      } else {
        lmcontext.push_back(&(getSample().GetTargetWords()[targetPos]));
      }
    }
      
      //debug
    IFVERBOSE(2) {
      VERBOSE(2,"Segment: " << targetSegment << " phrase: " << targetPhrase << endl);
      VERBOSE(2,"LM context ");
      for (size_t j = 0;  j < lmcontext.size(); ++j) {
        if (j == order-1) {
          //VERBOSE(3, "[ ");
        }
        if (j == (targetPhrase.GetSize() + (order-1))) {
          //VERBOSE(3,"] ");
        }
        VERBOSE(2,*(lmcontext[j]) << " ");
      }
      VERBOSE(2,endl);
    }
      
    //score lm
    double lmscore = 0;
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
        //cerr << "ngram: ";
        for (size_t j = ngramstart; j < ngramstart+order; ++j) {
          ngram[ngramCtr++] = lmcontext[j];
          //cerr << *lmcontext[j] << " ";
        }
        lmscore += lm->GetValue(ngram);
        //cerr << lm->GetValue(ngram)/log(10) << endl;
        //cache disabled for now
        //lmscore += m_cache.find(lm)->second.GetValue(ngram);
      }
    }
      
    if (useLeftOptionCacheLM) {
      const ScoreComponentCollection & sc = leftOption->GetScoreBreakdown();
      lmscore += sc.GetScoreForProducer(lm);
    }
      
    if (useRightOptionCacheLM) {
      const ScoreComponentCollection & sc = rightOption->GetScoreBreakdown();
      lmscore += sc.GetScoreForProducer(lm);
    }
      
    VERBOSE(2,"Language model score: " << lmscore << endl);
    m_scores.Assign(lm,lmscore);
  }
}
  
  
void  TranslationDelta::addDiscontiguousPairedOptionLMScore(const TranslationOption* leftOption, const TranslationOption* rightOption, const WordsRange* leftSegment, const WordsRange* rightSegment) {
  //LM
  const LMList& languageModels = StaticData::Instance().GetAllLM();
  const Phrase& leftTgtPhrase = leftOption->GetTargetPhrase();
  const Phrase& rightTgtPhrase = rightOption->GetTargetPhrase();
    
  for (LMList::const_iterator i = languageModels.begin(); i != languageModels.end(); ++i) {
    LanguageModel* lm = *i;
    /*
     map<LanguageModel*,LanguageModelCache>::iterator lmci = m_cache.find(lm);
     if (lmci == m_cache.end()) {
       m_cache.insert(pair<LanguageModel*,LanguageModelCache>(lm,LanguageModelCache(lm)));
     }*/
    size_t order = lm->GetNGramOrder();
    vector<const Word*> lmcontext;
    lmcontext.reserve(max(leftTgtPhrase.GetSize(), rightTgtPhrase.GetSize()) + 2*(order-1));
      
    int start = leftSegment->GetStartPos() - (order-1);
      
    //fill in the pre-context
    for (size_t i = 0; i < order-1; ++i) {
      if (start+(int)i < 0) {
        lmcontext.push_back(&(lm->GetSentenceStartArray()));
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
    size_t gapSize = rightSegment->GetStartPos() - leftSegment->GetEndPos() - 1;
    size_t leftSegmentEndPos = leftSegment->GetEndPos();
      
    for (size_t i = 0; i < order - 1; i++) {
      int rightOffset = i - gapSize;
      if (rightOffset < 0) {
        lmcontext.push_back(&(getSample().GetTargetWords()[leftSegmentEndPos + i + 1]));    
      }
      else if (rightOffset < rightTgtPhrase.GetSize() ) {
        if (rightOffset == 0) {
          rightStartPos = lmcontext.size();
        }
        lmcontext.push_back(&(rightTgtPhrase.GetWord(rightOffset)));
        rightEndPos = lmcontext.size();
      }
      else if (rightOffset - rightTgtPhrase.GetSize() + rightSegment->GetEndPos() + 1 < getSample().GetTargetWords().size() ) {
        lmcontext.push_back(&(getSample().GetTargetWords()[(rightOffset - rightTgtPhrase.GetSize() + rightSegment->GetEndPos()  + 1)]));  
      }
      else {
        lmcontext.push_back(&(lm->GetSentenceEndArray()));
        break;
      }
    }
      
      
    VERBOSE(2,"Left LM Context : "); 
    for (int i = 0; i < lmcontext.size(); i++) {
      VERBOSE(2,*lmcontext[i] << " ");
    }
    VERBOSE(2, endl);
      
    //score lm
    double lmscore = 0;
    vector<const Word*> ngram(order);
    size_t ngramCtr;
    bool useLeftOptionCacheLM(false), useRightOptionCacheLM(false) ;
    
    for (size_t ngramstart = 0; ngramstart < lmcontext.size() - (order -1); ++ngramstart) {
      if (ngramstart >= leftStartPos && ngramstart + order - 1 < leftEndPos) {
        useLeftOptionCacheLM = true;
        VERBOSE(2, "In flip, Left LM Context, Using cached option LM score for left Option: " << leftOption->GetTargetPhrase() << endl;)
      }  
      else if (ngramstart >= rightStartPos && ngramstart + order - 1 < rightEndPos) {
        useRightOptionCacheLM = true;
        VERBOSE(2, "In flip, Left LM Context, Using cached option LM score for right Option: " << rightOption->GetTargetPhrase() << endl;)
      }
      else {
        ngramCtr =0;
        //cerr << "ngram: ";
        for (size_t j = ngramstart; j < ngramstart+order; ++j) {
          ngram[ngramCtr++] =lmcontext[j];
          //cerr << *lmcontext[j] << " ";
        }
        lmscore += lm->GetValue(ngram);
        //cerr << lm->GetValue(ngram)/log(10) << endl;
        //cache disabled for now
        //lmscore += m_cache.find(lm)->second.GetValue(ngram);  
      }  
    }
      
    if (useLeftOptionCacheLM) {
      const ScoreComponentCollection & sc = leftOption->GetScoreBreakdown();
      lmscore += sc.GetScoreForProducer(lm);
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
      if (i > leftOffset + leftSegment->GetStartPos()) {                      
        lmcontext.push_back(&(lm->GetSentenceStartArray()));
      }
      else if (i > leftOffset) {
        lmcontext.push_back(&(getSample().GetTargetWords()[leftOffset - i + leftSegment->GetStartPos() ]));
      }                      
      else if ( i > gapSize) {
        if (i - gapSize == 1){
          leftStartPos = lmcontext.size();
        }
        lmcontext.push_back(&(leftTgtPhrase.GetWord(leftOffset - i)));
        leftEndPos = lmcontext.size();
      }
      else {
        lmcontext.push_back(&(getSample().GetTargetWords()[leftSegment->GetEndPos() + gapSize - i + 1 ]));
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
      if ( i + rightSegment->GetEndPos() + 1 < getSample().GetTargetWords().size() ) {
        lmcontext.push_back(&(getSample().GetTargetWords()[i + rightSegment->GetEndPos() + 1]));         
      }
      else { 
        lmcontext.push_back(&(lm->GetSentenceEndArray()));
        break;
      }
    }  
      
    VERBOSE(2,"Right LM Context : "); 
    for (int i = 0; i < lmcontext.size(); i++) {
      VERBOSE(2,*lmcontext[i] << " ");
    }
    VERBOSE(2, endl);
      
    //useLeftOptionCacheLM = false;
    useRightOptionCacheLM = false;
      
    //size_t maxNgram = max(lmcontext.size() - (order -1), static_cast<size_t>(1)); 
    size_t maxNgram = lmcontext.size() - (order -1);
    //assert (maxNgram > 0);
      
    for (size_t ngramstart = 0; ngramstart < maxNgram; ++ngramstart) {
      if (ngramstart >= leftStartPos && ngramstart + order - 1 < leftEndPos) {
         useLeftOptionCacheLM = true;
         VERBOSE(2, "In flip, Right LM Context, Using cached option LM score for left Option: " << leftOption->GetTargetPhrase() << endl;)
      }  
      if (ngramstart >= rightStartPos && ngramstart + order - 1 < rightEndPos) {
        useRightOptionCacheLM = true;
        VERBOSE(2, "In flip, Right LM Context, Using cached option LM score for right Option: " << rightOption->GetTargetPhrase() << endl;)
      }
      else {
        ngramCtr = 0;
        //cerr << "ngram: ";
        for (size_t j = ngramstart; j < ngramstart+order; ++j) {
          ngram[ngramCtr++] = lmcontext[j]; 
          //cerr << *lmcontext[j] << " ";
        }
        lmscore += lm->GetValue(ngram);
        //cerr << lm->GetValue(ngram)/log(10) << endl;
        //cache disabled for now
        //lmscore += m_cache.find(lm)->second.GetValue(ngram);  
      }  
    }
      
    if (useRightOptionCacheLM) {
      const ScoreComponentCollection & sc = rightOption->GetScoreBreakdown();
      lmscore += sc.GetScoreForProducer(lm);
    }
      
      
    VERBOSE(2,"Language model score: " << lmscore << endl); 
    m_scores.Assign(lm,lmscore);   
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
}
  
}//namespace
