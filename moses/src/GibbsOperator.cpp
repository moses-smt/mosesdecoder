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


/**
  * Extract the target words in the sentence into a vector.
**/
static void getTargetWords(const Sample& sample, vector<Word>& words) {
  const Hypothesis* currHypo = sample.GetTargetTail(); //target tail
  
  //we're now at the dummy hypo at the start of the sentence
  while ((currHypo = (currHypo->GetNextHypo()))) {
    TargetPhrase targetPhrase = currHypo->GetTargetPhrase();
    for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
      words.push_back(targetPhrase.GetWord(i));
    }
  }
  
  IFVERBOSE(2) {
    VERBOSE(2,"Sentence: ");
    for (size_t i = 0; i < words.size(); ++i) {
      VERBOSE(2,words[i] << " ");
    }
    VERBOSE(2,endl);
  }
}


size_t GibbsOperator::getSample(const vector<double>& scores) {
  double sum = scores[0];
  for (size_t i = 1; i < scores.size(); ++i) {
    sum = log_sum(sum,scores[i]);
  }
  
  //random number between 0 and exp(sum)
  double random = exp(sum) * (double)rand() / RAND_MAX;
 
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
    vector<Word> targetWords; //needed to calc lm scores
    getTargetWords(sample,targetWords);
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
      noChangeDelta = new   PairedTranslationUpdateDelta(targetWords,&(prev->GetTranslationOption())
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
          TranslationDelta* delta = new MergeDelta(targetWords,*i,targetSegment);
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
          TranslationDelta* delta = new PairedTranslationUpdateDelta(targetWords, *li, *ri, leftTargetSegment, rightTargetSegment);
          deltas.push_back(delta);
        }
      }
    } else {
      VERBOSE(3, "No existing split" << endl);
      WordsRange sourceSegment = hypothesis->GetCurrSourceWordsRange();
      WordsRange targetSegment = hypothesis->GetCurrTargetWordsRange();
      noChangeDelta = new TranslationUpdateDelta(targetWords,&(hypothesis->GetTranslationOption()),targetSegment);
      //Add TranslationUpdateDeltas
      const TranslationOptionList&  options = toc.GetTranslationOptionList(sourceSegment);
      VERBOSE(3, "Creating simple deltas for source segment " << sourceSegment << " and target segment " << targetSegment  << endl);
      for (TranslationOptionList::const_iterator i = options.begin(); i != options.end(); ++i) {
        TranslationDelta* delta = new TranslationUpdateDelta(targetWords,*i,targetSegment);
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
          TranslationDelta* delta = new SplitDelta(targetWords, *li, *ri, targetSegment);
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
      deltas[chosen]->apply(sample,*noChangeDelta);
      
      VERBOSE(2,"Updated to " << *sample.GetSampleHypothesis() << endl);
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
    vector<Word> targetWords; //needed to calc lm scores
    getTargetWords(sample,targetWords);
    const WordsRange& targetSegment = currHypo->GetCurrTargetWordsRange();
    const WordsRange& sourceSegment = currHypo->GetCurrSourceWordsRange();
    VERBOSE(3, "Considering source segment " << sourceSegment << " and target segment " << targetSegment << endl); 
    TranslationUpdateDelta noChangeDelta(targetWords,&(currHypo->GetTranslationOption()),targetSegment);
    vector<TranslationDelta*> deltas;
    
    const TranslationOptionList&  options = toc.GetTranslationOptionList(sourceSegment);
    for (TranslationOptionList::const_iterator i = options.begin(); i != options.end(); ++i) {
      TranslationDelta* delta = new TranslationUpdateDelta(targetWords,*i,targetSegment);
      deltas.push_back(delta);
    }
    
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
    deltas[chosen]->apply(sample,noChangeDelta);
   }
    
    
    
    currHypo = currHypo->GetSourceNextHypo();
  }
}

  
  
  
}//namespace
