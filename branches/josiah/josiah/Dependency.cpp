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
  float interruptionCount = 0.0;
  Hypothesis *prev = NULL;
  for (Hypothesis* h = const_cast<Hypothesis*>(const_cast<Sample*>(m_sample)->GetTargetTail()); h; h = const_cast<Hypothesis*>(h->GetNextHypo())) {
    if (prev && h->GetCurrSourceWordsRange().GetStartPos() > 0) {
      interruptionCount += getSingleUpdateScore(&(h->GetTranslationOption()), h->GetCurrTargetWordsRange());
    } 
    prev = h;
  }
  return interruptionCount;
}

/** Score due to flip */
float CherrySyntacticCohesionFeature::getFlipUpdateScore(const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
                                                           const Hypothesis* leftTgtHypPred, const Hypothesis* rightTgtHypSucc, 
                                                           const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment) {
  float interruptionCnt = 0.0;                                            
    
  //Let's sort out the order of the segments
  WordsRange* leftSegment = const_cast<WordsRange*> (&leftTargetSegment);
  WordsRange* rightSegment = const_cast<WordsRange*> (&rightTargetSegment);
    
  if (rightTargetSegment < leftTargetSegment) {
    leftSegment = const_cast<WordsRange*> (&rightTargetSegment);
    rightSegment = const_cast<WordsRange*>(&leftTargetSegment);
  }
    
  //Left tgt option and its predecessor
  if (leftTgtHypPred) {
    interruptionCnt +=  getInterruptions(leftTgtHypPred->GetCurrSourceWordsRange(), leftTgtOption, *leftSegment);  
  }
    
  //Right tgt option and its successor
  if (rightTgtHypSucc) {
    interruptionCnt +=  getInterruptions(rightTgtOption->GetSourceWordsRange()  ,&(rightTgtHypSucc->GetTranslationOption()), rightTgtHypSucc->GetCurrTargetWordsRange());    
  }
    
  //Are the options contiguous on the target side?
  bool contiguous = (leftSegment->GetEndPos() + 1 == rightSegment->GetStartPos()) ;
    
  if (contiguous) {
    interruptionCnt +=  getInterruptions(leftTgtOption->GetSourceWordsRange(), rightTgtOption, *rightSegment);  
  }
  else {
    //Left tgt option and its successor
    Hypothesis* leftTgtSuccessorHyp = const_cast<Sample*>(m_sample)->GetHypAtTgtIndex(leftTargetSegment.GetEndPos()+1);
    if (leftTgtSuccessorHyp) {
      interruptionCnt +=  getInterruptions(leftTgtOption->GetSourceWordsRange(), &(leftTgtSuccessorHyp->GetTranslationOption()), leftTgtSuccessorHyp->GetCurrTargetWordsRange());  
    }
      
    //Right tgt option and its predecessor
    Hypothesis* rightTgtPredecessorHyp = const_cast<Sample*>(m_sample)->GetHypAtTgtIndex(rightTargetSegment.GetStartPos()-1);
      
    if (rightTgtPredecessorHyp) {
      interruptionCnt +=  getInterruptions(rightTgtPredecessorHyp->GetCurrSourceWordsRange(), rightTgtOption, *rightSegment);
    }  
  }
    
  return interruptionCnt; 
}
  
  /** Score due to  one segment */

//NB : Target Segment is the old one  
float CherrySyntacticCohesionFeature::getSingleUpdateScore(const TranslationOption* option, const WordsRange& targetSegment) {
  size_t prevTgtIndex = targetSegment.GetStartPos() -1 ;
  Hypothesis *prevTgt = const_cast<Sample*>(m_sample)->GetHypAtTgtIndex(prevTgtIndex); // the prev hyp on the tgt side
  if (!prevTgt) { //dummy hyp at start of sent, no cohesion violation
    return 0.0;
  }
  return getInterruptions(prevTgt->GetCurrSourceWordsRange(), option, targetSegment);
}



/** Score due to two segments **/
float CherrySyntacticCohesionFeature::getPairedUpdateScore(const TranslationOption* leftOption,
    const TranslationOption* rightOption, const WordsRange& leftTargetSegment, 
    const WordsRange& rightTargetSegment,  const Phrase& targetPhrase) {

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
