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

using namespace std;

namespace Moses {
  
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
  
  RandomNumberGenerator GibbsOperator::m_random;
  
static double log_sum (double log_a, double log_b)
{
	double v;
	if (log_a < log_b) {
		v = log_b+log ( 1 + exp ( log_a-log_b ));
	} else {
		v = log_a+log ( 1 + exp ( log_b-log_a ));
	}
	return ( v );
}

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



size_t GibbsOperator::getSample(const vector<double>& scores) {
  double sum = scores[0];
  for (size_t i = 1; i < scores.size(); ++i) {
    sum = log_sum(sum,scores[i]);
  }
  
  //random number between 0 and exp(sum)
  double random = exp(sum) * m_random.next();//(double)rand() / RAND_MAX;
 
  random = log(random);
  
  //now figure out which sample
  size_t position = 1;
  sum = scores[0];
  for (; position < scores.size() && sum < random; ++position) {
    sum = log_sum(sum,scores[position]);
  }
   //cout << "random: " << exp(random) <<  " sample: " << position << endl;
  return position-1;
}

void MergeSplitOperator::doIteration(Sample& sample, const TranslationOptionCollection& toc) {
  

  size_t sourceSize = sample.GetSourceSize();
  for (size_t splitIndex = 1; splitIndex < sourceSize; ++splitIndex) {
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
      noChangeDelta = new   PairedTranslationUpdateDelta(sample,&(prev->GetTranslationOption())
          ,&(hypothesis->GetTranslationOption()),leftTargetSegment,rightTargetSegment);
      if (leftTargetSegment.GetEndPos() + 1 ==  rightTargetSegment.GetStartPos()) {
        //contiguous on target side.
        //Add MergeDeltas
        WordsRange sourceSegment(leftSourceSegment.GetStartPos(), rightSourceSegment.GetEndPos());
        WordsRange targetSegment(leftTargetSegment.GetStartPos(), rightTargetSegment.GetEndPos());
        VERBOSE(3, "Creating merge deltas for merging source segments  " << leftSourceSegment << " with " <<
              rightSourceSegment << " and target segments " << leftTargetSegment << " with " << rightTargetSegment  << endl);
        const TranslationOptionList&  options = toc.GetTranslationOptionList(sourceSegment);
        for (TranslationOptionList::const_iterator i = options.begin(); i != options.end(); ++i) {
          TranslationDelta* delta = new MergeDelta(sample,*i,targetSegment);
          deltas.push_back(delta);
        }
      }
           
      //Add PairedTranslationUpdateDeltas
      VERBOSE(3, "Creating paired updates for source segments " << leftSourceSegment << " and " << rightSourceSegment <<
          " and target segments " << leftTargetSegment << " and " << rightTargetSegment << endl);
      const TranslationOptionList&  leftOptions = toc.GetTranslationOptionList(leftSourceSegment);
      const TranslationOptionList&  rightOptions = toc.GetTranslationOptionList(rightSourceSegment);
      for (TranslationOptionList::const_iterator ri = rightOptions.begin(); ri != rightOptions.end(); ++ri) {
        for (TranslationOptionList::const_iterator li = leftOptions.begin(); li != leftOptions.end(); ++li) {
          TranslationDelta* delta = new PairedTranslationUpdateDelta(sample,*li, *ri, leftTargetSegment, rightTargetSegment);
          deltas.push_back(delta);
        }
      }
    } else {
      VERBOSE(3, "No existing split" << endl);
      WordsRange sourceSegment = hypothesis->GetCurrSourceWordsRange();
      WordsRange targetSegment = hypothesis->GetCurrTargetWordsRange();
      noChangeDelta = new TranslationUpdateDelta(sample,&(hypothesis->GetTranslationOption()),targetSegment);
      //Add TranslationUpdateDeltas
      const TranslationOptionList&  options = toc.GetTranslationOptionList(sourceSegment);
      VERBOSE(3, "Creating simple deltas for source segment " << sourceSegment << " and target segment " << targetSegment  << endl);
      for (TranslationOptionList::const_iterator i = options.begin(); i != options.end(); ++i) {
        TranslationDelta* delta = new TranslationUpdateDelta(sample,*i,targetSegment);
        deltas.push_back(delta);
      }

      
      //Add SplitDeltas
      VERBOSE(3, "Adding deltas to split " << sourceSegment << " at " << splitIndex << endl);
      WordsRange leftSourceSegment(sourceSegment.GetStartPos(),splitIndex-1);
      WordsRange rightSourceSegment(splitIndex,sourceSegment.GetEndPos());
      const TranslationOptionList&  leftOptions = toc.GetTranslationOptionList(leftSourceSegment);
      const TranslationOptionList&  rightOptions = toc.GetTranslationOptionList(rightSourceSegment);
      for (TranslationOptionList::const_iterator ri = rightOptions.begin(); ri != rightOptions.end(); ++ri) {
        for (TranslationOptionList::const_iterator li = leftOptions.begin(); li != leftOptions.end(); ++li) {
          TranslationDelta* delta = new SplitDelta(sample, *li, *ri, targetSegment);
          deltas.push_back(delta);
        }
      }
      
    }
    
    
    
    VERBOSE(3,"Created " << deltas.size() << " delta(s)" << endl);
    
    //get the scores
    vector<double> scores;
    for (vector<TranslationDelta*>::iterator i = deltas.begin(); i != deltas.end(); ++i) {
      scores.push_back((**i).getScore());
    }
    
    //randomly pick one of the deltas
    if (scores.size() > 0) {
      size_t chosen = getSample(scores);
      IFVERBOSE(4) {
        VERBOSE(4,"Scores: ");
        for (size_t i = 0; i < scores.size(); ++i) {
          VERBOSE(4,scores[i] << ",");
        }
        VERBOSE(4,endl);
      }
      VERBOSE(3,"The chosen sample is " << chosen << endl);
      
      //apply it to the sample
      deltas[chosen]->apply(*noChangeDelta);
      
      VERBOSE(2,"Updated to " << *sample.GetSampleHypothesis() << endl);
      VERBOSE(2,"Sample fvs " << sample.GetFeatureValues() << endl);
    }
    
    
    //clean up
    RemoveAllInColl(deltas);
    delete noChangeDelta;
  }
}

void TranslationSwapOperator::doIteration(Sample& sample, const TranslationOptionCollection& toc) {
  const Hypothesis* currHypo = sample.GetHypAtSourceIndex(0);
  //Iterate in source order
  while (currHypo) {
    const WordsRange& targetSegment = currHypo->GetCurrTargetWordsRange();
    const WordsRange& sourceSegment = currHypo->GetCurrSourceWordsRange();
    VERBOSE(3, "Considering source segment " << sourceSegment << " and target segment " << targetSegment << endl); 
    
    vector<TranslationDelta*> deltas;
    const TranslationOption* noChangeOption = &(currHypo->GetTranslationOption());
    TranslationDelta* noChangeDelta = new TranslationUpdateDelta(sample,noChangeOption,targetSegment);
    deltas.push_back(noChangeDelta);
    
    
    const TranslationOptionList&  options = toc.GetTranslationOptionList(sourceSegment);
    for (TranslationOptionList::const_iterator i = options.begin(); i != options.end(); ++i) {
      if (*i != noChangeOption) {
        TranslationDelta* delta = new TranslationUpdateDelta(sample,*i,targetSegment);
        deltas.push_back(delta);  
      }
    }
    
    //advance thru the linked list now, before currHypo gets invalidated
    currHypo = currHypo->GetSourceNextHypo();
    
    //get the scores
    vector<double> scores;
    for (vector<TranslationDelta*>::iterator i = deltas.begin(); i != deltas.end(); ++i) {
      scores.push_back((**i).getScore());
    }
    
   //randomly pick one of the deltas
   if (scores.size() > 0) {
   size_t chosen = getSample(scores);
   IFVERBOSE(4) {
    VERBOSE(4,"Scores: ");
    for (size_t i = 0; i < scores.size(); ++i) {
      VERBOSE(4,scores[i] << ",");
    }
    VERBOSE(4,endl);
    }
    VERBOSE(3,"The chosen sample is " << chosen << endl);
    
     
    //apply it to the sample
    if (deltas[chosen] !=  noChangeDelta);{
      deltas[chosen]->apply(*noChangeDelta);     
    }
     VERBOSE(2,"Updated to " << *sample.GetSampleHypothesis() << endl);
     VERBOSE(2,"Sample fvs " << sample.GetFeatureValues() << endl);
 
   }
    
   RemoveAllInColl(deltas);
  }
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

void FlipOperator::CollectAllSplitPoints(Sample& sample, vector<int> &splitPoints) {
  size_t sourceSize = sample.GetSourceSize();
  for (size_t splitIndex = 0; splitIndex < sourceSize; ++splitIndex) {
    Hypothesis* hypothesis = sample.GetHypAtSourceIndex(splitIndex);
    if (hypothesis->GetCurrSourceWordsRange().GetEndPos() == splitIndex) {
      splitPoints.push_back(static_cast<int>(splitIndex));
    }
  }
}
  
  
void FlipOperator::doIteration(Sample& sample, const TranslationOptionCollection&) {
  VERBOSE(2, "Running an iteration of the flip operator" << endl);
  vector <int> splitPoints;
  CollectAllSplitPoints(sample, splitPoints);  //collect all split points for this sample
  
    
  for (unsigned int i = 0; i < splitPoints.size(); ++i) {
    for (unsigned int j = i+1; j < splitPoints.size(); ++j) {//let's just look at the source side successors  
      VERBOSE(2, "Forward Flipping phrases at pos" << splitPoints[i] << " and "  << splitPoints[j] << endl);
      
      Hypothesis* hypothesis = sample.GetHypAtSourceIndex(splitPoints[i]);
      WordsRange thisSourceSegment = hypothesis->GetCurrSourceWordsRange();
      WordsRange thisTargetSegment = hypothesis->GetCurrTargetWordsRange();
      
      Hypothesis* followingHyp = sample.GetHypAtSourceIndex(splitPoints[j]);  
      //would this be a valid reordering?
      WordsRange followingSourceSegment = followingHyp->GetCurrSourceWordsRange();
      WordsRange followingTargetSegment = followingHyp->GetCurrTargetWordsRange();  
      
      //the delta corresponding to the current translation scores, needs to be subtracted off the delta before applying
      TranslationDelta* noChangeDelta = NULL; 
      vector<TranslationDelta*> deltas;
      
      
      if (thisTargetSegment <  followingTargetSegment ) {
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
          TranslationDelta* delta = new FlipDelta(sample, &(followingHyp->GetTranslationOption()), 
                                                  &(hypothesis->GetTranslationOption()),  hypothesis->GetPrevHypo(), followingHyp->GetNextHypo(), followingTargetSegment,thisTargetSegment, totalDistortion);
          deltas.push_back(delta);
          
          CheckValidReordering(hypothesis, followingHyp, hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), followingHyp->GetPrevHypo(),  followingHyp->GetNextHypo(), totalDistortion); 
          
          noChangeDelta = new   FlipDelta(sample, &(hypothesis->GetTranslationOption()), 
                                        &(followingHyp->GetTranslationOption()), hypothesis->GetPrevHypo(), followingHyp->GetNextHypo() ,thisTargetSegment,followingTargetSegment, totalDistortion); 
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
          TranslationDelta* delta = new FlipDelta(sample, &(hypothesis->GetTranslationOption()), 
                                                  &(followingHyp->GetTranslationOption()),  followingHyp->GetPrevHypo(), hypothesis->GetNextHypo(), thisTargetSegment,followingTargetSegment, totalDistortion);
          deltas.push_back(delta);
          
          
          CheckValidReordering(followingHyp,hypothesis, followingHyp->GetPrevHypo(), followingHyp->GetNextHypo(), hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), totalDistortion);        
          noChangeDelta = new FlipDelta(sample,&(followingHyp->GetTranslationOption()), 
                                      &(hypothesis->GetTranslationOption()), followingHyp->GetPrevHypo(), hypothesis->GetNextHypo(), followingTargetSegment,thisTargetSegment, totalDistortion);
          deltas.push_back(noChangeDelta); 
        }  
      }
      
      VERBOSE(3,"Created " << deltas.size() << " delta(s)" << endl);
      
      //get the scores
      vector<double> scores;
      for (vector<TranslationDelta*>::iterator i = deltas.begin(); i != deltas.end(); ++i) {
        scores.push_back((**i).getScore());
      }
      
      //randomly pick one of the deltas
      if (scores.size() > 0) {
        size_t chosen = getSample(scores);
        IFVERBOSE(4) {
          VERBOSE(4,"Scores: ");
          for (size_t i = 0; i < scores.size(); ++i) {
            VERBOSE(4,scores[i] << ",");
          }
          VERBOSE(4,endl);
        }
        VERBOSE(3,"The chosen sample is " << chosen << endl);
        
        //apply it to the sample
        if (deltas[chosen] != noChangeDelta) {
          deltas[chosen]->apply(*noChangeDelta);  
        }
        
        VERBOSE(2,"Updated to " << *sample.GetSampleHypothesis() << endl);
        VERBOSE(2,"Sample fvs " << sample.GetFeatureValues() << endl);

      }
      
      //clean up
      RemoveAllInColl(deltas);
    }
    for (int j = i-1; j >= 0; --j) {//let's just look at the source side successors  
      VERBOSE(2, "Backward Flipping phrases at pos" << splitPoints[j] << " and "  << splitPoints[i] << endl);
      Hypothesis* hypothesis = sample.GetHypAtSourceIndex(splitPoints[j]);
      WordsRange thisSourceSegment = hypothesis->GetCurrSourceWordsRange();
      WordsRange thisTargetSegment = hypothesis->GetCurrTargetWordsRange();
      
      Hypothesis* followingHyp = sample.GetHypAtSourceIndex(splitPoints[i]);  
      //would this be a valid reordering?
      WordsRange followingSourceSegment = followingHyp->GetCurrSourceWordsRange();
      WordsRange followingTargetSegment = followingHyp->GetCurrTargetWordsRange();  
      
      //the delta corresponding to the current translation scores, needs to be subtracted off the delta before applying
      TranslationDelta* noChangeDelta = NULL; 
      vector<TranslationDelta*> deltas;
      
      
      if (thisTargetSegment <  followingTargetSegment ) {
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
          TranslationDelta* delta = new FlipDelta(sample, &(followingHyp->GetTranslationOption()), 
                                                  &(hypothesis->GetTranslationOption()),  hypothesis->GetPrevHypo(), followingHyp->GetNextHypo(), followingTargetSegment,thisTargetSegment, totalDistortion);
          deltas.push_back(delta);          
        
          CheckValidReordering(hypothesis, followingHyp, hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), followingHyp->GetPrevHypo(),  followingHyp->GetNextHypo(), totalDistortion); 
          noChangeDelta = new   FlipDelta(sample, &(hypothesis->GetTranslationOption()), 
                                        &(followingHyp->GetTranslationOption()), hypothesis->GetPrevHypo(), followingHyp->GetNextHypo() ,thisTargetSegment,followingTargetSegment, totalDistortion); 
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
          TranslationDelta* delta = new FlipDelta(sample, &(hypothesis->GetTranslationOption()), 
                                                  &(followingHyp->GetTranslationOption()),  followingHyp->GetPrevHypo(), hypothesis->GetNextHypo(), thisTargetSegment,followingTargetSegment, totalDistortion);
          deltas.push_back(delta);
          
          CheckValidReordering(followingHyp,hypothesis, followingHyp->GetPrevHypo(), followingHyp->GetNextHypo(), hypothesis->GetPrevHypo(), hypothesis->GetNextHypo(), totalDistortion);        
          noChangeDelta = new FlipDelta(sample,&(followingHyp->GetTranslationOption()), 
                                      &(hypothesis->GetTranslationOption()), followingHyp->GetPrevHypo(), hypothesis->GetNextHypo(), followingTargetSegment,thisTargetSegment, totalDistortion);
          deltas.push_back(noChangeDelta); 
        }  
      }
      
      VERBOSE(3,"Created " << deltas.size() << " delta(s)" << endl);
      
      //get the scores
      vector<double> scores;
      for (vector<TranslationDelta*>::iterator i = deltas.begin(); i != deltas.end(); ++i) {
        scores.push_back((**i).getScore());
      }
      
      //randomly pick one of the deltas
      if (scores.size() > 0) {
        size_t chosen = getSample(scores);
        IFVERBOSE(4) {
          VERBOSE(4,"Scores: ");
          for (size_t i = 0; i < scores.size(); ++i) {
            VERBOSE(4,scores[i] << ",");
          }
          VERBOSE(4,endl);
        }
        VERBOSE(3,"The chosen sample is " << chosen << endl);
        
        //apply it to the sample
        if (deltas[chosen] != noChangeDelta) {
          deltas[chosen]->apply(*noChangeDelta);  
        }
        
        VERBOSE(2,"Updated to " << *sample.GetSampleHypothesis() << endl);
        VERBOSE(2,"Sample fvs " << sample.GetFeatureValues() << endl);

      }
      
      //clean up
      RemoveAllInColl(deltas);
    }
  }  
    
}  
  
  
}//namespace
