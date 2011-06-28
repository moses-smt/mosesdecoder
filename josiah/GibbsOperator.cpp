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

#include "Hypothesis.h"
#include "TranslationOptionCollection.h"
#include "Utils.h"
#include "WordsRange.h"

#include "GibbsOperator.h"
#include "Selector.h"


using namespace std;
using namespace Moses;

namespace Josiah {
  
  
  


static float ComputeDistortionDistance(const WordsRange& prev, const WordsRange& current) 
{
  int dist = 0;
  if (prev.GetNumWordsCovered() == 0) {
    dist = current.GetStartPos();
  } else {
    dist = (int)prev.GetEndPos() - (int)current.GetStartPos() + 1 ;
  }
  //cerr << "Computing dist " << prev << " " << current << " " << -abs(dist) << endl;
  return - (float) abs(dist);
}

 
GibbsOperator::~GibbsOperator() {} 
  
PrunedTranslationOptionList::PrunedTranslationOptionList(
        const TranslationOptionCollection& toc, 
        const WordsRange& segment,
        size_t count) :
  m_options(toc.GetTranslationOptionList(segment)),
  m_count(count)
{ }


TranslationOptionList::const_iterator
PrunedTranslationOptionList::begin() const {
  return m_options.begin();
}

TranslationOptionList::const_iterator
PrunedTranslationOptionList::end() const {
  if (!m_count || m_count > m_options.size()) {
    return m_options.end();
  } else {
    return m_options.begin() + m_count;
  }
}
  
  
/*
  if (sample.DoRaoBlackwell()) {
  FVector fv(sample.GetFeatureValues());
  fv -= noChangeDelta->getScores();
      //Add FV(d)*p(d) for each delta.
  vector<double> scores;
      //m_acceptor->getNormalisedScores(deltas,scores);
      //scores now contain the normalised logprobs
  assert(scores.size() == deltas.size());
  for (size_t i = 0; i < deltas.size(); ++i) {
    if (scores[i] < -30) continue; //floor
    FVector deltaFv = deltas[i]->getScores();
    deltaFv *= exp(scores[i]);
    fv +=deltaFv;
  }
      //cout << "Rao-Blackwellised fv: " << fv << endl;
  sample.AddConditionalFeatureValues(fv);
} */
  
  
  
void MergeSplitOperator::propose(Sample& sample, const TranslationOptionCollection& toc,
                             TDeltaVector& deltas, TDeltaHandle& noChangeDelta)
{
 
  
  size_t sourceSize = sample.GetSourceSize();
  if (sourceSize == 1) return;
  size_t splitIndex = RandomNumberGenerator::instance().
          getRandomIndexFromZeroToN(sourceSize-1) + 1;
 
  //NB splitIndex n refers to the position between word n-1 and word n. Words are zero indexed
  VERBOSE(3,"Sampling at source index " << splitIndex << endl);
    
  Hypothesis* hypothesis = sample.GetHypAtSourceIndex(splitIndex);
  
  auto_ptr<TargetGap> gap;
  auto_ptr<TargetGap> leftGap;
  auto_ptr<TargetGap> rightGap;
    
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
      gap.reset( new TargetGap(prev->GetPrevHypo(), hypothesis->GetNextHypo(), targetSegment));
      VERBOSE(3, "Creating merge deltas for merging source segments  " << leftSourceSegment << " with " <<
             rightSourceSegment << " and target segments " << leftTargetSegment << " with " << rightTargetSegment  << endl);
      PrunedTranslationOptionList  options(toc, sourceSegment, m_toptionLimit);
      for (TranslationOptionList::const_iterator i = options.begin(); i != options.end(); ++i) {
        TDeltaHandle delta(new MergeDelta(sample,*i,*(gap.get())));
        deltas.push_back(delta);
      }
    }
    
    //make sure that the 'left' and 'right' refer to the target order
    auto_ptr<PrunedTranslationOptionList> leftOptions;
    auto_ptr<PrunedTranslationOptionList> rightOptions;
    
    if (leftTargetSegment < rightTargetSegment) {
        //source and target order same
        leftOptions.reset(new PrunedTranslationOptionList(toc,leftSourceSegment,m_toptionLimit));
        rightOptions.reset(new PrunedTranslationOptionList(toc,rightSourceSegment,m_toptionLimit));
        leftGap.reset(new TargetGap(prev->GetPrevHypo(), prev->GetNextHypo(), prev->GetCurrTargetWordsRange()));
        rightGap.reset(new TargetGap(hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), 
              hypothesis->GetCurrTargetWordsRange()));
        noChangeDelta.reset(new   PairedTranslationUpdateDelta(sample,&(prev->GetTranslationOption())
          ,&(hypothesis->GetTranslationOption()),*leftGap, *rightGap));
        
    } else {
        //target in opposite order to source
        leftOptions.reset(new PrunedTranslationOptionList(toc,rightSourceSegment,m_toptionLimit));
        rightOptions.reset(new PrunedTranslationOptionList(toc,leftSourceSegment,m_toptionLimit));
        leftGap.reset(new TargetGap(hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), 
              hypothesis->GetCurrTargetWordsRange()));
        rightGap.reset(new TargetGap(prev->GetPrevHypo(), prev->GetNextHypo(), prev->GetCurrTargetWordsRange()));
        noChangeDelta.reset(new   PairedTranslationUpdateDelta(sample,&(hypothesis->GetTranslationOption())
          ,&(prev->GetTranslationOption()),*leftGap, *rightGap));
    }
      

    //Add PairedTranslationUpdateDeltas
      
    for (TranslationOptionList::const_iterator ri = rightOptions->begin(); ri != rightOptions->end(); ++ri) {
      for (TranslationOptionList::const_iterator li = leftOptions->begin(); li != leftOptions->end(); ++li) {
        TDeltaHandle delta(new PairedTranslationUpdateDelta(sample,*li, *ri, *leftGap, *rightGap));
        deltas.push_back(delta);
      }
    }
      //cerr << "Added " << ds << " deltas" << endl;
  } else {
      VERBOSE(3, "No existing split" << endl);
      WordsRange sourceSegment = hypothesis->GetCurrSourceWordsRange();
      gap.reset( new TargetGap(hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), hypothesis->GetCurrTargetWordsRange()));
      noChangeDelta.reset(new TranslationUpdateDelta(sample,&(hypothesis->GetTranslationOption()),*(gap.get())));
      //Add TranslationUpdateDeltas
      PrunedTranslationOptionList options(toc,sourceSegment,m_toptionLimit);
      //cerr << "Got " << options.size() << " options for " << sourceSegment << endl;
      VERBOSE(3, "Creating simple deltas for source segment " << sourceSegment << " and target segment " <<gap.get()->segment
            << endl);
      for (TranslationOptionList::const_iterator i = options.begin(); i != options.end(); ++i) {
        TDeltaHandle delta(new TranslationUpdateDelta(sample,*i,*(gap.get())));
        deltas.push_back(delta);
      }
      //cerr << "Added " << ds << " deltas" << endl;

      
      //Add SplitDeltas
      VERBOSE(3, "Adding deltas to split " << sourceSegment << " at " << splitIndex << endl);
      //Note no reordering in split
      WordsRange leftSourceSegment(sourceSegment.GetStartPos(),splitIndex-1);
      WordsRange rightSourceSegment(splitIndex,sourceSegment.GetEndPos());
      PrunedTranslationOptionList leftOptions(toc,leftSourceSegment,m_toptionLimit);
      PrunedTranslationOptionList rightOptions(toc,rightSourceSegment,m_toptionLimit);
      for (TranslationOptionList::const_iterator ri = rightOptions.begin(); ri != rightOptions.end(); ++ri) {
        for (TranslationOptionList::const_iterator li = leftOptions.begin(); li != leftOptions.end(); ++li) {
          TDeltaHandle delta(new SplitDelta(sample, *li, *ri, *(gap.get())));
          deltas.push_back(delta);
        }
      }
      
  }
}

void TranslationSwapOperator::propose(Sample& sample, const TranslationOptionCollection& toc,
                             TDeltaVector& deltas, TDeltaHandle& noChangeDelta)  {
  
  
  size_t curPos = RandomNumberGenerator::instance().getRandomIndexFromZeroToN(sample.GetSourceSize());
  const Hypothesis* currHypo = sample.GetHypAtSourceIndex(curPos);
  
  TargetGap gap(currHypo->GetPrevHypo(), currHypo->GetNextHypo(), currHypo->GetCurrTargetWordsRange());
  const WordsRange& sourceSegment = currHypo->GetCurrSourceWordsRange();
  VERBOSE(3, "Considering source segment " << sourceSegment << " and target segment " << gap.segment << endl); 
    
  const TranslationOption* noChangeOption = &(currHypo->GetTranslationOption());
  noChangeDelta.reset(new TranslationUpdateDelta(sample,noChangeOption,gap));
    
    
  //const TranslationOptionList&  options = toc.GetTranslationOptionList(sourceSegment);
  PrunedTranslationOptionList options(toc,sourceSegment,m_toptionLimit);
  for (TranslationOptionList::const_iterator i = options.begin(); i != options.end(); ++i) {
      TDeltaHandle delta(new TranslationUpdateDelta(sample,*i,gap));
      deltas.push_back(delta);
  }
}

  
void FlipOperator::propose(Sample& sample, const TranslationOptionCollection& toc,
                             TDeltaVector& deltas, TDeltaHandle& noChangeDelta)
{
  VERBOSE(2, "Running an iteration of the flip operator" << endl);
  CollectAllSplitPoints(sample);
  if (m_splitPoints.size() < 2) {
    return;
  }
  
  size_t i = RandomNumberGenerator::instance().getRandomIndexFromZeroToN(GetSplitPoints().size());
  size_t j = i;
  while (i == j) {
    j = RandomNumberGenerator::instance().getRandomIndexFromZeroToN(GetSplitPoints().size());
  }
  
  if (i < j) {
    VERBOSE(2, "Forward Flipping phrases at pos " << m_splitPoints[i] << " and "  << m_splitPoints[j] << endl);
  } else {
    VERBOSE(2, "Backward Flipping phrases at pos " << m_splitPoints[i] << " and "  << m_splitPoints[j] << endl);
  }
    
  Hypothesis* hypothesis = sample.GetHypAtSourceIndex(m_splitPoints[i]);
  WordsRange thisSourceSegment = hypothesis->GetCurrSourceWordsRange();
  WordsRange thisTargetSegment = hypothesis->GetCurrTargetWordsRange();
  
  Hypothesis* followingHyp = sample.GetHypAtSourceIndex(m_splitPoints[j]);  
  //would this be a valid reordering?
  WordsRange followingSourceSegment = followingHyp->GetCurrSourceWordsRange();
  WordsRange followingTargetSegment = followingHyp->GetCurrTargetWordsRange();  
  
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
    
    bool isValidSwap = CheckValidReordering(followingHyp->GetCurrSourceWordsRange(), hypothesis->GetCurrSourceWordsRange(), hypothesis->GetPrevHypo(), newLeftNextHypo, newRightPrevHypo, followingHyp->GetNextHypo(), totalDistortion);
    if (isValidSwap) {//yes
      TargetGap leftGap(hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), thisTargetSegment);
      TargetGap rightGap(followingHyp->GetPrevHypo(), followingHyp->GetNextHypo(), followingTargetSegment);
      TDeltaHandle delta(new FlipDelta(sample, &(followingHyp->GetTranslationOption()), 
                                              &(hypothesis->GetTranslationOption()), 
                                              leftGap, rightGap));
      deltas.push_back(delta);
      
      CheckValidReordering(hypothesis->GetCurrSourceWordsRange(), followingHyp->GetCurrSourceWordsRange(), hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), followingHyp->GetPrevHypo(),  followingHyp->GetNextHypo(), totalDistortion); 
      
      noChangeDelta.reset(new  FlipDelta(sample, &(hypothesis->GetTranslationOption()), 
                       &(followingHyp->GetTranslationOption()), leftGap, rightGap)); 
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
    bool isValidSwap = CheckValidReordering(hypothesis->GetCurrSourceWordsRange(), followingHyp->GetCurrSourceWordsRange(), followingHyp->GetPrevHypo(), newLeftNextHypo, newRightPrevHypo, hypothesis->GetNextHypo(), totalDistortion);        
    if (isValidSwap) {//yes
      TargetGap leftGap(followingHyp->GetPrevHypo(), followingHyp->GetNextHypo(), followingTargetSegment);
      TargetGap rightGap(hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), thisTargetSegment);
      
      
      TDeltaHandle delta(new FlipDelta(sample, &(hypothesis->GetTranslationOption()), 
                                              &(followingHyp->GetTranslationOption()),  leftGap, rightGap));
      deltas.push_back(delta);
      
      
      CheckValidReordering(followingHyp->GetCurrSourceWordsRange(),hypothesis->GetCurrSourceWordsRange(), followingHyp->GetPrevHypo(), followingHyp->GetNextHypo(), hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), totalDistortion);        
      noChangeDelta.reset(new FlipDelta(sample,&(followingHyp->GetTranslationOption()), 
                       &(hypothesis->GetTranslationOption()), leftGap, rightGap));
      deltas.push_back(noChangeDelta); 
    }  
  }
} 

  bool CheckValidReordering(const WordsRange& leftSourceSegment, const WordsRange& rightSourceSegment, const Hypothesis* leftTgtPrevHypo, const Hypothesis* leftTgtNextHypo, const Hypothesis* rightTgtPrevHypo, const Hypothesis* rightTgtNextHypo, float & totalDistortion){
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
                                                  leftSourceSegment
                                                  );
      
      if (abs(distortionScore) > StaticData::Instance().GetMaxDistortion()) {
        return false;
      }  
      totalDistortion += distortionScore;
    }
    
    
    
    if (leftTgtNextHypo) {  
      //Calculate distortion from leftmost target to right target
      distortionScore = ComputeDistortionDistance(
                                                  leftSourceSegment,
                                                  leftTgtNextHypo->GetCurrSourceWordsRange()
                                                  ); 
      
      if (abs(distortionScore) > StaticData::Instance().GetMaxDistortion()) {
        return false;
      }
      
      totalDistortion += distortionScore;
    }  
    
    //Calculate distortion from rightmost target to its successor
    //Hypothesis *rightNextHypo = const_cast<Hypothesis*> (leftTgtHypo->GetNextHypo());  
    
    if (rightTgtPrevHypo  && rightTgtPrevHypo->GetCurrSourceWordsRange() != leftSourceSegment) {  
      distortionScore = ComputeDistortionDistance(
                                                  rightTgtPrevHypo->GetCurrSourceWordsRange(),
                                                  rightSourceSegment
                                                  );
      
      if (abs(distortionScore) > StaticData::Instance().GetMaxDistortion()) {
        return false;
      }
      
      totalDistortion += distortionScore;
    } 
    
    if (rightTgtNextHypo) {  
      //Calculate distortion from leftmost target to right target
      distortionScore = ComputeDistortionDistance(
                                                  rightSourceSegment,
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
  
  
}//namespace
