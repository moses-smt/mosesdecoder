// vim:tabstop=2
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
#include "GibbsOperator.h"
#include "OnlineLearner.h"
#include "SampleAcceptor.h"
#include "TranslationDelta.h"
#include "Gibbler.h"
#include "Sampler.h"
#include "SampleCollector.h"


using namespace std;
using namespace Moses;

namespace Josiah {
  
  RandomNumberGenerator::RandomNumberGenerator() :m_dist(0,1), m_generator(), m_random(m_generator,m_dist) {
    uint32_t seed;
    std::ifstream r("/dev/urandom");
    if (r) {
      r.read((char*)&seed,sizeof(uint32_t));
    }
    if (r.fail() || !r) {
      std::cerr << "Warning: could not read from /dev/urandom. Seeding from clock" << std::endl;
      seed = time(NULL);
    }
    std::cerr << "Seeding random number sequence to " << seed << endl;
    m_generator.seed(seed);
  }
  
  RandomNumberGenerator RandomNumberGenerator::s_instance;
  


static float ComputeDistortionDistance(const WordsRange& prev, const WordsRange& current) 
{
  int dist = 0;
  if (prev.GetNumWordsCovered() == 0) {
    dist = current.GetStartPos();
  } else {
    dist = (int)prev.GetEndPos() - (int)current.GetStartPos() + 1 ;
  }
  return - (float) abs(dist);
}
 
GibbsOperator::~GibbsOperator() {
  delete m_OpIterator;
} 
  
  
void GibbsOperator::SetAnnealingTemperature(const double t) {
  m_acceptor = new FixedTempAcceptor(t);    
}  

void GibbsOperator::Quench() {
  cerr << "About to delete acceptor" << endl;
  delete m_acceptor;
  m_acceptor = NULL;    
}  
  
  
void GibbsOperator::doSample(vector<TranslationDelta*>& deltas, TranslationDelta* noChangeDelta) {
  if (deltas.empty()) return;
  
  size_t chosen = m_acceptor->choose(deltas);
  
  if (m_gf)
    doOnlineLearning(deltas, noChangeDelta, chosen);
  
  //apply it to the sample
  if (deltas[chosen] != noChangeDelta) {
    deltas[chosen]->apply(*noChangeDelta);
  }
  
}

void GibbsOperator::UpdateGainOptimalSol(const vector<TranslationDelta*>& deltas, int chosen, int target, TranslationDelta* noChangeDelta){
  int currentBestGainSol = -1;
  //Store best gain solution
  if (m_assigner->m_name == "Best") {
    currentBestGainSol = target;
  }
  else {
    BestNeighbourTgtAssigner assigner;
    currentBestGainSol = assigner.getTarget(deltas, deltas[chosen]);
  }
  
  float currentBestGain = deltas[currentBestGainSol]->getGain();
  
  if (currentBestGain > m_sampler->GetOptimalGain()) {
    m_sampler->SetOptimalGain(currentBestGain);
    m_sampler->SetOptimalGainSol(deltas[currentBestGainSol], noChangeDelta);
    cerr << "New optimal gain " << currentBestGain << endl;
  }
}
  
void GibbsOperator::doOnlineLearning(vector<TranslationDelta*>& deltas, TranslationDelta* noChangeDelta, size_t chosen) {
  bool error = false;
  
  float chosenScore = deltas[chosen]->getScore();
  float chosenGain = deltas[chosen]->getGain();

  if (m_useApproxDocBleu)
    m_sampler->UpdateGainFunctionStats(deltas[chosen]->getGainSufficientStats());
  
  int target = m_assigner->getTarget(deltas, deltas[chosen]);
  
  if (target == -1)
    return;
  
  
  if (m_sampler->GetOnlineLearner()->GetName() == "MIRA++"){
    UpdateGainOptimalSol(deltas, chosen, target, noChangeDelta);
  }
  
  float targetScore = deltas[target]->getScore();
  float targetGain = deltas[target]->getGain();
  
  if (chosenScore > targetScore && chosenGain < targetGain  ||
        chosenScore < targetScore && chosenGain > targetGain ) {
    error = true;
    IFVERBOSE(1) {
      cerr << "In " << m_name << ", there is an error because chosen sol has model score" << chosenScore << " and gain " << chosenGain << endl;
      cerr << "while target sol has model score " <<  targetScore << " and gain " << targetGain << endl;  
    }
  }
  else {
    IFVERBOSE(1) {
      cerr << "In " << m_name << ", there is no error because chosen sol has model score" <<  chosenScore << " and gain " << chosenGain << endl;
      cerr << "while target sol has model score " <<  targetScore << " and gain " << targetGain << endl;
    }
  }
    
  if (error) {
    cerr << "Hill-climbed gain function to gain = " << deltas[target]->getGain() << endl;
    m_sampler->GetOnlineLearner()->doUpdate(deltas[chosen], deltas[target], noChangeDelta, *m_sampler);//deltas[target], noChangeDelta  
  }
  else {
    m_sampler->GetOnlineLearner()->UpdateCumul();//deltas[target], noChangeDelta  
  }
}
  
void MergeSplitOperator::scan(
    Sample& sample,
    const TranslationOptionCollection& toc) {
  
  if (m_OpIterator->isStartScan()) {
    m_OpIterator->init(sample);
    m_OpIterator->next(); 
  }
  
  m_OpIterator->next(); 

  if (static_cast<MergeSplitIterator*>(m_OpIterator)->GetCurPos() >= m_OpIterator->GetInputSize())
    return;
  
  size_t splitIndex = static_cast<MergeSplitIterator*>(m_OpIterator)->GetCurPos() ; 
 
  //NB splitIndex n refers to the position between word n-1 and word n. Words are zero indexed
  VERBOSE(3,"Sampling at source index " << splitIndex << endl);
    
  Hypothesis* hypothesis = sample.GetHypAtSourceIndex(splitIndex);
  //the delta corresponding to the current translation scores, needs to be subtracted off the delta before applying
  TranslationDelta* noChangeDelta = NULL; 
  vector<TranslationDelta*> deltas;
    
  //find out which source and target segments this split-merge operator should consider
  //if we're at the left edge of a segment, then we're on a split
  if (hypothesis->GetCurrSourceWordsRange().GetStartPos() == splitIndex) {
    VERBOSE(3, "Existing split" << endl);
    WordsRange rightSourceSegment = hypothesis->GetCurrSourceWordsRange();
    WordsRange rightTargetSegment = hypothesis->GetCurrTargetWordsRange();
    const Hypothesis* prev = hypothesis->GetSourcePrevHypo();
    assert(prev);
    assert(prev->GetSourcePrevHypo()); //must be a valid hypo
    WordsRange leftSourceSegment = prev->GetCurrSourceWordsRange();
    WordsRange leftTargetSegment = prev->GetCurrTargetWordsRange();
    if (leftTargetSegment.GetEndPos() + 1 ==  rightTargetSegment.GetStartPos()) {
      //contiguous on target side.
      //In this case source and target order are the same
      //Add MergeDeltas
      WordsRange sourceSegment(leftSourceSegment.GetStartPos(), rightSourceSegment.GetEndPos());
      WordsRange targetSegment(leftTargetSegment.GetStartPos(), rightTargetSegment.GetEndPos());
      TargetGap gap(prev->GetPrevHypo(), hypothesis->GetNextHypo(), targetSegment);
      VERBOSE(3, "Creating merge deltas for merging source segments  " << leftSourceSegment << " with " <<
             rightSourceSegment << " and target segments " << leftTargetSegment << " with " << rightTargetSegment  << endl);
      const TranslationOptionList&  options = toc.GetTranslationOptionList(sourceSegment);
      for (TranslationOptionList::const_iterator i = options.begin(); i != options.end(); ++i) {
        TranslationDelta* delta = new MergeDelta(sample,*i,gap, GetGainFunction());
        deltas.push_back(delta);
      }
    }
    
    //make sure that the 'left' and 'right' refer to the target order
    const TranslationOptionList* leftOptions = NULL;
    const TranslationOptionList* rightOptions = NULL;
    auto_ptr<TargetGap> leftGap;
    auto_ptr<TargetGap> rightGap;
    if (leftTargetSegment < rightTargetSegment) {
        //source and target order same
        leftOptions = &(toc.GetTranslationOptionList(leftSourceSegment));
        rightOptions = &(toc.GetTranslationOptionList(rightSourceSegment));
        leftGap.reset(new TargetGap(prev->GetPrevHypo(), prev->GetNextHypo(), prev->GetCurrTargetWordsRange()));
        rightGap.reset(new TargetGap(hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), 
              hypothesis->GetCurrTargetWordsRange()));
        noChangeDelta = new   PairedTranslationUpdateDelta(sample,&(prev->GetTranslationOption())
          ,&(hypothesis->GetTranslationOption()),*leftGap, *rightGap, GetGainFunction());
        
    } else {
        //target in opposite order to source
        leftOptions = &(toc.GetTranslationOptionList(rightSourceSegment));
        rightOptions = &(toc.GetTranslationOptionList(leftSourceSegment));
        leftGap.reset(new TargetGap(hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), 
              hypothesis->GetCurrTargetWordsRange()));
        rightGap.reset(new TargetGap(prev->GetPrevHypo(), prev->GetNextHypo(), prev->GetCurrTargetWordsRange()));
        noChangeDelta = new   PairedTranslationUpdateDelta(sample,&(hypothesis->GetTranslationOption())
          ,&(prev->GetTranslationOption()),*leftGap, *rightGap, GetGainFunction());
    }
      

    //Add PairedTranslationUpdateDeltas
      
    for (TranslationOptionList::const_iterator ri = rightOptions->begin(); ri != rightOptions->end(); ++ri) {
      for (TranslationOptionList::const_iterator li = leftOptions->begin(); li != leftOptions->end(); ++li) {
        TranslationDelta* delta = new PairedTranslationUpdateDelta(sample,*li, *ri, *leftGap, *rightGap, GetGainFunction());
        deltas.push_back(delta);
      }
    }
      //cerr << "Added " << ds << " deltas" << endl;
  } else {
      VERBOSE(3, "No existing split" << endl);
      WordsRange sourceSegment = hypothesis->GetCurrSourceWordsRange();
      TargetGap gap(hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), hypothesis->GetCurrTargetWordsRange());
      noChangeDelta = new TranslationUpdateDelta(sample,&(hypothesis->GetTranslationOption()),gap, GetGainFunction());
      //Add TranslationUpdateDeltas
      const TranslationOptionList&  options = toc.GetTranslationOptionList(sourceSegment);
      //cerr << "Got " << options.size() << " options for " << sourceSegment << endl;
      VERBOSE(3, "Creating simple deltas for source segment " << sourceSegment << " and target segment " <<gap.segment
            << endl);
      for (TranslationOptionList::const_iterator i = options.begin(); i != options.end(); ++i) {
        TranslationDelta* delta = new TranslationUpdateDelta(sample,*i,gap, GetGainFunction());
        deltas.push_back(delta);
      }
      //cerr << "Added " << ds << " deltas" << endl;

      
      //Add SplitDeltas
      VERBOSE(3, "Adding deltas to split " << sourceSegment << " at " << splitIndex << endl);
      //Note no reordering in split
      WordsRange leftSourceSegment(sourceSegment.GetStartPos(),splitIndex-1);
      WordsRange rightSourceSegment(splitIndex,sourceSegment.GetEndPos());
      const TranslationOptionList&  leftOptions = toc.GetTranslationOptionList(leftSourceSegment);
      const TranslationOptionList&  rightOptions = toc.GetTranslationOptionList(rightSourceSegment);
      for (TranslationOptionList::const_iterator ri = rightOptions.begin(); ri != rightOptions.end(); ++ri) {
        for (TranslationOptionList::const_iterator li = leftOptions.begin(); li != leftOptions.end(); ++li) {
          TranslationDelta* delta = new SplitDelta(sample, *li, *ri, gap, GetGainFunction());
          deltas.push_back(delta);
        }
      }
      
  }
    
  VERBOSE(3,"Created " << deltas.size() << " delta(s)" << endl);
  doSample(deltas, noChangeDelta);
    
  //clean up
  RemoveAllInColl(deltas);
  delete noChangeDelta;

}

void TranslationSwapOperator::scan(
    Sample& sample,
    const TranslationOptionCollection& toc) {
  
  if (m_OpIterator->isStartScan()) {
    m_OpIterator->init(sample);
  }
  else {
    m_OpIterator->next();
  }
  
  size_t curPos = static_cast<SwapIterator*>(m_OpIterator)->GetCurPos(); 
  const Hypothesis* currHypo = sample.GetHypAtSourceIndex(curPos);
  
  TargetGap gap(currHypo->GetPrevHypo(), currHypo->GetNextHypo(), currHypo->GetCurrTargetWordsRange());
  const WordsRange& sourceSegment = currHypo->GetCurrSourceWordsRange();
  VERBOSE(3, "Considering source segment " << sourceSegment << " and target segment " << gap.segment << endl); 
    
  vector<TranslationDelta*> deltas;
  const TranslationOption* noChangeOption = &(currHypo->GetTranslationOption());
  TranslationDelta* noChangeDelta = new TranslationUpdateDelta(sample,noChangeOption,gap, GetGainFunction());
  deltas.push_back(noChangeDelta);
    
    
  const TranslationOptionList&  options = toc.GetTranslationOptionList(sourceSegment);
  for (TranslationOptionList::const_iterator i = options.begin(); i != options.end(); ++i) {
    if (*i != noChangeOption) {
      TranslationDelta* delta = new TranslationUpdateDelta(sample,*i,gap, GetGainFunction());
      deltas.push_back(delta);  
    }
  }
  
  static_cast<SwapIterator*>(m_OpIterator)->SetNextHypo(const_cast<Hypothesis*>(currHypo->GetSourceNextHypo()));
  
  doSample(deltas, noChangeDelta);
    
  RemoveAllInColl(deltas);
  
}

//FIXME - not doing this properly
bool FlipOperator::CheckValidReordering(const Hypothesis* leftTgtHypo, const Hypothesis *rightTgtHypo, const Hypothesis* leftTgtPrevHypo, const Hypothesis* leftTgtNextHypo, const Hypothesis* rightTgtPrevHypo, const Hypothesis* rightTgtNextHypo, float & totalDistortion){
  totalDistortion = 0;
  //linear distortion
  //const DistortionScoreProducer *dsp = StaticData::Instance().GetDistortionScoreProducer();
  //Calculate distortion for leftmost target 
  //who is proposed new leftmost's predecessor?   
//  Hypothesis *leftPrevHypo = const_cast<Hypothesis*>(rightTgtHypo->GetPrevHypo());      
  float distortionScore = 0.0;


  if (leftTgtPrevHypo) {
    distortionScore = ComputeDistortionDistance(
                                                leftTgtPrevHypo->GetCurrSourceWordsRange(),
                                                leftTgtHypo->GetCurrSourceWordsRange()
                                                );
    
    if (abs(distortionScore) > StaticData::Instance().GetMaxDistortion()) {
      return false;
    }  
    totalDistortion += distortionScore;
  }
  
    
  
  if (leftTgtNextHypo) {  
    //Calculate distortion from leftmost target to right target
    distortionScore = ComputeDistortionDistance(
                                                  leftTgtHypo->GetCurrSourceWordsRange(),
                                                  leftTgtNextHypo->GetCurrSourceWordsRange()
                                                  ); 
    
    if (abs(distortionScore) > StaticData::Instance().GetMaxDistortion()) {
      return false;
    }
    
    totalDistortion += distortionScore;
  }  
    
  //Calculate distortion from rightmost target to its successor
  //Hypothesis *rightNextHypo = const_cast<Hypothesis*> (leftTgtHypo->GetNextHypo());  
  
  if (rightTgtPrevHypo  && rightTgtPrevHypo != leftTgtHypo) {  
    distortionScore = ComputeDistortionDistance(
                                              rightTgtPrevHypo->GetCurrSourceWordsRange(),
                                              rightTgtHypo->GetCurrSourceWordsRange()
                                              );
  
    if (abs(distortionScore) > StaticData::Instance().GetMaxDistortion()) {
      return false;
    }
  
    totalDistortion += distortionScore;
  } 
  
  if (rightTgtNextHypo) {  
    //Calculate distortion from leftmost target to right target
    distortionScore = ComputeDistortionDistance(
                                              rightTgtHypo->GetCurrSourceWordsRange(),
                                              rightTgtNextHypo->GetCurrSourceWordsRange()
                                              ); 
  
    if (abs(distortionScore) > StaticData::Instance().GetMaxDistortion()) {
      return false;
    }
  
    totalDistortion += distortionScore;
  }
  
  return true;
}
  
void FlipOperator::CollectAllSplitPoints(Sample& sample) {
  m_splitPoints.clear();
  size_t sourceSize = sample.GetSourceSize();
  for (size_t splitIndex = 0; splitIndex < sourceSize; ++splitIndex) {
    Hypothesis* hypothesis = sample.GetHypAtSourceIndex(splitIndex);
    if (hypothesis->GetCurrSourceWordsRange().GetEndPos() == splitIndex) {
      m_splitPoints.push_back(splitIndex);
    }
  }
}
  
  
void FlipOperator::scan(
    Sample& sample,
    const TranslationOptionCollection&) {
  VERBOSE(2, "Running an iteration of the flip operator" << endl);
  
  if (m_OpIterator->isStartScan()) {
    m_OpIterator->init(sample);
    CollectAllSplitPoints(sample);
  }
  
  if (m_splitPoints.size() < 2)
    return;
  
  m_OpIterator->next();
    
  size_t i = static_cast<FlipIterator*>(m_OpIterator)->GetThisPos();
  size_t j = static_cast<FlipIterator*>(m_OpIterator)->GetThatPos();
  
  if (static_cast<FlipIterator*>(m_OpIterator)->GetDirection() == 1)
    VERBOSE(2, "Forward Flipping phrases at pos" << m_splitPoints[i] << " and "  << m_splitPoints[j] << endl)
  else
    VERBOSE(2, "Backward Flipping phrases at pos" << m_splitPoints[i] << " and "  << m_splitPoints[j] << endl)
    
  Hypothesis* hypothesis = sample.GetHypAtSourceIndex(m_splitPoints[i]);
  WordsRange thisSourceSegment = hypothesis->GetCurrSourceWordsRange();
  WordsRange thisTargetSegment = hypothesis->GetCurrTargetWordsRange();
  
  Hypothesis* followingHyp = sample.GetHypAtSourceIndex(m_splitPoints[j]);  
  //would this be a valid reordering?
  WordsRange followingSourceSegment = followingHyp->GetCurrSourceWordsRange();
  WordsRange followingTargetSegment = followingHyp->GetCurrTargetWordsRange();  
  
  //the delta corresponding to the current translation scores, needs to be subtracted off the delta before applying
  TranslationDelta* noChangeDelta = NULL; 
  vector<TranslationDelta*> deltas;
  
  
  if (thisTargetSegment <  followingTargetSegment ) {
    //source and target order are the same
    bool contiguous = (thisTargetSegment.GetEndPos() + 1 ==  followingTargetSegment.GetStartPos());
    
    /*contiguous on target side, flipping would make this a swap
     would this be a valid reordering if we flipped?*/
    float totalDistortion = 0;
    
    Hypothesis *newLeftNextHypo, *newRightPrevHypo;
    if  (contiguous) {
      newLeftNextHypo = hypothesis;
      newRightPrevHypo = followingHyp;
    } 
    else {
      newLeftNextHypo = const_cast<Hypothesis*>(hypothesis->GetNextHypo());
      newRightPrevHypo = const_cast<Hypothesis*>(followingHyp->GetPrevHypo());
    }
    
    bool isValidSwap = CheckValidReordering(followingHyp, hypothesis, hypothesis->GetPrevHypo(), newLeftNextHypo, newRightPrevHypo, followingHyp->GetNextHypo(), totalDistortion);
    if (isValidSwap) {//yes
      TargetGap leftGap(hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), thisTargetSegment);
      TargetGap rightGap(followingHyp->GetPrevHypo(), followingHyp->GetNextHypo(), followingTargetSegment);
      TranslationDelta* delta = new FlipDelta(sample, &(followingHyp->GetTranslationOption()), 
                                              &(hypothesis->GetTranslationOption()), 
                                              leftGap, rightGap, totalDistortion, GetGainFunction());
      deltas.push_back(delta);
      
      CheckValidReordering(hypothesis, followingHyp, hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), followingHyp->GetPrevHypo(),  followingHyp->GetNextHypo(), totalDistortion); 
      
      noChangeDelta = new   FlipDelta(sample, &(hypothesis->GetTranslationOption()), 
                                      &(followingHyp->GetTranslationOption()), leftGap, rightGap,
                                      totalDistortion, GetGainFunction()); 
      deltas.push_back(noChangeDelta);   
      
    }  
  }
  else {
    //swapped on target side, flipping would make this monotone
    bool contiguous = (thisTargetSegment.GetStartPos() ==  followingTargetSegment.GetEndPos() + 1);
    float totalDistortion = 0;
    
    Hypothesis *newLeftNextHypo, *newRightPrevHypo;
    if  (contiguous) {
      newLeftNextHypo = followingHyp; 
      newRightPrevHypo = hypothesis;
    } 
    else {
      newLeftNextHypo = const_cast<Hypothesis*>(followingHyp->GetNextHypo());
      newRightPrevHypo = const_cast<Hypothesis*>(hypothesis->GetPrevHypo());
    }
    bool isValidSwap = CheckValidReordering(hypothesis, followingHyp, followingHyp->GetPrevHypo(), newLeftNextHypo, newRightPrevHypo, hypothesis->GetNextHypo(), totalDistortion);        
    if (isValidSwap) {//yes
      TargetGap leftGap(followingHyp->GetPrevHypo(), followingHyp->GetNextHypo(), followingTargetSegment);
      TargetGap rightGap(hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), thisTargetSegment);
      
      
      TranslationDelta* delta = new FlipDelta(sample, &(hypothesis->GetTranslationOption()), 
                                              &(followingHyp->GetTranslationOption()),  leftGap, rightGap,
                                              totalDistortion, GetGainFunction());
      deltas.push_back(delta);
      
      
      CheckValidReordering(followingHyp,hypothesis, followingHyp->GetPrevHypo(), followingHyp->GetNextHypo(), hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), totalDistortion);        
      noChangeDelta = new FlipDelta(sample,&(followingHyp->GetTranslationOption()), 
                                    &(hypothesis->GetTranslationOption()), leftGap, rightGap, totalDistortion, GetGainFunction());
      deltas.push_back(noChangeDelta); 
    }  
  }
  
  VERBOSE(3,"Created " << deltas.size() << " delta(s)" << endl);
  
  doSample(deltas, noChangeDelta);
  
  //clean up
  RemoveAllInColl(deltas);

} 
  
}//namespace
