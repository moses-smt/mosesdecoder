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

#include "Derivation.h"

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
  int root = -1;
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

float CherrySyntacticCohesionFeatureFunction::computeScore() {
  float interruptionCount = 0.0;
  Hypothesis *prev = const_cast<Hypothesis*>(const_cast<Sample&>(getSample()).GetTargetTail()->GetNextHypo()); //first hypo in tgt order
  
  for (Hypothesis* h =  const_cast<Hypothesis*>(prev->GetNextHypo()); h; h = const_cast<Hypothesis*>(h->GetNextHypo())) {
    Context context = { &(prev->GetCurrSourceWordsRange()), &(h->GetCurrSourceWordsRange()), &(prev->GetCurrTargetWordsRange()), &(h->GetCurrTargetWordsRange())  };
    interruptionCount += getInterruptions(prev->GetCurrSourceWordsRange(), &(h->GetTranslationOption()), h->GetCurrTargetWordsRange(), context);
    prev = h;
  }
  VERBOSE(2,"In compute score, interr cnt = " << interruptionCount << endl);
  return interruptionCount;
}
  
/** Score due to  one segment */
  
//NB : Target Segment is the old one  
float CherrySyntacticCohesionFeatureFunction::getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap) {
  const Hypothesis* prevTgt = gap.leftHypo;
  if (!prevTgt->GetPrevHypo()) { //dummy hyp at start of sent, no cohesion violation
    return 0.0;
  }
  Context context = { &(prevTgt->GetCurrSourceWordsRange()), &(option->GetSourceWordsRange()), &(prevTgt->GetCurrTargetWordsRange()), &(gap.segment)  };
  float interruptionCnt =  getInterruptions(prevTgt->GetCurrSourceWordsRange(), option, gap.segment, context);
  VERBOSE(2, "In single upd, int cnt " << interruptionCnt << endl);
  return interruptionCnt;
}  

/** Score due to flip */
float CherrySyntacticCohesionFeatureFunction::getFlipUpdateScore(
    const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption,
    const TargetGap& leftGap, const TargetGap& rightGap) {
  float interruptionCnt = 0.0;                                            
    
  //Let's sort out the order of the segments
  WordsRange* leftTgtSegment = const_cast<WordsRange*> (&leftGap.segment);
  WordsRange* rightTgtSegment = const_cast<WordsRange*> (&rightGap.segment);
  
  assert(*leftTgtSegment < *rightTgtSegment); //should already be in target order!
  
  const Hypothesis* leftTgtHypPred = leftGap.leftHypo;
  const Hypothesis* rightTgtHypSucc = rightGap.rightHypo;
  
  Context context = { &(leftTgtOption->GetSourceWordsRange()), &(rightTgtOption->GetSourceWordsRange()), leftTgtSegment, rightTgtSegment };
  
  //Left tgt option and its predecessor
  if (leftTgtHypPred && leftTgtHypPred->GetPrevHypo()) {
    interruptionCnt +=  getInterruptions(leftTgtHypPred->GetCurrSourceWordsRange(), leftTgtOption, *leftTgtSegment, context);  
  }
    
  //Right tgt option and its successor
  if (rightTgtHypSucc) {
    interruptionCnt +=  getInterruptions(rightTgtOption->GetSourceWordsRange()  ,&(rightTgtHypSucc->GetTranslationOption()), rightTgtHypSucc->GetCurrTargetWordsRange(), context);    
  }
    
  //Are the options contiguous on the target side?
  bool contiguous = (leftTgtSegment->GetEndPos() + 1 == rightTgtSegment->GetStartPos()) ;
    
  if (contiguous) {
    interruptionCnt +=  getInterruptions(leftTgtOption->GetSourceWordsRange(), rightTgtOption, *rightTgtSegment, context);  
  }
  else {
    //Left tgt option and its successor
    const Hypothesis* leftTgtSuccessorHyp = leftGap.rightHypo;
    if (leftTgtSuccessorHyp) {
      interruptionCnt +=  getInterruptions(leftTgtOption->GetSourceWordsRange(), &(leftTgtSuccessorHyp->GetTranslationOption()), leftTgtSuccessorHyp->GetCurrTargetWordsRange(), context );  
    }
    
    //Right tgt option and its predecessor
    const Hypothesis* rightTgtPredecessorHyp = rightGap.leftHypo;
    if (rightTgtPredecessorHyp) {
      interruptionCnt +=  getInterruptions(rightTgtPredecessorHyp->GetCurrSourceWordsRange(), rightTgtOption, *rightTgtSegment, context);
    }
    
    //Everything in between
    if (leftTgtSuccessorHyp != rightTgtPredecessorHyp) {
      TranslationOption *prevOption =  const_cast<TranslationOption*>(&(leftTgtSuccessorHyp->GetTranslationOption()));
      for (Hypothesis *hyp = const_cast<Hypothesis*>(leftTgtSuccessorHyp->GetNextHypo()); ; hyp = const_cast<Hypothesis*>(hyp->GetNextHypo())) {
        if (hyp) {
          interruptionCnt +=  getInterruptions(prevOption->GetSourceWordsRange(), &(hyp->GetTranslationOption()), hyp->GetCurrTargetWordsRange(), context);  
          prevOption = const_cast<TranslationOption*>(&(hyp->GetTranslationOption()));
        }
        if (hyp == rightGap.leftHypo) {
          break;
        }
      }  
    }
        
  }
  VERBOSE (2, "In flip, interr cnt = " << interruptionCnt << endl);  
  return interruptionCnt; 
}
  
/** Score due to two segments **/
float CherrySyntacticCohesionFeatureFunction::getContiguousPairedUpdateScore(
    const TranslationOption* leftTgtOption,const TranslationOption* rightTgtOption, 
    const TargetGap& gap) {
  
  float interruptionCnt = 0.0;
  
  const Hypothesis*  leftTgtHypPred = gap.leftHypo;
  const Hypothesis*  rightTgtHypSucc = gap.rightHypo;
  
  Context context = { &(leftTgtOption->GetSourceWordsRange()), &(rightTgtOption->GetSourceWordsRange()), &(gap.segment), &(gap.segment)  };
  //Left tgt option and its predecessor
  if (gap.segment.GetStartPos() > 0) {
    if (leftTgtHypPred) {
      interruptionCnt +=  getInterruptions(leftTgtHypPred->GetCurrSourceWordsRange(), leftTgtOption, gap.segment, context);  
    }  
  } 
  
  //Right tgt option and its successor
  if (rightTgtHypSucc) {
    interruptionCnt +=  getInterruptions(rightTgtOption->GetSourceWordsRange()  ,&(rightTgtHypSucc->GetTranslationOption()), rightTgtHypSucc->GetCurrTargetWordsRange(), context);    
  }
  
  interruptionCnt +=  getInterruptions(leftTgtOption->GetSourceWordsRange(), rightTgtOption, gap.segment, context);  
  
  VERBOSE(2, "In paired update, interr cnt = " << interruptionCnt << endl); 
  return interruptionCnt; 
}

float CherrySyntacticCohesionFeatureFunction::getDiscontiguousPairedUpdateScore(
    const TranslationOption* leftTgtOption,const TranslationOption* rightTgtOption, 
    const TargetGap& leftGap, const TargetGap& rightGap) {
  float interruptionCnt = 0.0;
  
  WordsRange* leftTgtSegment = const_cast<WordsRange*> (&leftGap.segment);
  WordsRange* rightTgtSegment = const_cast<WordsRange*> (&rightGap.segment);
  assert(*leftTgtSegment < *rightTgtSegment); //should already be in target order!
  
  const Hypothesis*  leftTgtHypPred = leftGap.leftHypo;
  const Hypothesis*  leftTgtSuccessorHyp = leftGap.rightHypo;
  const Hypothesis*  rightTgtPredecessorHyp = rightGap.leftHypo;
  const Hypothesis*  rightTgtHypSucc = rightGap.rightHypo;
  
  Context context = { &(leftTgtOption->GetSourceWordsRange()), &(rightTgtOption->GetSourceWordsRange()), leftTgtSegment, rightTgtSegment };
  
  //Left tgt option and its predecessor
  if (leftTgtSegment->GetStartPos() > 0) {
    if (leftTgtHypPred) {
      interruptionCnt +=  getInterruptions(leftTgtHypPred->GetCurrSourceWordsRange(), leftTgtOption, *leftTgtSegment, context);  
    }  
  } 
  
  //Right tgt option and its successor
  if (rightTgtHypSucc) {
    interruptionCnt +=  getInterruptions(rightTgtOption->GetSourceWordsRange()  ,&(rightTgtHypSucc->GetTranslationOption()), rightTgtHypSucc->GetCurrTargetWordsRange(), context);    
  }
  
  //Left tgt option and its successor
  if (leftTgtSuccessorHyp) {
    interruptionCnt +=  getInterruptions(leftTgtOption->GetSourceWordsRange(), &(leftTgtSuccessorHyp->GetTranslationOption()), leftTgtSuccessorHyp->GetCurrTargetWordsRange(), context);  
  }
    
  //Right tgt option and its predecessor
  if (rightTgtPredecessorHyp) {
    interruptionCnt +=  getInterruptions(rightTgtPredecessorHyp->GetCurrSourceWordsRange(), rightTgtOption, *rightTgtSegment, context);
  }
  
  //Everything in between
  if (leftTgtSuccessorHyp != rightTgtPredecessorHyp) {
    TranslationOption *prevOption =  const_cast<TranslationOption*>(&(leftTgtSuccessorHyp->GetTranslationOption()));
    for (Hypothesis *hyp = const_cast<Hypothesis*>(leftTgtSuccessorHyp->GetNextHypo()); ; hyp = const_cast<Hypothesis*>(hyp->GetNextHypo())) {
      if (hyp) {
        interruptionCnt +=  getInterruptions(prevOption->GetSourceWordsRange(), &(hyp->GetTranslationOption()), hyp->GetCurrTargetWordsRange(), context);  
        prevOption = const_cast<TranslationOption*>(&(hyp->GetTranslationOption()));
      }
      if (hyp == rightGap.leftHypo) {
        break;
      }
    }  
  }
  
  
  VERBOSE (2,  "In paired update, interr cnt = " << interruptionCnt << endl); 
  return interruptionCnt; 
}



/**Helper method */
float CherrySyntacticCohesionFeatureFunction::getInterruptions(const WordsRange& prevSourceRange, const TranslationOption *option, const WordsRange& targetSegment, const Context& context) {
  float interruptionCnt = 0.0;
  size_t f_L =  prevSourceRange.GetStartPos(); 
  size_t f_R =  prevSourceRange.GetEndPos();
    
  interruptionCnt = getInterruptionCount(option, targetSegment, f_L, context);
  if (interruptionCnt == 0 && f_L != f_R)
    interruptionCnt = getInterruptionCount(option, targetSegment, f_R, context);  
    
  return interruptionCnt;
  
}  
  
float CherrySyntacticCohesionFeatureFunction::getInterruptionCount(const TranslationOption *option, const WordsRange& targetSegment, size_t f, const Context& context) {
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
    const WordsRange* otherSegment; 
    if (context.leftSrcRange->covers(child)) {
      otherSegment = context.leftTgtRange  ;
    }
    else if (context.rightSrcRange->covers(child)) {
      otherSegment = context.rightTgtRange  ;
  }
    else {
      Hypothesis* hyp = const_cast<Sample&>(getSample()).GetHypAtSourceIndex(child);
      otherSegment = &(hyp->GetCurrTargetWordsRange());
  }
    
    if (isInterrupting(*otherSegment, targetSegment)) {
      return 1.0;
    }
  }
  return 0.0;
}  
  
bool CherrySyntacticCohesionFeatureFunction::isInterrupting(const WordsRange& otherSegment, const WordsRange& targetSegment) {
  return otherSegment > targetSegment;    
}  
  
bool CherrySyntacticCohesionFeatureFunction::notAllWordsCoveredByTree(const TranslationOption* option, size_t parent) {
  for (size_t s = option->GetStartPos(); s <= option->GetEndPos(); ++s) {
    if (!m_sourceTree->covers(parent, s))
      return true;
  }
  return false;       
}

//new sample
DependencyDistortionFeatureFunction::DependencyDistortionFeatureFunction(const Sample& sample, Moses::FactorType parentFactor) :
    DependencyFeatureFunction(sample,"DependencyDistortion",parentFactor) {
  size_t sourceSize = getSample().GetSourceSize();
  size_matrix_t::extent_gen extents;
  m_distances.resize(extents[sourceSize][sourceSize]);
  
  //Use Floyd-Warshall to compute all the distances
  
  //Initialise with  the (undirected) tree structure
  for (size_t i = 0; i < sourceSize; ++i) {
    size_t iparent = (size_t)m_sourceTree->getParent(i);
    for (size_t j = 0; j < sourceSize; ++j) {
      size_t jparent = (size_t)m_sourceTree->getParent(j);
      if (i == j) {
        m_distances[i][j] = 0;
      } else if (iparent == j || jparent == i) {
        m_distances[i][j] = 1;
      } else {
        m_distances[i][j] = sourceSize*2; //no path - infinity
      }
    }
  } 
  //run algorithm
  for (size_t k = 0; k < sourceSize; ++k) {
    for (size_t i = 0; i < sourceSize; ++i) {
      for (size_t j = 0; j < sourceSize; ++j) {
        m_distances[i][j] = min(m_distances[i][j], m_distances[i][k] + m_distances[k][j]);
        
      }
    }
  }
  
  /*for (size_t i = 0; i < sourceSize; ++i) {
    for (size_t j = 0; j < sourceSize; ++j) {
      cerr << "p[" << i << "][" << j << "] = " << m_distances[i][j] << " ";
    }
    cerr << endl;
}*/
  
}


size_t DependencyDistortionFeatureFunction::getDistortionDistance(const WordsRange& leftRange, const WordsRange& rightRange) {
  size_t leftSourcePos = leftRange.GetEndPos();
  size_t rightSourcePos = rightRange.GetStartPos();
  return m_distances[leftSourcePos][rightSourcePos] - 1;
}


/** Compute full score of a sample from scratch **/
float DependencyDistortionFeatureFunction::computeScore() {
  //
  // The score for each pair of adjacent target phrases is the tree distance of the corresponding edge source words
  //
  float score = 0;
  const Hypothesis* currHypo = getSample().GetTargetTail();
  while ((currHypo = (currHypo->GetNextHypo()))) {
    const Hypothesis* nextHypo = currHypo->GetNextHypo();
    if (nextHypo) {
      score += getDistortionDistance(currHypo->GetCurrSourceWordsRange(),nextHypo->GetCurrSourceWordsRange());
    }
  }
  return score;
}

/** Score due to  one segment */
float DependencyDistortionFeatureFunction::getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap) {
  float score = 0;
  if (gap.leftHypo->GetPrevHypo()) {
    score += getDistortionDistance(gap.leftHypo->GetCurrSourceWordsRange(), option->GetSourceWordsRange());
  }
  if (gap.rightHypo) {
    score += getDistortionDistance(option->GetSourceWordsRange(), gap.rightHypo->GetCurrSourceWordsRange());
  }
  return score;
}

/** Score due to two segments **/
float DependencyDistortionFeatureFunction::getContiguousPairedUpdateScore
    (const TranslationOption* leftOption, const TranslationOption* rightOption,  const TargetGap& gap) {
  float score = 0;
  if (gap.leftHypo->GetPrevHypo()) {
    score += getDistortionDistance(gap.leftHypo->GetCurrSourceWordsRange(), leftOption->GetSourceWordsRange());
  }
  score += getDistortionDistance(leftOption->GetSourceWordsRange(), rightOption->GetSourceWordsRange());
  if (gap.rightHypo) {
    score += getDistortionDistance(rightOption->GetSourceWordsRange(), gap.rightHypo->GetCurrSourceWordsRange());
  }
  return score;
}


float DependencyDistortionFeatureFunction::getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption, 
    const TargetGap& leftGap, const TargetGap& rightGap) {
      return getSingleUpdateScore(leftOption,leftGap) + getSingleUpdateScore(rightOption,rightGap);
}
  
/** Score due to flip */
float DependencyDistortionFeatureFunction::getFlipUpdateScore(
    const TranslationOption* leftOption, const TranslationOption* rightOption, 
    const TargetGap& leftGap, const TargetGap& rightGap) {
  float score = 0;
  if (leftGap.leftHypo->GetPrevHypo()) {
    score += getDistortionDistance(leftGap.leftHypo->GetCurrSourceWordsRange(), leftOption->GetSourceWordsRange());
  }
  bool contiguous = (leftGap.segment.GetEndPos() + 1 == rightGap.segment.GetStartPos());
  if (contiguous) {
    score += getDistortionDistance(leftOption->GetSourceWordsRange(), rightOption->GetSourceWordsRange());
  } else {
    score += getDistortionDistance(leftOption->GetSourceWordsRange(),leftGap.rightHypo->GetCurrSourceWordsRange());
    score += getDistortionDistance(rightGap.leftHypo->GetCurrSourceWordsRange(), rightOption->GetSourceWordsRange());
  }
  if (rightGap.rightHypo) {
    score += getDistortionDistance(rightOption->GetSourceWordsRange(), rightGap.rightHypo->GetCurrSourceWordsRange());
  }
  
  return score;
}

}
