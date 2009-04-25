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

#include "Dependency.h"

using namespace Moses;
using namespace std;

namespace Josiah {

static void addChildren(vector<set<size_t> >& tree, size_t parent, set<size_t>& children) {
  for (set<size_t>::const_iterator i = tree[parent].begin(); i!= tree[parent].end(); ++i) {
    children.insert(*i);
    addChildren(tree,*i,children);
  }
}
  
DependencyTree::DependencyTree(const vector<Word>& words, FactorType parentFactor) {
  vector<set<size_t> > tree(words.size()); // map parents to their immediate children
  size_t root = -1;
  for (size_t child = 0; child < words.size(); ++child) {
    int parent = atoi(words[child][parentFactor]->GetString().c_str());
    if (parent < 0) {
      root = child;
    } else {
      tree[(size_t)parent].insert(child);
    }
    m_parents.push_back(parent);
  }
  m_spans.resize(words.size());
  for (size_t i = 0; i < m_parents.size(); ++i) {
    addChildren(tree,i,m_spans[i]);
    m_spans[i].insert(i); // the head covers itself
  }
  
}

static string ToString(const DependencyTree& t)
{
  ostringstream os;
  for (size_t i = 0; i < t.getLength(); ++i) {
    os << i << "->" << t.getParent(i) << ", ";
  }
  return os.str();
}

ostream& operator<<(ostream& out, const DependencyTree& t)
{
  out << ToString(t);
  return out;
}

/** Parent of this index, -1 if root*/
int DependencyTree::getParent(size_t index) const {
  return m_parents[index];
}

/** Does the parent word cover the child word? */
bool DependencyTree::covers(size_t parent, size_t descendent) const {
  return m_spans[parent].count(descendent);
}

float CherrySyntacticCohesionFeature::computeScore() {
  //TODO
  return 0.0;
  /*float interruptionCount = 0.0;
  Hypothesis *prev = NULL;
  for (Hypothesis* h = const_cast<Hypothesis*>(const_cast<Sample*>(m_sample)->GetTargetTail()); h; h = const_cast<Hypothesis*>(h->GetNextHypo())) {
    if (prev && h->GetCurrSourceWordsRange().GetStartPos() > 0) {
      interruptionCount += getSingleUpdateScore(&(h->GetTranslationOption()), h->GetCurrTargetWordsRange());
    } 
    prev = h;
  }
  cerr << "In compute score, interr cnt = " << interruptionCount << endl;
  return interruptionCount;*/
}
  
/** Score due to  one segment */
  
//NB : Target Segment is the old one  
float CherrySyntacticCohesionFeature::getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap) {
  const Hypothesis* prevTgt = gap.leftHypo;
  if (!prevTgt->GetPrevHypo()) { //dummy hyp at start of sent, no cohesion violation
    return 0.0;
  }
  
  float interruptionCnt =  getInterruptions(prevTgt->GetCurrSourceWordsRange(), option, gap.segment);
  cerr << "In single upd, int cnt " << interruptionCnt << endl;
  return interruptionCnt;
}  

/** Score due to flip */
float CherrySyntacticCohesionFeature::getFlipUpdateScore(
    const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption,
    const TargetGap& leftGap, const TargetGap& rightGap) {
  float interruptionCnt = 0.0;                                            
    
  //Let's sort out the order of the segments
  WordsRange* leftTgtSegment = const_cast<WordsRange*> (&leftGap.segment);
  WordsRange* rightTgtSegment = const_cast<WordsRange*> (&rightGap.segment);
  assert(leftTgtSegment < rightTgtSegment); //should already be in target order!
  
  const Hypothesis* leftTgtHypPred = leftGap.leftHypo;
  const Hypothesis* rightTgtHypSucc = rightGap.rightHypo;
  
    
  //Left tgt option and its predecessor
  if (leftTgtHypPred && leftTgtHypPred->GetPrevHypo()) {
    interruptionCnt +=  getInterruptions(leftTgtHypPred->GetCurrSourceWordsRange(), leftTgtOption, *leftTgtSegment);  
  }
    
  //Right tgt option and its successor
  if (rightTgtHypSucc) {
    interruptionCnt +=  getInterruptions(rightTgtOption->GetSourceWordsRange()  ,&(rightTgtHypSucc->GetTranslationOption()), rightTgtHypSucc->GetCurrTargetWordsRange());    
  }
    
  //Are the options contiguous on the target side?
  bool contiguous = (leftTgtSegment->GetEndPos() + 1 == rightTgtSegment->GetStartPos()) ;
    
  if (contiguous) {
    interruptionCnt +=  getInterruptions(leftTgtOption->GetSourceWordsRange(), rightTgtOption, *rightTgtSegment);  
  }
  else {
    //Left tgt option and its successor
    Hypothesis* leftTgtSuccessorHyp = const_cast<Sample*>(m_sample)->GetHypAtTgtIndex(leftTgtSegment->GetEndPos()+1);
    if (leftTgtSuccessorHyp) {
      interruptionCnt +=  getInterruptions(leftTgtOption->GetSourceWordsRange(), &(leftTgtSuccessorHyp->GetTranslationOption()), leftTgtSuccessorHyp->GetCurrTargetWordsRange());  
    }
      
    //Right tgt option and its predecessor
    Hypothesis* rightTgtPredecessorHyp = const_cast<Sample*>(m_sample)->GetHypAtTgtIndex(rightTgtSegment->GetStartPos()-1);
      
    if (rightTgtPredecessorHyp) {
      interruptionCnt +=  getInterruptions(rightTgtPredecessorHyp->GetCurrSourceWordsRange(), rightTgtOption, *rightTgtSegment);
    }  
  }
  cerr << "In flip, interr cnt = " << interruptionCnt << endl;  
  return interruptionCnt; 
}
  
/** Score due to two segments **/
float CherrySyntacticCohesionFeature::getContiguousPairedUpdateScore(
    const TranslationOption* leftOption,const TranslationOption* rightOption, 
    const TargetGap& gap) {
      return 0.0;
  
  /*float interruptionCnt = 0.0;
  bool split = false;
  
  //Order the options and segments by tgt order instead of src order
  TranslationOption* leftTgtOption = const_cast<TranslationOption*>(leftOption);
  TranslationOption* rightTgtOption = const_cast<TranslationOption*>(rightOption);
  WordsRange* leftTgtSegment = const_cast<WordsRange*> (&leftSegment);
  WordsRange* rightTgtSegment = const_cast<WordsRange*> (&rightSegment);
  
  if (rightSegment == leftSegment) { //we're dealing with split options
    split = true;  
  }   
  else if (rightSegment < leftSegment) {
    leftTgtSegment = const_cast<WordsRange*> (&rightSegment);
    rightTgtSegment = const_cast<WordsRange*>(&leftSegment);
    leftTgtOption = const_cast<TranslationOption*>(rightOption);
    rightTgtOption = const_cast<TranslationOption*>(leftOption);
  }
  
  //Left tgt option and its predecessor
  if (leftTgtSegment->GetStartPos() > 0) {
    Hypothesis*  leftTgtHypPred =  const_cast<Sample*>(m_sample)->GetHypAtTgtIndex(leftTgtSegment->GetStartPos()-1);
    if (leftTgtHypPred) {
      interruptionCnt +=  getInterruptions(leftTgtHypPred->GetCurrSourceWordsRange(), leftTgtOption, *leftTgtSegment);  
    }  
  } 
  
  
  
  //Right tgt option and its successor
  Hypothesis*  rightTgtHypSucc =  const_cast<Sample*>(m_sample)->GetHypAtTgtIndex(rightTgtSegment->GetEndPos()+1);
  if (rightTgtHypSucc) {
    interruptionCnt +=  getInterruptions(rightTgtOption->GetSourceWordsRange()  ,&(rightTgtHypSucc->GetTranslationOption()), rightTgtHypSucc->GetCurrTargetWordsRange());    
  }
  
  //Are the options contiguous on the target side?
  bool contiguous = (leftTgtSegment->GetEndPos() + 1 == rightTgtSegment->GetStartPos()) || split;
  
  if (contiguous) {
    interruptionCnt +=  getInterruptions(leftTgtOption->GetSourceWordsRange(), rightTgtOption, *rightTgtSegment);  
  }
  else {
    //Left tgt option and its successor
    Hypothesis* leftTgtSuccessorHyp = const_cast<Sample*>(m_sample)->GetHypAtTgtIndex(leftTgtSegment->GetEndPos()+1);
    if (leftTgtSuccessorHyp) {
      interruptionCnt +=  getInterruptions(leftTgtOption->GetSourceWordsRange(), &(leftTgtSuccessorHyp->GetTranslationOption()), leftTgtSuccessorHyp->GetCurrTargetWordsRange());  
    }
    
    //Right tgt option and its predecessor
    Hypothesis* rightTgtPredecessorHyp = const_cast<Sample*>(m_sample)->GetHypAtTgtIndex(rightTgtSegment->GetStartPos()-1);
    
    if (rightTgtPredecessorHyp) {
      interruptionCnt +=  getInterruptions(rightTgtPredecessorHyp->GetCurrSourceWordsRange(), rightTgtOption, *rightTgtSegment);
    }  
  }
  cerr << "In paired update, interr cnt = " << interruptionCnt << endl; 
  return interruptionCnt; */
}

float CherrySyntacticCohesionFeature::getDiscontiguousPairedUpdateScore(
    const TranslationOption* leftOption,const TranslationOption* rightOption, 
    const TargetGap& leftGap, const TargetGap& rightGap) {
      //TODO
      return 0.0;
    }



/**Helper method */
float CherrySyntacticCohesionFeature::getInterruptions(const WordsRange& prevSourceRange, const TranslationOption *option, const WordsRange& targetSegment) {
  float interruptionCnt = 0.0;
  size_t f_L =  prevSourceRange.GetStartPos(); 
  size_t f_R =  prevSourceRange.GetEndPos();
    
  interruptionCnt = getInterruptionCount(option, targetSegment, f_L);
  if (interruptionCnt == 0 && f_L != f_R)
    interruptionCnt = getInterruptionCount(option, targetSegment, f_R);  
    
  return interruptionCnt;
  
}  
  
float CherrySyntacticCohesionFeature::getInterruptionCount(const TranslationOption *option, const WordsRange& targetSegment, size_t f) {
  size_t r_prime = f;
  size_t r = NOT_FOUND;
    
  while (notAllWordsCoveredByTree(option, r_prime)) {
    r = r_prime;
    r_prime = m_sourceTree->getParent(r_prime);
  }
    
  if (r == NOT_FOUND)
    return 0.0; 
    
  const set<size_t> & children = m_sourceTree->getChildren(r);
  for (set<size_t>::const_iterator it = children.begin(); it != children.end(); ++it) {
    size_t child = *it;
    Hypothesis* hyp = const_cast<Sample*>(m_sample)->GetHypAtSourceIndex(child);
    if (isInterrupting(hyp, targetSegment)) {
      return 1.0;
    }
  }
  return 0.0;
}  
  
bool CherrySyntacticCohesionFeature::isInterrupting(Hypothesis* hyp, const WordsRange& targetSegment) {
  return hyp->GetCurrTargetWordsRange() > targetSegment;    
}  
  
bool CherrySyntacticCohesionFeature::notAllWordsCoveredByTree(const TranslationOption* option, size_t parent) {
  for (size_t s = option->GetStartPos(); s <= option->GetEndPos(); ++s) {
    if (!m_sourceTree->covers(parent, s))
      return true;
  }
  return false;       
}

}
