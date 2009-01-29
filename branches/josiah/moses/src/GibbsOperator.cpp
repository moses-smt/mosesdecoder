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
  * This class hierarchy represents the possible changes in the translation effected
  * by the merge-split operator.
  **/
class TDelta {
  public:
    TDelta(): m_score(-1e6) {}
  
    
    double getScore() {
      return m_score;
    }
    virtual ~TDelta() {}
  protected:
    ScoreComponentCollection m_scores;
    double m_score;
    
};
 
/**
  * A single phrase on source and target side.
  **/
class SingleTDelta : public virtual TDelta {
  public:
     SingleTDelta (const Hypothesis* hypothesis,  const TranslationOption* option , const WordsRange& targetSegment) :
        m_option(option), m_targetSegment(targetSegment) {
        m_scores = m_option->GetScoreBreakdown();
        
        //translation scores are already there
        const vector<PhraseDictionary*>& translationModels = StaticData::Instance().GetPhraseDictionaries();
        //cout << "Got " << translationModels.size() << " dictionary(s)" << endl;
        for (vector<PhraseDictionary*>::const_iterator i = translationModels.begin(); i != translationModels.end(); ++i) {
          vector<float> translationScores = m_scores.GetScoresForProducer(*i);
          cout << "Translation scores: ";
          copy(translationScores.begin(), translationScores.end(), ostream_iterator<float>(cout,","));
          cout << endl;
        }
        
        //don't worry about reordering because they don't change
        
        //TODO: word penalty
        
        //language model scores
        const LMList& languageModels = StaticData::Instance().GetAllLM();
        for (LMList::const_iterator i = languageModels.begin(); i != languageModels.end(); ++i) {
          LanguageModel* lm = *i;
          size_t order = lm->GetNGramOrder();
          int start = m_targetSegment.GetStartPos() - (order-1);
          int end = m_targetSegment.GetEndPos() + (order - 1);
          cout << "LM for " << start << " to " << end << endl;
          //build up the context, possibly starting with before sentence indicators
          vector<const Word*> lmcontext;;
          int currPos = start;
          for (; currPos < 0; ++currPos) {
            lmcontext.push_back(&(lm->GetSentenceStartArray()));
          }
          
          //find the hypothesis which covers the target position curr
          const Hypothesis* currHypo = hypothesis;
          while (currHypo->GetCurrTargetWordsRange().GetStartPos() > currPos) {
            currHypo = currHypo->GetPrevHypo();
            assert (currHypo);
          }
          
          //read words up to end
          size_t eoscount = 0;
          while (currPos <= end) {
            while (currHypo && currPos > currHypo->GetCurrTargetWordsRange().GetEndPos()) {
              currHypo = currHypo->GetNextHypo();
            }
            if (currPos >= m_targetSegment.GetStartPos() && currPos <= m_targetSegment.GetEndPos()) {
              lmcontext.push_back(&(m_option->GetTargetPhrase().GetWord(currPos-m_targetSegment.GetStartPos())));
            } else if (currHypo) {
              lmcontext.push_back(&(currHypo->GetCurrWord(currPos - currHypo->GetCurrTargetWordsRange().GetStartPos())));
            } else {
              lmcontext.push_back(&(lm->GetSentenceEndArray()));
              ++eoscount;
            }
            ++currPos;
          }
          cout << "Target range " << m_targetSegment << endl;
          //cout << "Phrase for LM: " << phrase.GetStringRep(StaticData::Instance().GetOutputFactorOrder()) << " size=" << 
            //phrase.GetSize() << endl;
            
          //calc lm score
          cout << "LM context ";
          for (size_t j = 0;  j < lmcontext.size(); ++j) {
            cout << *(lmcontext[j]) << " ";
          }
          cout << endl;
          double lmscore = 0;
          vector<const Word*> ngram;
          //remember to only include max of 1 eos marker
          size_t maxend = min(lmcontext.size(), lmcontext.size() - (eoscount-1));
          for (size_t ngramstart = 0; ngramstart < lmcontext.size() - (order -1); ++ngramstart) {
            ngram.clear();
            for (size_t j = ngramstart; j < ngramstart+order && j < maxend; ++j) {
              ngram.push_back(lmcontext[j]);
              
              
            }
            lmscore += lm->GetValue(ngram);
          }
          cout << "Language model score: " << lmscore << endl;
          m_scores.Assign(lm,lmscore);
          
        }
        
        //weight the scores
        const vector<float> weights = StaticData::Instance().GetAllWeights();
        cout << " weights: ";
        copy(weights.begin(), weights.end(), ostream_iterator<float>(cout," "));
        cout << endl;
        m_score = m_scores.InnerProduct(weights);
        cout << "Calculated score as " << m_score << endl;
        
     }
  private:
    const TranslationOption* m_option;
    WordsRange m_targetSegment;
    
};

/**
  * A pair of source phrase mapping to contigous pair on target side.
  **/
class ContigTDelta: public virtual TDelta {
  public:
    ContigTDelta(const TranslationOption* option1, const TranslationOption* option2,
      const WordsRange& targetSegment) :
      m_option1(option1), m_option2(option2), m_targetSegment(targetSegment) {}
  private:
    const TranslationOption* m_option1;
    const TranslationOption* m_option2;
    WordsRange m_targetSegment;
};

/**
  * A pair of source phrases mapping to discontiguous pair on target side.
**/
class DiscontigTDelta: public virtual TDelta {
  public:
    DiscontigTDelta(const TranslationOption* option1, const TranslationOption* option2,
      const WordsRange& targetSegment1, const WordsRange& targetSegment2 ) :
      m_option1(option1), m_option2(option2), m_targetSegment1(targetSegment1), m_targetSegment2(targetSegment2) {}
  private:
    const TranslationOption* m_option1;
    const TranslationOption* m_option2;
    WordsRange m_targetSegment1;
    WordsRange m_targetSegment2;
};





size_t GibbsOperator::getSample(const vector<double>& scores) {
  double sum = scores[0];
  for (size_t i = 1; i < scores.size(); ++i) {
    sum = log_sum(sum,scores[i]);
  }
  
  //random number between 0 and exp(sum)
  double random = exp(sum) * (double)rand() / RAND_MAX;
 
  random = log(random);
  
  //now figure out which sample
  sum = scores[0];
  size_t position = 0;
  for (; position < scores.size() && sum < random; ++position) {
    sum = log_sum(sum,scores[position]);
  }
   cout << "random: " << exp(random) <<  " sample: " << position << endl;
  return position;
}

void MergeSplitOperator::doIteration(Sample& sample, const TranslationOptionCollection& toc) {
  VERBOSE(2, "Running an iteration of the merge split operator" << endl);
  //get the size of the sentence
  //for each of the split points; do
  //  if the point is a segment split:
  //    Get the two segments on either side
  //    if these two segments map to discontiguous target side segments, then no sample
  //      at this split point
  //    Otherwise, get the two target segments mapped from the source segment
  //. if the split point is not a segment split:
  //    Get the target segment corresponding to this source segment
  //  Given the source, and its related targets, generate all possible
  //  translation options, and their scores, then take a sample, and update
  //  the hypothesis.
  //  To work out the score, we have a sequence of source words, which can either be split
  //  at a particular point 
  //  or combined. These map to some possible sequence of target words, same number
  //  of segments as in source, possibly swapped, and translated arbitrarily?
 
  
//   vector<double> probs;
//   probs.push_back(log(0.5));
//   probs.push_back(log(0.25));
//   probs.push_back(log(0.25));
//   probs.push_back(log(1.0));
//   for (size_t i = 0; i < 1000; ++i) {
//     getSample(probs);
//   }
  //cout << RAND_MAX << endl;
  size_t sourceSize = sample.GetSourceSize();
  for (size_t splitIndex = 1; splitIndex < sourceSize; ++splitIndex) {
    //NB splitIndex n refers to the position between word n-1 and word n. Words are zero indexed
    VERBOSE(3,"Sampling at source index " << splitIndex << endl);
    
    Hypothesis* hypothesis = sample.GetHypAtSourceIndex(splitIndex);
    assert(hypothesis);
    
    //find out which source and target segments this split-merge operator should consider
    WordsRange sourceSegment = hypothesis->GetCurrSourceWordsRange();
    WordsRange targetSegment = hypothesis->GetCurrTargetWordsRange();
    
    vector<WordsRange> targetSegments; //could be 1 or 2
    
    if (sourceSegment.GetStartPos() == splitIndex) {
      //we're at the left edge of this segment, so we need to add on the
      //segment to the right.
      const Hypothesis* prev = hypothesis->GetPrevHypo();
      WordsRange prevTargetSegment = prev->GetCurrTargetWordsRange();
      if (prevTargetSegment.GetEndPos() + 1 != targetSegment.GetStartPos()) {
        //targets discontiguous
        //NOTE: if the target segments are contiguous but in reverse order to 
        //source segments, then they count as discontiguous
        targetSegments.push_back(prevTargetSegment);
        targetSegments.push_back(targetSegment);
      } else {
        //targets contiguous, create one big segment
        targetSegment = WordsRange(prevTargetSegment.GetStartPos(),targetSegment.GetEndPos());
        targetSegments.push_back(targetSegment);
      } 
     
      //source segment should be prev and current combined
      sourceSegment = WordsRange(prev->GetCurrSourceWordsRange().GetStartPos(),sourceSegment.GetEndPos());
    } else {
      targetSegments.push_back(targetSegment);
    }
    cout << splitIndex << " ";
    cout << "Source: " << sourceSegment << " ";
    if (targetSegments.size() == 2) {
      cout << "Discontiguous targets: " << targetSegments[0] << "," << targetSegments[1];
    } else {
      cout << "Target: " << targetSegments[0];
    }
    cout << endl;
    
    vector<TDelta*> deltas;
    if (targetSegments.size() == 1) {
      //contiguous
      //case 1: no split
      const TranslationOptionList&  options = toc.GetTranslationOptionList(sourceSegment);
      cout << "Options for : " << sourceSegment << " - " << options.size() << endl;
      for (TranslationOptionList::const_iterator i = options.begin(); 
           i != options.end(); ++i) {
        //cout << "Source: " << *((*i)->GetSourcePhrase()) << " Target: " << ((*i)->GetTargetPhrase()) << endl;
        TDelta* delta = new SingleTDelta(hypothesis, *i,targetSegments[0]);
        deltas.push_back(delta);
     }
      
      //case 2: split
      const TranslationOptionList&  options1 = 
        toc.GetTranslationOptionList(WordsRange(sourceSegment.GetStartPos(),splitIndex-1));
      const TranslationOptionList& options2 = 
        toc.GetTranslationOptionList(WordsRange(splitIndex,sourceSegment.GetEndPos()));
      for (TranslationOptionList::const_iterator i1 = options1.begin(); 
            i1 != options1.end(); ++i1) {
         for (TranslationOptionList::const_iterator i2 = options2.begin(); 
            i2 != options2.end(); ++i2) {
              TDelta* delta = new ContigTDelta(*i1,*i2,targetSegments[0]);
              deltas.push_back(delta);
              //currently no reordering in this operator
              //delta = new ContigTDelta(*i2,*i1,targetSegments[0]); //reordered
              //deltas.push_back(delta);
              //cout << **i << endl;
              //cout << "Source: " << *((*i)->GetSourcePhrase()) << " Target: " << ((*i)->GetTargetPhrase()) << endl;
              //cout << "Source: " << *((*i1)->GetSourcePhrase()) << "," << *((*i2)->GetSourcePhrase())<< 
              //  " Target: " << ((*i1)->GetTargetPhrase()) <<  "," << ((*i2)->GetTargetPhrase()) << endl;
        }
      }
    } else {
      //discontiguous - FIXME This is almost the same as the split case above - combine
      const TranslationOptionList&  options1 = 
        toc.GetTranslationOptionList(WordsRange(sourceSegment.GetStartPos(),splitIndex-1));
      const TranslationOptionList& options2 = 
        toc.GetTranslationOptionList(WordsRange(splitIndex,sourceSegment.GetEndPos()));
      for (TranslationOptionList::const_iterator i1 = options1.begin(); 
            i1 != options1.end(); ++i1) {
         for (TranslationOptionList::const_iterator i2 = options2.begin(); 
            i2 != options2.end(); ++i2) {
              TDelta* delta = new DiscontigTDelta(*i1,*i2,targetSegments[0],targetSegments[1]);
              deltas.push_back(delta);
              //delta = new DiscontigTDelta(*i2,*i1,targetSegments[0],targetSegments[1]); //reordered
              //deltas.push_back(delta);
         }
      }
    }
    
    cout << "Created " << deltas.size() << " delta(s)" << endl;
    
    //get the scores
    vector<double> scores;
    for (vector<TDelta*>::iterator i = deltas.begin(); i != deltas.end(); ++i) {
      scores.push_back((**i).getScore());
    }
    
    //randomly pick one of the deltas
    size_t chosen = getSample(scores);
    copy(scores.begin(),scores.end(),ostream_iterator<double>(cout,","));
    cout << endl;
    cout << "**The chosen sample is " << chosen << endl;
    
    //apply it to the sample

    
    
    //clean up
    for (size_t i = 0; i < deltas.size(); ++i) {
      delete deltas[i];
    }
  }
}

  
  
  
}//namespace
